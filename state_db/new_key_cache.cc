#include "state_db/new_key_cache.h"

#include "crypto/crypto_utils.h"

namespace scs {

NewKeyCacheLine::NewKeyCacheLine()
    : map()
    , mtx()
{}

std::optional<RevertableObject::DeltaRewind>
NewKeyCacheLine::try_reserve_delta(const AddressAndKey& key,
                                   const StorageDelta& delta)
{
    std::lock_guard lock(mtx);
    auto it = map.find(key);
    if (it == map.end()) {
        auto* ptr = allocator.allocate();
        
        if (ptr == nullptr)
        {
            backup_allocator.emplace_back(std::make_unique<RevertableObject>());
            ptr = backup_allocator.back().get();
        }
        it = map.emplace(key, ptr).first;

        //it = map.emplace(key, std::make_unique<RevertableObject>()).first;
    }

    return it->second->try_add_delta(delta);
}

std::optional<StorageObject> const&
NewKeyCacheLine::commit_and_get(const AddressAndKey& key)
{
    auto it = map.find(key);

    if (it == map.end()) {
        return null_obj;
    }
    it->second->commit_round();

    return it->second->get_committed_object();
}

void
NewKeyCacheLine::clear()
{
    map.clear();
    backup_allocator.clear();
    allocator.clear_and_reset();
}

NewKeyCache::NewKeyCache()
    : lines()
    , in_try_reserve_mode(true)
{
    // sets hash_key
    update_key_for_round();
}

void
NewKeyCache::update_key_for_round()
{
    crypto_shorthash_keygen(hash_key);
}

void
NewKeyCache::assert_try_reserve_mode() const
{
    if (!in_try_reserve_mode) {
        throw std::runtime_error("bad access on new key cache (try reserve mode)");
    }
}
void
NewKeyCache::assert_get_mode() const
{
    if (in_try_reserve_mode) {
        throw std::runtime_error("bad access on new key cache (get mode)");
    }
}

uint16_t
NewKeyCache::get_cache_line(const AddressAndKey& key, const uint8_t* hash_key)
{
    static_assert(crypto_shorthash_BYTES == 8);

    uint64_t hash_out;

    if (crypto_shorthash(reinterpret_cast<uint8_t*>(&hash_out),
                         key.data(),
                         key.size(),
                         hash_key)
        != 0) {
        throw std::runtime_error("shorthash fail");
    }

    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
    return ((hash_out & 0xFFFF'FFFF) /* 32 bits */
            * static_cast<uint64_t>(CACHE_LINES))
           >> 32;
}

std::optional<RevertableObject::DeltaRewind>
NewKeyCache::try_reserve_delta(const AddressAndKey& key,
                               const StorageDelta& delta)
{
    assert_try_reserve_mode();
    uint16_t cache_line = get_cache_line(key, hash_key);

    return lines[cache_line].try_reserve_delta(key, delta);
}

void
NewKeyCache::finalize_modifications()
{
    in_try_reserve_mode = false;
}

std::optional<StorageObject> const&
NewKeyCache::commit_and_get(const AddressAndKey& key)
{
    assert_get_mode();
    uint16_t cache_line = get_cache_line(key, hash_key);

    return lines[cache_line].commit_and_get(key);
}

void
NewKeyCache::clear_for_next_block()
{
    for (auto& line : lines) {
        line.clear();
    }
    in_try_reserve_mode = true;
    update_key_for_round();
}

} // namespace scs