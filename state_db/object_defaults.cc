#include "state_db/object_defaults.h"

namespace scs
{

StorageObject
make_default_object(DeltaType d_type)
{
	ObjectType type;
	switch (d_type) {
		case RAW_MEMORY_WRITE:
			type = ObjectType::RAW_MEMORY;
		default:
			throw std::runtime_error("unimpl");
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
