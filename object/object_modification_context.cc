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
ObjectModificationContext::accept_mod(StorageDelta const& d)
{

	auto dt = d.type();

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
		{
			int64_set_add_called = true;
			int64_t delta = d.set_add_nonnegative_int64().delta;
			if (delta < 0)
			{
				if (__builtin_sub_overflow_p(subtracted_amount, delta, static_cast<int64_t>(0)))
				{
					throw std::runtime_error("overflow in mod_context::accept_mod!  Should have been handled in DeltaApplicator");
				}
				subtracted_amount += -delta;
			}
			else
			{
				if (__builtin_uaddll_overflow(added_amount, (uint64_t)delta, &added_amount))
				{
					added_amount = UINT64_MAX;
				}
			}
		}
		break;
		default:
			throw std::runtime_error("unknown delta type in accept mod");
	}
}


} /* scs */
