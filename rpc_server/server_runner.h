#pragma once

#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <mutex>

#include <xdrpp/pollset.h>

namespace scs
{

class ServerRunner
{
	xdr::pollset ps;

	bool ps_is_shutdown = false;
	std::atomic<bool> start_shutdown = false;

	std::mutex mtx;
	std::condition_variable cv;

public:

	ServerRunner() {
		std::thread([this] {
			while(!start_shutdown)
			{
				std::printf("run serverRunner loop\n");
				ps.run();
				//while(ps.pending())
				//{
				//	ps.poll(1000);
				//}
			}
			std::printf("done server runner thread\n");
			std::lock_guard lock(mtx);
			ps_is_shutdown = true;
			cv.notify_all();
		}).detach();
	}

	xdr::pollset& get_ps()
	{
		return ps;
	}

	~ServerRunner()
	{
		std::printf("start shutdown\n");
		start_shutdown = true;
		auto done_lambda = [this] () -> bool {
			return ps_is_shutdown;
		};

		std::unique_lock lock(mtx);
		if (!done_lambda()) {
			cv.wait(lock, done_lambda);
		}
		std::printf("done shutdown serverRunner\n");
	}
};


}