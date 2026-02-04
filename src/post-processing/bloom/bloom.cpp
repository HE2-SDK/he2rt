#include "bloom.h"
#include <bloom.slang.spv.h>

namespace he2rt::shaders::bloom {
	Bloom::Bloom(Device& device)
		: histogramBuffer{ device.createBuffer<uint32_t[EXPOSURE_HISTOGRAM_SIZE]>(vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eTransferSrc | vk::BufferUsageFlagBits2::eTransferDst) }
		, luminanceBuffer{ device.createBuffer<float>(vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eTransferSrc | vk::BufferUsageFlagBits2::eTransferDst) }
		, descriptorSetLayout{ createDescriptorSetLayout(device) }
		, pipelineLayout{ createPipelineLayout(device) }
		, tonemapPipeline{ createTonemapPipeline(device) }
		, histogramPipeline{ createHistogramPipeline(device) }
		, autoExposurePipeline{ createAutoExposurePipeline(device) }
	{
	}

	void Bloom::run(vk::CommandBuffer& cmd, const vk::Extent2D& extent, const vk::DescriptorImageInfo& in, const vk::DescriptorImageInfo& out) {
		cmd.pushConstants<Config>(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { config });

		vk::DescriptorBufferInfo histogramInfo{ histogramBuffer.buffer, 0, sizeof(uint32_t) * EXPOSURE_HISTOGRAM_SIZE };
		vk::DescriptorBufferInfo luminanceInfo{ luminanceBuffer.buffer, 0, sizeof(float) };

		std::array<vk::WriteDescriptorSet, 4> descriptorSets{ {
			{ {}, Binding::input, 0, 1, vk::DescriptorType::eCombinedImageSampler, &in },
			{ {}, Binding::output, 0, 1, vk::DescriptorType::eStorageImage, &out },
			{ {}, Binding::histogram, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &histogramInfo },
			{ {}, Binding::luminance, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &luminanceInfo },
		} };
		cmd.pushDescriptorSet(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSets);

		if (config.autoExposure) {
			if (firstRun) {
				runClearHistogram(cmd);
				firstRun = false;
			}

			runAutoExposureHistogram(cmd, extent);
			runAutoExposure(cmd);
		}

		runBloom(cmd, extent);
	}

	void Bloom::runClearHistogram(vk::CommandBuffer& cmd) {
		std::array<uint32_t, EXPOSURE_HISTOGRAM_SIZE> histogramData{ 0 };
		cmd.updateBuffer(histogramBuffer.buffer, 0, sizeof(uint32_t[EXPOSURE_HISTOGRAM_SIZE]), histogramData.data());

		std::array<vk::BufferMemoryBarrier2, 1> bufferMemoryBarriers{ {
			{ vk::PipelineStageFlagBits2::eClear, vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite, vk::QueueFamilyIgnored, vk::QueueFamilyIgnored, histogramBuffer.buffer, 0, sizeof(uint32_t[EXPOSURE_HISTOGRAM_SIZE]) },
		} };
		cmd.pipelineBarrier2({ {}, {}, bufferMemoryBarriers });
	}

	void Bloom::runAutoExposureHistogram(vk::CommandBuffer& cmd, const vk::Extent2D& extent) {
		vk::Extent2D groupSize{ (extent.width + (BLOOM_WORKGROUP_SIZE - 1)) / BLOOM_WORKGROUP_SIZE, (extent.height + (BLOOM_WORKGROUP_SIZE - 1)) / BLOOM_WORKGROUP_SIZE };

		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, histogramPipeline);
		cmd.dispatch(groupSize.width, groupSize.height, 1);

