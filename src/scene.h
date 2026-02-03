#pragma once
#include <vector>
#include <unordered_map>
#include <optional>
#include <vk_mem_alloc.hpp>
#include <vk_mem_alloc_raii.hpp>
#include "math.h"
#include "acceleration-structures.h"
#include "util.h"
#include "staging-area.h"
#include "device.h"
#include "buffer.h"
#include "image.h"

#include <rip/binary/containers/needle-archive/v1.h>
#include <rip/binary/containers/mirage/v2.h>
#include <rip/binary/containers/binary-file/v2.h>
#include <rip/binary/containers/ntsi/v1.h>
#include <rip/binary/serialization/ReflectionDeserializer.h>
#include <rip/binary/stream.h>
#include <ucsl-reflection/game-interfaces/standalone/game-interface.h>
#include <ucsl-reflection/providers/simplerfl.h>
#include <ucsl-reflection/reflections/resources/material/v3.h>
#include <ucsl-reflection/reflections/resources/model/v5.h>
#include <ucsl-reflection/reflections/resources/pointcloud/v2.h>
#include <ucsl-reflection/reflections/resources/ntsp/v1.h>
#include <ucsl-reflection/bound-reflection.h>
#include <rip/binary/accessors/binary-stream.h>
#include <dds.hpp>
#include "mem_stream.h"
#include "shader-types.h"

#include <entt/entt.hpp>

namespace he2rt {
	class Device;
	struct Scene {
		struct MeshInstance {
			uint32_t meshId{};
			float3x4 transform{};
		};

		struct Light {
			float3 color{};
		};

		struct LightInstance {
			uint32_t lightId{};
			float3x4 transform{};
		};

		struct DDSImageCreateInfo {
			dds::Image ddsImage{};
			vk::ImageCreateInfo imageCreateInfo;
			vk::ImageViewCreateInfo imageViewCreateInfo;
		};

		struct TextureStreamingPackage {
			struct TextureInfo {
				struct MipmapInfo {
					size_t offset{};
					size_t size{};
				};

				std::vector<MipmapInfo> mipmapInfos{};
				unsigned short width{};
				unsigned short height{};

				TextureInfo(std::vector<MipmapInfo>&& mipmapInfos, unsigned short width, unsigned short height);
				TextureInfo(const TextureInfo& other) = delete;
				TextureInfo(TextureInfo&& other);

				TextureInfo& operator=(const TextureInfo& other) = delete;
				TextureInfo& operator=(TextureInfo&& other);
			};

			std::string filename{};
			std::unordered_map<std::string, TextureInfo> textures{};

			TextureStreamingPackage(const std::string& filename, std::unordered_map<std::string, TextureInfo>&& textures);
			TextureStreamingPackage(const TextureStreamingPackage& other) = delete;
			TextureStreamingPackage(TextureStreamingPackage&& other);

			TextureStreamingPackage& operator=(const TextureStreamingPackage& other) = delete;
			TextureStreamingPackage& operator=(TextureStreamingPackage&& other);
		};

		struct Texture {
			Image image;
			vk::raii::ImageView imageView;
			Sampler sampler;
			vk::DescriptorImageInfo descriptor;
		};

		struct Shader {
			const uint32_t* code;
			size_t size;
			//vk::raii::ShaderModule shaderModule;
		};

		struct MaterialShader {
			struct Reflection {
				struct InputLayout {
					size_t size{};
					std::unordered_map<std::string, uint32_t> offsets{};
				};

				InputLayout inputLayout{};
			};

			uint32_t shaderId{};
			Reflection reflection{};
		};

		struct Material {
			Buffer shaderParamBuffer;
		};

		struct Model {
			struct Mesh {
				Buffer indexBuffer;
				Buffer vertexBuffer;

				Mesh(Buffer&& indexBuffer, Buffer&& vertexBuffer);
				Mesh(const Mesh& other) = delete;
				Mesh(Mesh&& other);

				Mesh& operator=(const Mesh& other) = delete;
				Mesh& operator=(Mesh&& other);
			};

