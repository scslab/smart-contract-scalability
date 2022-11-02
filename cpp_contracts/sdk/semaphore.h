#include "sdk/types.h"
#include "sdk/nonnegative_int64.h"

namespace sdk
{

class Semaphore
{
	const sdk::StorageKey key;

public:

	constexpr Semaphore(sdk::StorageKey k)
		: key(std::move(k))
		{}

	void acquire()
	{
		int64_set_add(key, 1, -1);
	}
};

}