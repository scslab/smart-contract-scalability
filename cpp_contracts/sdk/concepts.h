#pragma once

#include <cstdint>
#include <type_traits>

namespace sdk
{

template<typename T>
concept TriviallyCopyable
= requires {
	typename std::enable_if<std::is_trivially_copyable<T>::value>::type;
};

template<TriviallyCopyable T>
uint32_t 
to_offset(const T* addr)
{
	static_assert(sizeof(void*) == 4, "wasm pointer size");
	return reinterpret_cast<uint32_t>(reinterpret_cast<const void*>(addr));
}

template<typename T>
concept VectorLike
= requires (T object)
{
	object.data();
	object.size();
} && !TriviallyCopyable<T>;

} /* sdk */
