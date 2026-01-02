#include "staging-area.h"

namespace he2rt {
	StagingArea::StagingArea(Allocator& allocator)
		: allocator{ allocator }
	{
	}

	void StagingArea::addBuffer(Buffer& buffer, void* data, size_t size) {
		assert(size <= buffer.size);

		items.emplace_back(buffer, allocator.createBuffer(
			buffer.size,
			0,
			vk::BufferUsageFlagBits2::eTransferSrc | vk::BufferUsageFlagBits2::eTransferDst | vk::BufferUsageFlagBits2::eShaderDeviceAddress,
			vma::MemoryUsage::eAutoPreferHost,
			vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
			vk::MemoryPropertyFlagBits::eHostCoherent
		));
	}

	void StagingArea::upload(vk::CommandBuffer cmdBuffer) {
		for (auto& item : items) {
			cmdBuffer.copyBuffer2({ item.stagingBuffer.buffer, item.buffer.buffer });
		}
	}
}
