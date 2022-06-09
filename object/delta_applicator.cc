#include "object/delta_applicator.h"

#include "object/object_defaults.h"

#include "debug/debug_utils.h"
#include "debug/debug_macros.h"

#include "xdr/storage_delta.h"

namespace scs
{

bool 
DeltaApplicator::try_apply(StorageDelta const& d)
{
	std::printf("start try apply with delta type %lu\n", d.type());
	if (!typeclass)
	{
		typeclass = std::make_optional<DeltaTypeClass>(d);
	}

	if (!(*typeclass).accepts(d))
	{
		std::printf("typeclass rejects delta\n");
		return false;
	}

//	if (!mod_context.can_accept_mod(d.type()))
//	{
//		return false;
//	}

	if (!base)
	{
		base = make_default_object_by_delta(d);
	}

	if (d.type() == DeltaType::DELETE_FIRST)
	{
		base = std::nullopt;
	}
	else if (base.has_value() && d.type() != DeltaType::DELETE_LAST)
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
					std::printf("return by mismatch reject\n");
					return false;
				}
				std::printf("nothing to do\n");
			}
			break;

			default:
				throw std::runtime_error("unknown object type");
		}
	}

	mod_context.accept_mod(d.type());
	OBJECT_INFO("updated state to %s", debug::storage_object_to_str(base).c_str());
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
	if (mod_context.is_deleted_last)
	{
		OBJECT_INFO("returning deleted object");
		return null_obj;
	}
	OBJECT_INFO("returning %s", debug::storage_object_to_str(base).c_str());
	return base;
}

} /* scs */
