#pragma once

#include <map>
#include <utility>

#include "xdr/types.h"

#include "transaction_context/threadlocal_types.h"
#include "object/delta_type_class.h"

#include "state_db/delta_batch_value.h"

namespace scs 
{

class DeltaVector;
class ObjectMutator;
class SerialDeltaBatch;
class TxBlock;
class StateDB;

class DeltaBatch
{

	using map_t = std::map<AddressAndKey, DeltaBatchValue>;
	using value_t = DeltaBatchValue;

	map_t deltas;

	using batch_array_t = batch_delta_array_t;

	bool populated = false;
	bool filtered = false;
	bool applied = false;
	mutable bool written_to_state_db = false;

	friend class StateDB;

public:

	void merge_in_serial_batches(batch_array_t&& batches);

	void filter_invalid_deltas(TxBlock& txs);
	void apply_valid_deltas(TxBlock const& txs);
};

} /* scs */
