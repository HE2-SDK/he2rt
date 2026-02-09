#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "../../device.h"
#include "shader-types.h"

namespace he2rt::heightmap::buildas {
	class GenerateMipsShader {
		vk::raii::DescriptorSetLayout descriptorSetLayout;
		vk::raii::PipelineLayout pipelineLayout;
		vk::raii::Pipeline pipeline;

	public:
		GenerateMipsShader(Device& device);
		void run(vk::CommandBuffer& cmd, const vk::Extent2D& extent, const vk::DescriptorImageInfo& in, const std::vector<vk::raii::ImageView>& mipViews, size_t startMip);

	private:
		vk::raii::DescriptorSetLayout createDescriptorSetLayout(Device& device);
		vk::raii::PipelineLayout createPipelineLayout(Device& device);
		vk::raii::Pipeline createPipeline(Device& device);
	};
}
