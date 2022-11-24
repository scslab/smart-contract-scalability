#pragma once

#include "xdr/types.h"

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>

#include <xdrpp/msgsock.h>
#include <xdrpp/pollset.h>
#include <xdrpp/socket.h>

namespace scs {

struct RpcAddress
{
    std::string addr;
    std::string port;
};

class RpcAddressDB
{
    std::map<Hash, RpcAddress> address_db;

    mutable std::shared_mutex mtx;

    std::condition_variable_any cv;

    xdr::pollset ps;
    bool ps_is_shutdown = false;
    std::atomic<bool> start_shutdown = false;

    void await_ps_shutdown();

  public:
    RpcAddressDB();

    std::optional<xdr::rpc_sock> connect(Hash const& h);

    void add_mapping(Hash const& h, RpcAddress addr);

    ~RpcAddressDB();
};

} // namespace scs