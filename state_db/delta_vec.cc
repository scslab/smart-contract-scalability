#include "state_db/delta_vec.h"

using xdr::operator==;

namespace scs
{

void
DeltaVector::add_delta(StorageDelta&& d, Hash&& p)
{
	deltas.emplace_back(std::move(d), std::move(p));
}
/*
DeltaTypeClass
DeltaVector::get_typeclass_vote() const
{
	if (deltas.size() == 0)
	{
		throw std::runtime_error("cannot get tc vote from empty vector");
	}

	return DeltaTypeClass(deltas.begin() -> first, deltas.begin() -> second);
} */

void 
DeltaVector::add(DeltaVector&& other)
{
	deltas.insert(
		deltas.end(),
		std::make_move_iterator(other.deltas.begin()),
		std::make_move_iterator(other.deltas.end()));

	//deltas.merge(other.deltas);
	//if (other.deltas.size() != 0)
//	{
//		throw std::runtime_error("collision within delta_vec");
//	}
}

} /* scs */
