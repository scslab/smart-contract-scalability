#pragma once

#include <cstdint>

#include "proto/external_call.pb.h"

namespace scs
{

class EchoServer final : public ExternalCall::Service
{
    grpc::Status SendCall(grpc::ServerContext* context, const GRpcCall* request, GRpcResult* response) override 
    {
    	response -> set_result(request -> call());
    	return grpc::Status::OK;
    }
};

}
