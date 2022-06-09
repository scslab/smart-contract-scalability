%#include "xdr/types.h"
%#include "xdr/storage.h"

namespace scs
{

enum DeltaType
{
	DELETE = 0,
	RAW_MEMORY_WRITE = 1,
	NONNEGATIVE_INT64_SET = 2,
	NONNEGATIVE_INT64_ADD = 3,
	NONNEGATIVE_INT64_SUB = 4,
};

union StorageDelta switch (DeltaType type)
{
	case DELETE:
		void;
	case RAW_MEMORY_WRITE:
		opaque data<RAW_MEMORY_MAX_LEN>;
	case NONNEGATIVE_INT64_SET:
		int64 set_nonnegative_int64;
	case NONNEGATIVE_INT64_ADD:
		int64 add_nonnegative_int64;
	case NONNEGATIVE_INT64_SUB:
		int64 sub_nonnegative_int64;
};

} /* scs */
