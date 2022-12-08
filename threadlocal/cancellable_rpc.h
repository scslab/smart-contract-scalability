#pragma once

#include <mutex>
#include <optional>
#include <cstdint>

#include "config.h"

#if USE_RPC
#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>

#include "proto/external_call.grpc.pb.h"

#endif

#include "xdr/rpc.h"

namespace scs
{

class CancellableRPC
{
	std::mutex mtx;

	bool rpcs_allowed;

	#if USE_RPC
		std::optional<grpc::ClientContext> context;
		grpc::CompletionQueue cq;
	#endif

public:

	CancellableRPC();

	// can be called from any thread
	void set_allowed();
	void cancel_and_set_disallowed();

	#if USE_RPC
	// should only be called from one thread
	std::optional<RpcResult> send_query(RpcCall const& request, uint64_t uid, std::unique_ptr<ExternalCall::Stub> const& stub);
	#endif
};


}
