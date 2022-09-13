#include "object/revertable_object.h"

#include "threadlocal/threadlocal_context.h"

using xdr::operator==;

namespace scs {

/**
 * Algorithm: try_set/revert sets a uint64 to be <unique tag, 48 bits> <active
 *count, 8 bits> <7 empty><is_finalized flag, 1 bit> then sets pointer (separate
 *word).
 *
 * Specifically:
 *
 * 	every modification does the following:
 * 		read tag, then pointer atomically (separately)
 * 			if tag.active_count > 0 but pointer is nullptr, or
 *              tag.active_count == 0 but pointer is not nullpointer, then we
 *              had a shorn read -- retry compute next tag, pointer
 *              compare_exchange on tag, then write pointer
 *
 * 			    uses release_acquire ordering
 *
 * TODO: The extra flags could be used for DELETE_FIRST, but ultimately
 * without a way to prioritize the DELETE_FIRST over other writes,
 * DELETE_FIRST doesn't accomplish all that much (over e.g. instead a 2phase
 * declare_close + close paradigm).
 * That said, DELETE_FIRST doesn't really make sense for the use case I'd been
 * thinking about (closing an account) -- what I really need is a lock,
 * and moreover I need some kind of precedence for some writers over others
 * What I really want is a 2phase account close setup, I think.
 *
 * The unique tag ids ensure that there's a single total order of modifications
 *on the tag. Release-acquire ordering means that one thread might read none of
 *the writes of another thread, the first write, or the first + second (but not
 *the second without the first).
 *
 * The pointer value only changes to/from nullptr (never from one valid pointer
 *to another).
 *
 * If a thread reads "active_count==1" and "non nullptr" in a revert() call,
 * then it writes "active_count=0" and "nullptr".
 *
 * Whichever op goes next must read "active_count=0", and will retry until it
 *sees the nullptr.
 *
 * Because the pointer is read with acquire, and then the tag is written with
 *release, any subsequent op cannot read the earlier nonnull value of the ptr
 *(that was overwritten to nullptr).
 *
 **/

uint64_t
swap_id(uint64_t old_tag, uint64_t new_id)
{
    return new_id + (old_tag & 0xFFFF);
    // return input + (static_cast<uint64_t>(1) << 16);
}

uint64_t
add_inflight(uint64_t input)
{
    return input + (static_cast<uint64_t>(1) << 8);
}

uint64_t
remove_inflight(uint64_t input)
{
    return input - (static_cast<uint64_t>(1) << 8);
}

bool
has_inflight(uint64_t input)
{
    return (input && 0xFF00);
}

bool
is_finalized(uint64_t input)
{
    return input & 1;
}

bool
expect_nonnull(uint64_t input)
{
    return (input & 0xFFFF);
}

void
RevertableBaseObject::Rewind::commit()
{
    if (do_revert) {
        obj->commit();
    }
    do_revert = false;
}

RevertableBaseObject::Rewind::~Rewind()
{
    if (do_revert) {
        obj->revert();
    }
    do_revert = false;
}

std::optional<RevertableBaseObject::Rewind>
RevertableBaseObject::try_set(StorageObject const& new_obj)
{
    if (required_type) {
        if (new_obj.type() != *required_type) {
            return std::nullopt;
        }
    }

    uint64_t new_id = ThreadlocalContextStore::get_uid();
    while (true) {
        const uint64_t t = tag.load(std::memory_order_acquire);
        StorageObject* current = obj.load(std::memory_order_acquire);
        uint64_t expect = t;

        if (expect_nonnull(t) && (current == nullptr)) {
            // shorn read
            continue;
        }
        if ((!expect_nonnull(t)) && (current != nullptr)) {
            // shorn read
            continue;
        }

        if (is_finalized(t)) {
            // never finalize a nullptr
            bool matches = ((*current) == new_obj);
            if (matches) {
                return { Rewind(*this, false) };
            } else {
                return std::nullopt;
            }
        }

        if (current == nullptr) {
            StorageObject* new_candidate = new StorageObject(new_obj);
            uint64_t new_tag = swap_id(t, new_id);
            new_tag = add_inflight(new_tag);

            if (tag.compare_exchange_weak(
                    expect, new_tag, std::memory_order_release)) {
                obj.store(new_candidate, std::memory_order_release);
                return { Rewind(*this, true) };
            }
            delete new_candidate;
            continue;
        }

        // current != nullptr

        bool matches = ((*current) == new_obj);
        if (!matches) {
            return std::nullopt;
        }

        uint64_t new_tag = swap_id(t, new_id);
        new_tag = add_inflight(new_tag);

        if (tag.compare_exchange_weak(
                expect, new_tag, std::memory_order_release)) {
            return { Rewind(*this, true) };
        }
    }
}

void
RevertableBaseObject::commit()
{
    tag.fetch_or(1, std::memory_order_acq_rel);
}

void
RevertableBaseObject::revert()
{
    uint64_t new_id = ThreadlocalContextStore::get_uid();
    while (true) {
        const uint64_t t = tag.load(std::memory_order_acquire);
        StorageObject* current = obj.load(std::memory_order_acquire);
        uint64_t expect = t;

        if (expect_nonnull(t) && (current == nullptr)) {
            // shorn read
            continue;
        }
        if ((!expect_nonnull(t)) && (current != nullptr)) {
            // shorn read
            continue;
        }
        if (is_finalized(t)) {
            return;
        }

        uint64_t new_tag = swap_id(t, new_id);
        new_tag = remove_inflight(new_tag);

        if (tag.compare_exchange_weak(
                expect, new_tag, std::memory_order_release)) {
            if (!has_inflight(new_tag)) {
                obj.store(nullptr, std::memory_order_release);
                ThreadlocalContext::defer_delete(current);
            }
            return;
        }
    }
}

RevertableBaseObject::RevertableBaseObject()
    : tag(0)
    , obj(nullptr)
    , required_type(std::nullopt)
{}

RevertableBaseObject::RevertableBaseObject(const StorageObject& obj)
    : tag(0)
    , obj(nullptr)
    , required_type(obj.type())
{}

RevertableBaseObject::~RevertableBaseObject()
{
    StorageObject* ptr = obj.load(std::memory_order_acquire);

    if (ptr != nullptr) {
        delete ptr;
    }
}

std::optional<RevertableObject::DeltaRewind>
RevertableObject::try_add_delta(const StorageDelta& delta)
{
    switch (delta.type()) {
        case DeltaType::DELETE_LAST: {
            inflight_delete_lasts.fetch_add(1, std::memory_order_relaxed);
            return DeltaRewind(RevertableBaseObject::Rewind(), delta, this);
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD: {
            StorageObject obj;
            obj.type(ObjectType::NONNEGATIVE_INT64);
            obj.nonnegative_int64()
                = delta.set_add_nonnegative_int64().set_value;

            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            int64_t d = delta.set_add_nonnegative_int64().delta;
            if (d < 0) {
                while (true) {
                    int64_t cur_value
                        = total_subtracted.load(std::memory_order_relaxed);
                    if (__builtin_add_overflow_p(
                            cur_value, d, static_cast<int64_t>(0))) {
                        return std::nullopt;
                    }

                    int64_t new_value = cur_value + d;
                    int64_t base = delta.set_add_nonnegative_int64().set_value;

                    if (base < 0 || base + new_value < 0) {
                        return std::nullopt;
                    }

                    if (total_subtracted.compare_exchange_weak(
                            cur_value, new_value, std::memory_order_relaxed)) {
                        return DeltaRewind(std::move(*res), delta, this);
                    }
                }
            } else {
                total_added.add(d);
                return DeltaRewind(std::move(*res), delta, this);
            }
            throw std::runtime_error("assert unreachable");
        }
        case DeltaType::RAW_MEMORY_WRITE: {
            StorageObject obj;
            obj.type(ObjectType::RAW_MEMORY);
            obj.raw_memory_storage().data = delta.data();

            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            return DeltaRewind(std::move(*res), delta, this);
        }
        default:
            throw std::runtime_error(
                "unimplemented deltatype in RevertableObject::try_set");
    }
    throw std::runtime_error("assert unreachable");
}

void
RevertableObject::commit_delta(const StorageDelta& delta)
{
    // no op
}

void
RevertableObject::revert_delta(const StorageDelta& delta)
{
    switch (delta.type()) {
        case DeltaType::DELETE_LAST: {
            inflight_delete_lasts.fetch_sub(1, std::memory_order_relaxed);
            return;
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD: {
            int64_t d = delta.set_add_nonnegative_int64().delta;
            if (d < 0) {
                total_subtracted.fetch_sub(d, std::memory_order_relaxed);
                return;
            }

            // ok to implicitly cast pos int64 to uint64
            total_added.sub(d);
            return;
        }
        case DeltaType::RAW_MEMORY_WRITE: {
            // no op
            return;
        }
    }
    throw std::runtime_error("assert unreachable");
}

RevertableObject::DeltaRewind::DeltaRewind(
    RevertableBaseObject::Rewind&& rewind_base,
    const StorageDelta& delta,
    RevertableObject* obj)
    : rewind_base(std::move(rewind_base))
    , delta(delta)
    , obj(obj)
{}

RevertableObject::DeltaRewind::DeltaRewind(DeltaRewind&& other)
    : rewind_base(std::move(other.rewind_base))
    , delta(other.delta)
    , obj(other.obj)
    , do_rewind(other.do_rewind)
{
    other.do_rewind = false;
}

RevertableObject::DeltaRewind&
RevertableObject::DeltaRewind::operator=(DeltaRewind&& other)
{
    if (do_rewind) {
        obj->revert_delta(delta);
    }

    rewind_base = std::move(other.rewind_base);
    delta = other.delta;
    obj = other.obj;
    do_rewind = other.do_rewind;
    other.do_rewind = false;
    return *this;
}

void
RevertableObject::DeltaRewind::commit()
{
    obj->commit_delta(delta);
    do_rewind = false;
}

RevertableObject::DeltaRewind::~DeltaRewind()
{
    if (do_rewind) {
        obj->revert_delta(delta);
    }
    do_rewind = false;
}

std::optional<StorageObject>
RevertableBaseObject::commit_round_and_reset()
{
    StorageObject* base_obj_ptr = obj.load(std::memory_order_relaxed);
    tag = 0;

    if (base_obj_ptr == nullptr) {
        return std::nullopt;
    }
    obj = nullptr;

    required_type = base_obj_ptr->type();

    StorageObject out = *base_obj_ptr;
    delete base_obj_ptr;

    return out;
}

void
RevertableObject::clear_mods()
{
    total_subtracted.store(0, std::memory_order_relaxed);
    total_added.clear();

    inflight_delete_lasts.store(0, std::memory_order_relaxed);
}

void
RevertableObject::commit_round()
{

    auto new_base = base_obj.commit_round_and_reset();

    if (new_base) {
        committed_base = new_base;
    }

    if (!committed_base) {
        clear_mods();
        return;
    }

    if (inflight_delete_lasts.load(std::memory_order_relaxed) > 0) {
        clear_mods();
        committed_base = std::nullopt;
        return;
    }

    switch (committed_base->type()) {
        case ObjectType::NONNEGATIVE_INT64: {
            int64_t sub = total_subtracted.load(std::memory_order_relaxed);
            committed_base->nonnegative_int64() += sub;

            uint64_t add = total_added.fetch_cap();

            if (__builtin_add_overflow_p(committed_base->nonnegative_int64(),
                                         add,
                                         static_cast<int64_t>(0))) {
                committed_base->nonnegative_int64() = INT64_MAX;
            } else {
                committed_base->nonnegative_int64() += add;
            }
            break;
        }
        case ObjectType::RAW_MEMORY:
            // no op
            break;
        default:
            throw std::runtime_error("unimplemented object type in "
                                     "RevertableObject::commit_round()");
    }
    clear_mods();
}

RevertableObject::RevertableObject()
    : base_obj()
    , total_subtracted(0)
    , total_added()
    , inflight_delete_lasts(0)
    , committed_base(std::nullopt)
{}

RevertableObject::RevertableObject(const StorageObject& committed_base)
    : base_obj(committed_base)
    , total_subtracted(0)
    , total_added()
    , inflight_delete_lasts(0)
    , committed_base(committed_base)
{}

} // namespace scs