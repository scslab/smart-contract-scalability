%#include "xdr/types.h"
%#include "xdr/storage.h"

namespace scs
{

enum DeltaType
{
	DELETE = 0,
	RAW_MEMORY_WRITE = 1,
};

union StorageDelta switch (DeltaType type)
{
	case DELETE:
		void;
	case RAW_MEMORY_WRITE:
		opaque data<RAW_MEMORY_MAX_LEN>;
};

} /* scs */
