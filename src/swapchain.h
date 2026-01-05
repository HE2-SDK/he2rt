#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vector>

namespace he2rt {
    class Device;

    struct SwapchainDescription {
        vk::Extent2D extent{};
        vk::SurfaceCapabilitiesKHR capabilities{};
        vk::SurfaceFormatKHR surfaceFormat{};
        vk::PresentModeKHR presentMode{};
    };

	class Swapchain {
        Device& device;
        //std::vector<vk::raii::ImageView> createImageViews();
        std::vector<vk::raii::Semaphore> createImageReadySemaphores();
    public:
        vk::raii::SwapchainKHR swapchain;
        SwapchainDescription description{};
        std::vector<vk::Image> images{};
        std::vector<vk::raii::ImageView> imageViews{};
        std::vector<vk::raii::Semaphore> imageReadySemaphores{};

        Swapchain(Device& device, vk::raii::SwapchainKHR&& swapchain, const SwapchainDescription& description);
	};
}