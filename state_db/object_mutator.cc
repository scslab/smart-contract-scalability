#include "state_db/object_mutator.h"

#include "state_db/delta_vec.h"
#include "tx_block/tx_block.h"

namespace scs
{

StorageObject 
make_default_object(ObjectType type)
{
	StorageObject obj;
	obj.type(type);

	switch (type) {
		case RAW_MEMORY:
			// default is empty bytestring
			return obj;
		default:
			throw std::runtime_error("unimpl");
	}
}

void 
ObjectMutator::filter_invalid_deltas(const DeltaVector& deltas, TxBlock& txs) const
{
	ObjectType t = deltas.get_initial_type_if_default();
	if (base)
	{
		t = base->type();
	}

	switch(t)
	{
		case ObjectType::RAW_MEMORY:
		{
			filter_invalid_deltas_RAW_MEMORY(deltas, txs);
			return;
		}
		default:
			throw std::runtime_error("unimpl");
	}


}

void 
ObjectMutator::filter_invalid_deltas_RAW_MEMORY(const DeltaVector& deltas, TxBlock& txs) const
{
	std::optional<StorageObject> filter_base = base;
	bool has_been_modified = false;

	for (auto const& [d, p] : deltas.get_sorted_deltas())
	{
		if (!txs.is_valid(TransactionFailurePoint::COMPUTE, p.tx_hash))
		{
			// ignore txs that failed during execution
			continue;
		}
		if (d.type() != ObjectType::RAW_MEMORY)
		{
			txs.invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(p.tx_hash);
			continue;
		}
		// 0: no set, 1: has been set
		if (!has_been_modified)
		{
			if (!filter_base)
			{
				filter_base = make_default_object(ObjectType::RAW_MEMORY);
			}
			filter_base->raw_memory_storage().data = d.data();
			has_been_modified = true;
		} else
		{
			if (d.data() != filter_base->raw_memory_storage().data)
			{
				txs.invalidate<CONFLICT_PHASE_1>(p.tx_hash);
			}
		}
	}
}


void
ObjectMutator::apply_valid_deltas(const DeltaVector& deltas, const TxBlock& txs)
{
	ObjectType t = deltas.get_initial_type_if_default();
	if (base)
	{
		t = base->type();
	}
	switch(t)
	{
		case ObjectType::RAW_MEMORY:
		{
			apply_valid_deltas_RAW_MEMORY(deltas, txs);
			return;
		}
		default:
			throw std::runtime_error("unimpl");
	}
}

void
ObjectMutator::apply_valid_deltas_RAW_MEMORY(const DeltaVector& deltas, const TxBlock& txs)
{
	for (auto const& [d, p] : deltas.get_sorted_deltas())
	{
		if (!txs.is_valid(TransactionFailurePoint::FINAL, p.tx_hash))
		{
			continue;
		}

		if (!base)
		{
			base = make_default_object(ObjectType::RAW_MEMORY);
		}

		base->raw_memory_storage().data = d.data();
	}
}

} /* scs */
