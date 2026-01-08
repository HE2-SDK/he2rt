#include "raytracing-engine.h"
#include "staging-area.h"
#include "raytracing.slang.spv.h"

namespace he2rt {
    RaytracingEngine::RaytracingEngine(Device& device, Scene& scene)
        : device{ device }
        , scene{ scene }
        , descriptorSetLayout{ createDescriptorSetLayout() }
        , pipelineLayout{ createPipelineLayout() }
        , pipeline{ createPipeline() }
        , sbtBuffer{ createSBTBuffer() }
    {
    }

    vk::raii::DescriptorSetLayout RaytracingEngine::createDescriptorSetLayout() {
        std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ {
            { 0u, vk::DescriptorType::eAccelerationStructureKHR, 1, vk::ShaderStageFlagBits::eAll },
            { 1u, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eAll },
        } };

        return device.device.createDescriptorSetLayout({ vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor, bindings });
    }

    vk::raii::PipelineLayout RaytracingEngine::createPipelineLayout() {
        std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts{ {
            descriptorSetLayout,
        } };

        std::array<vk::PushConstantRange, 1> pushConstantRanges{ {
            { vk::ShaderStageFlagBits::eAll, 0u, sizeof(shaders::PushConstant) },
        } };

        return device.device.createPipelineLayout({ {}, descriptorSetLayouts, pushConstantRanges });
    }

    vk::raii::Pipeline RaytracingEngine::createPipeline() {
        vk::ShaderModuleCreateInfo shader{ {}, data_sizeInBytes, (uint32_t*)data };
        vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> rgenShader{ { {}, vk::ShaderStageFlagBits::eRaygenKHR, {}, "rgenMain" }, shader };
        vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> rmissShader{ { {}, vk::ShaderStageFlagBits::eMissKHR, {}, "rmissMain" }, shader };
        vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> rhitShader{ { {}, vk::ShaderStageFlagBits::eClosestHitKHR, {}, "rchitMain" }, shader };

        std::array<vk::PipelineShaderStageCreateInfo, 3> stages{ {
            rgenShader.get<vk::PipelineShaderStageCreateInfo>(),
            rmissShader.get<vk::PipelineShaderStageCreateInfo>(),
            rhitShader.get<vk::PipelineShaderStageCreateInfo>(),
        } };

        std::array<vk::RayTracingShaderGroupCreateInfoKHR, 3> groups{ {
            { vk::RayTracingShaderGroupTypeKHR::eGeneral, 0u },
            { vk::RayTracingShaderGroupTypeKHR::eGeneral, 1u },
            { vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, vk::ShaderUnusedKHR, 2u },
        } };

        return device.device.createRayTracingPipelineKHR(nullptr, nullptr, { {}, stages, groups, 3u, {}, {}, {}, pipelineLayout });
    }

    Buffer RaytracingEngine::createSBTBuffer() {
        auto& rtpProps = device.physicalDevice.rayTracingPipelineProperties;
        auto sbtData = pipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, groupCount, rtpProps.shaderGroupHandleSize * groupCount);

        uint32_t currentOffset{};

        std::array<uint32_t, groupCount> offsets{};
        std::array<vk::StridedDeviceAddressRegionKHR, groupCount> regions{};

        for (size_t i = 0; i < groupCount; i++) {
            offsets[i] = currentOffset;
            currentOffset = util::align(util::align(currentOffset + rtpProps.shaderGroupHandleSize, rtpProps.shaderGroupHandleAlignment), rtpProps.shaderGroupBaseAlignment);
        }
        
        std::cout << "Handle size: " << std::hex << rtpProps.shaderGroupHandleSize << std::dec << std::endl;
        std::cout << "Handle alignment: " << std::hex << rtpProps.shaderGroupHandleAlignment << std::dec << std::endl;
        std::cout << "Base alignment: " << std::hex << rtpProps.shaderGroupBaseAlignment << std::dec << std::endl;
        std::cout << "Buffer size: " << std::hex << currentOffset << std::dec << std::endl;

        for (uint32_t offset : offsets) {
            std::cout << "Offset: " << std::hex << offset << std::dec << std::endl;
        }

        auto buffer = device.createBuffer(
            currentOffset,
            rtpProps.shaderGroupBaseAlignment,
            vk::BufferUsageFlagBits2::eShaderBindingTableKHR,
            vma::MemoryUsage::eAutoPreferDevice,
            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom
        );

        uint32_t handleSize = util::align(rtpProps.shaderGroupHandleSize, rtpProps.shaderGroupHandleAlignment);
        uint8_t* sbtBuf = (uint8_t*)buffer.hostAddress;

        for (size_t i = 0; i < groupCount; i++) {
            memcpy(sbtBuf + offsets[i], sbtData.data() + i * rtpProps.shaderGroupHandleSize, rtpProps.shaderGroupHandleSize);
            regions[i].deviceAddress = buffer.deviceAddress + offsets[i];
            regions[i].stride = handleSize;
            regions[i].size = handleSize;
        }

        raygenRegion = regions[0];
        missRegion = regions[1];
        hitRegion = regions[2];
        callableRegion.deviceAddress = 0;
        callableRegion.stride = 0;
        callableRegion.size = 0;

        std::cout << std::hex << "RAYGEN region: " << raygenRegion.deviceAddress << ", " << raygenRegion.stride << ", " << raygenRegion.size << std::dec << std::endl;
        std::cout << std::hex << "MISS region: " << missRegion.deviceAddress << ", " << missRegion.stride << ", " << missRegion.size << std::dec << std::endl;
        std::cout << std::hex << "HIT region: " << hitRegion.deviceAddress << ", " << hitRegion.stride << ", " << hitRegion.size << std::dec << std::endl;
        std::cout << std::hex << "CALLABLE region: " << callableRegion.deviceAddress << ", " << callableRegion.stride << ", " << callableRegion.size << std::dec << std::endl;

        return buffer;
    }

    void RaytracingEngine::raytrace(vk::CommandBuffer& cmd, vk::ImageView& outputImage, const vk::Extent2D& viewport) {
        Eigen::Matrix4f view{ Eigen::Matrix4f::Identity() };

        globalPushConstant.viewInverse = view; // view.inverse().matrix();

        cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline);
        //cmd.bindDescriptorSets2({ vk::ShaderStageFlagBits::eAll, pipelineLayout, 0, 1, WQEFVEWQFEW });

        std::array<vk::AccelerationStructureKHR, 1> tlases{ { scene.tlas.value().structure } };
        vk::StructureChain<vk::WriteDescriptorSet, vk::WriteDescriptorSetAccelerationStructureKHR> tlasWrite{
            { {}, 0, 0, 1, vk::DescriptorType::eAccelerationStructureKHR },
            { tlases },
        };

        vk::DescriptorImageInfo outImgInfo{ {}, outputImage, vk::ImageLayout::eGeneral };
        vk::WriteDescriptorSet outImgWrite{ {}, 1, 0, 1, vk::DescriptorType::eStorageImage, &outImgInfo };

        std::array<const vk::WriteDescriptorSet, 2> writes{ { tlasWrite.get<vk::WriteDescriptorSet>(), outImgWrite } };
        cmd.pushDescriptorSet(vk::PipelineBindPoint::eRayTracingKHR, pipelineLayout, 0, writes);

        cmd.pushConstants<shaders::PushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, { globalPushConstant });
        cmd.traceRaysKHR(raygenRegion, missRegion, hitRegion, callableRegion, viewport.width, viewport.height, 1);

        std::array<vk::MemoryBarrier2, 1> memoryBarriers{ {
            vk::MemoryBarrier2{ vk::PipelineStageFlagBits2::eRayTracingShaderKHR, vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderRead }
        } };
        cmd.pipelineBarrier2({ {}, memoryBarriers });
    }
}
