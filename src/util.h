#pragma once
#include <vulkan/vulkan.hpp>

namespace he2rt::util {
	inline auto align(auto ptr, size_t alignment) noexcept { return (decltype(ptr))(((size_t)ptr + alignment - 1) & ~(alignment - 1)); };

	inline vk::Extent3D getGroupCounts(uint32_t length, uint32_t workgroupSize) {
		return {
			(length + (workgroupSize - 1)) / workgroupSize,
			1,
			1,
		};
	}

	inline vk::Extent3D getGroupCounts(vk::Extent2D extent, uint32_t workgroupSize) {
		return {
			(extent.width + (workgroupSize - 1)) / workgroupSize,
			(extent.height + (workgroupSize - 1)) / workgroupSize,
			1,
		};
	}

	inline vk::Extent3D getGroupCounts(vk::Extent3D extent, uint32_t workgroupSize) {
		return {
			(extent.width + (workgroupSize - 1)) / workgroupSize,
			(extent.height + (workgroupSize - 1)) / workgroupSize,
			(extent.depth + (workgroupSize - 1)) / workgroupSize,
		};
	}
}
