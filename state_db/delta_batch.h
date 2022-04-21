#pragma once

#include <map>
#include <utility>

#include "xdr/types.h"

namespace scs 
{

class DeltaVector;
class ObjectMutator;
class SerialDeltaBatch;
class TxBlock;

class DeltaBatch
{
	using value_t = std::pair<DeltaVector, ObjectMutator>;
	using map_t = std::map<AddressAndKey, value_t>;

	map_t deltas;

public:

	void merge_in_serial_batch(SerialDeltaBatch& batch);
 
	void filter_invalid_deltas(TxBlock& txs);
	void apply_valid_deltas(TxBlock const& txs);

	const map_t& get_delta_map() const {
		return deltas;
	}
};

} /* scs */
