#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <optional>
#include "physical-device.h"
#include "device.h"

namespace he2rt {
	class Instance {

		static vk::raii::Instance createInstance(vk::raii::Context& context);

#ifdef _DEBUG
		static vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT messageType,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
		);
#endif

	public:
		vk::raii::Instance instance;
#ifdef _DEBUG
		vk::raii::DebugUtilsMessengerEXT debugMessenger;
#endif

		Instance(vk::raii::Context& context);

		std::optional<PhysicalDevice> findPhysicalDevice(const vk::Extent2D& desiredExtent, vk::SurfaceKHR surface, bool HDR = false);
	};
}