#pragma once

#include "rocksdb/db.h"

#include <string>

namespace scs {

class RocksdbWrapper
{
    const std::string dbdir;

    rocksdb::DB* db = nullptr;

    void open_guard() const
    {
        if (db == nullptr) {
            throw std::runtime_error("db not open");
        }
    }

    const rocksdb::ReadOptions read_options;
    const rocksdb::WriteOptions write_options;

  public:
    RocksdbWrapper(const std::string dbdir)
        : dbdir(dbdir + "/")
    {
    }

    void open();
    void clear_previous();
    ~RocksdbWrapper();

    void put(rocksdb::Slice const& key_slice,
             rocksdb::Slice const& value_slice);
    void get(rocksdb::Slice const& key_slice, std::string& value_out);
};

} // namespace scs
