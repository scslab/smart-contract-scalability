#pragma once

#include "sdk/macros.h"
#include "sdk/alloc.h"

namespace sdk
{

namespace detail
{

BUILTIN("log")
void log(uint32_t offset, uint32_t len);

} /* detail */

template<typename T, typename std::enable_if<std::is_trivially_copyable<T>::value, int>::type = 0>
void log(T const& val)
{
	const void* addr = reinterpret_cast<const void*>(&val);
	uint32_t len = sizeof(T);

	static_assert(sizeof(void*) == 4, "wasm pointer size");

	detail::log(reinterpret_cast<uint32_t>(addr), len);
}

} /* sdk */
