#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace he2rt {
	class CommandPool {
		vk::raii::CommandPool pool;

	public:
		CommandPool(vk::raii::CommandPool&& pool);
	};
}