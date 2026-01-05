#include "swapchain.h"
#include "device.h"

namespace he2rt {
    Swapchain::Swapchain(Device& device, vk::raii::SwapchainKHR&& swapchain, const SwapchainDescription& description)
        : device{ device }
        , swapchain{ std::move(swapchain) }
        , description { description }
        , images{ this->swapchain.getImages() }
        //, imageViews{ createImageViews() }
        , imageReadySemaphores{ createImageReadySemaphores() }
    {
	}

    //std::vector<vk::raii::ImageView> Swapchain::createImageViews() {
    //    std::vector<vk::raii::ImageView> result{};
    //    result.reserve(images.size());

    //    for (unsigned int i = 0; i < images.size(); i++) {
    //        result.push_back(device.device.createImageView({
    //            {},
    //            images[i],
    //            vk::ImageViewType::e2D,
    //            description.surfaceFormat.format,
    //            { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
    //            { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
    //        }));
    //    }

    //    return result;
    //}

    std::vector<vk::raii::Semaphore> Swapchain::createImageReadySemaphores() {
        std::vector<vk::raii::Semaphore> result{};
        result.reserve(images.size());

        for (unsigned int i = 0; i < images.size(); i++) {
            result.emplace_back(device.device.createSemaphore({}));
        }

        return result;
    }
}
