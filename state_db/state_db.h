#pragma once

#include "xdr/storage.h"
#include "xdr/types.h"

#include "mtt/trie/merkle_trie.h"

#include <optional>
#include <map>

#include <xdrpp/marshal.h>

namespace scs
{

class DeltaBatch;

class StateDB
{

	static
	std::vector<uint8_t> 
	serialize(const std::optional<StorageObject>& v)
	{
		if (!v)
		{
			return std::vector<uint8_t>();
		}
		return xdr::xdr_to_opaque(*v);
	}

public:

	using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;
	using metadata_t = trie::CombinedMetadata<trie::SizeMixin>;
	using value_t = trie::SerializeWrapper<std::optional<StorageObject>, &serialize>; 

	using trie_t = trie::MerkleTrie<prefix_t, value_t, metadata_t>;
private:
	//std::map<AddressAndKey, StorageObject> state_db;
	trie_t state_db;

public:

	std::optional<StorageObject> 
	get(const AddressAndKey& a) const;

	void 
	populate_delta_batch(DeltaBatch& delta_batch) const;

	void 
	apply_delta_batch(DeltaBatch const& delta_batch);
};


} /* scs */
