#include "raytracing-engine.h"
#include "staging-area.h"
#include "raytracing.slang.spv.h"

namespace he2rt {
    RaytracingEngine::RaytracingEngine(Device& device, Scene& scene)
        : device{ device }
        , scene{ scene }
        , staticDescriptorSetLayout{ createStaticDescriptorSetLayout() }
        , pushDescriptorSetLayout{ createPushDescriptorSetLayout() }
        , staticDescriptorSetPool{ createStaticDescriptorSetPool() }
        , staticDescriptorSet{ createStaticDescriptorSet() }
        , pipelineLayout{ createPipelineLayout() }
        , pipeline{ createPipeline() }
        , sbtBuffer{ createSBTBuffer() }
    {
    }

    vk::raii::DescriptorSetLayout RaytracingEngine::createStaticDescriptorSetLayout() {
        std::array<vk::DescriptorSetLayoutBinding, 1> bindings{ {
            { 0u, vk::DescriptorType::eCombinedImageSampler, (uint32_t)scene.textures.size(), vk::ShaderStageFlagBits::eAll },
        } };

        return device.device.createDescriptorSetLayout({ {}, bindings });
    }

    vk::raii::DescriptorSetLayout RaytracingEngine::createPushDescriptorSetLayout() {
        std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ {
            { 0u, vk::DescriptorType::eAccelerationStructureKHR, 1, vk::ShaderStageFlagBits::eAll },
            { 1u, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eAll },
        } };

