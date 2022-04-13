#include "tx_block/tx_block.h"

#include "crypto/crypto_utils.h"

namespace scs
{

void 
TxBlock::insert_tx(size_t idx, TransactionInvocation const& invocation)
{
	Hash h = hash_xdr(invocation);
}

	bool is_valid(const& Hash hash) const;

	void invalidate(const& Hash hash);


} /* scs */
