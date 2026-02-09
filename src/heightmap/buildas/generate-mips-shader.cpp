#include "generate-mips-shader.h"
#include "../../util.h"
#include <buildasgeneratemips.slang.spv.h>

namespace he2rt::heightmap::buildas {
	// Mips shader
	GenerateMipsShader::GenerateMipsShader(Device& device)
		: descriptorSetLayout{ createDescriptorSetLayout(device) }
		, pipelineLayout{ createPipelineLayout(device) }
		, pipeline{ createPipeline(device) }
	{
	}

	void GenerateMipsShader::run(vk::CommandBuffer& cmd, const vk::Extent2D& extent, const vk::DescriptorImageInfo& in, const std::vector<vk::raii::ImageView>& mipViews, size_t startMip) {
		std::vector<vk::DescriptorImageInfo> mipDbis{};

		for (uint32_t i = 0; i < MIP_COUNT; i++) {
			mipDbis.emplace_back(vk::Sampler{}, mipViews[startMip + i], vk::ImageLayout::eGeneral);
		}

		vk::Extent3D groupCounts{ util::getGroupCounts(extent, WORKGROUP_SIZE) };

		std::array<vk::WriteDescriptorSet, 2> descriptorSets{ {
			{ {}, Binding::input, 0, vk::DescriptorType::eSampledImage, { in } },
			{ {}, Binding::output, 0, vk::DescriptorType::eStorageImage, mipDbis },
		} };
		cmd.pushDescriptorSet(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSets);

		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
		cmd.dispatch(groupCounts.width, groupCounts.height, groupCounts.depth);
	}

	vk::raii::DescriptorSetLayout GenerateMipsShader::createDescriptorSetLayout(Device& device) {
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ {
			{ Binding::input, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eCompute },
			{ Binding::output, vk::DescriptorType::eStorageImage, 5, vk::ShaderStageFlagBits::eCompute },
		} };

		return device.device.createDescriptorSetLayout({ vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor, bindings });
	}

	vk::raii::PipelineLayout GenerateMipsShader::createPipelineLayout(Device& device) {
		std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts{ {
			descriptorSetLayout,
		} };
		std::array<vk::PushConstantRange, 0> pushConstantRanges{ {
		} };

		return device.device.createPipelineLayout({ {}, descriptorSetLayouts, pushConstantRanges });
	}

	vk::raii::Pipeline GenerateMipsShader::createPipeline(Device& device) {
		vk::ShaderModuleCreateInfo shaderCreateInfo{ {}, buildasgeneratemips_shader_sizeInBytes, (uint32_t*)buildasgeneratemips_shader };
		vk::StructureChain<vk::PipelineShaderStageCreateInfo, vk::ShaderModuleCreateInfo> tonemapPipeline{ { {}, vk::ShaderStageFlagBits::eCompute, {}, "main" }, shaderCreateInfo };

		return device.device.createComputePipeline(nullptr, { {}, tonemapPipeline.get<vk::PipelineShaderStageCreateInfo>(), pipelineLayout });
	}
}
