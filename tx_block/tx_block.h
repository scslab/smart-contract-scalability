#pragma once

#include "mtt/trie/merkle_trie.h"

#include <atomic>

namespace scs
{

class TxBlock
{
	using hash_prefix_t = ByteArrayPrefix<32>;
	struct ValueT
	{
		TransactionInvocation invocation;
		std::atomic_flag invalid;
	};

	using trie_t = trie::MerkleTrie<ValueT, hash_prefix_t>;

	trie_t tx_trie;

public:

	TxBlock()
		: tx_trie()
		{}

	void insert_tx(TransactionInvocation const& invocation);

	bool is_valid(const& Hash hash) const;

	void invalidate(const& Hash hash);

};

} /* scs */
