#include "sdk/hashset.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/invoke.h"

namespace sdk
{

namespace detail
{

constexpr static StorageKey replay_cache_storage_key
	= make_static_key(0);

} // detail

void record_self_replay(const uint64_t expiration_time)
{
	Hash hash_buf = get_invoked_hash();
	hashset_insert(detail::replay_cache_storage_key, hash_buf, expiration_time);

	hashset_clear(detail::replay_cache_storage_key, get_block_number());	
}

void replay_cache_size_increase(uint16_t amount)
{
	hashset_increase_limit(detail::replay_cache_storage_key, amount);
}

}



