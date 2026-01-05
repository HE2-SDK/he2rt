#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "device.h"

namespace he2rt {
	class StagingArea {
		Device& device;

		struct Item {
			Buffer& buffer;
			Buffer stagingBuffer;
			vk::BufferCopy2 bufferCopy;

			Item(Buffer& buffer, Buffer&& stagingBuffer, const vk::BufferCopy2& bufferCopy);
			Item(const Item& other) = delete;
			Item(Item&& other);

			Item& operator=(const Item& other) = delete;
			Item& operator=(Item&& other);
		};

		std::vector<Item> items{};

	public:
		StagingArea(Device& device);
		void addBuffer(Buffer& buffer, void* data, std::size_t size);
		template<typename T, std::size_t Extent> void addBuffer(Buffer& buffer, std::span<T, Extent> data) {
			addBuffer(buffer, data.data(), data.size_bytes());
		}
		void upload(vk::CommandBuffer cmdBuffer);
	};
}
