#include "state_db/prioritized_tc.h"

#include "object/object_defaults.h"

#include "threadlocal/threadlocal_context.h"

#include <utils/compat.h>

#include <stdexcept>

namespace scs
{

void
PrioritizedTC::log_prioritized_typeclass(PrioritizedStorageDelta const& delta)
{
	if (delta.delta.type() == DeltaType::DELETE_LAST) {
		return;
	}

	TCInstance* tc = new TCInstance(delta.priority, delta_class_from_delta(delta.delta));

	while (true) {
		const auto* ptr = cur_active_instance.load(std::memory_order_relaxed);
		if ((ptr == nullptr) || (ptr -> tc_priority > tc -> tc_priority)) {
			if (cur_active_instance.compare_exchange_strong(ptr, tc, std::memory_order_acq_rel))
			{
				ThreadlocalContextStore::defer_delete(ptr);
				return;
			}
		}
		else
		{
			return;
		}

		SPINLOCK_PAUSE();
	}
}

StorageDeltaClass const& 
PrioritizedTC::get_sdc() const
{
	auto const* ptr = cur_active_instance.load(std::memory_order_acquire);
	if (ptr == nullptr) {
		throw std::runtime_error("cannot get sdc if nexist");
	}
	return ptr -> sdc;
}

void 
PrioritizedTC::reset()
{
	cur_active_instance.store(nullptr, std::memory_order_release);
}

}
