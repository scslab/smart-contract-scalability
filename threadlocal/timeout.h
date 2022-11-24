#pragma once

#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <optional>

#include "xdr/rpc.h"

namespace scs
{

class Timeout
{
	std::mutex mtx;
	std::condition_variable cv;

	uint64_t request_uuid = 0;
	bool notified = false;
	std::optional<RpcResult> result; 

public:

	std::optional<RpcResult> await(uint64_t request_id);
	
	void notify(std::optional<RpcResult> res, uint64_t request_id);

	void timeout();
};

}
