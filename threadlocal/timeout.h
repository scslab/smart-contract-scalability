#pragma once

#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <optional>

#include "xdr/rpc.h"

namespace scs
{

class Timeout;

class Notifyable {
	Timeout& timeout;
	uint64_t request_id;

public:

	Notifyable(Timeout& t, uint64_t request_id)
		: timeout(t)
		, request_id(request_id) {}

	void notify(std::optional<RpcResult> const& res);
};

class Timeout
{
	std::mutex mtx;
	std::condition_variable cv;

	uint64_t request_uuid = 0;
	bool notified = false;
	std::optional<RpcResult> result; 

	friend class Notifyable;

	void notify(std::optional<RpcResult> res, uint64_t request_id);

public:

	Notifyable prep_timer_for(uint64_t request_id);

	std::optional<RpcResult> await(uint64_t request_id);

	void timeout();
};

}
