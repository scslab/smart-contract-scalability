#pragma once

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

#include <optional>

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

namespace scs {

class StorageDelta;

class ProxyApplicator
{
#if __cpp_lib_optional >= 202106L
    constexpr static std::optional<StorageObject> null_obj = std::nullopt;
#else
    const std::optional<StorageObject> null_obj = std::nullopt;
#endif

    std::optional<StorageObject> current;

    // memory write

    std::optional<RawMemoryStorage> memory_write;

    // nnint64

    std::optional<set_add_t> nnint64_delta;

    // hash set

    bool do_limit_increase = false;
    uint64_t hs_size_increase = 0;
    std::vector<HashSetEntry> new_hashes;
    std::optional<uint64_t> hs_clear_threshold = std::nullopt;

    // known supply asset
    std::optional<int64_t> delta;

    // deletions
    bool is_deleted = false;

    bool delta_apply_type_guard(StorageDelta const& d) const;
    void make_current(ObjectType obj_type);
    void make_current_nnint64(set_add_t const& delta);

  public:
    ProxyApplicator(std::optional<StorageObject> const& base)
        : current(base)
        , memory_write(std::nullopt)
        , nnint64_delta(std::nullopt)
        , do_limit_increase(false)
        , hs_size_increase(0)
        , new_hashes()
        , hs_clear_threshold(std::nullopt)
        , is_deleted(false)
    {}

    // no-op if result is false, apply if result is true
    // One caveat is that once you try_apply one delta,
    // future deltas will be forced to be of the type
    // This doesn't actually matter, given
    // that everywhere that this is used, a failed attempt
    // to apply reverts the whole transaction.
    bool __attribute__((warn_unused_result))
    try_apply(StorageDelta const& delta);

    std::optional<StorageObject> const& get() const;

    std::vector<StorageDelta> get_deltas() const;

    // type specific methods
    // returns nullopt if type mismatch
    std::optional<int64_t> get_base_nnint64_set_value() const;
};

} // namespace scs
