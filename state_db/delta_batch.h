#pragma once

#include <map>
#include <utility>

#include "xdr/types.h"

#include "transaction_context/threadlocal_types.h"
#include "object/delta_type_class.h"

namespace scs 
{

class DeltaVector;
class ObjectMutator;
class SerialDeltaBatch;
class TxBlock;
class StateDB;

class DeltaBatch
{
	struct value_t {
		DeltaVector vec;
		ObjectMutator mutator;
		DeltaTypeClass typeclass;
		value_t(DeltaTypeClass& tc)
			: vec()
			, mutator()
			, typeclass(tc)
			{}
	};

	using map_t = std::map<AddressAndKey, value_t>;

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
