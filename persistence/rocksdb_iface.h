#pragma once

#include <cstdint>
#include "mtt/memcached_snapshot_trie/durable_interface.h"

#include "persistence/rocksdb_wrapper.h"

namespace scs
{

template<uint8_t _KEY_LEN_BYTES>
class DirectWriteRocksDBIface
{
    RocksdbWrapper& rdb;

  public:

    DirectWriteRocksDBIface(RocksdbWrapper& rdb)
        : rdb(rdb)
        {}

    constexpr static uint8_t KEY_LEN_BYTES = _KEY_LEN_BYTES;

    using result_t = trie::DurableResult<KEY_LEN_BYTES, std::string>;

    static_assert(std::is_trivially_copyable<trie::TimestampPointerPair>::value, "design error");

    void log_durable_value(trie::TimestampPointerPair const& key,
                           trie::DurableValue<KEY_LEN_BYTES> const& value)
    {

        auto const* ptr = reinterpret_cast<const char*>(&key);
        rocksdb::Slice key_slice(ptr, sizeof(trie::TimestampPointerPair));

        auto const& value_buf = value.get_buffer();

        rocksdb::Slice value_slice(reinterpret_cast<const char*>(value_buf.data()), value_buf.size());

        rdb.put(key_slice, value_slice);
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
};

} // scs
