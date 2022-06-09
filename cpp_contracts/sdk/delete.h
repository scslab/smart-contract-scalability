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

BUILTIN("delete_key_first")
void 
delete_first(
	uint32_t key_offset,
	uint32_t priority);

BUILTIN("delete_key_last")
void
delete_last(
	uint32_t key_offset,
	uint32_t priority);

} /* detail */

void delete_first(StorageKey const& key)
{
	detail::delete_first(to_offset(&key), 0);
}

void delete_last(StorageKey const& key)
{
	detail::delete_last(to_offset(&key), 0);
}

} /* sdk */
