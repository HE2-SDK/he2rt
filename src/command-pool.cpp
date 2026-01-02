#include "command-pool.h"

namespace he2rt {
	CommandPool::CommandPool(vk::raii::CommandPool&& pool) : pool{ std::move(pool) } {
		
	}
}