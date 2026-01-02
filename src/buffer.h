#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace he2rt {
	class Buffer {
	public:
		vk::raii::Buffer buffer;
		vk::DeviceSize size;
		vk::DeviceAddress deviceAddress;
		const void* hostAddress;

		Buffer(vk::raii::Buffer&& buffer, vk::DeviceSize size) noexcept;
		Buffer(Buffer&& other) noexcept;

		Buffer& operator=(Buffer&& other) noexcept;
	};
}
