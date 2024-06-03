#include "state_db/typed_mod_trie.h"

namespace scs
{


void 
TypedModificationTrie::prune_conflicts(node_t const* node, ModificationMetadata offset_meta) const
{
	constexpr static MIN_CHECK_LEN = sizeof(AddressAndKey);
}


}