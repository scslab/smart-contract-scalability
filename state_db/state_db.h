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

	/*static
	std::vector<uint8_t> 
	serialize(const std::optional<StorageObject>& v)
	{
		if (!v)
		{
			return std::vector<uint8_t>();
		}
		return xdr::xdr_to_opaque(*v);
	} */

	static std::vector<uint8_t> 
	serialize(const StorageObject& v)
	{
		return xdr::xdr_to_opaque(v);
	}

	using base_value_struct = trie::SerializeWrapper<StorageObject, &serialize>;

	struct value_struct : public base_value_struct
	{
		value_struct(const StorageObject& obj)
			: base_value_struct(obj)
			{}

		value_struct()
			: base_value_struct()
			{}
	};

public:

	using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;
	using metadata_t = trie::CombinedMetadata<trie::SizeMixin, trie::DeletableMixin>;

	using value_t = value_struct;//trie::SerializeWrapper<StorageObject, &serialize>; 

	using trie_t = trie::MerkleTrie<prefix_t, value_t, metadata_t>;

	using cache_t = utils::ThreadlocalCache<trie_t>;

private:

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
