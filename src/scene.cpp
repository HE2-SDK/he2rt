#include "scene.h"
#include <iostream>
#include <dds.hpp>

using GI = ucsl::reflection::game_interfaces::standalone::StandaloneGameInterface;

namespace he2rt {
    Scene::TextureStreamingPackage::TextureStreamingPackage(const std::string& filename, std::unordered_map<std::string, TextureInfo>&& textures) : filename{ filename }, textures { std::move(textures) } {
    }

    Scene::TextureStreamingPackage::TextureStreamingPackage(TextureStreamingPackage&& other) : filename{ std::move(other.filename) }, textures{ std::move(other.textures) } {
    }

    Scene::TextureStreamingPackage& Scene::TextureStreamingPackage::operator=(TextureStreamingPackage&& other) {
        filename = std::move(other.filename);
        textures = std::move(other.textures);
        return *this;
    }

    Scene::TextureStreamingPackage::TextureInfo::TextureInfo(std::vector<MipmapInfo>&& mipmapInfos, unsigned short width, unsigned short height) : mipmapInfos{ std::move(mipmapInfos) }, width{ width }, height{ height } {
    }

    Scene::TextureStreamingPackage::TextureInfo::TextureInfo(TextureInfo&& other) : mipmapInfos{ std::move(other.mipmapInfos) }, width{ width }, height{ height } {
    }

    Scene::TextureStreamingPackage::TextureInfo& Scene::TextureStreamingPackage::TextureInfo::operator=(TextureInfo&& other) {
        mipmapInfos = std::move(other.mipmapInfos);
        width = other.width;
        height = other.height;
        return *this;
    }

    Scene::Model::Model(Model&& other)
        : meshes{ std::move(other.meshes) }
        , shaderMeshes{ std::move(other.shaderMeshes) }
        , meshBuffer{ std::move(other.meshBuffer) }
        , blas{ std::move(other.blas) }
    {}

    Scene::Model& Scene::Model::operator=(Model&& other) {
        meshes = std::move(other.meshes);
        shaderMeshes = std::move(other.shaderMeshes);
        meshBuffer = std::move(other.meshBuffer);
        blas = std::move(other.blas);
        return *this;
    }

    Buffer Scene::Model::createMeshBuffer(Scene& scene) {
        auto buffer = scene.device.createBuffer(
            shaderMeshes.size() * sizeof(shaders::Mesh),
            alignof(shaders::Mesh),
            vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eTransferDst
        );

        {
            StagingArea staging{ scene.device };
            staging.addBuffer(buffer, std::span{ shaderMeshes });
            scene.device.runOneTimeCommandBuffer([&](auto& cmd) { staging.upload(cmd); });
        }

        return buffer;
    }

    Scene::Model::Mesh::Mesh(Buffer&& indexBuffer, Buffer&& vertexBuffer) : indexBuffer{ std::move(indexBuffer) }, vertexBuffer{ std::move(vertexBuffer) } {
    }

    Scene::Model::Mesh::Mesh(Mesh&& other) : indexBuffer{ std::move(other.indexBuffer) }, vertexBuffer{ std::move(other.vertexBuffer) } {
    }

    Scene::Model::Mesh& Scene::Model::Mesh::operator=(Mesh&& other) {
        indexBuffer = std::move(other.indexBuffer);
        vertexBuffer = std::move(other.vertexBuffer);
        return *this;
    }

    Scene::Scene(Device& device) : device{ device } {
    }

    void Scene::addModel(const std::string& name, const std::string& filename) {
        std::ifstream ifs{ filename, std::ios::binary | std::ios::ate };
        size_t fileSize = ifs.tellg();

        auto fileData = std::make_unique<uint8_t[]>(fileSize);

        ifs.seekg(std::ios::beg);
        ifs.read((char*)&fileData[0], fileSize);

        imemstream ims{ (char*)&fileData[0], fileSize };

        ucsl::magic_t<8> magic{};
        ims.read(magic.buffer.data(), 8);
        ims.seekg(0);

        if (magic == "NEDARCV1") {
            bool found{};
            rip::binary::containers::needle_archive::v1::NeedleArchiveReader<unsigned int> reader{ ims };
            reader.forEachChunk([&](auto& chunk) {
                if (found) return;
                if (chunk.header.magic == "NEDMDLV5") {
                    addModel<false, true>(name, ims);
                    found = true;
                }
            });
        }
        else {
            addModel(name, ims);
        }
    }

    void Scene::addTerrainModel(const std::string& name, const std::string& filename) {
        std::ifstream ifs{ filename, std::ios::binary | std::ios::ate };
        size_t fileSize = ifs.tellg();

        auto fileData = std::make_unique<uint8_t[]>(fileSize);

        ifs.seekg(std::ios::beg);
        ifs.read((char*)&fileData[0], fileSize);

        imemstream ims{ (char*)&fileData[0], fileSize };

        ucsl::magic_t<8> magic{};
        ims.read(magic.buffer.data(), 8);
        ims.seekg(0);

        if (magic == "NEDARCV1") {
            bool found{};
            rip::binary::containers::needle_archive::v1::NeedleArchiveReader<unsigned int> reader{ ims };
            reader.forEachChunk([&](auto& chunk) {
                if (found) return;
                if (chunk.header.magic == "NEDMDLV5") {
                    addTerrainModel<false, true>(name, ims);
                    found = true;
                }
            });
        }
        else {
            addTerrainModel(name, ims);
        }
    }

