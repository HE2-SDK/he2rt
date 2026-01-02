#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "buffer.h"

namespace he2rt {
	struct AccelerationStructure {
		Buffer buffer;
		vk::raii::AccelerationStructureKHR structure;
		vk::DeviceAddress deviceAddress;
	};
}
