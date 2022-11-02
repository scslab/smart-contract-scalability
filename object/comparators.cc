#include "object/comparators.h"

#include "xdr/storage.h"

namespace scs
{

std::strong_ordering 
operator<=>(const HashSetEntry& d1, const HashSetEntry& d2)
{
	auto res = d1.index <=> d2.index;
	if (res != std::strong_ordering::equal)
	{
		return res;
	}

	return d1.hash <=> d2.hash;
}


} // scs