			std::vector<Mesh> meshes{};
			std::vector<shaders::Mesh> shaderMeshes{};
			AccelerationStructure blas;
			Buffer meshBuffer;

			std::tuple<vk::AccelerationStructureGeometryKHR, vk::AccelerationStructureBuildRangeInfoKHR> addMesh(Scene& scene, auto mesh) {
				std::string name = mesh.get_field<"name">().as_primitive().as<const char*>();

				unsigned int indexCount = mesh.get_field<"indexCount">().as_primitive().as<unsigned int>();
				unsigned int vertexCount = mesh.get_field<"vertexCount">().as_primitive().as<unsigned int>();
				unsigned int triangleCount = indexCount / 3u;

				auto indices = mesh.get_field<"indices">().as_pointer().get().value().as_carray();
				size_t indexBufSize = indexCount * sizeof(unsigned short);
				std::vector<unsigned short> indexBuf(indexCount);

				for (size_t i = 0; i < indexCount; i++)
					indexBuf[i] = indices[i].as_primitive().as<unsigned short>();

				auto vertexData = mesh.get_field<"vertexData">().as_pointer().get().value().as_carray();
				unsigned int vertexStride = mesh.get_field<"vertexStride">().as_primitive().as<unsigned int>();

				size_t vertexBufSize = vertexCount * vertexStride;

				std::vector<unsigned char> vertexBuf(vertexBufSize);

				for (size_t i = 0; i < vertexBufSize; i++)
					vertexBuf[i] = vertexData[i].as_primitive().as<unsigned char>();

				for (size_t i = 0; i < vertexCount; i++) {
					auto* pos = reinterpret_cast<float*>(&vertexBuf[i * vertexStride]);

					pos[0] = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned int>(pos[0])));
					pos[1] = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned int>(pos[1])));
					pos[2] = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned int>(pos[2])));

					auto* nrm = reinterpret_cast<uint8_t*>(&vertexBuf[i * vertexStride + 0xC]);
					auto nrmSwapped = _byteswap_ulong(*reinterpret_cast<uint32_t*>(nrm));

					nrm[0] = static_cast<uint8_t>(nrmSwapped >> 2);
					nrm[1] = static_cast<uint8_t>(nrmSwapped >> 12);
					nrm[2] = static_cast<uint8_t>(nrmSwapped >> 22);
					nrm[3] = static_cast<uint8_t>(nrmSwapped >> 24) | 0x3F;

					auto* tng = reinterpret_cast<uint8_t*>(&vertexBuf[i * vertexStride + 0x10]);
					auto tngSwapped = _byteswap_ulong(*reinterpret_cast<uint32_t*>(tng));

					tng[0] = static_cast<uint8_t>(tngSwapped >> 2);
					tng[1] = static_cast<uint8_t>(tngSwapped >> 12);
					tng[2] = static_cast<uint8_t>(tngSwapped >> 22);
					tng[3] = static_cast<uint8_t>(tngSwapped >> 24) | 0x3F;

					auto* bin = reinterpret_cast<uint8_t*>(&vertexBuf[i * vertexStride + 0x14]);
					auto binSwapped = _byteswap_ulong(*reinterpret_cast<uint32_t*>(bin));

					bin[0] = static_cast<uint8_t>(binSwapped >> 2);
					bin[1] = static_cast<uint8_t>(binSwapped >> 12);
					bin[2] = static_cast<uint8_t>(binSwapped >> 22);
					bin[3] = static_cast<uint8_t>(binSwapped >> 24) | 0x3F;

					auto* uv = reinterpret_cast<uint16_t*>(&vertexBuf[i * vertexStride + 0x18]);

					uv[0] = _byteswap_ushort(uv[0]);
					uv[1] = _byteswap_ushort(uv[1]);

					auto* uv2 = reinterpret_cast<uint16_t*>(&vertexBuf[i * vertexStride + 0x1C]);

					uv2[0] = _byteswap_ushort(uv2[0]);
					uv2[1] = _byteswap_ushort(uv2[1]);

					auto* col = reinterpret_cast<uint32_t*>(&vertexBuf[i * vertexStride + 0x20]);
					*col = _byteswap_ulong(*col);
				}

				//std::cout << " ==== Loading mesh " << name << " ====" << std::endl;
				//std::cout << "  Index count: " << indexCount << std::endl;
				//std::cout << "  Vertex count: " << vertexCount << std::endl;
				//std::cout << "  Vertex stride: " << vertexStride << std::endl;
				//std::cout << "  Vertex buffer size: " << vertexBufSize << std::endl;
				//std::cout << "  Bone indices: " << mesh.get_field<"boneIndexCount">().as_primitive().as<unsigned int>() << std::endl;
				//std::cout << "  Textures: " << mesh.get_field<"textureCount">().as_primitive().as<unsigned int>() << std::endl;

				auto materialIdIt = scene.materialIdsByName.find(name);

				if (materialIdIt == scene.materialIdsByName.end())
					throw std::runtime_error{ std::string{ "Unknown material " } + name };

				auto materialId = materialIdIt->second;

				//std::cout << "  Material ID: " << materialId << std::endl;

				auto indexBuffer = scene.device.createBuffer(
					util::align(indexBufSize, 16),
					16,
					vk::BufferUsageFlagBits2::eIndexBuffer | vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eTransferDst
				);

				auto vertexBuffer = scene.device.createBuffer(
					util::align(vertexBufSize, 16),
					16,
					vk::BufferUsageFlagBits2::eVertexBuffer | vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eTransferDst
				);

				{
					StagingArea staging{ scene.device };

					staging.addBuffer(indexBuffer, indexBuf.data(), indexBufSize);
					staging.addBuffer(vertexBuffer, vertexBuf.data(), vertexBufSize);

					scene.device.runOneTimeCommandBuffer([&](auto& cmd) {
						staging.upload(cmd);
					});
				}

				meshes.emplace_back(std::move(indexBuffer), std::move(vertexBuffer));
				shaderMeshes.emplace_back(
					(uint16_t*)indexBuffer.deviceAddress,
					(uint8_t*)vertexBuffer.deviceAddress,
					materialId,
					shaders::VertexDataReflection{ static_cast<uint8_t>(vertexStride), 0, 0xC, 0x10, 0x14, 0x18, 0x1C, 0x20 }
				);

				return {
					vk::AccelerationStructureGeometryKHR{
						vk::GeometryTypeKHR::eTriangles,
						vk::AccelerationStructureGeometryTrianglesDataKHR{
							vk::Format::eR32G32B32Sfloat,
							vertexBuffer.deviceAddress,
							vertexStride,
							vertexCount - 1,
							vk::IndexType::eUint16,
							indexBuffer.deviceAddress,
						},
						vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation | vk::GeometryFlagBitsKHR::eOpaque,
					},
					vk::AccelerationStructureBuildRangeInfoKHR{
						triangleCount,
					},
				};

				//auto maybeTextures = mesh.get_field<"textures">().as_pointer().get();

				//if (maybeTextures.has_value()) {
				//	auto textures = maybeTextures.value().as_carray();

				//	for (size_t k = 0; k < textures.get_length(); k++) {
				//		auto texture = textures[k].as_pointer().get().value().as_structure();

				//		std::string textureName = texture.get_field<"name">().as_primitive().as<const char*>();

				//		std::cout << "      " << textureName << " - " << static_cast<unsigned int>(texture.get_field<"index">().as_primitive().as<unsigned char>()) << std::endl;
				//	}
				//}
				//std::cout << " ==== Finished loading mesh " << name << " ====" << std::endl;
			}

			void addMeshes(Scene& scene, std::vector<vk::AccelerationStructureGeometryKHR>& geometries, std::vector<vk::AccelerationStructureBuildRangeInfoKHR>& rangeInfos, auto meshes) {
				for (size_t i = 0; i < meshes.get_length(); i++) {
					//model.meshIds.emplace_back((uint32_t)this->meshes.size());

					auto meshResult = addMesh(scene, meshes[i].as_pointer().get().value().as_structure());

					geometries.emplace_back(std::get<vk::AccelerationStructureGeometryKHR>(meshResult));
					rangeInfos.emplace_back(std::get<vk::AccelerationStructureBuildRangeInfoKHR>(meshResult));
				}
			}

			void addMeshGroup(Scene& scene, std::vector<vk::AccelerationStructureGeometryKHR>& geometries, std::vector<vk::AccelerationStructureBuildRangeInfoKHR>& rangeInfos, auto meshGroup) {
				addMeshes(scene, geometries, rangeInfos, meshGroup.get_field<"opaqueMeshes">().as_pointer().get().value().as_carray());
				//addMeshes(scene, geometries, rangeInfos, meshGroup.get_field<"transparentMeshes">().as_pointer().get().value().as_carray());
				//addMeshes(scene, geometries, rangeInfos, meshGroup.get_field<"punchthroughMeshes">().as_pointer().get().value().as_carray());
			}

			AccelerationStructure createBLAS(Scene& scene, auto contexts) {
				std::vector<vk::AccelerationStructureGeometryKHR> geometries{};
				std::vector<vk::AccelerationStructureBuildRangeInfoKHR> rangeInfos{};

				std::cout << "Mesh group count: " << contexts.get_field<"meshGroupCount">().as_primitive().as<unsigned int>() << std::endl;

				auto meshGroups = contexts.get_field<"meshGroups">().as_pointer().get().value().as_carray();

				addMeshGroup(scene, geometries, rangeInfos, meshGroups[0].as_pointer().get().value().as_structure());

				return scene.device.buildBLAS(geometries, rangeInfos);
			}

			Buffer createMeshBuffer(Scene& scene);

			Model(Scene& scene, auto contexts)
				: blas{ createBLAS(scene, contexts) }
				, meshBuffer{ createMeshBuffer(scene) }
			{
			}

			Model(const Model& other) = delete;
			Model(Model&& other);

			Model& operator=(const Model& other) = delete;
			Model& operator=(Model&& other);
		};

		Device& device;

		entt::registry registry{};

		std::vector<TextureStreamingPackage> textureStreamingPackages{};
		std::unordered_map<std::string, uint32_t> textureStreamingPackageIdsByName{};

		std::vector<Texture> textures{};
		std::unordered_map<std::string, uint32_t> textureIdsByName{};

		std::vector<Shader> shaders{};

		std::unordered_map<std::string, MaterialShader> materialShaders{};

		std::vector<Material> materials{};
		std::unordered_map<std::string, uint32_t> materialIdsByName{};
		std::vector<shaders::Material> shaderMaterials{};
		std::optional<Buffer> materialBuffer{};

		std::vector<Model> models{};
		std::unordered_map<std::string, uint32_t> modelIdsByName{};
		std::vector<shaders::Model> shaderModels{};
		std::optional<Buffer> modelBuffer{};

		std::vector<MeshInstance> meshInstances{};

		std::vector<Light> lights{};
		std::vector<LightInstance> lightInstances{};

		std::vector<vk::AccelerationStructureInstanceKHR> tlasInstances{};
		std::optional<AccelerationStructure> tlas{};


		Scene(Device& device);

		void addModelContexts(const std::string& name, auto contexts) {
			if (contexts.get_field<"meshGroups">().as_pointer().get().value().as_carray()[0].as_pointer().get().value().as_structure().get_field<"opaqueMeshCount">().as_primitive().as<unsigned int>() > 0) {
				auto modelId = models.size();
				auto& model = models.emplace_back(*this, contexts);
				shaderModels.emplace_back((shaders::Mesh*)model.meshBuffer.deviceAddress);
				modelIdsByName[name] = modelId;
			}
		}

		template<bool byteswap_offsets = true, bool relative_offsets = false>
		void addModel(const std::string& name, std::istream& stream) {
			rip::binary::containers::mirage::v2::MirageResourceImageReader<unsigned int, byteswap_offsets, relative_offsets> reader{ stream };

			auto rootNode = reader.get_root_node();

			if (rootNode.header.magic == "Model   ") {
				rootNode.for_each_child([&](auto& modelChild) {
					if (modelChild.header.magic == "Contexts") {
						auto& stream = modelChild.get_stream();

						ucsl::reflection::providers::BoundProvider::RootType refl{ ucsl::reflection::providers::simplerfl<ucsl::reflection::game_interfaces::standalone::StandaloneGameInterface, unsigned int>::Type<ucsl::resources::model::v5::reflections::ModelContexts>{} };
						typename rip::accessors::binary_istream<std::remove_reference_t<decltype(stream)>>::template ValueAccessor<decltype(refl)> acc{ stream, refl };

						addModelContexts(name, acc.as_structure());

						std::cout << "Loaded model " << name << std::endl;
						//auto nodes = contexts.get_field<"nodes">().as_pointer().get().value().as_carray();
						//for (size_t i = 0; i < nodes.get_length(); i++) {
						//    auto node = nodes[i].as_pointer().get().value().as_structure();
						//    std::string name = node.get_field<"name">().as_primitive().as<const char*>();
						//    std::cout << "Node: " << name << std::endl;
						//}

					}
					else if (modelChild.header.magic == "Topology") {

					}
					else if (modelChild.header.magic == "UserAABB") {

					}
					else if (modelChild.header.magic == "NodesExt") {

					}
				});
			}
		}

		template<bool byteswap_offsets = true, bool relative_offsets = false>
		void addTerrainModel(const std::string& name, std::istream& stream) {
			rip::binary::containers::mirage::v2::MirageResourceImageReader<unsigned int, byteswap_offsets, relative_offsets> reader{ stream };

			auto rootNode = reader.get_root_node();

			if (rootNode.header.magic == "Model   ") {
				rootNode.for_each_child([&](auto& modelChild) {
					if (modelChild.header.magic == "Contexts") {
						auto& stream = modelChild.get_stream();

						ucsl::reflection::providers::BoundProvider::RootType refl{ ucsl::reflection::providers::simplerfl<ucsl::reflection::game_interfaces::standalone::StandaloneGameInterface, unsigned int>::Type<ucsl::resources::model::v5::reflections::TerrainModelContexts>{} };
						typename rip::accessors::binary_istream<std::remove_reference_t<decltype(stream)>>::template ValueAccessor<decltype(refl)> acc{ stream, refl };

						addModelContexts(name, acc.as_structure());

						std::cout << "Loaded terrain model " << name << std::endl;
						//auto nodes = contexts.get_field<"nodes">().as_pointer().get().value().as_carray();
						//for (size_t i = 0; i < nodes.get_length(); i++) {
						//    auto node = nodes[i].as_pointer().get().value().as_structure();
						//    std::string name = node.get_field<"name">().as_primitive().as<const char*>();
						//    std::cout << "Node: " << name << std::endl;
						//}

					}
					else if (modelChild.header.magic == "Topology") {

					}
					else if (modelChild.header.magic == "UserAABB") {

					}
					else if (modelChild.header.magic == "NodesExt") {

					}
				});
			}
		}

		void addTextureStreamingPackage(const std::string& name, const std::string& filename);
		void addTexture(const std::string& name, const std::string& filename);
		void addMaterial(const std::string& name, std::istream& stream);
		void addMaterial(const std::string& name, const std::string& filename);
		void addMaterialShader(const std::string& name, const uint32_t* data, size_t size, const MaterialShader::Reflection& reflection);
		void addMaterialShader(const std::string& name, std::istream& stream);
		void addModel(const std::string& name, const std::string& filename);
		void addTerrainModel(const std::string& name, const std::string& filename);
		void addModelInstance(const std::string& modelName, const Eigen::Affine3f& transform);
		void addModelInstances(const std::string& filename);
		void addModelInstances(std::istream& stream);

		void generateMaterialBuffer();
		void generateModelBuffer();
		void buildTLAS();
		void finalize();

	private:
		DDSImageCreateInfo readDDSImage(uint8_t* data, size_t size);
		void addTexture(const std::string& name, const DDSImageCreateInfo& ddsImage, uint8_t* data, size_t size);
		void addStandardTexture(const std::string& name, uint8_t* data, size_t size);
		void addStreamingTexture(const std::string& name, uint8_t* data, size_t size);
	};
}
