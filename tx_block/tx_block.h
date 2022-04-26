#pragma once

#include "mtt/trie/prefix.h"
#include "mtt/trie/merkle_trie.h"

#include "xdr/transaction.h"
#include "xdr/types.h"

#include <atomic>

namespace scs
{

class TxBlock
{
	using hash_prefix_t = trie::ByteArrayPrefix<32>;
	struct ValueT
	{
		TransactionInvocation invocation;
		std::atomic<uint32_t> validity;

		ValueT(TransactionInvocation const& invocation)
			: invocation(invocation)
			, validity(0)
			{}
	};

	using ptr_value_t = trie::PointerValue<ValueT>;

	using trie_t = trie::MerkleTrie<hash_prefix_t, ptr_value_t>;

	trie_t tx_trie;

public:

	TxBlock()
		: tx_trie()
		{}

	void insert_tx(TransactionInvocation const& invocation);

	bool is_valid(TransactionFailurePoint failure_point, const Hash& hash) const;

	template<TransactionFailurePoint failure_point>
	void invalidate(const Hash& hash);

};

} /* scs */
