#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#define VMA_DEBUG_LOG_FORMAT(format, ...) do { \
       printf((format), __VA_ARGS__); \
       printf("\n"); \
   } while(false)
#include <vk_mem_alloc.hpp>
#include <vk_mem_alloc_raii.hpp>

namespace he2rt {
	class Image {
	public:
		vma::raii::Image image;
		vk::ImageCreateInfo createInfo;

		Image(vma::raii::Image&& image, const vk::ImageCreateInfo& createInfo) noexcept;
		Image(const Image& other) = delete;
		Image(Image&& other) noexcept;

		Image& operator=(const Image& other) = delete;
		Image& operator=(Image&& other) noexcept;
	};
}