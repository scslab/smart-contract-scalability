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

BUILTIN("nn_int64_set_add")
void
nonnegative_int64_set_add(
	uint32_t key_offset,
	/* key_len = 32 */
	int64_t set_value,
	int64_t delta);

BUILTIN("nn_int64_get")
uint32_t 
nonnegative_int64_get(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t output_offset
	/* output_len = 8 */);
} /* detail */

// creates if doesn't already exist
void
int64_add(StorageKey const& key, int64_t delta)
{
	int64_t res = 0;
	detail::nonnegative_int64_get(
		to_offset(&key),
		to_offset(&res));

	detail::nonnegative_int64_set_add(
		to_offset(&key),
		res,
		delta);
}

void 
int64_set_add(StorageKey const& key, int64_t set_value, int64_t delta)
{
	detail::nonnegative_int64_set_add(
		to_offset(&key),
		set_value,
		delta);
}

// returns 0 in default case where it does not exist
int64_t
int64_get(StorageKey const& key)
{
	int64_t res;
	detail::nonnegative_int64_get(
		to_offset(&key),
		to_offset(&res));
	return res;
}

class NNInt64
{
	StorageKey key;

public:

	const int64_t set_value;
	int64_t delta;

	NNInt64(StorageKey&& key)
		: key(key)
		, set_value(int64_get(key))
		, delta(0)
		{}

	NNInt64(StorageKey&& key, int64_t set)
		: key(key)
		, set_value(set)
		, delta(0)
	{}

	int64_t safe_get_value() const
	{
		int64_t out;
		if (__builtin_add_overflow(set_value, delta, &out))
		{
			abort();
		}
		return out;
	}

	~NNInt64()
	{
		int64_set_add(key, set_value, delta);
	}

};

} /* sdk */
