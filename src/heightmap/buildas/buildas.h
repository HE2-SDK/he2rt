#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "../../device.h"
#include "shader-types.h"
#include "initialize-shader.h"
#include "generate-mips-shader.h"

namespace he2rt::heightmap::buildas {
	class BuildAS {
		InitializeShader initializeShader;
		GenerateMipsShader generateMipsShader;

	public:
		BuildAS(Device& device);

		Image run(Device& device, const vk::Extent2D& extent, const vk::DescriptorImageInfo& in);
	};
}
