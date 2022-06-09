#include "state_db/delta_vec.h"

using xdr::operator==;

namespace scs
{

void
DeltaVector::add_delta(StorageDelta&& d, DeltaPriority&& p)
{
	deltas.insert(std::make_pair(std::move(d), std::move(p)));
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
