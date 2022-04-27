#include "transaction_context/storage_cache.h"

#include "transaction_context/global_context.h"

#include "state_db/state_db.h"

namespace scs
{

StorageCache::StorageCache(const StateDB& state_db)
	: state_db(state_db)
	, cache()
	{}





} /* scs */
