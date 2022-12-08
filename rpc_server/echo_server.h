#pragma once

#include <cstdint>

#include "config.h"


#if USE_RPC
#include "proto/external_call.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#else
#include "rpc_server/mock_external_call.h"
#endif

namespace scs
{

class EchoServer final : public ExternalCall::Service
{
	#if USE_RPC
    grpc::Status SendCall(grpc::ServerContext* context, const GRpcCall* request, GRpcResult* response) override 
    {
    	response -> set_result(request -> call());
    	return grpc::Status::OK;
    }
    #endif
};

}
