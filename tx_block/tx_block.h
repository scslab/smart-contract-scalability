#pragma once

#include "mtt/trie/prefix.h"
#include "mtt/trie/merkle_trie.h"
#include "mtt/trie/utils.h"

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
		Transaction tx;
		std::atomic<uint32_t> validity;

		ValueT(Transaction const& tx)
			: tx(tx)
			, validity(0)
			{}
	};

	static
	std::vector<uint8_t> serialize(const ValueT& v)
	{
		return trie::no_serialization_fn<ValueT>(v);
	}

	//TODO for reasons I don't quite understand, swapping in &trie::no_serialization_fn<ValueT> for &serialize
	// results in "warning: 'scs::TxBlock' has a field 'scs::TxBlock::tx_trie' whose type uses the anonymous namespace [-Wsubobject-linkage]"
	using ptr_value_t = trie::PointerValue<ValueT, &serialize>;//trie::no_serialization_fn<ValueT>>;

	using trie_t = trie::MerkleTrie<hash_prefix_t, ptr_value_t>;

	trie_t tx_trie;

public:

	TxBlock()
		: tx_trie()
		{}

	Hash insert_tx(Transaction const& tx);

	bool is_valid(TransactionFailurePoint failure_point, const Hash& hash) const;

	template<TransactionFailurePoint failure_point>
	void invalidate(const Hash& hash);

};

} /* scs */
