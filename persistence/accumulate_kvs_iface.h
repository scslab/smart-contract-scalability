#pragma once

#include "mtt/memcached_snapshot_trie/durable_interface.h"

#include <map>
#include <mutex>
#include <type_traits>
#include <utility>

#include <utils/debug_utils.h>
#include <utils/threadlocal_cache.h>

#include "persistence/rocksdb_wrapper.h"


namespace scs {

template<uint8_t _KEY_LEN_BYTES, uint32_t TLCACHE_SIZE>
class AccumulateKVsInterface
{
  public:
    constexpr static uint8_t KEY_LEN_BYTES = _KEY_LEN_BYTES;
    using result_t = trie::DurableResult<KEY_LEN_BYTES, std::string>;
    using value_t = trie::DurableValue<KEY_LEN_BYTES>;
    using pair_t = std::pair<trie::TimestampPointerPair, value_t>;

  private:
    utils::ThreadlocalCache<std::vector<pair_t>, TLCACHE_SIZE> cache;
    RocksdbWrapper& rdb;

  public:

    AccumulateKVsInterface(RocksdbWrapper& rdb)
        : cache()
        , rdb(rdb)
        {}

    void log_durable_value(trie::TimestampPointerPair const& key,
                           trie::DurableValue<KEY_LEN_BYTES> const& value)
    {
        auto& buf = cache.get();
        buf.emplace_back(key, value);
    }

    result_t restore_durable_value(trie::TimestampPointerPair const& key) const
    {
        auto const* ptr = reinterpret_cast<const char*>(&key);

        result_t out;

        auto& data = out.get_backing_data();

        rocksdb::Slice key_slice(ptr, sizeof(trie::TimestampPointerPair));

        rdb.get(key_slice, data);

        return out;
    }

    void swap_buffers(std::array<std::vector<pair_t>, TLCACHE_SIZE>& old_buffers) {
        auto& bufs = cache.get_objects();
        for (size_t i = 0; i < TLCACHE_SIZE; i++)
        {
            if (!bufs[i])
            {
                bufs[i].emplace();
            }
            std::swap(*bufs[i], old_buffers[i]);
        }
    }
};

} // namespace scs
