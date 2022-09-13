#include "tx_block/tx_block.h"

#include "crypto/hash.h"

namespace scs
{

Hash 
TxBlock::insert_tx(Transaction const& tx)
{
	Hash h = hash_xdr(tx);

	hash_prefix_t prefix(h);

	tx_trie.insert(prefix, ptr_value_t(std::make_unique<ValueT>(tx)));

	return h;
}

bool 
TxBlock::is_valid(TransactionFailurePoint failure_point, const Hash& hash) const
{
	auto const* res = tx_trie.get_value_nolocks(hash_prefix_t(hash));
	if (res == nullptr)
	{
		throw std::runtime_error("query validity on nexist hash");
	}
	return (res->v -> validity.load(std::memory_order_relaxed) <= static_cast<uint32_t>(failure_point));
}

template<TransactionFailurePoint failure_point>
struct InvalidateFn
{
	template<typename ptr_value_t> 
	static void
	apply(ptr_value_t& val)
	{
		val.v -> validity.fetch_or(failure_point, std::memory_order_relaxed);
	}
};

template<TransactionFailurePoint failure_point>
void 
TxBlock::invalidate(const Hash& hash)
{
	tx_trie.template modify_value_nolocks<InvalidateFn<failure_point>>(hash_prefix_t(hash));
}

template
void TxBlock::invalidate<TransactionFailurePoint::COMPUTE>(const Hash& hash);
template
void TxBlock::invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(const Hash& hash);

} /* scs */
