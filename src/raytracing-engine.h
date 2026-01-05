#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include "acceleration-structures.h"
#include "device.h"
#include "scene.h"

namespace he2rt {
	class RaytracingEngine {
		struct PushConstant {
			float unk1{};
			mat34 viewInverse{};
			mat44 projInverse{};
		};

		static constexpr uint32_t groupCount{ 3 };

		Device& device;
		Scene& scene;
		vk::raii::DescriptorSetLayout descriptorSetLayout;
		vk::raii::PipelineLayout pipelineLayout;
		vk::raii::Pipeline pipeline;
		Buffer sbtBuffer;
		vk::StridedDeviceAddressRegionKHR raygenRegion{};
		vk::StridedDeviceAddressRegionKHR missRegion{};
		vk::StridedDeviceAddressRegionKHR hitRegion{};
		vk::StridedDeviceAddressRegionKHR callableRegion{};

		vk::raii::DescriptorSetLayout createDescriptorSetLayout();
		vk::raii::PipelineLayout createPipelineLayout();
		vk::raii::Pipeline createPipeline();
		Buffer createSBTBuffer();

	public:
		PushConstant globalPushConstant{};

		RaytracingEngine(Device& device, Scene& scene);

		void raytrace(vk::CommandBuffer& cmd, vk::ImageView& outputImage, const vk::Extent2D& viewport);
	};
}