    void Scene::addTextureStreamingPackage(const std::string& name, const std::string& filename) {
        std::ifstream ifs{ filename, std::ios::binary };
        rip::binary::fast_istream fis{ ifs };
        rip::binary::binary_istream<size_t> stream{ fis, std::endian::native, 0ull };

        ucsl::reflection::providers::BoundProvider::RootType refl{ ucsl::reflection::providers::simplerfl<GI, size_t>::Type<ucsl::resources::ntsp::v1::reflections::NeedleTextureStreamingPackage>{} };
        typename rip::accessors::binary_istream<std::remove_reference_t<decltype(stream)>>::template ValueAccessor<decltype(refl)> acc{ stream, refl };

        auto ntsp = acc.as_structure();

        auto textures = ntsp.get_field<"textures">().as_carray();
        auto mipmaps = ntsp.get_field<"mipmaps">().as_carray();
        size_t tocSize = ntsp.get_field<"tocSize">().as_primitive().as<size_t>();
        size_t textureCount = textures.get_length();

        std::unordered_map<std::string, TextureStreamingPackage::TextureInfo> textureInfos{};
        textureInfos.reserve(textureCount);

        for (size_t i = 0; i < textureCount; i++) {
            auto texture = textures[i].as_structure();

            unsigned int mipmapOffset = texture.get_field<"mipmapOffset">().as_primitive().as<unsigned int>();
            unsigned int mipmapCount = texture.get_field<"mipmapCount">().as_primitive().as<unsigned int>();
            unsigned short width = texture.get_field<"width">().as_primitive().as<unsigned short>();
            unsigned short height = texture.get_field<"height">().as_primitive().as<unsigned short>();
            std::string textureName = texture.get_field<"name">().as_primitive().as<const char*>();

            //std::cout << textureName << ": " << width << "x" << height << ", " << mipmapCount << " mips" << std::endl;

            std::vector<TextureStreamingPackage::TextureInfo::MipmapInfo> mipmapInfos{};
            mipmapInfos.reserve(mipmapCount);

            for (size_t j = 0; j < mipmapCount; j++) {
                auto mipmap = mipmaps[mipmapOffset + j].as_structure();

                auto data = mipmap.get_field<"data">().as_pointer().get().value().as_carray();
                auto size = mipmap.get_field<"size">().as_primitive().as<size_t>();

                mipmapInfos.emplace_back(data.reference.offset, size);
            }

            textureInfos.emplace(textureName, TextureStreamingPackage::TextureInfo{ std::move(mipmapInfos), width, height });
        }

        uint32_t textureStreamingPackageId{ (uint32_t)textureStreamingPackages.size() };
        textureStreamingPackages.emplace_back(filename, std::move(textureInfos));
        textureStreamingPackageIdsByName[name] = textureStreamingPackageId;

        std::cout << "Loaded texture streaming package " << name << std::endl;
    }

    void Scene::addTexture(const std::string& name, const std::string& filename) {
        std::ifstream ifs{ filename, std::ios::binary | std::ios::ate };
        size_t fileSize = ifs.tellg();

        auto fileData = std::make_unique<uint8_t[]>(fileSize);

        ifs.seekg(std::ios::beg);
        ifs.read((char*)&fileData[0], fileSize);

        if (*((ucsl::magic_t<4>*)fileData.get()) == "NTSI") {
            addStreamingTexture(name, fileData.get(), fileSize);
        }
        else {
            addStandardTexture(name, fileData.get(), fileSize);
        }
    }

