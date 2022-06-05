%#include "xdr/types.h"

namespace scs
{

enum ObjectType
{
	RAW_MEMORY = 0,
};

const RAW_MEMORY_MAX_LEN = 512;

struct RawMemoryStorage
{
	opaque data<RAW_MEMORY_MAX_LEN>;
};

// these unions should never have a NONE case arm
union StorageObject switch (ObjectType type)
{
	case RAW_MEMORY:
		RawMemoryStorage raw_memory_storage;
};

struct DeltaPriority
{
	// higher custom_priority beats lower (empty) custom_priority
	uint64 custom_priority;

	//higher gas bid wins ties
	uint64 gas_rate_bid;

	Hash tx_hash;

	// the ith delta created by a tx gets id i.  Final uniqueness tiebreaker.
	uint32 delta_id_number;
};

// in case of none:
// first ensure all are of same type -- if not, all fail
// then apply the single type semantics, using default starting value

} /* scs */
