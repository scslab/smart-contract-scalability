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
		case DeltaType::NONNEGATIVE_INT64_SET:
		case DeltaType::NONNEGATIVE_INT64_ADD:
		case DeltaType::NONNEGATIVE_INT64_SUB:
			type = ObjectType::NONNEGATIVE_INT64;
			break;
		default:
			throw std::runtime_error(std::string("invalid type passed to make_default_object: ") + std::to_string(d_type));
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
		case ObjectType::NONNEGATIVE_INT64:
			out.nonnegative_int64() = 0;
			break;
		default:
			throw std::runtime_error("unknown object type");
			
	}
	return out;
}

} /* scs */
