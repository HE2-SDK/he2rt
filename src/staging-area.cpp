#include "staging-area.h"

namespace he2rt {
	StagingArea::Item::Item(Buffer& buffer, Buffer&& stagingBuffer, const vk::BufferCopy2& bufferCopy) : buffer{ buffer }, stagingBuffer{ std::move(stagingBuffer) }, bufferCopy{ bufferCopy } {
	}

	StagingArea::Item::Item(Item&& other) : buffer{ other.buffer }, stagingBuffer{ std::move(other.stagingBuffer) }, bufferCopy{ std::move(other.bufferCopy) } {
	}

	StagingArea::Item& StagingArea::Item::operator=(Item&& other) {
		buffer = std::move(other.buffer);
		stagingBuffer = std::move(other.stagingBuffer);
		bufferCopy = std::move(other.bufferCopy);
		return *this;
	}

	StagingArea::StagingArea(Device& device)
		: device{ device }
	{
	}

	void StagingArea::addBuffer(Buffer& buffer, void* data, size_t size) {
		assert(size <= buffer.size);

		auto stagingBuffer = device.createBuffer(
			buffer.size,
			16,
			vk::BufferUsageFlagBits2::eTransferSrc | vk::BufferUsageFlagBits2::eTransferDst | vk::BufferUsageFlagBits2::eShaderDeviceAddress,
			vma::MemoryUsage::eAutoPreferHost,
			vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
			vk::MemoryPropertyFlagBits::eHostCoherent
		);

		memcpy(stagingBuffer.hostAddress, data, size);

		items.emplace_back(buffer, std::move(stagingBuffer), vk::BufferCopy2{0, 0, buffer.size});
	}

	void StagingArea::upload(vk::CommandBuffer cmdBuffer) {
		for (auto& item : items) {
			cmdBuffer.copyBuffer2({ item.stagingBuffer.buffer, item.buffer.buffer, 1, &item.bufferCopy });
		}
	}
}
