#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include "acceleration-structures.h"
#include "device.h"
#include "scene.h"
#include "math.h"

namespace he2rt {
	class RaytracingEngine {
		vk::StridedDeviceAddressRegionKHR raygenRegion{};
		vk::StridedDeviceAddressRegionKHR missRegion{};
		vk::StridedDeviceAddressRegionKHR hitRegion{};
		vk::StridedDeviceAddressRegionKHR callableRegion{};

		Device& device;
		Scene& scene;
		vk::raii::DescriptorSetLayout staticDescriptorSetLayout;
		vk::raii::DescriptorSetLayout pushDescriptorSetLayout;
		vk::raii::DescriptorPool staticDescriptorSetPool;
		vk::raii::DescriptorSet staticDescriptorSet;
		vk::raii::PipelineLayout pipelineLayout;
		vk::raii::Pipeline pipeline;
		Buffer sbtBuffer;

		vk::raii::DescriptorSetLayout createStaticDescriptorSetLayout();
		vk::raii::DescriptorSetLayout createPushDescriptorSetLayout();
		vk::raii::DescriptorPool createStaticDescriptorSetPool();
		vk::raii::DescriptorSet createStaticDescriptorSet();
		vk::raii::PipelineLayout createPipelineLayout();
		vk::raii::Pipeline createPipeline();
		Buffer createSBTBuffer();

		Eigen::Affine3f lastViewMatrix{};

	public:
		shaders::PushConstant globalPushConstant{};

		RaytracingEngine(Device& device, Scene& scene);

		void raytrace(vk::CommandBuffer& cmd, vk::ImageView& outputImage, const vk::Extent2D& viewport, const Eigen::Affine3f& viewMatrix);
	};
}
