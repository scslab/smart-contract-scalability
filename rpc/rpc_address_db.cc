#include "rpc/rpc_address_db.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

namespace scs {

RpcAddressDB::RpcAddressDB()
    : address_db()
    , mtx()
{}

std::unique_ptr<ExternalCall::Stub>
RpcAddressDB::connect(Hash const& h)
{
    std::shared_lock lock(mtx);

    auto it = address_db.find(h);
    if (it == address_db.end()) {
        return {};
    }

    // for now, make a new channel on each call
    return 
        ExternalCall::NewStub(grpc::CreateChannel(it -> second.addr, grpc::InsecureChannelCredentials()));
    //return std::make_shared<xdr::unique_sock>(
    //    xdr::tcp_connect(it->second.addr.c_str(), it->second.port.c_str()));
}

void
RpcAddressDB::add_mapping(Hash const& h, RpcAddress addr)
{
    std::lock_guard lock(mtx);

    address_db[h] = addr;
}

} // namespace scs
