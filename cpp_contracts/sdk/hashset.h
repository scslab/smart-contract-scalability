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
	uint32_t hash_offset,
	/* hash_len = 32 */
	uint64_t threshold);

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
	/* key_len = 32 */,
	uint64_t threshold);

BUILTIN("hashset_get_size")
uint32_t hashset_get_size(
	uint32_t key_offset
	/* key_len = 32 */);

BUILTIN("hashset_get_max_size")
uint32_t
hashset_get_max_size(
	uint32_t key_offset
	/* key_len = 32 */);

/**
 *  if two things with same threshold, 
 * returns lowest index.
 * If none, returns UINT32_MAX
 **/
BUILTIN("hashset_get_index_of")
uint32_t
hashset_get_index_of(
	uint32_t key_offset,
	/* key_len = 32 */
	uint64_t threshold);

// get the index'th item in the hashset
BUILTIN("hashset_get_index")
uint64_t
hashset_get_index(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t output_offset,
	/* output_len = 32 */
	uint32_t index);


} /* detail */

void hashset_insert(
	StorageKey const& key,
	Hash const& hash,
	uint64_t threshold)
{
	detail::hashset_insert(
		to_offset(&key), 
		to_offset(&hash),
		threshold);
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
	StorageKey const& key,
	uint64_t threshold)
{
	detail::hashset_clear(
		to_offset(&key),
		threshold);
}

uint32_t
hashset_get_size(
	StorageKey const& key)
{
	return detail::hashset_get_size(to_offset(&key));
}

uint32_t
hashset_get_max_size(
	StorageKey const& key)
{
	return detail::hashset_get_max_size(to_offset(&key));
}

/**
 *  if two things with same threshold, 
 * returns lowest index.
 * If none, returns UINT32_MAX
 **/
uint32_t
hashset_get_index_of(
	StorageKey const& key,
	uint64_t threshold)
{
	return detail::hashset_get_index_of(
		to_offset(&key),
		threshold);
}

std::pair<uint64_t, Hash>
hashset_get_index(
	StorageKey const& key,
	uint32_t index)
{
	Hash out;
	uint64_t threshold = detail::hashset_get_index(
		to_offset(&key),
		to_offset(&out),
		index);
	return {threshold, out};
}

} /* sdk */
