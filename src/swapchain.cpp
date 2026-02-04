#include "swapchain.h"
#include "device.h"

namespace he2rt {
    Swapchain::Swapchain(Device& device, vk::raii::SwapchainKHR&& swapchain, const SwapchainDescription& description)
        : device{ device }
        , swapchain{ std::move(swapchain) }
        , description { description }
        , images{ this->swapchain.getImages() }
        , imageViews{ createImageViews() }
        , imageReadySemaphores{ createImageReadySemaphores() }
    {
        if (device.physicalDevice.HDR) {
            device.device.setHdrMetadataEXT({ this->swapchain }, { {
                { 0.708f, 0.292f },
                { 0.170f, 0.797f },
                { 0.131f, 0.046f },
                { 0.3127f, 0.3290f },
                1000.0f,
                0.05f,
                1000.0f,
                400.0f,
            } });
        }

        device.runOneTimeCommandBuffer([&](vk::CommandBuffer& cmd) {
            std::vector<vk::ImageMemoryBarrier2> imageBarriers{};

            for (auto& image : images) {
                imageBarriers.emplace_back(vk::ImageMemoryBarrier2{
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    image,
                    { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
                });
            }

            cmd.pipelineBarrier2({ {}, {}, {}, imageBarriers });
        });
	}

    std::vector<vk::raii::ImageView> Swapchain::createImageViews() {
        std::vector<vk::raii::ImageView> result{};
        result.reserve(images.size());

        for (unsigned int i = 0; i < images.size(); i++) {
            result.push_back(device.device.createImageView({
                {},
                images[i],
                vk::ImageViewType::e2D,
                description.surfaceFormat.format,
                { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
            }));
        }

        return result;
    }

    std::vector<vk::raii::Semaphore> Swapchain::createImageReadySemaphores() {
        std::vector<vk::raii::Semaphore> result{};
        result.reserve(images.size());

        for (unsigned int i = 0; i < images.size(); i++) {
            result.emplace_back(device.device.createSemaphore({}));
        }

        return result;
    }
}
