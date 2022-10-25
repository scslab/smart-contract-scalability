#pragma once

#include <utils/threadlocal_cache.h>

namespace scs
{

/**
 * Outputs a 48-bit unique ID
 * at every call to get().
 * 
 * Output is <48 bit tag><16 bit 0s>
 */ 
class UniqueIdentifier
{

	constexpr static uint64_t inc = ((uint64_t)1) << 16;
	uint64_t id;

public:

	UniqueIdentifier()
		: id(static_cast<uint64_t>(utils::ThreadlocalIdentifier::get()) << 56)
		{}

	uint64_t get()
	{
		return id += inc;
	}
};

}