    void Scene::addMaterial(const std::string& name, std::istream& stream) {
        rip::binary::containers::mirage::v2::MirageResourceImageReader<unsigned int, true, false> reader{ stream };

        auto rootNode = reader.get_root_node();

        if (rootNode.header.magic == "Material") {
            rootNode.for_each_child([&](auto& materialChild) {
                if (materialChild.header.magic == "Contexts") {
                    auto& stream = materialChild.get_stream();

                    ucsl::reflection::providers::BoundProvider::RootType refl{ ucsl::reflection::providers::simplerfl<GI, unsigned int>::Type<ucsl::resources::material::v3::reflections::ContextsData>{} };
                    typename rip::accessors::binary_istream<std::remove_reference_t<decltype(stream)>>::template ValueAccessor<decltype(refl)> acc{ stream, refl };

                    auto contexts = acc.as_structure();

                    std::string shaderName = contexts.get_field<"shaderName">().as_primitive().as<const char*>();
                    std::string subShaderName = contexts.get_field<"subShaderName">().as_primitive().as<const char*>();

                    if (!materialShaders.contains(shaderName)) {
                        std::cout << "WARN: Could not find shader " << shaderName << ". Falling back to Common_dpn" << std::endl;

                        shaderName = "Common_dpn";
                    }

                    if (!materialShaders.contains(subShaderName)) {
                        std::cout << "WARN: Could not find subshader " << subShaderName << ". Falling back to Common_dpn" << std::endl;

                        subShaderName = "Common_dpn";
                    }

                    auto textureNames = contexts.get_field<"textureNames">().as_pointer().get().value().as_carray();
                    auto textures = contexts.get_field<"textures">().as_pointer().get().value().as_carray();
                    auto alphaThreshold = contexts.get_field<"alphaThreshold">().as_primitive().as<unsigned char>();
                    auto noBackfaceCulling = contexts.get_field<"noBackfaceCulling">().as_primitive().as<bool>();
                    auto blendMode = contexts.get_field<"blendMode">().as_enum();
                    auto unknownFlag1 = contexts.get_field<"unknownFlag1">().as_primitive().as<unsigned char>();
                    auto floatParamCount = contexts.get_field<"floatParamCount">().as_primitive().as<unsigned char>();
                    auto uintParamCount = contexts.get_field<"uintParamCount">().as_primitive().as<unsigned char>();
                    auto boolParamCount = contexts.get_field<"boolParamCount">().as_primitive().as<unsigned char>();
                    auto textureCount = contexts.get_field<"textureCount">().as_primitive().as<unsigned char>();

                    //std::cout << "Shader name: " << shaderName << std::endl;
                    //std::cout << "Shader subname: " << subShaderName << std::endl;
                    //std::cout << "Texture names:" << std::endl;
                    //for (size_t i = 0; i < textureNames.get_length(); i++) {
                    //    std::string textureName = textureNames[i].as_primitive().as<const char*>();

                    //    std::cout << "  " << textureName << std::endl;
                    //}

                    //std::cout << "Textures:" << std::endl;
                    //for (size_t i = 0; i < textures.get_length(); i++) {
                    //    auto texture = textures[i].as_pointer().get().value().as_structure();

                    //    std::string name = texture.get_field<"name">().as_primitive().as<const char*>();
                    //    std::string type = texture.get_field<"type">().as_primitive().as<const char*>();
                    //    auto texCoordIndex = texture.get_field<"texCoordIndex">().as_primitive().as<unsigned char>();
                    //    auto wrapModeU = texture.get_field<"wrapModeU">().as_enum();
                    //    auto wrapModeV = texture.get_field<"wrapModeV">().as_enum();

                    //    std::cout << "  Name: " << name << std::endl;
                    //    std::cout << "  Type: " << type << std::endl;
                    //    std::cout << "  TexCoordIdx: " << static_cast<unsigned int>(texCoordIndex) << std::endl;

                    //    for (auto& enumOption : wrapModeU.refl.get_options()) {
                    //        if (wrapModeU == enumOption.GetIndex()) {
                    //            std::cout << "  WrapModeU: " << enumOption.GetEnglishName() << std::endl;
                    //        }
                    //    }

                    //    for (auto& enumOption : wrapModeV.refl.get_options()) {
                    //        if (wrapModeV == enumOption.GetIndex()) {
                    //            std::cout << "  WrapModeV: " << enumOption.GetEnglishName() << std::endl;
                    //        }
                    //    }
                    //}

                    //for (auto& enumOption : blendMode.refl.get_options()) {
                    //    if (blendMode == enumOption.GetIndex()) {
                    //        std::cout << "Blend Mode: " << enumOption.GetEnglishName() << std::endl;
                    //    }
                    //}

                    //std::cout << "Unknown flag: " << static_cast<unsigned int>(unknownFlag1) << std::endl;

                    //std::cout << "Float params:" << std::endl;

                    //if (floatParamCount > 0) {
                    //    auto floatParams = contexts.get_field<"floatParams">().as_pointer().get().value().as_carray();

                    //    for (size_t i = 0; i < floatParams.get_length(); i++) {
                    //        auto param = floatParams[i].as_pointer().get().value().as_structure();

                    //        std::string name = param.get_field<"name">().as_primitive().as<const char*>();
                    //        auto values = param.get_field<"values">().as_pointer().get().value().as_carray();

                    //        std::cout << "  " << name << std::endl;

                    //        for (size_t j = 0; j < values.get_length(); j++) {
                    //            auto singleValue = values[j].as_carray();

                    //            std::cout << "    ";

                    //            for (size_t k = 0; k < singleValue.get_length(); k++) {
                    //                std::cout << singleValue[k].as_primitive().as<float>() << " ";
                    //            }

                    //            std::cout << std::endl;
                    //        }
                    //    }
                    //}

                    //std::cout << "Uint params:" << std::endl;

                    //if (uintParamCount > 0) {
                    //    auto uintParams = contexts.get_field<"uintParams">().as_pointer().get().value().as_carray();

                    //    for (size_t i = 0; i < uintParams.get_length(); i++) {
                    //        auto param = uintParams[i].as_pointer().get().value().as_structure();

                    //        std::string name = param.get_field<"name">().as_primitive().as<const char*>();
                    //        auto values = param.get_field<"values">().as_pointer().get().value().as_carray();

                    //        std::cout << "  " << name << std::endl;

                    //        for (size_t j = 0; j < values.get_length(); j++) {
                    //            auto singleValue = values[j].as_carray();

                    //            std::cout << "    ";

                    //            for (size_t k = 0; k < singleValue.get_length(); k++) {
                    //                std::cout << singleValue[k].as_primitive().as<unsigned int>() << " ";
                    //            }

                    //            std::cout << std::endl;
                    //        }
                    //    }
                    //}

                    //std::cout << "Bool params:" << std::endl;

                    //if (boolParamCount > 0) {
                    //    auto boolParams = contexts.get_field<"boolParams">().as_pointer().get().value().as_carray();

                    //    for (size_t i = 0; i < boolParams.get_length(); i++) {
                    //        auto param = boolParams[i].as_pointer().get().value().as_structure();

                    //        std::string name = param.get_field<"name">().as_primitive().as<const char*>();
                    //        auto values = param.get_field<"values">().as_pointer().get().value().as_carray();

                    //        std::cout << "  " << name << std::endl;

                    //        for (size_t j = 0; j < values.get_length(); j++) {
                    //            auto singleValue = values[j].as_carray();

                    //            std::cout << "    ";

                    //            for (size_t k = 0; k < singleValue.get_length(); k++) {
                    //                std::cout << singleValue[k].as_primitive().as<bool>() << " ";
                    //            }

                    //            std::cout << std::endl;
                    //        }
                    //    }
                    //}

                    auto materialShaderIt{ materialShaders.find(shaderName) };

                    if (materialShaderIt == materialShaders.end())
                        throw std::runtime_error{ std::string{ "Unknown material shader: " } + shaderName };

                    auto& materialShader{ materialShaderIt->second };
                    auto& inputLayout{ materialShader.reflection.inputLayout };
                    auto shaderId{ materialShader.shaderId };
                    auto shaderParamsPtr{ std::make_unique<char[]>(inputLayout.size + 16) };
                    auto* shaderParams{ util::align(shaderParamsPtr.get(), 16)};
                    std::unordered_map<std::string, uint32_t> curIndices{};

                    for (size_t i = 0; i < textures.get_length(); i++) {
                        auto texture = textures[i].as_pointer().get().value().as_structure();
                        std::string name = texture.get_field<"name">().as_primitive().as<const char*>();
                        std::string type = texture.get_field<"type">().as_primitive().as<const char*>();
                        uint32_t idx = curIndices[type]++;

                        auto offsetIt = inputLayout.offsets.find(type + (idx == 0 ? "" : std::to_string(idx)) + "TextureId");

                        std::cout << type << " = " << name << std::endl;

                        if (offsetIt == inputLayout.offsets.end())
                            continue;

                        auto* param = (shaders::Sampler*)&shaderParams[offsetIt->second];

                        if (!textureIdsByName.contains(name)) {
                            std::cout << "WARN: Unknown texture " << name << std::endl;
                            param->textureId = 0;
                            continue;
                            //throw std::runtime_error{ std::string{ "Unknown texture " } + name };
                        }

                        param->textureId = textureIdsByName[name];

                        //auto texCoordIndex = texture.get_field<"texCoordIndex">().as_primitive().as<unsigned char>();
                        //auto wrapModeU = texture.get_field<"wrapModeU">().as_enum();
                        //auto wrapModeV = texture.get_field<"wrapModeV">().as_enum();

                        //std::cout << "  Name: " << name << std::endl;
                        //std::cout << "  Type: " << type << std::endl;
                        //std::cout << "  TexCoordIdx: " << static_cast<unsigned int>(texCoordIndex) << std::endl;

                        //for (auto& enumOption : wrapModeU.refl.get_options()) {
                        //    if (wrapModeU == enumOption.GetIndex()) {
                        //        std::cout << "  WrapModeU: " << enumOption.GetEnglishName() << std::endl;
                        //    }
                        //}

                        //for (auto& enumOption : wrapModeV.refl.get_options()) {
                        //    if (wrapModeV == enumOption.GetIndex()) {
                        //        std::cout << "  WrapModeV: " << enumOption.GetEnglishName() << std::endl;
                        //    }
                        //}
                    }

                    if (floatParamCount > 0) {
                        auto floatParams = contexts.get_field<"floatParams">().as_pointer().get().value().as_carray();

                        for (size_t i = 0; i < floatParams.get_length(); i++) {
                            auto param = floatParams[i].as_pointer().get().value().as_structure();

                            std::string name = param.get_field<"name">().as_primitive().as<const char*>();
                            auto values = param.get_field<"values">().as_pointer().get().value().as_carray();
                            auto offsetIt = inputLayout.offsets.find(name);

                            if (offsetIt == inputLayout.offsets.end())
                                continue;

                            auto* shaderParam = (float4*)&shaderParams[offsetIt->second];

                            for (size_t j = 0; j < values.get_length(); j++) {
                                auto singleValue = values[j].as_carray();

                                std::cout << name << ": "
                                    << singleValue[0].as_primitive().as<float>() << " "
                                    << singleValue[1].as_primitive().as<float>() << " "
                                    << singleValue[2].as_primitive().as<float>() << " "
                                    << singleValue[3].as_primitive().as<float>() << " "
                                    << std::endl;

                                shaderParam[j] = {
                                    singleValue[0].as_primitive().as<float>(),
                                    singleValue[1].as_primitive().as<float>(),
                                    singleValue[2].as_primitive().as<float>(),
                                    singleValue[3].as_primitive().as<float>(),
                                };
                            }
                        }
                    }

                    if (uintParamCount > 0) {
                        auto uintParams = contexts.get_field<"uintParams">().as_pointer().get().value().as_carray();

                        for (size_t i = 0; i < uintParams.get_length(); i++) {
                            auto param = uintParams[i].as_pointer().get().value().as_structure();

                            std::string name = param.get_field<"name">().as_primitive().as<const char*>();
                            auto values = param.get_field<"values">().as_pointer().get().value().as_carray();
                            auto offsetIt = inputLayout.offsets.find(name);

                            if (offsetIt == inputLayout.offsets.end())
                                continue;

                            auto* shaderParam = (uint4*)&shaderParams[offsetIt->second];

                            for (size_t j = 0; j < values.get_length(); j++) {
                                auto singleValue = values[j].as_carray();

                                std::cout << name << ": "
                                    << singleValue[0].as_primitive().as<unsigned int>() << " "
                                    << singleValue[1].as_primitive().as<unsigned int>() << " "
                                    << singleValue[2].as_primitive().as<unsigned int>() << " "
                                    << singleValue[3].as_primitive().as<unsigned int>() << " "
                                    << std::endl;

                                shaderParam[j] = {
                                    singleValue[0].as_primitive().as<unsigned int>(),
                                    singleValue[1].as_primitive().as<unsigned int>(),
                                    singleValue[2].as_primitive().as<unsigned int>(),
                                    singleValue[3].as_primitive().as<unsigned int>(),
                                };
                            }
                        }
                    }

                    if (boolParamCount > 0) {
                        auto boolParams = contexts.get_field<"boolParams">().as_pointer().get().value().as_carray();

                        for (size_t i = 0; i < boolParams.get_length(); i++) {
                            auto param = boolParams[i].as_pointer().get().value().as_structure();

                            std::string name = param.get_field<"name">().as_primitive().as<const char*>();
                            auto values = param.get_field<"values">().as_pointer().get().value().as_carray();
                            auto offsetIt = inputLayout.offsets.find(name);

                            if (offsetIt == inputLayout.offsets.end())
                                continue;

                            auto* shaderParam = (bool4*)&shaderParams[offsetIt->second];

                            for (size_t j = 0; j < values.get_length(); j++) {
                                auto singleValue = values[j].as_carray();

                                std::cout << name << ": "
                                    << singleValue[0].as_primitive().as<bool>() << " "
                                    << singleValue[1].as_primitive().as<bool>() << " "
                                    << singleValue[2].as_primitive().as<bool>() << " "
                                    << singleValue[3].as_primitive().as<bool>() << " "
                                    << std::endl;

                                shaderParam[j] = {
                                    singleValue[0].as_primitive().as<bool>(),
                                    singleValue[1].as_primitive().as<bool>(),
                                    singleValue[2].as_primitive().as<bool>(),
                                    singleValue[3].as_primitive().as<bool>(),
                                };
                            }
                        }
                    }

                    auto paramBuffer = device.createBuffer(
                        util::align(inputLayout.size, 16),
                        16,
                        vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eTransferDst
                    );

                    {
                        StagingArea staging{ device };

                        staging.addBuffer(paramBuffer, shaderParams, inputLayout.size);

                        device.runOneTimeCommandBuffer([&](auto& cmd) {
                            staging.upload(cmd);
                        });
                    }

                    uint32_t materialId{ (uint32_t)materials.size() };
                    shaderMaterials.emplace_back((uint8_t*)paramBuffer.deviceAddress, shaderId);
                    materials.emplace_back(std::move(paramBuffer));
                    materialIdsByName[name] = materialId;

                    std::cout << "Loaded material " << name << std::endl;
                }
                else if (materialChild.header.magic == "SCAParam") {
                    materialChild.for_each_child([&](auto& scaParamChild) {
                        if (scaParamChild.header.magic == "UsePrior") {
                            // enables render priority overrides in HE2
                        }
                        else if (scaParamChild.header.magic == "RenderCa") {
                            // enables "renderCa" (render category?) in the priority parameter in HE2. value is set to the node value.
                        }
                        else if (scaParamChild.header.magic == "UseGrass") {
                            // does nothing in in HE2
                        }
                        else if (scaParamChild.header.magic == "Property") {
                            // allows adding extra GatherRenderingPassHint values in HE2:
                            //
                            // matBuilder.SetParameterHintValueUint("GatherRenderingPassHint", scaParamChild.header.magic, { scaParamChild.header.value, 0, 0, 0 });
                        }
                        else if (scaParamChild.header.magic == "MaxAniso") {
                            // sets max anisotropy in HE2
                        }
                        else if (scaParamChild.header.magic == "MipBias ") {
                            // sets mip bias in HE2
                        }
                        else if (scaParamChild.header.magic == "ExtnAABB") {
                            // sets a "use extended AABB" GatherRenderingPassHint in HE2
                            //
                            // matBuilder.SetParameterHintValueUint("GatherRenderingPassHint", "UseExtendedAABB", { scaParamChild.header.value, 0, 0, 0 });
                        }
                        else if (scaParamChild.header.magic == "TrnsType") {
                            // 0 = no billboarding, 1 = horizontal billboarding, 2 = horizontal and vertical billboarding
                            //
                            // switch (scaParamChild.header.value) {
                            //     case 0:
                            //         matBuilder.SetParameterBool("u_enable_billboard", 0);
                            //         matBuilder.SetParameterBool("u_enable_billboard_y", 0);
                            //     case 1:
                            //         matBuilder.SetParameterBool("u_enable_billboard", 1);
                            //         matBuilder.SetParameterBool("u_enable_billboard_y", 0);
                            //     case 2:
                            //         matBuilder.SetParameterBool("u_enable_billboard", 1);
                            //         matBuilder.SetParameterBool("u_enable_billboard_y", 1);
                            // }
                        }
                        else if (scaParamChild.header.magic == "TrnsPrio") {
                            // sets transparency render priority in HE2. value is set to the node value. useprio must be enabled to activate this.
                        }
                        else {
                            // any other nodes are added as follows in HE2:
                            // 
                            // matBuilder.SetParameterHintValueUint("OtherAttribute", scaParamChild.header.magic, { scaParamChild.header.value, 0, 0, 0 });
                            // matBuilder.SetParameterBool(scaParamChild.header.magic, scaParamChild.header.value != 0);
                            // matBuilder.SetParameterInt(scaParamChild.header.magic, { scaParamChild.header.value, 0, 0, 0 });
                        }
                    });
                }
            });
        }
    }

