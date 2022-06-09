#include "debug/debug_utils.h"

#include "mtt/trie/debug_macros.h"

namespace debug
{

std::string 
storage_object_to_str(std::optional<scs::StorageObject> const& obj)
{
	if (!obj)
	{
		return "[nullopt]";
	}
	return storage_object_to_str(*obj);
}

std::string 
storage_object_to_str(scs::StorageObject const& obj)
{
	switch(obj.type())
	{
		case scs::ObjectType::RAW_MEMORY:
			return "[raw_memory: " + debug::array_to_str(obj.raw_memory_storage().data) + "]";
		case scs::ObjectType::NONNEGATIVE_INT64:
			return "[nn_int64: " + std::to_string(obj.nonnegative_int64()) + "]";	
	}
	throw std::runtime_error("unknown object type in storage_object_to_str");
}

} /* scs */
