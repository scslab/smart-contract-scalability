#include "object/delta_applicator.h"

#include "object/object_defaults.h"

#include "xdr/storage_delta.h"

namespace scs
{

bool 
DeltaApplicator::try_apply(StorageDelta const& d)
{
	if (!mod_context.can_accept_mod(d.type()))
	{
		return false;
	}

	if (!base)
	{
		base = make_default_object(d.type());
	}

	// base might be nullopt, if d.type() == DELETE;
	// only stays that way if it (1) starts as nullopt and (2) sees an uninterrupted sequence
	// of DELETE
	if (d.type() != DeltaType::DELETE)
	{
		switch(base -> type())
		{
			case ObjectType::RAW_MEMORY:
			{
				if (!mod_context.raw_mem_set_called)
				{
					base -> raw_memory_storage().data = d.data();
				}
				else if (d.data() != base -> raw_memory_storage().data)
				{
					return false;
				}
			}
			break;
			default:
				throw std::runtime_error("unimpl");
		}
	}

	mod_context.accept_mod(d.type());
	return true;
}

std::optional<StorageObject> const& 
DeltaApplicator::get() const
{
	// deletions are always applied at end
	// can't materialize the deletion
	// internally until we know no more deltas
	// will be applied (i.e. until the obj is
	// taken by the application for use elsewhere).
	if (mod_context.is_deleted)
	{
		return null_obj;
	}
	return base;
}

} /* scs */
