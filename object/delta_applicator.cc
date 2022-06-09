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

	struct resetter
	{
		bool do_reset_typeclass = false;
		bool do_reset_base = false;

		std::optional<DeltaTypeClass>& ref;
		std::optional<StorageObject>& base;

		resetter(std::optional<DeltaTypeClass>& ref, std::optional<StorageObject>& base)
			: ref(ref)
			, base(base)
			{}
		~resetter()
		{
			if (do_reset_typeclass)
			{
				ref = std::nullopt;
			}
			if (do_reset_base) {
				base = std::nullopt;
			}
		}

		void clear()
		{
			do_reset_typeclass = false;
			do_reset_base = false;
		}
	};

	resetter r(typeclass, base);

	if (!typeclass)
	{
		r.do_reset_typeclass = true;
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
		r.do_reset_base = true;
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
			}
			break;
			case ObjectType::NONNEGATIVE_INT64:
			{
				static_assert(sizeof(long long) == 8, "int width mismatch");

				int64_t base_val = d.set_add_nonnegative_int64().set_value;
				int64_t delta = d.set_add_nonnegative_int64().delta;

				if (delta < 0)
				{
					if (delta == INT64_MIN || base_val < -delta)
					{
						std::printf("returning bc invalid delta\n");
						// such a delta is always invalid, should always be rejected
						return false;
					}
				}


				if (delta < 0)
				{
					int64_t trial = 0;
					if (__builtin_saddll_overflow(mod_context.subtracted_amount, -delta, &trial))
					{
						return false;
					}
					if (trial > base_val)
					{
						return false;
					}
				}

				if (!mod_context.int64_set_add_called)
				{
					base -> nonnegative_int64() = base_val;
				}
			}
			break;

			default:
				throw std::runtime_error("unknown object type");
		}
	}

	mod_context.accept_mod(d);
	r.clear();
	return true;
}

std::optional<StorageObject>
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
		return std::nullopt;
	}

	if (!base)
	{
		return std::nullopt;
	}

	if (base -> type() == ObjectType::NONNEGATIVE_INT64)
	{
		StorageObject out = *base;

		out.nonnegative_int64() = base -> nonnegative_int64() - mod_context.subtracted_amount;

		if (__builtin_add_overflow_p(out.nonnegative_int64(), mod_context.added_amount, static_cast<int64_t>(0)))
		{
			out.nonnegative_int64() = INT64_MAX;
		} else
		{
			out.nonnegative_int64() += mod_context.added_amount;
		}
		OBJECT_INFO("returning %s", debug::storage_object_to_str(out).c_str());
		return out;
	}

	OBJECT_INFO("returning %s", debug::storage_object_to_str(base).c_str());
	return base;
}

} /* scs */
