#pragma once

#include "xdr/storage.h"

#include <optional>

namespace scs
{

class TxBlock;
class DeltaVector;

class ObjectMutator
{
	std::optional<StorageObject> base;

	void filter_invalid_deltas_RAW_MEMORY(const DeltaVector& deltas, TxBlock& txs) const;

	void apply_valid_deltas_RAW_MEMORY(const DeltaVector& deltas, const TxBlock& txs);

public:

	ObjectMutator()
		: base(std::nullopt)
		{
		}

	void populate_base(StorageObject const& obj)
	{
		base = obj;
	}



	void filter_invalid_deltas(const DeltaVector& deltas, TxBlock& txs) const;

	void apply_valid_deltas(const DeltaVector& deltas, const TxBlock& txs);

	std::optional<StorageObject> get_object() const {
		return base;
	}
};

} /* scs */
