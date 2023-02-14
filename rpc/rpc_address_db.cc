/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "rpc/rpc_address_db.h"

#if USE_RPC
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#endif

namespace scs {

RpcAddressDB::RpcAddressDB()
    : address_db()
    , mtx()
{}

#if USE_RPC
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
}
#endif

void
RpcAddressDB::add_mapping(Hash const& h, RpcAddress addr)
{
    std::lock_guard lock(mtx);

    address_db[h] = addr;
}

} // namespace scs
