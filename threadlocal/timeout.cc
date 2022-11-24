#include "threadlocal/timeout.h"

namespace scs
{

std::optional<RpcResult> 
Timeout::await(uint64_t request_id)
{
	std::unique_lock lock(mtx);

	auto done = [this, request_id] {
		return (request_id != request_uuid) || notified;
	};

	if (!done())
	{
		cv.wait(lock, done);
	}
	notified = false;

	std::optional<RpcResult> out = result;
	result = std::nullopt;

	request_uuid = 0;

	return out;
}

void 
Timeout::notify(std::optional<RpcResult> res, uint64_t request_id)
{
	std::lock_guard lock(mtx);

	if (request_id != request_uuid)
	{
		return;
	}

	result = res;
	notified = true;
	cv.notify_all();
}

void 
Timeout::timeout()
{
	std::lock_guard lock(mtx);

	request_uuid = 0;

	notified = false;
	result = std::nullopt;

	cv.notify_all();
}

}
