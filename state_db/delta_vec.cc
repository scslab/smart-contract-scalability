#include "state_db/delta_vec.h"

using xdr::operator==;

namespace scs
{

void
DeltaVector::add_delta(StorageDelta&& d, DeltaPriority&& p)
{
	deltas.insert(std::make_pair(std::move(d), std::move(p)));
}

} /* scs */
