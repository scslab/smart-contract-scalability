#include "state_db/delta_vec.h"

using xdr::operator==;

namespace scs
{

void
DeltaVector::add_delta(StorageDelta&& d, DeltaPriority&& p)
{
	std::printf("insertion oldsz %lu \n", deltas.size());
	deltas.insert(std::make_pair(std::move(d), std::move(p)));
	std::printf("new sz: %lu\n", deltas.size());
}

DeltaTypeClass
DeltaVector::get_typeclass_vote() const
{
	if (deltas.size() == 0)
	{
		throw std::runtime_error("cannot get tc vote from empty vector");
	}

	return DeltaTypeClass(deltas.begin() -> first, deltas.begin() -> second);
}


} /* scs */
