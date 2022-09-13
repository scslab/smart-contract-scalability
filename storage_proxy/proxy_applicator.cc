#include "storage_proxy/proxy_applicator.h"

#include "object/object_defaults.h"

#include "debug/debug_utils.h"
#include "debug/debug_macros.h"

#include "xdr/storage_delta.h"

namespace scs
{

bool 
ProxyApplicator::try_apply(StorageDelta const& d)
{
	std::printf("start try apply with delta type %lu\n", d.type());

	struct resetter
	{
		bool do_reset_base = false;

		std::optional<StorageObject>& base;

		resetter(std::optional<StorageObject>& base)
			: base(base)
			{}
		~resetter()
		{
			if (do_reset_base) {
				base = std::nullopt;
			}
		}

		void clear()
		{
			do_reset_base = false;
		}
	};

	resetter r(base);

	if (!typeclass.can_accept(d))
	{
		return false;
	}

	if (!base)
	{
		r.do_reset_base = true;
		base = make_default_object_by_delta(d);
	}

	//if (d.type() == DeltaType::DELETE_FIRST
	//	|| d.type() == DeltaType::DELETE_LAST)
	if (d.type() == DeltaType::DELETE_LAST)
	{
		base = std::nullopt;
		is_deleted = true;
	}
	else
	{
		if (is_deleted)
		{
			return false;
		}

		if (!base.has_value())
		{
			throw std::runtime_error("no value when there should be");
		}

		switch(base -> type())
		{
			case ObjectType::RAW_MEMORY:
			{
				if (!mem_set_called)
				{
					base -> raw_memory_storage().data = d.data();
					mem_set_called = true;
				}
				else if (d.data() != base -> raw_memory_storage().data)
				{
					return false;
				}
			}
			break;
			case ObjectType::NONNEGATIVE_INT64:
			{
				static_assert(sizeof(long long) == 8, "int width mismatch");

				int64_t base_val = d.set_add_nonnegative_int64().set_value;
				int64_t delta = d.set_add_nonnegative_int64().delta;

				// typeclass check means that all deltas that get this far have the same
				// set_value

				int64_t prev_value = base -> nonnegative_int64();
				if (!nn_int64_set_called)
				{
					prev_value = base_val;
				}

				if (delta < 0)
				{
					if (delta == INT64_MIN || base_val < -delta)
					{
						std::printf("returning bc invalid delta\n");
						// such a delta is always invalid, should always be rejected
						// can't take -INT64_MIN
						return false;
					}

					if (__builtin_add_overflow_p(prev_value, delta, static_cast<int64_t>(0)))
					{
						return false;
					}

					prev_value += delta;
					if (prev_value < 0)
					{
						return false;
					}
				}
				else
				{
					if (__builtin_add_overflow_p(prev_value, delta, static_cast<int64_t>(0)))
					{
						prev_value = INT64_MAX;
					} 
					else
					{
						prev_value += delta;
					}
				}

				base -> nonnegative_int64() = prev_value;
				nn_int64_set_called = true;
			}
			break;

			default:
				throw std::runtime_error("unknown object type");
		}
	}
	typeclass.add(d);
	r.clear();
	return true;
}

std::optional<StorageObject> const&
ProxyApplicator::get() const
{
	// deletions are always applied at end
	// can't materialize the deletion
	// internally until we know no more deltas
	// will be applied (i.e. until the obj is
	// taken by the application for use elsewhere).
	if (is_deleted)
	{
		OBJECT_INFO("returning deleted object");
		return null_obj;
	}

	if (!base)
	{
		return null_obj;
	}
/*
	if (base -> type() == ObjectType::NONNEGATIVE_INT64)
	{
		StorageObject out = *base;

		out.nonnegative_int64() = base -> nonnegative_int64() - subtracted_amount;

		if (__builtin_add_overflow_p(out.nonnegative_int64(), mod_context.added_amount, static_cast<int64_t>(0)))
		{
			out.nonnegative_int64() = INT64_MAX;
		} else
		{
			out.nonnegative_int64() += mod_context.added_amount;
		}
		OBJECT_INFO("returning %s", debug::storage_object_to_str(out).c_str());
		return out;
	}*/

	OBJECT_INFO("returning %s", debug::storage_object_to_str(base).c_str());
	return base;
}

} /* scs */
