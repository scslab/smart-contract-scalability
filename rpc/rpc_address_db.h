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

#include "proto/external_call.grpc.pb.h"

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

    std::unique_ptr<ExternalCall::Stub> connect(Hash const& h);

    void add_mapping(Hash const& h, RpcAddress addr);
};

} // namespace scs