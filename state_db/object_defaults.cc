#include "state_db/object_defaults.h"

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
		case RAW_MEMORY_WRITE:
			type = ObjectType::RAW_MEMORY;
		default:
			throw std::runtime_error("invalid type passed to make_default_object (or unimpl)");
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
