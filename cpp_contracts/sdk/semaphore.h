#include "sdk/types.h"
#include "sdk/nonnegative_int64.h"

namespace sdk
{

template<uint32_t count = 1>
class Semaphore
{
	const sdk::StorageKey key;

public:

	constexpr Semaphore(sdk::StorageKey k)
		: key(std::move(k))
		{}

	void acquire()
	{
		int64_set_add(key, count, -1);
	}
};

}