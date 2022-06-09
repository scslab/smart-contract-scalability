%#include "xdr/types.h"
%#include "xdr/storage.h"

namespace scs
{

enum DeltaType
{
	DELETE_FIRST = 0,
	DELETE_LAST = 1,
	RAW_MEMORY_WRITE = 2,
	NONNEGATIVE_INT64_SET_ADD = 3,
};

struct set_add_t
{
	int64 set_value;
	int64 delta;
};

union StorageDelta switch (DeltaType type)
{
	case DELETE_FIRST:
		void;
	case DELETE_LAST:
		void;
	case RAW_MEMORY_WRITE:
		opaque data<RAW_MEMORY_MAX_LEN>;
	case NONNEGATIVE_INT64_SET_ADD:
		set_add_t set_add_nonnegative_int64;
};

} /* scs */
