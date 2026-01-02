#include "allocator.h"
#include "device.h"

namespace he2rt {
	Allocator::Allocator(vma::raii::Allocator&& allocator) : allocator{ std::move(allocator) } {
	}

	Buffer Allocator::createBuffer(vk::DeviceSize size, vk::DeviceSize alignment, vk::BufferUsageFlags2 bufferUsage, vma::MemoryUsage memoryUsage, vma::AllocationCreateFlags allocationCreateFlags, vk::MemoryPropertyFlags memoryPropertyRequiredFlags) {
		vk::BufferCreateInfo createInfo{
			{},
			size,
			{},
			vk::SharingMode::eExclusive,
			{},
		};
		vk::BufferUsageFlags2CreateInfo usageCreateInfo{ bufferUsage };
		
		auto createInfoChain = vk::StructureChain{ createInfo, usageCreateInfo };

		return createBuffer(createInfoChain.get<vk::BufferCreateInfo>(), { allocationCreateFlags, memoryUsage, memoryPropertyRequiredFlags }, alignment);
	}

	Buffer Allocator::createBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const vma::AllocationCreateInfo& allocationCreateInfo, vk::DeviceSize alignment) {
		return { allocator.createBufferWithAlignment(bufferCreateInfo, allocationCreateInfo, alignment), bufferCreateInfo.size };
	}
}
