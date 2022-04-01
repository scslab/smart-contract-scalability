#pragma once

#include <cstdio>

#include "xdr/types.h"

#include "utils/serialize_endian.h"

namespace scs
{

namespace test
{

[[maybe_unused]]
Address address_from_uint64(uint64_t addr)
{
	Address out;
	utils::write_unsigned_big_endian(out, addr);
	return out;	
}

} /* test */

} /* scs */
