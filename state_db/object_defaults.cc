#include "state_db/object_defaults.h"

#include <cstring>

namespace scs
{

std::optional<StorageObject>
make_default_object(DeltaType d_type)
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
