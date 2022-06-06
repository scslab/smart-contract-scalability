#include "object/object_mutator.h"

#include "state_db/delta_vec.h"
#include "object/object_defaults.h"

#include "tx_block/tx_block.h"
#include "object/object_modification_context.h"
#include "object/delta_applicator.h"

namespace scs
{

/**
 * Mod rules on something:
 * 		- mod, delete
 * 			- if not exist, create something of default value per type
 * 			- delete: can coexist with other stuff, but no matter what object's gone at end.
 * Mod rules on an int:
 * 		- exclusive: either max(stuff), min(stuff), xor(stuff), add/sub(stuff)
 */


template<TransactionFailurePoint prev_failure_point>
class DeconflictingTxBlock
{
	TxBlock& base;

public:

	DeconflictingTxBlock(TxBlock& block)
		: base(block)
		{}


	bool is_valid(const Hash& tx_hash)
	{
		return base.is_valid(prev_failure_point, tx_hash);
	}

	void invalidate(const Hash& tx_hash)
	{
		base.template invalidate<prev_failure_point>(tx_hash);
	}
};

class FinalCheckTxBlock
{
	TxBlock const& base;

public:
	FinalCheckTxBlock(const TxBlock& block)
		: base(block)
		{}

	bool is_valid(const Hash& tx_hash)
	{
		return base.is_valid(TransactionFailurePoint::FINAL, tx_hash);
	}

	void invalidate(const Hash& tx_hash)
	{
		throw std::runtime_error("cannot invalidate in final check");
	}
};

template<typename TxBlockWrapper>
void 
apply_deltas(const DeltaVector& deltas, TxBlockWrapper& txs, std::optional<StorageObject>& base)
{
	DeltaApplicator applicator(base);

	for (auto const& [d, p] : deltas.get_sorted_deltas())
	{
		if (!txs.is_valid(p.tx_hash))
		{
			//ignore, failed during execution or was pruned out
			continue;
		}

		if (!applicator.try_apply(d))
		{
			txs.invalidate(p.tx_hash);
			continue;
		}
	}

	base = applicator.get();

	/*ObjectModificationContext mod_context(base);
	
	for (auto const& [d, p] : deltas.get_sorted_deltas())
	{
		if (!txs.is_valid(p.tx_hash))
		{
			// ignore txs that failed during execution
			continue;
		}
		if (!mod_context.can_accept_mod(d.type()))
		{
			txs.invalidate(p.tx_hash);
			continue;
		}

		if (!base)
		{
			base = make_default_object(d.type());
		}

		// base might be nullopt, if d.type() == DELETE;
		// only stays that way if it (1) starts as nullopt and (2) sees an uninterrupted sequence
		// of DELETE
		if (d.type() != DeltaType::DELETE)
		{
			switch(base -> type())
			{
				case ObjectType::RAW_MEMORY:
				{
					if (!mod_context.raw_mem_set_called)
					{
						base -> raw_memory_storage().data = d.data();
						break;
					}
					if (d.data() != base -> raw_memory_storage().data)
					{
						txs.invalidate(p.tx_hash);
						continue;
					}
				}
				break;
				default:
					throw std::runtime_error("unimpl");
			}
		}

		mod_context.accept_mod(d.type());
	}

	if (mod_context.is_deleted)
	{
		base = std::nullopt;
	} */
}

template<TransactionFailurePoint prev_failure_point>
void
ObjectMutator::filter_invalid_deltas(const DeltaVector& deltas, TxBlock& txs) const
{
	DeconflictingTxBlock<prev_failure_point> block(txs);

	auto base_copy = base;

	apply_deltas(deltas, block, base_copy);
}

template
void ObjectMutator::filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(const DeltaVector&, TxBlock&) const;

void
ObjectMutator::apply_valid_deltas(const DeltaVector& deltas, const TxBlock& txs)
{
	FinalCheckTxBlock block(txs);
	apply_deltas(deltas, block, base);
}

std::optional<StorageObject> 
ObjectMutator::get_object() const {
	return base;
}

} /* scs */
