#pragma once

#include "xdr/storage_delta.h"

#include <atomic>

namespace scs {

struct TCInstance
{
    uint64_t tc_priority;
    CompressedStorageDeltaClass sdc;
};

class PrioritizedTC
{
    std::atomic<const TCInstance*> cur_active_instance;

  public:
    // This doesn't check validity of the typeclass if applied to an existing
    // storage object. This should only be sent the typeclasses _after_ they've
    // been validated as applicable to the object in the latest snapshot.
    PrioritizedTC()
        : cur_active_instance(nullptr)
    {
    }

    void log_prioritized_typeclass(PrioritizedStorageDelta const& delta);

    CompressedStorageDeltaClass const& get_sdc() const;

    void reset();
};

} // namespace scs