#pragma once

namespace he2rt::util {
	inline auto align(auto ptr, size_t alignment) noexcept { return (decltype(ptr))(((size_t)ptr + alignment - 1) & ~(alignment - 1)); };
}
