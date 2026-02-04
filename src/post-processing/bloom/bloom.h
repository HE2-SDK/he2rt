#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "../../buffer.h"
#include "../../math.h"
#include "../../device.h"
#include "shader-types.h"

namespace he2rt::shaders::bloom {
	class Bloom {
		Image scratchImage;
		vk::raii::ImageView scratchImageView;
		Sampler inputSampler;
		vk::raii::DescriptorSetLayout descriptorSetLayout;
		vk::raii::PipelineLayout pipelineLayout;
		vk::raii::Pipeline tonemapPipeline;
		vk::raii::Pipeline histogramPipeline;
		vk::raii::Pipeline autoExposurePipeline;
		bool firstRun{ true };

	public:
		Config config{};

		Bloom(Device& device);

		void run(vk::CommandBuffer& cmd, const vk::Extent2D& extent, const vk::DescriptorImageInfo& in, const vk::DescriptorImageInfo& out);

	private:
		void runClearHistogram(vk::CommandBuffer& cmd);
		void runAutoExposureHistogram(vk::CommandBuffer& cmd, const vk::Extent2D& extent);
		void runAutoExposure(vk::CommandBuffer& cmd);
		void runBloom(vk::CommandBuffer& cmd, const vk::Extent2D& extent);

		Image createScratchImage(Device& device);
		vk::raii::ImageView createScratchImageView(Device& device);
		vk::raii::DescriptorSetLayout createDescriptorSetLayout(Device& device);
		vk::raii::PipelineLayout createPipelineLayout(Device& device);
		vk::raii::Pipeline createTonemapPipeline(Device& device);
		vk::raii::Pipeline createHistogramPipeline(Device& device);
		vk::raii::Pipeline createAutoExposurePipeline(Device& device);
	};
}