    void Scene::addMaterial(const std::string& name, const std::string& filename) {
        std::ifstream ifs{ filename, std::ios::binary | std::ios::ate };
        size_t fileSize = ifs.tellg();

        auto fileData = std::make_unique<uint8_t[]>(fileSize);

        ifs.seekg(std::ios::beg);
        ifs.read((char*)&fileData[0], fileSize);

        imemstream ims{ (char*)&fileData[0], fileSize };

        addMaterial(name, ims);
    }

    void Scene::addMaterialShader(const std::string& name, const uint32_t* data, size_t size, const MaterialShader::Reflection& reflection) {
        uint32_t shaderId{ (uint32_t)shaders.size() };
        shaders.emplace_back(data, size);//device.device.createShaderModule({ {}, size, data }));
        materialShaders[name] = { shaderId, reflection };
    }

    void Scene::addMaterialShader(const std::string& name, std::istream& stream) {

    }

    void Scene::addModelInstance(const std::string& modelName, const Eigen::Affine3f& transform) {
        auto modelIt{ modelIdsByName.find(modelName) };

        if (modelIt == modelIdsByName.end())
            throw std::runtime_error{ std::string{ "Unknown model: " } + modelName };

        auto modelId = modelIt->second;
        auto& model = models[modelId];

        using VulkanCompatibleTransform = Eigen::Transform<float, 3, Eigen::AffineCompact, Eigen::RowMajor>;

        VulkanCompatibleTransform tf{ transform };

        tlasInstances.push_back(vk::AccelerationStructureInstanceKHR{
            *reinterpret_cast<vk::TransformMatrixKHR*>(tf.data()),
            modelId,
            0xFF,
            0,
            vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
            model.blas.deviceAddress,
        });
    }

