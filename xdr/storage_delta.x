%#include "xdr/types.h"
%#include "xdr/storage.h"

namespace scs
{

enum DeltaType
{
	DELETE_LAST = 0,
	RAW_MEMORY_WRITE = 1,
	NONNEGATIVE_INT64_SET_ADD = 2,
};

struct set_add_t
{
	int64 set_value;
	int64 delta;
};

union StorageDelta switch (DeltaType type)
{
	case DELETE_LAST:
		void;
	case RAW_MEMORY_WRITE:
		opaque data<RAW_MEMORY_MAX_LEN>;
	case NONNEGATIVE_INT64_SET_ADD:
		set_add_t set_add_nonnegative_int64;
};

struct DeltaPriority
{
	// higher custom_priority beats lower (empty) custom_priority
	uint32 custom_priority;

	//higher gas bid wins ties
	uint64 gas_rate_bid;

	Hash tx_hash;

	// the ith delta created by a tx gets id i.  Final uniqueness tiebreaker.
	uint32 delta_id_number;
};

enum TypeclassValence
{
	TV_FREE = 0,
	TV_RAW_MEMORY_WRITE = 1,
	TV_NONNEGATIVE_INT64_SET = 2,
	TV_ERROR = 3,
};

struct DeltaValence
{
	union switch(TypeclassValence type)
	{
		case TV_FREE:
			void;
		case TV_RAW_MEMORY_WRITE:
			opaque data<RAW_MEMORY_MAX_LEN>;
		case TV_NONNEGATIVE_INT64_SET:
			int64 set_value;
		case TV_ERROR:
			void;
	} tv;

	uint32 deleted_last; // 0 = false, nonzero = true;
};

} /* scs */
