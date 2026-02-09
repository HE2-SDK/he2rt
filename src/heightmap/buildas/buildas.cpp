#include "buildas.h"
#include "../../util.h"
#include <buildas.slang.spv.h>

namespace he2rt::heightmap::buildas {

	// Combiner
	BuildAS::BuildAS(Device& device)
		: initializeShader{ device }
		, generateMipsShader{ device }
	{
	}

	Image BuildAS::run(Device& device, const vk::Extent2D& extent, const vk::DescriptorImageInfo& in) {
		auto image = device.createImage({
			{},
			vk::ImageType::e2D,
			vk::Format::eR32G32Sfloat,
			{ extent.width, extent.height, 1 },
			2 * MIP_COUNT + 1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive,
			{},
			vk::ImageLayout::eUndefined,
		});

		std::vector<vk::raii::ImageView> mipViews{};
		for (uint32_t i = 0; i < 2 * MIP_COUNT + 1; i++) {
			auto& imageView = mipViews.emplace_back(device.device.createImageView({
				{},
				image.image,
				vk::ImageViewType::e2D,
				vk::Format::eR32G32Sfloat,
				{ vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
				{ vk::ImageAspectFlagBits::eColor, i, 1, 0, 1 },
			}));
		}

		device.runOneTimeCommandBuffer([&](vk::CommandBuffer& cmd) {
			std::array<vk::ImageMemoryBarrier2, 1> imageBarrierInitialize{ {
				{
					vk::PipelineStageFlagBits2::eNone,
					vk::AccessFlagBits2::eNone,
					vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eAllTransfer | vk::PipelineStageFlagBits2::eAllCommands,
					vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
					vk::ImageLayout::eUndefined,
					vk::ImageLayout::eGeneral,
					vk::QueueFamilyIgnored,
					vk::QueueFamilyIgnored,
					image.image,
					vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 2 * MIP_COUNT + 1, 0, 1 },
				},
			} };
			cmd.pipelineBarrier2({ {}, {}, {}, imageBarrierInitialize });

			initializeShader.run(cmd, extent, in, mipViews);

			std::array<vk::ImageMemoryBarrier2, 1> imageBarrierAfterCopyFirst{ {
				{
					vk::PipelineStageFlagBits2::eComputeShader,
					vk::AccessFlagBits2::eShaderWrite,
					vk::PipelineStageFlagBits2::eComputeShader,
					vk::AccessFlagBits2::eShaderRead,
					vk::ImageLayout::eGeneral,
					vk::ImageLayout::eGeneral,
					vk::QueueFamilyIgnored,
					vk::QueueFamilyIgnored,
					image.image,
					vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
				},
			} };
			cmd.pipelineBarrier2({ {}, {}, {}, imageBarrierAfterCopyFirst });

			// first pass (mips 0 - 4)
			generateMipsShader.run(cmd, { extent.width, extent.height }, { vk::Sampler{}, mipViews[0], vk::ImageLayout::eGeneral }, mipViews, 1);

			// barrier on the 4th mip because we will use it as a source image
			std::array<vk::ImageMemoryBarrier2, 1> imageMemoryBarriers{ {
				vk::ImageMemoryBarrier2{
					vk::PipelineStageFlagBits2::eComputeShader,
					vk::AccessFlagBits2::eShaderWrite,
					vk::PipelineStageFlagBits2::eComputeShader,
					vk::AccessFlagBits2::eShaderRead,
					vk::ImageLayout::eGeneral,
					vk::ImageLayout::eGeneral,
					vk::QueueFamilyIgnored,
					vk::QueueFamilyIgnored,
					image.image,
					vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 5, 1, 0, 1 },
				},
			} };
			cmd.pipelineBarrier2({ {}, {}, {}, imageMemoryBarriers });

			// second pass (mips 5 - 9)
			generateMipsShader.run(cmd, { extent.width >> 5, extent.height >> 5 }, { vk::Sampler{}, mipViews[5], vk::ImageLayout::eGeneral }, mipViews, 6);
		});

		return std::move(image);
	}
}