    void Scene::addModelInstances(const std::string& filename) {
        std::ifstream ifs{ filename, std::ios::binary | std::ios::ate };
        size_t fileSize = ifs.tellg();

        auto fileData = std::make_unique<uint8_t[]>(fileSize);

        ifs.seekg(std::ios::beg);
        ifs.read((char*)&fileData[0], fileSize);

        imemstream ims{ (char*)&fileData[0], fileSize };

        addModelInstances(ims);
    }

    Eigen::Quaternionf FromPCRotation(const Eigen::Vector3f& rotation, ucsl::resources::pointcloud::v2::RotationOrder rotationOrder) {
        switch (rotationOrder) {
        case ucsl::resources::pointcloud::v2::RotationOrder::XYZ: return Eigen::AngleAxisf(rotation[2], Eigen::Vector3f::UnitZ()) * Eigen::AngleAxisf(rotation[1], Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(rotation[0], Eigen::Vector3f::UnitX());
        case ucsl::resources::pointcloud::v2::RotationOrder::YZX: return Eigen::AngleAxisf(rotation[0], Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(rotation[2], Eigen::Vector3f::UnitZ()) * Eigen::AngleAxisf(rotation[1], Eigen::Vector3f::UnitY());
        case ucsl::resources::pointcloud::v2::RotationOrder::ZXY: return Eigen::AngleAxisf(rotation[1], Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(rotation[0], Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(rotation[2], Eigen::Vector3f::UnitZ());
        case ucsl::resources::pointcloud::v2::RotationOrder::XZY: return Eigen::AngleAxisf(rotation[1], Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(rotation[2], Eigen::Vector3f::UnitZ()) * Eigen::AngleAxisf(rotation[0], Eigen::Vector3f::UnitX());
        case ucsl::resources::pointcloud::v2::RotationOrder::YXZ: return Eigen::AngleAxisf(rotation[2], Eigen::Vector3f::UnitZ()) * Eigen::AngleAxisf(rotation[0], Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(rotation[1], Eigen::Vector3f::UnitY());
        case ucsl::resources::pointcloud::v2::RotationOrder::ZYX: return Eigen::AngleAxisf(rotation[0], Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(rotation[1], Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(rotation[2], Eigen::Vector3f::UnitZ());
        default: return Eigen::AngleAxisf(rotation[0], Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(rotation[1], Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(rotation[2], Eigen::Vector3f::UnitZ());
        }
    }

    void Scene::addModelInstances(std::istream& stream) {
        rip::binary::containers::binary_file::v2::BinaryFileReader<uint64_t> bfr{ stream };
        
        auto chunk = bfr.getNextDataChunk();

        ucsl::reflection::providers::BoundProvider::RootType refl{ ucsl::reflection::providers::simplerfl<GI, uint64_t>::Type<ucsl::resources::pointcloud::v2::reflections::PointcloudData>{} };

        typename rip::accessors::binary_istream<std::remove_reference_t<decltype(chunk)>>::template ValueAccessor<decltype(refl)> acc{ chunk, refl };

        auto pointcloudData = acc.as_structure();
        auto instances = pointcloudData.get_field<"instances">().as_pointer().get().value().as_carray();

        for (size_t i = 0; i < instances.get_length(); i++) {
            auto instance = instances[i].as_structure();

            std::string name = instance.get_field<"name">().as_primitive().as<const char*>();
            std::string modelName = instance.get_field<"resourceName">().as_primitive().as<const char*>();
            ucsl::math::Position position = instance.get_field<"position">().as_primitive().as<ucsl::math::Position>();
            ucsl::math::Position rotation = instance.get_field<"rotation">().as_primitive().as<ucsl::math::Position>();
            ucsl::math::Position scale = instance.get_field<"scale">().as_primitive().as<ucsl::math::Position>();
            auto rotationOrder = static_cast<ucsl::resources::pointcloud::v2::RotationOrder>(static_cast<long long>(instance.get_field<"rotationOrder">().as_enum()));

            auto rotationQ = FromPCRotation(rotation, rotationOrder);

            Eigen::Affine3f tf{};
            tf.fromPositionOrientationScale(position, rotationQ, scale);

            try {
                addModelInstance(modelName, tf);
                std::cout << "Loaded model instance " << name << std::endl;
            }
            catch (std::runtime_error& err) {
                std::cout << "could not load " << modelName << std::endl;
            }
        }
    }

    void Scene::generateMaterialBuffer() {
        materialBuffer = device.createBuffer(
            shaderMaterials.size() * sizeof(shaders::Material),
            alignof(shaders::Material),
            vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eTransferDst
        );

        {
            StagingArea staging{ device };
            staging.addBuffer(materialBuffer.value(), std::span{ shaderMaterials });
            device.runOneTimeCommandBuffer([&](auto& cmd) { staging.upload(cmd); });
        }
    }

    void Scene::generateModelBuffer() {
        modelBuffer = device.createBuffer(
            shaderModels.size() * sizeof(shaders::Model),
            alignof(shaders::Mesh),
            vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eTransferDst
        );

        {
            StagingArea staging{ device };
            staging.addBuffer(modelBuffer.value(), std::span{ shaderModels });
            device.runOneTimeCommandBuffer([&](auto& cmd) { staging.upload(cmd); });
        }
    }

    void Scene::buildTLAS() {
        auto buffer = device.createBuffer(
            tlasInstances.size() * sizeof(vk::AccelerationStructureInstanceKHR),
            16,
            vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eTransferDst
        );

        {
            StagingArea staging{ device };
            staging.addBuffer(buffer, std::span{ tlasInstances });
            device.runOneTimeCommandBuffer([&](auto& cmd) { staging.upload(cmd); });
        }

        vk::AccelerationStructureGeometryInstancesDataKHR geometryInstances{ {}, { buffer.deviceAddress } };
        std::array<vk::AccelerationStructureGeometryKHR, 1> geometries{ { { vk::GeometryTypeKHR::eInstances, geometryInstances } } };
        std::array<vk::AccelerationStructureBuildRangeInfoKHR, 1> rangeInfos{ { (uint32_t)tlasInstances.size() } };

        tlas = device.buildTLAS(geometries, rangeInfos);
    }

    void Scene::finalize() {
        generateMaterialBuffer();
        generateModelBuffer();
        buildTLAS();
    }

    Scene::DDSImageCreateInfo Scene::readDDSImage(uint8_t* data, size_t size) {
        dds::Image ddsImage;
        dds::readImage(data, size, &ddsImage);

        vk::ImageCreateInfo imageCreateInfo{ dds::getVulkanImageCreateInfo(&ddsImage) };
        vk::ImageViewCreateInfo imageViewCreateInfo{ dds::getVulkanImageViewCreateInfo(&ddsImage) };

        imageCreateInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        imageCreateInfo.samples = vk::SampleCountFlagBits::e1;

        if (ddsImage.format == DXGI_FORMAT_BC6H_UF16) {
            imageCreateInfo.format = vk::Format::eBc6HUfloatBlock;
            imageViewCreateInfo.format = vk::Format::eBc6HUfloatBlock;
        }

        if (ddsImage.format == DXGI_FORMAT_BC6H_SF16) {
            imageCreateInfo.format = vk::Format::eBc6HSfloatBlock;
            imageViewCreateInfo.format = vk::Format::eBc6HSfloatBlock;
        }

        return { std::move(ddsImage), std::move(imageCreateInfo), std::move(imageViewCreateInfo) };
    }

    void Scene::addTexture(const std::string& name, const DDSImageCreateInfo& ddsImage, uint8_t* data, size_t size) {
        auto image = device.createImage(ddsImage.imageCreateInfo);

        vk::ImageViewCreateInfo imageViewCreateInfo = ddsImage.imageViewCreateInfo;

        imageViewCreateInfo.image = image.image;

        auto stagingBuffer = device.createBuffer(
            size,
            16,
            vk::BufferUsageFlagBits2::eTransferSrc | vk::BufferUsageFlagBits2::eTransferDst | vk::BufferUsageFlagBits2::eShaderDeviceAddress,
            vma::MemoryUsage::eAutoPreferHost,
            vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
            vk::MemoryPropertyFlagBits::eHostCoherent
        );

        memcpy(stagingBuffer.hostAddress, data, size);

        vk::ImageSubresourceRange colorRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        device.runOneTimeCommandBuffer([&](vk::CommandBuffer& cmd) {
            std::array<vk::ImageMemoryBarrier2, 1> imageBarrierBeforeClear{ {
                {
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    image.image,
                    colorRange,
                },
            } };
            cmd.pipelineBarrier2({ {}, {}, {}, imageBarrierBeforeClear });

            std::array<vk::BufferImageCopy2, 1> bufferImageCopies{ {
                { 0, ddsImage.ddsImage.width, ddsImage.ddsImage.height, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 }, { ddsImage.ddsImage.width, ddsImage.ddsImage.height, ddsImage.ddsImage.depth } },
            } };
            cmd.copyBufferToImage2({ stagingBuffer.buffer, image.image, vk::ImageLayout::eTransferDstOptimal, bufferImageCopies });
            std::array<vk::ImageMemoryBarrier2, 1> imageBarrierAfterClear{ {
                {
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eAllCommands | vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                    vk::AccessFlagBits2::eShaderRead,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eGeneral,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    image.image,
                    colorRange,
                },
            } };
            cmd.pipelineBarrier2({ {}, {}, {}, imageBarrierAfterClear });
            });

        auto imageView = device.device.createImageView(imageViewCreateInfo);
        auto sampler = device.createSampler({
            {},
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            {},
        });
        vk::DescriptorImageInfo descriptor{ sampler.sampler, imageView, vk::ImageLayout::eGeneral };

        uint32_t textureId = (uint32_t)textures.size();
        textures.emplace_back(std::move(image), std::move(imageView), std::move(sampler), std::move(descriptor));
        textureIdsByName[name] = textureId;

        std::cout << "Loaded texture " << name << std::endl;
    }

    void Scene::addStandardTexture(const std::string& name, uint8_t* data, size_t size) {
        DDSImageCreateInfo ddsImage = readDDSImage(data, size);

        if (ddsImage.ddsImage.mipmaps.size() == 0)
            return;

        addTexture(name, ddsImage, ddsImage.ddsImage.mipmaps[0].data(), ddsImage.ddsImage.mipmaps[0].size_bytes());
    }

    void Scene::addStreamingTexture(const std::string& name, uint8_t* data, size_t size) {
        imemstream ims{ (char*)data, size };
        rip::binary::containers::ntsi::v1::NeedleTextureStreamingInfoReader<size_t> reader{ ims };

        auto data$ = reader.getStream();

        data$.skip_padding_bytes(reader.header.defaultMipSize);
        
        size_t ddsHeaderSize = size - data$.tellg();
        auto ddsHeaderData = std::make_unique<uint8_t[]>(ddsHeaderSize);

        ims.read((char*)ddsHeaderData.get(), ddsHeaderSize);

        DDSImageCreateInfo ddsImage = readDDSImage(ddsHeaderData.get(), ddsHeaderSize);

        auto textureStreamingPackageIt{ textureStreamingPackageIdsByName.find(reader.packageName) };

        if (textureStreamingPackageIt == textureStreamingPackageIdsByName.end())
            throw std::runtime_error{ std::string{ "Unknown texture streaming package: " } + reader.packageName };

        auto textureStreamingPackageId{ textureStreamingPackageIt->second };
        auto& textureStreamingPackage{ textureStreamingPackages[textureStreamingPackageId] };

        auto textureInfoIt{ textureStreamingPackage.textures.find(name) };

        if (textureInfoIt == textureStreamingPackage.textures.end())
            throw std::runtime_error{ std::string{ "Texture streaming package " } + reader.packageName + " does not contain texture " + name };

        auto& textureInfo{ textureInfoIt->second };
        auto& mipmapInfo = textureInfo.mipmapInfos[0];

        std::ifstream ifs{ textureStreamingPackage.filename, std::ios::binary };

        auto imgData = std::make_unique<uint8_t[]>(mipmapInfo.size);

        ifs.seekg(mipmapInfo.offset);
        ifs.read((char*)imgData.get(), mipmapInfo.size);

        addTexture(name, ddsImage, imgData.get(), mipmapInfo.size);

        //std::ofstream ofs{ name + "_debug_dump.dds", std::ios::binary};
        //ofs.write((char*)ddsHeaderData.get(), ddsHeaderSize);
        //
        //for (auto& mip : textureInfo.mipmapInfos) {
        //    auto mipImgData = std::make_unique<uint8_t[]>(mip.size);
        //    ifs.seekg(mip.offset);
        //    ifs.read((char*)mipImgData.get(), mip.size);
        //    ofs.write((char*)mipImgData.get(), mip.size);
        //}
    }
}