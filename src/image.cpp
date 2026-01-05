#include "image.h"
#include <iostream>

namespace he2rt {
	Image::Image(vma::raii::Image&& image, const vk::ImageCreateInfo& createInfo) noexcept
		: image{ std::move(image) }
		, createInfo{ createInfo }
	{}

	Image::Image(Image&& other) noexcept
		: image{ std::move(other.image) }
		, createInfo{ std::move(other.createInfo) }
	{}

	Image& Image::operator=(Image&& other) noexcept {
		image = std::move(other.image);
		createInfo = std::move(other.createInfo);

		return *this;
	}
}
