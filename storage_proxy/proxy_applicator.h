#pragma once

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

    uint64_t hs_size_increase = 0;
    std::vector<Hash> new_hashes;
    bool do_hs_clear = false;

    // std::optional<StorageDelta> overall_delta;
    bool is_deleted = false;

    bool delta_apply_type_guard(StorageDelta const& d) const;
    void make_current(ObjectType obj_type);
    void make_current_nnint64(set_add_t const& delta);

  public:
    ProxyApplicator(std::optional<StorageObject> const& base)
        : current(base)
        , memory_write(std::nullopt)
        , nnint64_delta(std::nullopt)
        , hs_size_increase(0)
        , new_hashes()
        , do_hs_clear(false)
        , is_deleted(false)
    {}

    // no-op if result is false, apply if result is true
    bool __attribute__((warn_unused_result))
    try_apply(StorageDelta const& delta);

    std::optional<StorageObject> const& get() const;

    std::vector<StorageDelta> get_deltas() const;

    // type specific methods
    // returns nullopt if type mismatch
    std::optional<int64_t> get_base_nnint64_set_value() const;
};

} // namespace scs
