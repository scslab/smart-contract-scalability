#pragma once

#include "xdr/types.h"

#include <atomic>
#include <map>
#include <mutex>
#include <memory>
#include <shared_mutex>

#include <xdrpp/msgsock.h>
#include <xdrpp/pollset.h>
#include <xdrpp/socket.h>

#if USE_RPC
#include "proto/external_call.grpc.pb.h"
#endif

namespace scs {

struct RpcAddress
{
    std::string addr;
};

class RpcAddressDB
{
    std::map<Hash, RpcAddress> address_db;

    std::shared_mutex mtx;

  public:
    RpcAddressDB();

    #if USE_RPC
    std::unique_ptr<ExternalCall::Stub> connect(Hash const& h);
    #endif

    void add_mapping(Hash const& h, RpcAddress addr);
};

} // namespace scs
