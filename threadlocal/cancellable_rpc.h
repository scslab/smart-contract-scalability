#pragma once

#include <mutex>
#include <optional>
#include <cstdint>

#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>

#include "proto/external_call.grpc.pb.h"

#include "xdr/rpc.h"

namespace scs
{

class CancellableRPC
{
	std::mutex mtx;

	bool rpcs_allowed;

	std::optional<grpc::ClientContext> context;

	std::unique_ptr<grpc::CompletionQueue> cq;

public:

	CancellableRPC()
		: mtx()
		, rpcs_allowed(true)
		, context(std::nullopt)
		, cq()
		{
			std::printf("init a cancellable rpc\n");
			cq = std::make_unique<grpc::CompletionQueue>();
		}

	// can be called from any thread
	void set_allowed();
	void cancel_and_set_disallowed();

	// should only be called from one thread
	std::optional<RpcResult> send_query(RpcCall const& request, uint64_t uid, std::unique_ptr<ExternalCall::Stub> const& stub);
};


}
