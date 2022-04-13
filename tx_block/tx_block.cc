#include "tx_block/tx_block.h"

#include "crypto/hash.h"

namespace scs
{

void 
TxBlock::insert_tx(TransactionInvocation const& invocation)
{
	Hash h = hash_xdr(invocation);

	hash_prefix_t prefix(h);

	tx_trie.insert(prefix, ptr_value_t(std::make_unique<ValueT>(invocation)));
}

bool 
TxBlock::is_valid(const Hash& hash) const
{
	auto const& res = tx_trie.get_value_nolocks(hash_prefix_t(hash));
	return !(res.v -> invalid.test());
}

void 
TxBlock::invalidate(const Hash& hash)
{
	tx_trie.template modify_value_nolocks<InvalidateFn>(hash_prefix_t(hash));
}


} /* scs */
