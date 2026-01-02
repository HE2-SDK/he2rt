#include "buffer.h"

namespace he2rt {
	Buffer::Buffer(vk::raii::Buffer&& buffer, vk::DeviceSize size) noexcept
		: buffer{ std::move(buffer) }
		, size{ size }
	{}

	Buffer::Buffer(Buffer && other) noexcept
		: buffer{ std::move(other.buffer) }
		, size{ std::move(other.size) }
		, deviceAddress{ std::move(other.deviceAddress) }
		, hostAddress{ std::move(other.hostAddress) }
	{
	}

	Buffer& Buffer::operator=(Buffer && other) noexcept {
		buffer = std::move(other.buffer);
		size = std::move(other.size);
		deviceAddress = std::move(other.deviceAddress);
		hostAddress = std::move(other.hostAddress);

		return *this;
	}
}