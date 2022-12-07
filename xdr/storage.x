%#include "xdr/types.h"

namespace scs
{

enum ObjectType
{
	RAW_MEMORY = 0,
	NONNEGATIVE_INT64 = 1,
	HASH_SET = 2,
	UNCONSTRAINED_INT64 = 3,
};

const RAW_MEMORY_MAX_LEN = 4096;

struct RawMemoryStorage
{
	opaque data<RAW_MEMORY_MAX_LEN>;
};

struct HashSetEntry
{
	Hash hash;
	uint64 index;
};

struct HashSet
{
	HashSetEntry hashes<>;
	uint32 max_size;
};

const MAX_HASH_SET_SIZE = 65535;
const START_HASH_SET_SIZE = 64;

struct StorageObject
{
	union switch (ObjectType type)
	{
	case RAW_MEMORY:
		RawMemoryStorage raw_memory_storage;
	case NONNEGATIVE_INT64:
		int64 nonnegative_int64;
	case HASH_SET:
		HashSet hash_set;
	case UNCONSTRAINED_INT64:
		int64 unconstrained_int64;
	} body;

	uint64 escrowed_fee;
};

// in case of none:
// first ensure all are of same type -- if not, all fail
// then apply the single type semantics, using default starting value

} /* scs */
