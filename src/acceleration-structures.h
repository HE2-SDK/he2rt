#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "buffer.h"

namespace he2rt {
	struct AccelerationStructure {
		Buffer buffer;
		vk::raii::AccelerationStructureKHR structure;
		vk::DeviceAddress deviceAddress;

		AccelerationStructure(Buffer&& buffer, vk::raii::AccelerationStructureKHR&& structure, vk::DeviceAddress deviceAddress) noexcept;
		AccelerationStructure(const AccelerationStructure& other) = delete;
		AccelerationStructure(AccelerationStructure&& other) noexcept;

		AccelerationStructure& operator=(const AccelerationStructure& other) = delete;
		AccelerationStructure& operator=(AccelerationStructure&& other) noexcept;
	};
}
