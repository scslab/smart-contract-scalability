#pragma once

#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <mutex>

#include "proto/external_call.grpc.pb.h"
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>

namespace scs
{

class ServerRunner
{
	std::mutex mtx;
	std::condition_variable cv;

	bool done = false;

	std::unique_ptr<ExternalCall::Service> impl;
	std::unique_ptr<grpc::Server> server;

	void run()
	{
	   	server -> Wait();

	    std::lock_guard lock(mtx);
	    done = true;
	    cv.notify_all();
	}
public:

	ServerRunner(std::unique_ptr<ExternalCall::Service> _impl, std::string addr)
		: impl(std::move(_impl))
		, server(nullptr)
		{
			grpc::ServerBuilder builder;
			builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
			builder.RegisterService(impl.get());
			server = builder.BuildAndStart();

			std::thread([this] () {
				run();
			}).detach();
		}

	~ServerRunner()
	{
		server->Shutdown();
		auto done_lambda = [this] () {
			return !done;
		};

		std::unique_lock lock(mtx);
		if (done_lambda())
		{
			return;
		}
		cv.wait(lock, done_lambda);
	}

};


}
