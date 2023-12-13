#include "persistence/rocksdb_wrapper.h"

#include "utils/mkdir.h"

namespace scs {

namespace detail {

/**
 * This section adapted from rocksdb/util/stderr_logger.{cc,h}.
 * Original license reproduced below.
 */

//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

class OptionsLogger : public rocksdb::Logger
{
  public:
    explicit OptionsLogger()
        : Logger(rocksdb::InfoLogLevel::INFO_LEVEL)
    {
    }

    ~OptionsLogger() override {}

    // Brings overloaded Logv()s into scope so they're not hidden when we
    // override a subset of them.
    using rocksdb::Logger::Logv;

    virtual void Logv(const char* format, va_list ap) override
    {
        vfprintf(stdout, format, ap);
        fprintf(stdout, "\n");
    }
};

/**
 * End adaptation of rocksdb/util/stderr_logger.{cc,h}
 */

} // namespace detail

rocksdb::Options
make_options()
{
    rocksdb::Options options;

    options.create_if_missing = true;
    options.error_if_exists = true;

    detail::OptionsLogger logger;
    options.Dump(&logger);
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
