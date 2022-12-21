#pragma once

#include "xdr/types.h"

#include "object/revertable_object.h"

#include <array>
#include <cstdint>
#include <map>
#include <mutex>

#include <sodium.h>

#include "utils/recycling_allocator.h"

namespace scs {

class StorageDelta;


/**
 * IMPORTANT:
 * try_reserve_delta() cannot be called concurrently with get()
 * or clear()
 * This is enforced by the boolean flag in NewKeyCache
 **/
class NewKeyCacheLine
{
#if __cpp_lib_optional >= 202106L
    constexpr static std::optional<StorageObject> null_obj = std::nullopt;
#else
    const std::optional<StorageObject> null_obj = std::nullopt;
#endif

    using allocator_t = RecyclingAllocatorLine<RevertableObject, 100'000>;

    allocator_t allocator;
    std::vector<std::unique_ptr<RevertableObject>> backup_allocator;

    std::map<AddressAndKey, RevertableObject*> map;

    std::mutex mtx;

  public:
    NewKeyCacheLine();

    std::optional<RevertableObject::DeltaRewind> __attribute__((
        warn_unused_result))
    try_reserve_delta(const AddressAndKey& key, const StorageDelta& delta);

    std::optional<StorageObject> const& commit_and_get(
        const AddressAndKey& key);

    void clear();
};

class NewKeyCache
{
    constexpr static uint16_t CACHE_LINES = 73; // medium sized prime

    std::array<NewKeyCacheLine, CACHE_LINES> lines;

    uint8_t hash_key[crypto_shorthash_KEYBYTES];

    void update_key_for_round();

    static uint16_t get_cache_line(const AddressAndKey& key,
                                   const uint8_t* cur_seed);

    bool in_try_reserve_mode;

    void assert_try_reserve_mode() const;
    void assert_get_mode() const;

  public:
    NewKeyCache();

    std::optional<RevertableObject::DeltaRewind> __attribute__((
        warn_unused_result))
    try_reserve_delta(const AddressAndKey& key, const StorageDelta& delta);

    void finalize_modifications();

    std::optional<StorageObject> const& commit_and_get(
        const AddressAndKey& key);

    void clear_for_next_block();
};

} // namespace scs