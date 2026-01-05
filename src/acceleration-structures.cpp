#include "acceleration-structures.h"

namespace he2rt {
	AccelerationStructure::AccelerationStructure(Buffer&& buffer, vk::raii::AccelerationStructureKHR&& structure, vk::DeviceAddress deviceAddress) noexcept
		: buffer{ std::move(buffer) }, structure{ std::move(structure) }, deviceAddress{ deviceAddress } {
	}

	AccelerationStructure::AccelerationStructure(AccelerationStructure&& other) noexcept
		: buffer{ std::move(other.buffer) }, structure{ std::move(other.structure) }, deviceAddress{ other.deviceAddress } {
	}

	AccelerationStructure& AccelerationStructure::operator=(AccelerationStructure&& other) noexcept {
		buffer = std::move(other.buffer);
		structure = std::move(other.structure);
		deviceAddress = std::move(other.deviceAddress);
		return *this;
	}
}
