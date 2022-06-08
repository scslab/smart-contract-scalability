#include "object/object_modification_context.h"

namespace scs
{

ObjectModificationContext::ObjectModificationContext(std::optional<StorageObject> const& obj)
	: current_type((bool)obj ? std::make_optional(obj -> type()) : std::nullopt)
	{}


bool
ObjectModificationContext::can_accept_mod(DeltaType dt) const
{
	if (!current_type) 
	{
		return true;
	}

	switch (dt)
	{
		case DeltaType::DELETE:
			return true;
		case DeltaType::RAW_MEMORY_WRITE:
			return *current_type == ObjectType::RAW_MEMORY;
		default:
			throw std::runtime_error("unknown delta type in can accept mod");
	}
}

void
ObjectModificationContext::accept_mod(DeltaType dt)
{
	switch (dt)
	{
		case DeltaType::DELETE:
			is_deleted = true;
			break;
		case DeltaType::RAW_MEMORY_WRITE:
			raw_mem_set_called = true;
			break;
		default:
			throw std::runtime_error("unknown delta type in accept mod");
	}
}


} /* scs */
