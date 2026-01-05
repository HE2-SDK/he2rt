#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include "swapchain.h"

namespace he2rt {
    class Instance;

	class PhysicalDevice {
        struct Indices {
            uint32_t graphics{};
            uint32_t present{};
        };


        vk::Extent2D chooseSwapExtent(const vk::Extent2D& desiredExtent, const vk::SurfaceCapabilitiesKHR& capabilities);

	public:
        Instance& instance;
        vk::raii::PhysicalDevice physicalDevice;
        Indices queueIndices{};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};

		PhysicalDevice(Instance& instance, vk::raii::PhysicalDevice&& physicalDevice, const Indices& queueIndices);

        Device createDevice();
        std::optional<SwapchainDescription> findSwapchain(const vk::Extent2D& desiredExtent, vk::SurfaceKHR surface);
	};
}