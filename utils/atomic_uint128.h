#pragma once

#include <atomic>
#include <cstdint>

namespace scs
{


//atomic 128 that does not allow reads
//without a synchronization barrier (external)
class AtomicUint128
{
	std::atomic<uint64_t> lowbits, highbits;

public:

	void add(uint64_t v)
	{
		uint64_t prev_lowbits = lowbits.fetch_add(v, std::memory_order_relaxed);

		if (__builtin_add_overflow_p(prev_lowbits, v, static_cast<uint64_t>(0)))
		{
			highbits.fetch_add(1, std::memory_order_relaxed);
		}
	}

	void sub(uint64_t v)
	{
		uint64_t prev_lowbits = lowbits.fetch_sub(v, std::memory_order_relaxed);

		if (__builtin_sub_overflow_p(prev_lowbits, v, static_cast<uint64_t>(0)))
		{
			highbits.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	// returns UINT64_MAX if greater than uint64
	uint64_t fetch_cap()
	{
		if (highbits.load(std::memory_order_relaxed) > 0)
		{
			return UINT64_MAX;
		}
		return lowbits.load(std::memory_order_relaxed);
	}

	void clear()
	{
		highbits.store(0, std::memory_order_relaxed);
		lowbits.store(0, std::memory_order_relaxed);
	}
};

}