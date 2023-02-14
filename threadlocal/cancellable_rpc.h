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