		std::array<vk::BufferMemoryBarrier2, 1> bufferMemoryBarriers{ {
			{ vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageWrite, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageRead, vk::QueueFamilyIgnored, vk::QueueFamilyIgnored, histogramBuffer.buffer, 0, sizeof(uint32_t[EXPOSURE_HISTOGRAM_SIZE]) },
		} };
		cmd.pipelineBarrier2({ {}, {}, bufferMemoryBarriers });
	}

	void Bloom::runAutoExposure(vk::CommandBuffer& cmd) {
		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, autoExposurePipeline);
		cmd.dispatch(1, 1, 1);
	
		std::array<vk::BufferMemoryBarrier2, 1> bufferMemoryBarriers{ {
			{ vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageWrite, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderStorageRead, vk::QueueFamilyIgnored, vk::QueueFamilyIgnored, luminanceBuffer.buffer, 0, sizeof(float) },
		} };
		cmd.pipelineBarrier2({ {}, {}, bufferMemoryBarriers });
	}

	void Bloom::runBloom(vk::CommandBuffer& cmd, const vk::Extent2D& extent) {
		vk::Extent2D groupSize{ (extent.width + (BLOOM_WORKGROUP_SIZE - 1)) / BLOOM_WORKGROUP_SIZE, (extent.height + (BLOOM_WORKGROUP_SIZE - 1)) / BLOOM_WORKGROUP_SIZE };

		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, tonemapPipeline);
		cmd.dispatch(groupSize.width, groupSize.height, 1);
	}

	Image Bloom::createScratchImage(Device& device) {
		return device.createImage({
			{},
			vk::ImageType::e2D,
			vk::Format::eR32G32B32Sfloat,
			{ extent.width, extent.height, 1 },
			5,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
			vk::SharingMode::eExclusive,
			{},
			{},
			vk::ImageLayout::eGeneral,
		});
	}

	vk::raii::DescriptorSetLayout Bloom::createDescriptorSetLayout(Device& device) {
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ {
			{ Binding::input, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
			{ Binding::output, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
		} };
		
		return device.device.createDescriptorSetLayout({ vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor, bindings });
	}

	vk::raii::PipelineLayout Bloom::createPipelineLayout(Device& device) {
		std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts{ {
			descriptorSetLayout,
		} };
		std::array<vk::PushConstantRange, 1> pushConstantRanges{ {
			{ vk::ShaderStageFlagBits::eCompute, 0, sizeof(Config) },
		} };

		return device.device.createPipelineLayout({ {}, descriptorSetLayouts, pushConstantRanges });
	}

	vk::raii::Pipeline Bloom::createTonemapPipeline(Device& device) {
		vk::ShaderModuleCreateInfo shaderCreateInfo{ {}, tonemapper_shader_sizeInBytes, (uint32_t*)tonemapper_shader };
		vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> tonemapPipeline{ { {}, vk::ShaderStageFlagBits::eCompute, {}, "Tonemap" }, shaderCreateInfo };

		return device.device.createComputePipeline(nullptr, { {}, tonemapPipeline.get<vk::PipelineShaderStageCreateInfo>(), pipelineLayout });
	}

	vk::raii::Pipeline Bloom::createHistogramPipeline(Device& device) {
		vk::ShaderModuleCreateInfo shaderCreateInfo{ {}, tonemapper_shader_sizeInBytes, (uint32_t*)tonemapper_shader };
		vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> histogramPipeline{ { {}, vk::ShaderStageFlagBits::eCompute, {}, "Histogram" }, shaderCreateInfo };

		return device.device.createComputePipeline(nullptr, { {}, histogramPipeline.get<vk::PipelineShaderStageCreateInfo>(), pipelineLayout });
	}

	vk::raii::Pipeline Bloom::createAutoExposurePipeline(Device& device) {
		vk::ShaderModuleCreateInfo shaderCreateInfo{ {}, tonemapper_shader_sizeInBytes, (uint32_t*)tonemapper_shader };
		vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> autoExposurePipeline{ { {}, vk::ShaderStageFlagBits::eCompute, {}, "AutoExposure" }, shaderCreateInfo };

		return device.device.createComputePipeline(nullptr, { {}, autoExposurePipeline.get<vk::PipelineShaderStageCreateInfo>(), pipelineLayout });
	}
}
