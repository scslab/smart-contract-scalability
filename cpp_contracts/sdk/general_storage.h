#pragma once

#include "sdk/macros.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"

#include <cstdint>
#include <optional>

namespace sdk
{

namespace detail
{

BUILTIN("has_key")
uint32_t 
has_key(
	uint32_t key_offset);

} // namespace detail

bool
has_key(const StorageKey& key)
{
	return detail::has_key(to_offset(&key)) != 0;
}

}
