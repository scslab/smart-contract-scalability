#pragma once

#include "sdk/macros.h"
#include "sdk/concepts.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"

#include <cstdint>
#include <optional>

namespace sdk
{

namespace detail
{

BUILTIN("raw_mem_set")
void 
raw_mem_set(
	uint32_t key_offset,
	uint32_t mem_offset,
	uint32_t mem_len);

BUILTIN("raw_mem_get")
uint32_t
raw_mem_get(
	uint32_t key_offset,
	uint32_t output_offset,
	uint32_t output_max_len);

} /* detail */

template<TriviallyCopyable T>
void set_raw_memory(const StorageKey& key, const T& value)
{
	detail::raw_mem_set(
		to_offset(key.data()),
		to_offset(&value),
		sizeof(T));
}

template<TriviallyCopyable T>
std::optional<T>
get_raw_memory_opt(const StorageKey& key)
{
	std::optional<T> out = T{};
	uint32_t res = detail::raw_mem_get(
		to_offset(key.data()),
		to_offset(&(*out)),
		sizeof(T));

	if (res == 0)
	{
		return std::nullopt;
	}
	return out;
}

template<TriviallyCopyable T>
T
get_raw_memory(const StorageKey& key)
{
	T out;
	uint32_t res = detail::raw_mem_get(
		to_offset(key.data()),
		to_offset(&out),
		sizeof(T));

	if (res == 0)
	{
		abort();
	}
	return out;
}

} /* sdk */
