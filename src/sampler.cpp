#include "sampler.h"
#include <iostream>

namespace he2rt {
	Sampler::Sampler(vk::raii::Sampler&& sampler, const vk::SamplerCreateInfo& createInfo) noexcept
		: sampler{ std::move(sampler) }
		, createInfo{ createInfo }
	{
	}

	Sampler::Sampler(Sampler&& other) noexcept
		: sampler{ std::move(other.sampler) }
		, createInfo{ std::move(other.createInfo) }
	{
	}

	Sampler& Sampler::operator=(Sampler&& other) noexcept {
		sampler = std::move(other.sampler);
		createInfo = std::move(other.createInfo);

		return *this;
	}
}
