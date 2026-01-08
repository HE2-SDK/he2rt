#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace he2rt {
	class Sampler {
	public:
		vk::raii::Sampler sampler;
		vk::SamplerCreateInfo createInfo;

		Sampler(vk::raii::Sampler&& sampler, const vk::SamplerCreateInfo& createInfo) noexcept;
		Sampler(const Sampler& other) = delete;
		Sampler(Sampler&& other) noexcept;

		Sampler& operator=(const Sampler& other) = delete;
		Sampler& operator=(Sampler&& other) noexcept;
	};
}