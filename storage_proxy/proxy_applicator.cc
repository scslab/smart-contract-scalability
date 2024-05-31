/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "storage_proxy/proxy_applicator.h"

#include "object/object_defaults.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "xdr/storage_delta.h"

#include "object/make_delta.h"

#include "hash_set/utils.h"

using xdr::operator==;

namespace scs {

bool
ProxyApplicator::delta_apply_type_guard(StorageDelta const& d) const
{
    if (!current) {
        return true;
    }
    switch (d.type()) {
        case DeltaType::RAW_MEMORY_WRITE: {
            return current->body.type() == ObjectType::RAW_MEMORY;
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD: {
            return current->body.type() == ObjectType::NONNEGATIVE_INT64;
        }
        case DeltaType::HASH_SET_INSERT:
        case DeltaType::HASH_SET_INCREASE_LIMIT:
        case DeltaType::HASH_SET_CLEAR:
        {
        	return current -> body.type() == ObjectType::HASH_SET;
        }
        case DeltaType::ASSET_OBJECT_ADD:
        {
            return current -> body.type() == ObjectType::KNOWN_SUPPLY_ASSET;
        }
        default:
            throw std::runtime_error("unknown deltatype");
    }
}

std::optional<set_add_t>
make_nnint64_delta(int64_t base, int64_t old_delta, int64_t new_delta)
{
    if (new_delta < 0) {
        if (__builtin_add_overflow_p(
                new_delta, old_delta, static_cast<int64_t>(0))) {
            return std::nullopt;
        }

        new_delta += old_delta;

        if (__builtin_add_overflow_p(
                base, new_delta, static_cast<int64_t>(0))) {

            if (new_delta < 0) {
                // base + new_delta < INT64_MIN
                return std::nullopt;
            } else {
                // base + new_delta > INT64_MAX

                // reject if and only if base+old_delta < 0
                // but base + old_delta < 0 only if base < 0

                if (base < 0) {
                    if (old_delta < 0) {
                        throw std::runtime_error(
                            "invalid input to make_nnint64_delta");
                    }

                    return std::nullopt;
                }

                set_add_t out;
                out.set_value = base;
                out.delta = new_delta;
                return out;

                // return make_nonnegative_int64_set_add(base, new_delta);
            }
        }
        if (base + new_delta < 0) {
            return std::nullopt;
        }

        set_add_t out;
        out.set_value = base;
        out.delta = new_delta;
        return out;

        // return make_nonnegative_int64_set_add(base, new_delta);
    }
    // else new_delta >= 0
    if (__builtin_add_overflow_p(
            old_delta, new_delta, static_cast<int64_t>(0))) {
        new_delta = INT64_MAX;
    } else {
        new_delta += old_delta;
    }

    set_add_t out;
    out.set_value = base;
    out.delta = new_delta;
    return out;

    // return make_nonnegative_int64_set_add(base, new_delta);
}

void
ProxyApplicator::make_current(ObjectType obj_type)
{
    if (!current) {
        current = StorageObject();
        current->body.type(obj_type);

        switch(obj_type)
        {
        	case ObjectType::RAW_MEMORY:
        	case ObjectType::NONNEGATIVE_INT64:
        		break; // nothing to do
        	case ObjectType::HASH_SET:
        		current -> body.hash_set().max_size = START_HASH_SET_SIZE;
        		break;
            case ObjectType::KNOWN_SUPPLY_ASSET:
                // nothing to do
                break;
        	default:
        		throw std::runtime_error("unimplemented object type");
        }
    }
    if (current->body.type() != obj_type) {
        throw std::runtime_error("type mismatch");
    }
}

void
ProxyApplicator::make_current_nnint64(set_add_t const& delta)
{
    make_current(ObjectType::NONNEGATIVE_INT64);

    if (__builtin_add_overflow_p(
            delta.set_value, delta.delta, static_cast<int64_t>(0))) {
        if (delta.delta > 0) {
            current->body.nonnegative_int64() = INT64_MAX;
        } else {
            throw std::runtime_error("should not have negative overflow!");
        }
    } else {
        current->body.nonnegative_int64() = delta.set_value + delta.delta;
    }
}

bool
ProxyApplicator::try_apply(StorageDelta const& d, uint64_t priority)
{
    if (d.type() == DeltaType::DELETE_LAST) {
        is_deleted = true;
        current = std::nullopt;
        return true;
    }

    if (is_deleted) {
        return false;
    }

    if (!delta_apply_type_guard(d)) {
        return false;
    }

    switch (d.type()) {
        case DeltaType::RAW_MEMORY_WRITE: {
            if (!memory_write) {
                memory_write = RawMemoryStorage();
            }
            memory_write->data = d.data();
            make_current(ObjectType::RAW_MEMORY);
            current->body.raw_memory_storage() = *memory_write;
            write_priority = std::min(priority, write_priority);
            return true;
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD: {
            if (!nnint64_delta) {
                auto res = make_nnint64_delta(
                    d.set_add_nonnegative_int64().set_value,
                    0,
                    d.set_add_nonnegative_int64().delta);
                if (res) {
                    nnint64_delta = *res;
                    make_current_nnint64(*nnint64_delta);
                    nnint64_set_add_priority = std::min(nnint64_set_add_priority, priority);
                    return true;
                } else {
                    return false;
                }
            } else {
                if (nnint64_delta->set_value
                    != d.set_add_nonnegative_int64().set_value) {

                    return false;
                }

                auto res
                    = make_nnint64_delta(nnint64_delta->set_value,
                                         nnint64_delta->delta,
                                         d.set_add_nonnegative_int64().delta);

                if (!res) {
                    return false;
                }
                nnint64_delta = *res;
                make_current_nnint64(*nnint64_delta);
                nnint64_set_add_priority = std::min(nnint64_set_add_priority, priority);
                return true;
            }
        }
        case DeltaType::HASH_SET_INSERT:
        {
        	make_current(ObjectType::HASH_SET);

        	for (auto const& h : current -> body.hash_set().hashes)
        	{
        		if (h == d.hash())
        		{
        			return false;
        		}
        	}

        	if (current -> body.hash_set().hashes.size() >= current -> body.hash_set().max_size)
        	{
        		return false;
        	}

            if (hs_clear_threshold && *hs_clear_threshold >= d.hash().index)
            {
                return false;
            }

        	current -> body.hash_set().hashes.push_back(d.hash());
            normalize_hashset(current -> body.hash_set());
        	new_hashes.emplace_back(priority, d.hash());
        	return true;
        }
        case DeltaType::HASH_SET_INCREASE_LIMIT:
        {
        	make_current(ObjectType::HASH_SET);

        	hs_size_increase += d.limit_increase();
        	// limit change does not take effect until next block
            do_limit_increase = true;
        	return true;
        }
        case DeltaType::HASH_SET_CLEAR:
    	{
    		make_current(ObjectType::HASH_SET);
            if (!hs_clear_threshold)
            {
                hs_clear_threshold = d.threshold();
            } else
            {
                hs_clear_threshold = std::max(d.threshold(), *hs_clear_threshold);
            }

            clear_hashset(current -> body.hash_set(), *hs_clear_threshold);
    		return true;
    	}
        case DeltaType::ASSET_OBJECT_ADD:
        {
            make_current(ObjectType::KNOWN_SUPPLY_ASSET);

            // check overflow on delta
            if (delta.has_value() && __builtin_add_overflow_p(*delta, d.asset_delta(), static_cast<int64_t>(0)))
            {
                return false;
            }

            uint64_t cur_asset_value = current->body.asset().amount;
            if (__builtin_add_overflow_p(cur_asset_value, d.asset_delta(), static_cast<uint64_t>(0)))
            {
                return false;
            }

            if (!delta)
            {
                delta = d.asset_delta();
            }
            else
            {
                *delta += d.asset_delta();
            }

            current->body.asset().amount += d.asset_delta();
            asset_priority = std::min(asset_priority, priority);
            return true;
        }
        default:
            throw std::runtime_error("unknown deltatype");
    }

    throw std::runtime_error("unreachable");
}

std::optional<StorageObject> const&
ProxyApplicator::get() const
{
    // deletions are always applied at end
    // can't materialize the deletion
    // internally until we know no more deltas
    // will be applied (i.e. until the obj is
    // taken by the application for use elsewhere).
    if (is_deleted) {
        OBJECT_INFO("returning deleted object");
        return null_obj;
    }

    OBJECT_INFO("returning %s", debug::storage_object_to_str(current).c_str());
    return current;
}

std::vector<PrioritizedStorageDelta>
ProxyApplicator::get_deltas() const
{
    if (!is_deleted && (!current)) {
        return {};
    }

    std::vector<PrioritizedStorageDelta> out;

    if (memory_write) {
        out.emplace_back(make_raw_memory_write(
            xdr::opaque_vec<RAW_MEMORY_MAX_LEN>(memory_write->data)),
        write_priority);
    }

    if (nnint64_delta) {
        out.emplace_back(
            make_nonnegative_int64_set_add(nnint64_delta->set_value,
                                                     nnint64_delta->delta),
            nnint64_set_add_priority);
    }

    if (do_limit_increase) {
        static_assert(
            MAX_HASH_SET_SIZE <= UINT16_MAX,
            "change uint16 type on make_hash_set_increase_limit to wider uint");
        out.emplace_back(
            make_hash_set_increase_limit(std::min(
                hs_size_increase, static_cast<uint64_t>(MAX_HASH_SET_SIZE))),
            0);
    }

    for (auto const& h : new_hashes) {
        out.emplace_back(
            make_hash_set_insert(h.entry), h.priority);
    }

    if (hs_clear_threshold.has_value()) {
        out.emplace_back(
            make_hash_set_clear(*hs_clear_threshold),
            0); // empty priority
    }

    if (delta.has_value())
    {
        out.emplace_back(make_asset_add(*delta), asset_priority);
    }

    if (is_deleted) {
        out.emplace_back(make_delete_last(), 0);
    }

    return out;
}

std::optional<int64_t>
ProxyApplicator::get_base_nnint64_set_value() const
{

    if (nnint64_delta)
    {
        return nnint64_delta->set_value;
    }

    if (current) {
        if (current->body.type() != ObjectType::NONNEGATIVE_INT64) {
            return std::nullopt;
        }
        return current->body.nonnegative_int64();
    }
    return 0;
}

} // namespace scs
