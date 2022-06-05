#pragma once

#include "xdr/storage.h"
#include "xdr/transaction.h"

#include <optional>

namespace scs
{

class TxBlock;
class DeltaVector;

class ObjectMutator
{
	std::optional<StorageObject> base;

public:

	ObjectMutator()
		: base(std::nullopt)
		{
		}

	void populate_base(StorageObject const& obj)
	{
		base = obj;
	}



	template<TransactionFailurePoint prev_failure_point>
	void filter_invalid_deltas(const DeltaVector& deltas, TxBlock& txs) const;

	void apply_valid_deltas(const DeltaVector& deltas, const TxBlock& txs);

	std::optional<StorageObject> get_object() const {
		return base;
	}
};

} /* scs */
