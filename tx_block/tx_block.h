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
		std::atomic_flag invalid;

		ValueT(TransactionInvocation const& invocation)
			: invocation(invocation)
			, invalid()
			{}

		void copy_data(std::vector<uint8_t>& v) {}
	};

	using ptr_value_t = trie::PointerValue<ValueT>;

	struct InvalidateFn
	{
		static void
		apply(ptr_value_t& val)
		{
			val.v->invalid.test_and_set();
		}
	};

	using trie_t = trie::MerkleTrie<hash_prefix_t, ptr_value_t>;

	trie_t tx_trie;

public:

	TxBlock()
		: tx_trie()
		{}

	void insert_tx(TransactionInvocation const& invocation);

	bool is_valid(const Hash& hash) const;

	void invalidate(const Hash& hash);

};

} /* scs */
