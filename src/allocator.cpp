//#include "allocator.h"
//#include "device.h"
//
//namespace he2rt {
//	Allocator::Allocator(vma::raii::Allocator&& allocator, Device& device) : allocator{ std::move(allocator) }, device{ device } {
//	}
//
//	Buffer Allocator::createBuffer(vk::DeviceSize size, vk::DeviceSize alignment, vk::BufferUsageFlags2 bufferUsage, vma::MemoryUsage memoryUsage, vma::AllocationCreateFlags allocationCreateFlags, vk::MemoryPropertyFlags memoryPropertyRequiredFlags) {
//		vk::BufferCreateInfo createInfo{
//			{},
//			size,
//			{},
//			vk::SharingMode::eExclusive,
//			{},
//		};
//		vk::BufferUsageFlags2CreateInfo usageCreateInfo{ bufferUsage | vk::BufferUsageFlagBits2::eShaderDeviceAddress };
//		
//		auto createInfoChain = vk::StructureChain{ createInfo, usageCreateInfo };
//
//		return createBuffer(createInfoChain.get<vk::BufferCreateInfo>(), { allocationCreateFlags, memoryUsage, memoryPropertyRequiredFlags }, alignment);
//	}
//
//	Buffer Allocator::createBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const vma::AllocationCreateInfo& allocationCreateInfo, vk::DeviceSize alignment) {
//		vma::AllocationInfo allocInfo{};
//
//		vma::raii::Buffer buffer = allocator.createBufferWithAlignment(bufferCreateInfo, allocationCreateInfo, alignment, allocInfo);
//		vk::DeviceAddress addr = device.device.getBufferAddress({ buffer });
//
//		return { std::move(buffer), bufferCreateInfo.size, addr, allocInfo.pMappedData };
//	};
//}
