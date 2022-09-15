#pragma once

#include "xdr/storage.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <type_traits>

#include "mtt/utils/non_movable.h"

#include "utils/atomic_uint128.h"

#include "xdr/storage_delta.h"

namespace scs {

class RevertableBaseObject
{
    std::atomic<uint64_t> tag;

    std::atomic<StorageObject*> obj;

    std::optional<ObjectType> required_type;

  public:
    class Rewind : public utils::NonCopyable
    {
        RevertableBaseObject* obj;
        bool do_revert;

      public:
        Rewind(RevertableBaseObject& obj, bool do_revert)
            : obj(&obj)
            , do_revert(do_revert)
        {}

        Rewind(Rewind&& other)
            : obj(other.obj)
            , do_revert(other.do_revert)
        {
            other.do_revert = false;
        }

        Rewind()
            : obj(nullptr)
            , do_revert(false)
        {}

        Rewind& operator=(Rewind&& other)
        {
            if (do_revert) {
                obj->revert();
            }

            obj = other.obj;
            do_revert = other.do_revert;
            other.do_revert = false;
            return *this;
        }

        void commit();

        ~Rewind();
    };

  private:
    friend class Rewind;

    void revert();
    void commit();

  public:
    RevertableBaseObject();
    RevertableBaseObject(const StorageObject& obj);

    std::optional<Rewind> __attribute__((warn_unused_result))
    try_set(StorageObject const& new_obj);

    std::optional<StorageObject> __attribute__((warn_unused_result))
    commit_round_and_reset();

    ~RevertableBaseObject();
};

// try_add, commit, revert, get_committed need to be threadsafe wrt to each
// other. none of these should be called concurrently with commit_round.
class RevertableObject
{
    RevertableBaseObject base_obj;

    // for raw memory
    // nothing

    // for nn_int64

    std::atomic<int64_t> total_subtracted;
    AtomicUint128 total_added;

    std::atomic<uint32_t> inflight_delete_lasts;

    std::optional<StorageObject> committed_base;

  public:
    class DeltaRewind : public utils::NonCopyable
    {
        RevertableBaseObject::Rewind rewind_base;

        StorageDelta delta;

        RevertableObject* obj;

        bool do_rewind = true;

        std::optional<StorageObject> committed_base;

      public:
        DeltaRewind(RevertableBaseObject::Rewind&& rewind_base,
                    const StorageDelta& delta,
                    RevertableObject* obj);

        DeltaRewind(DeltaRewind&& other);
        DeltaRewind& operator=(DeltaRewind&& other);

        void commit();

        ~DeltaRewind();
    };

    RevertableObject();
    RevertableObject(const StorageObject& finalized_obj);

    // TODO make && on input
    std::optional<DeltaRewind> __attribute__((warn_unused_result))
    try_add_delta(const StorageDelta& delta);

  private:
    friend class DeltaRewind;

    void commit_delta(const StorageDelta& delta);
    void revert_delta(const StorageDelta& delta);
    void clear_mods();

  public:
    void commit_round();

    std::optional<StorageObject> const& get_committed_object() const
    {
        return committed_base;
    }
};

} // namespace scs
