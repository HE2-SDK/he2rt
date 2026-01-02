#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "allocator.h"

namespace he2rt {
	class StagingArea {
		Allocator& allocator;

		struct Item {
			Buffer& buffer;
			Buffer stagingBuffer;
		};

		std::vector<Item> items{};

	public:
		StagingArea(Allocator& allocator);
		void addBuffer(Buffer& buffer, void* data, std::size_t size);
		template<typename T, std::size_t Extent> void addBuffer(Buffer& buffer, std::span<T, Extent> data) {
			addBuffer(buffer, data.data(), data.size_bytes());
		}
		void upload(vk::CommandBuffer cmdBuffer);
	};
}
