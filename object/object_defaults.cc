#include "object/object_defaults.h"

#include <cstring>

namespace scs
{

std::optional<StorageObject>
make_default_object_by_delta(DeltaType d_type)
{
	if (d_type == DELETE)
	{
		return std::nullopt;
	}

	ObjectType type;
	switch (d_type) {
		case DeltaType::RAW_MEMORY_WRITE:
			type = ObjectType::RAW_MEMORY;
			break;
		default:
			throw std::runtime_error(std::string("invalid type passed to make_default_object (or unimpl): ") + std::to_string(d_type));
	}
	return make_default_object_by_type(type);
}

StorageObject
make_default_object_by_type(ObjectType type)
{
	StorageObject out;
	out.type(type);
	switch(type)
	{
		case ObjectType::RAW_MEMORY:
			// default is empty data field
			break;
			
	}
	return out;
}

} /* scs */
