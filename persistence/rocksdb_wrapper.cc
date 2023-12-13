#include "persistence/rocksdb_wrapper.h"

#include "utils/mkdir.h"

namespace scs {

rocksdb::Options
make_options()
{
    rocksdb::Options options;

    options.create_if_missing = true;
    options.error_if_exists = true;

    return options;
}

constexpr void
assert_rdb(bool b)
{
    if (!b) {
        throw std::runtime_error("status error");
    }
}

void
RocksdbWrapper::open()
{
    auto opts = make_options();

    utils::mkdir_safe(dbdir);

    rocksdb::Status status = rocksdb::DB::Open(opts, dbdir.c_str(), &db);
    assert_rdb(status.ok());
}
void
RocksdbWrapper::clear_previous()
{
    utils::clear_directory(dbdir);
}

RocksdbWrapper::~RocksdbWrapper()
{
    if (db != nullptr) {
        delete db;
    }
}

void
RocksdbWrapper::put(rocksdb::Slice const& key_slice,
                    rocksdb::Slice const& value_slice)
{
    auto s = db->Put(write_options, key_slice, value_slice);
    assert_rdb(s.ok());
}

void
RocksdbWrapper::get(rocksdb::Slice const& key_slice, std::string& value_out)
{
    // TODO consider rocksdb::PinnableSlice
    auto s = db->Get(read_options, key_slice, &value_out);
    assert_rdb(s.ok());
}

} // namespace scs
