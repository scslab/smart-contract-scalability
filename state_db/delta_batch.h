#pragma once

#include <map>
#include <utility>

#include "xdr/types.h"

#include "object/delta_type_class.h"

#include "state_db/delta_batch_value.h"

#include "mtt/trie/recycling_impl/trie.h"

#include "state_db/serial_delta_batch.h"

namespace scs 
{

class DeltaVector;
class TxBlock;
class StateDB;

class DeltaBatch
{

	using trie_prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;
public:
	using value_t = DeltaBatchValue;
	using map_t = trie::RecyclingTrie<value_t, trie_prefix_t, DeltaBatchValueMetadata>;
private:
	map_t deltas;

	using serial_trie_t = map_t::serial_trie_t;

	using cache_t = utils::ThreadlocalCache<serial_trie_t>;

	cache_t serial_cache;


	bool populated = false;
	bool filtered = false;
	bool applied = false;
	mutable bool written_to_state_db = false;

	friend class StateDB;
	friend class SerialDeltaBatch;

public:

	void merge_in_serial_batches();

	SerialDeltaBatch get_serial_subsidiary();

	void filter_invalid_deltas(TxBlock& txs);
	void apply_valid_deltas(TxBlock const& txs);
};

} /* scs */
