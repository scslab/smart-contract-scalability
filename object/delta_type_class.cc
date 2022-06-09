#include "object/delta_type_class.h"

#include "object/comparators.h"

namespace scs
{


DeltaTypeClass::DeltaTypeClass(StorageDelta const& origin_delta, DeltaPriority const& priority)
	: priority(priority)
	, base_delta(origin_delta)
	{}

DeltaTypeClass::DeltaTypeClass(StorageDelta const& origin_delta)
	: priority(std::nullopt)
	, base_delta(origin_delta)
	{}

bool
DeltaTypeClass::is_lower_rank_than(StorageDelta const& new_delta, DeltaPriority const& new_priority) const
{
	if (!priority)
	{
		throw std::runtime_error("cannot compare rank without self priority");
	}
	if (new_delta.type() == DeltaType::DELETE_LAST)
	{
		return false;
	}
	if (base_delta.type() == DeltaType::DELETE_FIRST)
	{
		return true;
	}
	return (*priority) < new_priority;
}

bool 
DeltaTypeClass::is_lower_rank_than(DeltaTypeClass const& other) const
{
	if (!other.priority)
	{
		throw std::runtime_error("cannot compare rank without other priority");
	}
	return is_lower_rank_than(other.base_delta, *other.priority);
}


bool 
DeltaTypeClass::accepts(StorageDelta const& delta) const
{
	if (base_delta.type() == DeltaType::DELETE_FIRST)
	{
		return delta.type() == DeltaType::DELETE_FIRST;
	}

	if (delta.type() == DeltaType::DELETE_LAST)
	{
		return true;
	}

	if (base_delta.type() != delta.type()) {
		return false;
	}

	if (base_delta.type() == DeltaType::RAW_MEMORY_WRITE)
	{
		return base_delta.data() == delta.data();
	}

	if (base_delta.type() == DeltaType::NONNEGATIVE_INT64_SET_ADD)
	{
		return base_delta.set_add_nonnegative_int64().set_value
			== delta.set_add_nonnegative_int64().set_value;
	}

	throw std::runtime_error("unhandled case in DeltaTypeClass::accepts");
}

} /* scs */
