#pragma once
#include <vk_mem_alloc.hpp>
#include <vk_mem_alloc_raii.hpp>
#include "buffer.h"

namespace he2rt {
	class Allocator {
		vma::raii::Allocator allocator;

	public:
		Allocator(vma::raii::Allocator&& allocator);
		Buffer createBuffer(vk::DeviceSize size, vk::DeviceSize alignment, vk::BufferUsageFlags2 bufferUsage, vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto, vma::AllocationCreateFlags allocationCreateFlags = {}, vk::MemoryPropertyFlags memoryPropertyRequiredFlags = {});
		Buffer createBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const vma::AllocationCreateInfo& allocationCreateInfo, vk::DeviceSize alignment);
	};
}
