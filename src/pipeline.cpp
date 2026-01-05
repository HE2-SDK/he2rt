//#include "pipeline.h"
//#include "util.h"
//
//namespace he2rt {
//	Pipeline::Pipeline(vk::raii::PipelineLayout&& pipelineLayout, vk::raii::DescriptorSetLayout&& descriptorSetLayout, vk::raii::Pipeline&& pipeline, uint32_t groupCount)
//		: pipelineLayout{ std::move(pipelineLayout) }
//		, descriptorSetLayout{ std::move(descriptorSetLayout) }
//		, pipeline{ std::move(pipeline) }
//	{
//        auto sbtData = pipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, groups.size(), physicalDevice.rayTracingPipelineProperties.shaderGroupHandleSize * groups.size());
//
//        uint32_t currentOffset{};
//
//        std::array<uint32_t, 3> offsets{};
//        std::array<vk::StridedDeviceAddressRegionKHR, 3> regions{};
//
//        for (size_t i = 0; i < 3; i++) {
//            offsets[i] = currentOffset;
//            currentOffset = util::align(util::align(currentOffset, physicalDevice.rayTracingPipelineProperties.shaderGroupHandleAlignment), physicalDevice.rayTracingPipelineProperties.shaderGroupBaseAlignment);
//        }
//
//        auto sbtBuffer = createBuffer(
//            currentOffset,
//            physicalDevice.rayTracingPipelineProperties.shaderGroupBaseAlignment,
//            vk::BufferUsageFlagBits2::eShaderBindingTableKHR,
//            vma::MemoryUsage::eAutoPreferDevice,
//            vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom
//        );
//
//        uint32_t handleSize = util::align(physicalDevice.rayTracingPipelineProperties.shaderGroupHandleSize, physicalDevice.rayTracingPipelineProperties.shaderGroupHandleAlignment);
//        uint8_t* sbtBuf = (uint8_t*)sbtBuffer.hostAddress;
//
//        for (size_t i = 0; i < 3; i++) {
//            memcpy(sbtBuf + offsets[i], sbtData.data() + i * physicalDevice.rayTracingPipelineProperties.shaderGroupHandleSize, physicalDevice.rayTracingPipelineProperties.shaderGroupHandleSize);
//            regions[i].deviceAddress = sbtBuffer.deviceAddress + offsets[i];
//            regions[i].stride = handleSize;
//            regions[i].size = handleSize;
//        }
//	}
//}
