#include "sdk/general_storage.h"
#include "sdk/nonnegative_int64.h"

namespace sdk
{

template<uint32_t count = 1>
class UniqueFlag
{
	const sdk::StorageKey key;

public:

	constexpr UniqueFlag(sdk::StorageKey k)
		: key(std::move(k))
		{}

	void acquire()
	{
		if (!has_key(key))
		{
			int64_set_add(key, count, 0);
		}

		int64_set_add(key, count, -1);
	}
};

}
