#include "object/object_modification_context.h"

namespace scs
{

ObjectModificationContext::ObjectModificationContext(std::optional<StorageObject> const& obj)
	//: current_type((bool)obj ? std::make_optional(obj -> type()) : std::nullopt)
	{}

/*
bool
ObjectModificationContext::can_accept_mod(DeltaType dt) const
{
	if (!current_type)
	{
		return true;
	}

	// accept no modifications on a key that was explicitly deleted,
	// but can always make some obj be deleted as first phase
	if (dt == DeltaType::DELETE_FIRST)
	{
		return true;
	}
	

	switch (dt)
	{
		case DeltaType::DELETE:
			return true;
		case DeltaType::RAW_MEMORY_WRITE:
			return *current_type == ObjectType::RAW_MEMORY;
		case DeltaType::NONNEGATIVE_INT64_SET:
		case DeltaType::NONNEGATIVE_INT64_ADD:
		case DeltaType::NONNEGATIVE_INT64_SUB:
			return *current_type == ObjectType::NONNEGATIVE_INT64;
		default:
			throw std::runtime_error("unknown delta type in can accept mod");
	}
} */

void
ObjectModificationContext::accept_mod(DeltaType dt)
{
	switch (dt)
	{
		case DeltaType::DELETE_LAST:
			is_deleted_last = true;
			break;
		case DeltaType::DELETE_FIRST:
			is_deleted_first = true;
			break;
		case DeltaType::RAW_MEMORY_WRITE:
			raw_mem_set_called = true;
			break;
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
			int64_set_add_called = true;
			break;
		default:
			throw std::runtime_error("unknown delta type in accept mod");
	}
}


} /* scs */
