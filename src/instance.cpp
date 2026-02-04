#pragma once
#include "instance.h"
#include <iostream>

namespace he2rt {
    Instance::Instance(vk::raii::Context& context)
        : instance{ createInstance(context) }
#ifdef _DEBUG
        , debugMessenger{ instance, createDebugUtilsMessengerCreateInfo() }
#endif
    {}

	vk::raii::Instance Instance::createInstance(vk::raii::Context& context) {
        uint32_t glfwExtensionCount;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> layers{};
        std::vector<const char*> extensions{ glfwExtensions, glfwExtensions + glfwExtensionCount };

#ifdef _DEBUG
        layers.push_back("VK_LAYER_KHRONOS_validation");
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
#endif
        extensions.push_back(vk::EXTSwapchainColorSpaceExtensionName);

        vk::ApplicationInfo appInfo{
            "he2rt",
            0,
            "he2rt",
            0,
            VK_API_VERSION_1_4,
        };
        vk::InstanceCreateInfo createInfo{
            {},
            &appInfo,
            layers,
            extensions,
        };

#ifdef _DEBUG
        vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = createDebugUtilsMessengerCreateInfo();
#endif

        vk::StructureChain<
            vk::InstanceCreateInfo
#ifdef _DEBUG
            , vk::DebugUtilsMessengerCreateInfoEXT
#endif
        > createInfoChain{
            createInfo
#ifdef _DEBUG
            , debugMessengerCreateInfo
#endif
        };

        return { context, createInfoChain.get<vk::InstanceCreateInfo>() };
	}

#ifdef _DEBUG
    vk::DebugUtilsMessengerCreateInfoEXT Instance::createDebugUtilsMessengerCreateInfo() {
        return {
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback,
            nullptr,
        };
    }
#endif

    std::optional<PhysicalDevice> Instance::findPhysicalDevice(const vk::Extent2D& desiredExtent, vk::SurfaceKHR surface, bool HDR) {
        for (auto& physicalDevice : instance.enumeratePhysicalDevices()) {
            auto properties = physicalDevice.getProperties();
            auto features = physicalDevice.getFeatures();

            if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
                continue;

            auto extensions = physicalDevice.enumerateDeviceExtensionProperties();

            std::vector<const char*> requiredExtensions{
                vk::KHRSwapchainExtensionName,
                vk::KHRRayTracingPipelineExtensionName,
                vk::KHRSpirv14ExtensionName,
                vk::KHRAccelerationStructureExtensionName,
                vk::KHRBufferDeviceAddressExtensionName,
                vk::NVRayTracingInvocationReorderExtensionName,
#ifdef _DEBUG
                vk::NVDeviceDiagnosticCheckpointsExtensionName,
                vk::NVDeviceDiagnosticsConfigExtensionName,
#endif
            };

            if (HDR) {
                requiredExtensions.emplace_back(vk::EXTHdrMetadataExtensionName);
            }

            if (std::any_of(requiredExtensions.begin(), requiredExtensions.end(), [&](auto* extensionName) {
                return !std::any_of(extensions.begin(), extensions.end(), [extensionName](auto& extension) { return !strcmp(extension.extensionName, extensionName); });
            }))
                continue;

            auto features2 = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features>();

            if (features2.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress != vk::True)
                continue;

            auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

            std::optional<uint32_t> graphicsQueueFamilyIdx{};
            std::optional<uint32_t> presentQueueFamilyIdx{};

            for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
                if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                    graphicsQueueFamilyIdx = i;
                }

                if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
                    presentQueueFamilyIdx = i;
                }
            }

            if (graphicsQueueFamilyIdx.has_value() && presentQueueFamilyIdx.has_value()) {
                PhysicalDevice pDev{ *this, std::move(physicalDevice), { graphicsQueueFamilyIdx.value(), presentQueueFamilyIdx.value() }, HDR };

                if (pDev.findSwapchain(desiredExtent, surface).has_value()) {
                    return pDev;
                }
            }
        }

        return std::nullopt;
    }

#ifdef _DEBUG
    VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
#endif
}