        return device.device.createDescriptorSetLayout({ vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor, bindings });
    }

    vk::raii::DescriptorPool RaytracingEngine::createStaticDescriptorSetPool() {
        std::array<vk::DescriptorPoolSize, 1> poolSizes{ {
            { vk::DescriptorType::eCombinedImageSampler, (uint32_t)scene.textures.size() },
        } };

        return device.device.createDescriptorPool({ {}, (uint32_t)scene.textures.size(), poolSizes });
    }

    vk::raii::DescriptorSet RaytracingEngine::createStaticDescriptorSet() {
        std::array<vk::DescriptorSetLayout, 1> layouts{ { staticDescriptorSetLayout } };

        auto sets = device.device.allocateDescriptorSets({ staticDescriptorSetPool, layouts });

        std::vector<vk::DescriptorImageInfo> imageInfos{};

        for (uint32_t i = 0; i < scene.textures.size(); i++) {
            imageInfos.emplace_back(scene.textures[i].descriptor);
        }

        std::array<vk::WriteDescriptorSet, 1> writes{ {
            { sets[0], 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfos },
        } };

        device.device.updateDescriptorSets(writes, {});

        return std::move(sets[0]);
    }

    vk::raii::PipelineLayout RaytracingEngine::createPipelineLayout() {
        std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts{ {
            staticDescriptorSetLayout,
            pushDescriptorSetLayout,
        } };

        std::array<vk::PushConstantRange, 1> pushConstantRanges{ {
            { vk::ShaderStageFlagBits::eAll, 0u, sizeof(shaders::PushConstant) },
        } };

        return device.device.createPipelineLayout({ {}, descriptorSetLayouts, pushConstantRanges });
    }

    vk::raii::Pipeline RaytracingEngine::createPipeline() {
        vk::ShaderModuleCreateInfo shader{ {}, raytracing_shader_sizeInBytes, raytracing_shader };
        vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> rgenShader{ { {}, vk::ShaderStageFlagBits::eRaygenKHR, {}, "rgenMain" }, shader };
        vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> rmissShader{ { {}, vk::ShaderStageFlagBits::eMissKHR, {}, "rmissMain" }, shader };
        vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> rmissShadowShader{ { {}, vk::ShaderStageFlagBits::eMissKHR, {}, "rmissShadowMain" }, shader };
        vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> rhitShader{ { {}, vk::ShaderStageFlagBits::eClosestHitKHR, {}, "rchitMain" }, shader };

        std::vector<vk::PipelineShaderStageCreateInfo> stages{ {
            rgenShader.get<vk::PipelineShaderStageCreateInfo>(),
            rmissShader.get<vk::PipelineShaderStageCreateInfo>(),
            rmissShadowShader.get<vk::PipelineShaderStageCreateInfo>(),
            rhitShader.get<vk::PipelineShaderStageCreateInfo>(),
        } };

        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups{ {
            { vk::RayTracingShaderGroupTypeKHR::eGeneral, 0u },
            { vk::RayTracingShaderGroupTypeKHR::eGeneral, 1u },
            { vk::RayTracingShaderGroupTypeKHR::eGeneral, 2u },
            { vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, vk::ShaderUnusedKHR, 3u },
        } };

        std::vector<vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo>> callableShaderCreateInfos{};
        callableShaderCreateInfos.reserve(scene.shaders.size());

        for (auto& shaderDef : scene.shaders) {
            callableShaderCreateInfos.emplace_back(vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eCallableKHR, {}, "main" }, vk::ShaderModuleCreateInfo{ {}, shaderDef.size, shaderDef.code });
        }

        for (auto& shaderStageCreateInfo : callableShaderCreateInfos) {
            stages.push_back(shaderStageCreateInfo.get<vk::PipelineShaderStageCreateInfo>());
            groups.push_back({ vk::RayTracingShaderGroupTypeKHR::eGeneral, (uint32_t)groups.size() });
        }

        return device.device.createRayTracingPipelineKHR(nullptr, nullptr, { {}, stages, groups, 3, {}, {}, {}, pipelineLayout });
    }

    Buffer RaytracingEngine::createSBTBuffer() {
        auto callableShaderCount = scene.shaders.size();

        std::array<size_t, 4> sizes{ { 1, 2, 1, callableShaderCount } };
        std::array<uint32_t, 4> offsets{};
        std::array<vk::StridedDeviceAddressRegionKHR, 4> regions{};

        auto groupCount = std::accumulate(sizes.begin(), sizes.end(), 0ull);
        auto& rtpProps = device.physicalDevice.rayTracingPipelineProperties;
        auto sbtData = pipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, groupCount, rtpProps.shaderGroupHandleSize * groupCount);

        uint32_t alignedHandleSize = util::align(rtpProps.shaderGroupHandleSize, rtpProps.shaderGroupHandleAlignment);

        uint32_t currentOffset{};

        for (size_t i = 0; i < 4; i++) {
            offsets[i] = currentOffset;
            currentOffset = util::align(currentOffset + sizes[i] * alignedHandleSize, rtpProps.shaderGroupBaseAlignment);
        }

        auto buffer = device.createBuffer(
            currentOffset,
            rtpProps.shaderGroupBaseAlignment,
            vk::BufferUsageFlagBits2::eShaderBindingTableKHR,
            vma::MemoryUsage::eAutoPreferDevice,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom
        );

        uint8_t* sbtBuf = (uint8_t*)buffer.hostAddress;

        size_t startHandle{};
        for (size_t i = 0; i < 4; i++) {
            auto offset = offsets[i];
            auto size = sizes[i];

            for (size_t j = 0; j < size; j++) {
                memcpy(sbtBuf + offset + alignedHandleSize * j, sbtData.data() + (startHandle + j) * rtpProps.shaderGroupHandleSize, rtpProps.shaderGroupHandleSize);
            }

            regions[i].deviceAddress = buffer.deviceAddress + offset;
            regions[i].stride = alignedHandleSize;
            regions[i].size = alignedHandleSize * size;

            startHandle += size;
        }

        raygenRegion = regions[0];
        missRegion = regions[1];
        hitRegion = regions[2];
        callableRegion = regions[3];

        std::cout << std::hex << "RAYGEN region: " << raygenRegion.deviceAddress << ", " << raygenRegion.stride << ", " << raygenRegion.size << std::dec << std::endl;
        std::cout << std::hex << "MISS region: " << missRegion.deviceAddress << ", " << missRegion.stride << ", " << missRegion.size << std::dec << std::endl;
        std::cout << std::hex << "HIT region: " << hitRegion.deviceAddress << ", " << hitRegion.stride << ", " << hitRegion.size << std::dec << std::endl;
        std::cout << std::hex << "CALLABLE region: " << callableRegion.deviceAddress << ", " << callableRegion.stride << ", " << callableRegion.size << std::dec << std::endl;

        return buffer;
    }

    void RaytracingEngine::raytrace(vk::CommandBuffer& cmd, vk::ImageView& outputImage, const vk::Extent2D& viewport, const Eigen::Affine3f& viewMatrix) {
        if (viewMatrix.matrix() != lastViewMatrix.matrix()) {
            globalPushConstant.frame = 0;
        }

        lastViewMatrix = viewMatrix;
        globalPushConstant.viewInverse = viewMatrix.inverse().matrix();

        cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline);

        std::array<vk::DescriptorSet, 1> descriptorSets{ { staticDescriptorSet } };
        cmd.bindDescriptorSets2({ vk::ShaderStageFlagBits::eAll, pipelineLayout, 0, descriptorSets });

        std::vector<vk::WriteDescriptorSet> writes{};

        std::array<vk::AccelerationStructureKHR, 1> tlases{ { scene.tlas.value().structure } };
        vk::StructureChain<vk::WriteDescriptorSet, vk::WriteDescriptorSetAccelerationStructureKHR> tlasWrite{
            { {}, 0, 0, 1, vk::DescriptorType::eAccelerationStructureKHR },
            { tlases },
        };
        writes.emplace_back(tlasWrite.get<vk::WriteDescriptorSet>());

        vk::DescriptorImageInfo outImgInfo{ {}, outputImage, vk::ImageLayout::eGeneral };
        vk::WriteDescriptorSet outImgWrite{ {}, 1, 0, 1, vk::DescriptorType::eStorageImage, &outImgInfo };
        writes.emplace_back(outImgWrite);

        cmd.pushDescriptorSet(vk::PipelineBindPoint::eRayTracingKHR, pipelineLayout, 1, writes);

        cmd.pushConstants<shaders::PushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, { globalPushConstant });
        cmd.traceRaysKHR(raygenRegion, missRegion, hitRegion, callableRegion, viewport.width, viewport.height, 1);

        std::array<vk::MemoryBarrier2, 1> memoryBarriers{ {
            vk::MemoryBarrier2{ vk::PipelineStageFlagBits2::eRayTracingShaderKHR, vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderRead }
        } };
        cmd.pipelineBarrier2({ {}, memoryBarriers });

        globalPushConstant.totalFrame++;
        globalPushConstant.frame++;
    }
}
