#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
//#define VMA_DEBUG_LOG_FORMAT(format, ...) do { \
//       printf((format), __VA_ARGS__); \
//       printf("\n"); \
//   } while(false)
#include <vk_mem_alloc.hpp>
#include <vk_mem_alloc_raii.hpp>

namespace he2rt {
	class Buffer {
	public:
		vma::raii::Buffer buffer;
		vk::DeviceSize size;
		vk::DeviceAddress deviceAddress;
		void* hostAddress;

		Buffer(vma::raii::Buffer&& buffer, vk::DeviceSize size, vk::DeviceAddress deviceAddress, void* hostAddress = nullptr) noexcept;
		Buffer(const Buffer& other) = delete;
		Buffer(Buffer&& other) noexcept;
		~Buffer() noexcept;

		Buffer& operator=(const Buffer& other) = delete;
		Buffer& operator=(Buffer&& other) noexcept;
	};
}
