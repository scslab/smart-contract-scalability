#include "storage_proxy/proxy_applicator.h"

#include "object/object_defaults.h"

#include "debug/debug_utils.h"
#include "debug/debug_macros.h"

#include "xdr/storage_delta.h"

#include "object/make_delta.h"

namespace scs
{

bool
ProxyApplicator::delta_apply_type_guard(StorageDelta const& d) const
{
	if (!current)
	{
		return true;
	}
	switch(d.type())
	{
		case DeltaType::RAW_MEMORY_WRITE:
		{
			return current -> type() == ObjectType::RAW_MEMORY;
		}
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
		{
			return current -> type() == ObjectType::NONNEGATIVE_INT64;
		}
		default:
			throw std::runtime_error("unknown deltatype");
	}
}

bool
ProxyApplicator::try_apply(StorageDelta const& d)
{
	if (d.type() == DeltaType::DELETE_LAST)
	{
		is_deleted = true;
		current = std::nullopt;
		return true;
	}

	if (is_deleted)
	{
		return false;
	}

	if (!delta_apply_type_guard(d))
	{
		return false;
	}

	switch(d.type())
	{
		case DeltaType::RAW_MEMORY_WRITE:
		{
			overall_delta = d;
			break;
		}
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
		{
			if (!overall_delta)
			{
				overall_delta = d;
				break;
			} 
			else
			{
				if (overall_delta->set_add_nonnegative_int64().set_value != d.set_add_nonnegative_int64().set_value)
				{
					return false;
				}
				int64_t d_delta = d.set_add_nonnegative_int64().delta;

				if (d_delta < 0)
				{
					if (__builtin_add_overflow_p(d_delta, overall_delta->set_add_nonnegative_int64().delta, static_cast<int64_t>(0)))
					{
						return false;
					}
					int64_t new_delta = overall_delta -> set_add_nonnegative_int64().delta + d_delta;
					int64_t set_value = overall_delta -> set_add_nonnegative_int64().set_value;

					if (__builtin_add_overflow_p(set_value, new_delta, static_cast<int64_t>(0)))
					{
						return false;
					}
					if (set_value + new_delta < 0)
					{
						return false;
					}

					overall_delta -> set_add_nonnegative_int64().delta = new_delta;
					break;
				}
				if (__builtin_add_overflow_p(d_delta, overall_delta -> set_add_nonnegative_int64().delta, static_cast<int64_t>(0)))
				{
					overall_delta -> set_add_nonnegative_int64().delta = INT64_MAX;
					break;
				}

				overall_delta -> set_add_nonnegative_int64().delta += d_delta;
				break;
			}
		}
		default:
			throw std::runtime_error("unknown deltatype");
	}

	if (!overall_delta)
	{
		throw std::runtime_error("should not reach end of try_apply without an overall_delta");
	}

	current = make_object_from_delta(*overall_delta);
	return true;
}

/*
bool 
ProxyApplicator::try_apply(StorageDelta const& d)
{
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
*/

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

	if (!current)
	{
		return null_obj;
	}

	OBJECT_INFO("returning %s", debug::storage_object_to_str(current).c_str());
	return current;
}

std::vector<StorageDelta>
ProxyApplicator::get_deltas() const
{
	if (!overall_delta)
	{
		if (is_deleted)
		{
			//std::printf("ProxyApplicator::get_deltas: return delete_last (no delta)\n");
			return { make_delete_last() };
		}
		//std::printf("ProxyApplicator::get_deltas: return null (no delta)\n");
		return {};
	}

	if (is_deleted)
	{
		//std::printf("ProxyApplicator::get_deltas: return (delta, delete_last)\n");
		return { *overall_delta, make_delete_last() };
	}

	//std::printf("ProxyApplicator::get_deltas: return delete_last\n");

	return { *overall_delta };
}

} /* scs */
