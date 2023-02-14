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

#include "threadlocal/cancellable_rpc.h"

namespace scs {

CancellableRPC::CancellableRPC()
    : mtx()
    , rpcs_allowed(true)
    #if USE_RPC
    , context(std::nullopt)
    , cq()
    #endif
    {
    }

void
CancellableRPC::set_allowed()
{
    rpcs_allowed = true;
}
void
CancellableRPC::cancel_and_set_disallowed()
{
    std::lock_guard lock(mtx);
    rpcs_allowed = false;
    #if USE_RPC
    if (context) {
        context->TryCancel();
    }
    #endif
}

#if USE_RPC
std::optional<RpcResult>
CancellableRPC::send_query(RpcCall const& request, uint64_t uid, std::unique_ptr<ExternalCall::Stub> const& stub)
{

    {
        std::lock_guard lock(mtx);

        if (!rpcs_allowed) {
            return std::nullopt;
        }
        context.reset();
        context.emplace();
    }

    grpc::Status status;

    GRpcCall call;

    const char* input_data_ptr = reinterpret_cast<const char*>(request.calldata.data());
    call.set_call(std::string(input_data_ptr, request.calldata.size()));

    std::unique_ptr<grpc::ClientAsyncResponseReader<GRpcResult> > req(
        stub->AsyncSendCall(&(*context), call, &cq));

    GRpcResult reply; 

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the integer 1.
    req->Finish(&reply, &status, (void*)uid);
    
    void* got_tag;
    bool ok = false;
    // Block until the next result is available in the completion queue "cq".
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or the cq_ is shutting down.
    while(true) {
        bool next_result = cq.Next(&got_tag, &ok);

        if (!next_result) {
            throw std::runtime_error("cq shut down prematurely!");
        }

        if (!ok)
        {
            throw std::runtime_error("docs say ok should always be true after a Finish() call!");
        }

        if (reinterpret_cast<uint64_t>(got_tag) == uid) {
            break;
        }
    }

    if (status.ok())
    {
        RpcResult out;
        const char* data_ptr = reply.result().data();
        out.result.insert(
            out.result.end(),
            data_ptr,
            data_ptr + reply.result().size());

        return out;
    } else if (status.error_code() == grpc::StatusCode::CANCELLED) {
        std::printf("rpc was cancelled, returning nullopt\n");
        return std::nullopt;
    }

    throw std::runtime_error("got invalid rpc status " + status.error_message());
}
#endif


} // namespace scs
