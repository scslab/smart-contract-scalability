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

BUILTIN("hashset_insert")
void
hashset_insert(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t hash_offset);

BUILTIN("hashset_increase_limit")
void
hashset_increase_limit(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t limit_increase);

BUILTIN("hashset_clear")
void
hashset_clear(
	uint32_t key_offset
	/* key_len = 32 */);

} /* detail */

void hashset_insert(
	StorageKey const& key,
	Hash const& hash)
{
	detail::hashset_insert(
		to_offset(&key), 
		to_offset(&hash));
}

void
hashset_increase_limit(
	StorageKey const& key,
	uint16_t limit_increase)
{
	detail::hashset_increase_limit(
		to_offset(&key),
		limit_increase);
}

void
hashset_clear(
	StorageKey const& key)
{
	detail::hashset_clear(
		to_offset(&key));
}

} /* sdk */
