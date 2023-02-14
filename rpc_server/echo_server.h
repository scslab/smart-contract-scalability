#pragma once

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

#include <cstdint>

#include "config.h"

#if USE_RPC
#include "proto/external_call.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#else
#include "rpc_server/mock_external_call.h"
#endif

namespace scs {

class EchoServer final : public ExternalCall::Service
{
#if USE_RPC
    grpc::Status SendCall(grpc::ServerContext* context,
                          const GRpcCall* request,
                          GRpcResult* response) override
    {
        response->set_result(request->call());
        return grpc::Status::OK;
    }
#endif
};

} // namespace scs
