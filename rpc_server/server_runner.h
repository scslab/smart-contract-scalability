#pragma once

#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <mutex>

#include "config.h"

#if USE_RPC
#include "proto/external_call.grpc.pb.h"
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#else
#include "rpc_server/mock_external_call.h"
#endif

namespace scs
{

class ServerRunner
{
	std::mutex mtx;
	std::condition_variable cv;

	bool done = false;

	#if USE_RPC
	std::unique_ptr<ExternalCall::Service> impl;
	std::unique_ptr<grpc::Server> server;
	#endif

	void run();
public:

	ServerRunner(std::unique_ptr<ExternalCall::Service> _impl, std::string addr);

	~ServerRunner();
};


}
