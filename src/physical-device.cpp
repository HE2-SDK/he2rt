#include "physical-device.h"
#include <set>
#include "device.h"

namespace he2rt {
    PhysicalDevice::PhysicalDevice(Instance& instance, vk::raii::PhysicalDevice&& physicalDevice, const Indices& queueIndices) : instance{ instance }, physicalDevice { std::move(physicalDevice) }, queueIndices{ queueIndices } {
	}

    Device PhysicalDevice::createDevice() {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
        std::set<uint32_t> uniqueQueueFamilies = { queueIndices.graphics, queueIndices.present };

        float queuePriority = 1.0f;

        for (uint32_t idx : uniqueQueueFamilies) {
            queueCreateInfos.push_back({ {}, idx, 1, &queuePriority });
        }

        std::vector<const char*> extensions{
            vk::KHRSwapchainExtensionName,
            vk::KHRRayTracingPipelineExtensionName,
            vk::KHRSpirv14ExtensionName,
            vk::KHRShaderFloatControlsExtensionName,
            vk::KHRAccelerationStructureExtensionName,
            vk::EXTDescriptorIndexingExtensionName,
            vk::KHRBufferDeviceAddressExtensionName,
            vk::KHRDeferredHostOperationsExtensionName,
        };

        vk::PhysicalDeviceFeatures deviceFeatures{};
        vk::DeviceCreateInfo createInfo{
            {},
            (uint32_t)queueCreateInfos.size(),
            queueCreateInfos.data(),
            0,
            nullptr,
            (uint32_t)extensions.size(),
            extensions.data(),
        };

        return { *this, { physicalDevice, createInfo } };
    }

    std::optional<SwapchainDescription> PhysicalDevice::findSwapchain(const vk::Extent2D& desiredExtent, vk::SurfaceKHR surface) {
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
        auto surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

        auto extent = chooseSwapExtent(desiredExtent, surfaceCapabilities);

        std::optional<vk::SurfaceFormatKHR> selectedFormat{};
        for (auto& format : surfaceFormats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                selectedFormat = format;
            }
        }

        std::optional<vk::PresentModeKHR> selectedPresentMode{};
        for (auto& presentMode : surfacePresentModes) {
            if (presentMode == vk::PresentModeKHR::eMailbox) {
                selectedPresentMode = presentMode;
            }
        }

        if (!selectedPresentMode.has_value()) {
            selectedPresentMode = vk::PresentModeKHR::eFifo;
        }

        if (selectedFormat.has_value()) {
            return SwapchainDescription{
                .extent = extent,
                .capabilities = surfaceCapabilities,
                .surfaceFormat = selectedFormat.value(),
                .presentMode = selectedPresentMode.value(),
            };
        }

        return std::nullopt;
    }

    vk::Extent2D PhysicalDevice::chooseSwapExtent(const vk::Extent2D& desiredExtent, const vk::SurfaceCapabilitiesKHR& capabilities) {
        return capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()
            ? capabilities.currentExtent
            : vk::Extent2D{
                std::clamp(desiredExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp(desiredExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
        };
    }
}