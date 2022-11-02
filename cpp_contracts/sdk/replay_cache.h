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

void record_self_replay(const bool do_clear = true)
{
	Hash hash_buf = get_tx_hash();
	hashset_insert(detail::replay_cache_storage_key, hash_buf, get_block_number());
	if (do_clear)
	{
		hashset_clear(detail::replay_cache_storage_key, get_block_number());
	}
}

}



