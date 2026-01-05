#include "buffer.h"
#include <iostream>

namespace he2rt {
	Buffer::Buffer(vma::raii::Buffer&& buffer, vk::DeviceSize size, vk::DeviceAddress deviceAddress, void* hostAddress) noexcept
		: buffer{ std::move(buffer) }
		, size{ size }
		, deviceAddress{ deviceAddress }
		, hostAddress{ hostAddress }
	{
		std::cout << "created buffer of size " << std::hex << size << ", at dev addr " << deviceAddress << ", with mapped addr " << hostAddress << std::dec << ", buffer id is" << VkBuffer{ vk::Buffer{ this->buffer } } << std::endl;
	}

	Buffer::Buffer(Buffer&& other) noexcept
		: buffer{ std::move(other.buffer) }
		, size{ std::move(other.size) }
		, deviceAddress{ std::move(other.deviceAddress) }
		, hostAddress{ std::move(other.hostAddress) }
	{
		other.size = 0;
		//std::cout << "moving buffer with size " << std::hex << size << " and addr " << deviceAddress << std::dec << std::endl;
	}

	Buffer::~Buffer() noexcept {
		if (size != 0)
			std::cout << "destroying buffer with size " << std::hex << size << " and addr " << deviceAddress << std::dec << std::endl;
	}

	Buffer& Buffer::operator=(Buffer&& other) noexcept {
		buffer = std::move(other.buffer);
		size = std::move(other.size);
		deviceAddress = std::move(other.deviceAddress);
		hostAddress = std::move(other.hostAddress);
		other.size = 0;

		return *this;
	}
}