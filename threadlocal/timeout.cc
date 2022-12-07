#include "threadlocal/timeout.h"

namespace scs
{

Notifyable 
Timeout::prep_timer_for(uint64_t request_id)
{
	std::lock_guard lock(mtx);
	request_uuid = request_id;
	std::printf("prepping timer for %lu\n", request_uuid);

	return Notifyable(*this, request_id);
}

std::optional<RpcResult> 
Timeout::await(uint64_t request_id)
{
	std::printf("await %llu\n", request_id);
	std::unique_lock lock(mtx);

	if (request_id != request_uuid) {
		std::printf("invalid await (call with %lu, got %lu)\n", request_id, request_uuid);
		return std::nullopt;
	}

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

	std::printf("setting uuid to 0\n");
	request_uuid = 0;

	return out;
}

void Notifyable::notify(std::optional<RpcResult> const& res)
{
	timeout.notify(res, request_id);
}

void 
Timeout::notify(std::optional<RpcResult> res, uint64_t request_id)
{
	std::lock_guard lock(mtx);
	std::printf("call notify with request id %lu expected %lu\n", request_id, request_uuid);

	if (request_id != request_uuid)
	{
		std::printf("return from invalid wakeup\n");
		return;
	}

	std::printf("return good case\n");
	result = res;
	notified = true;
	cv.notify_all();
}

void 
Timeout::timeout()
{
	std::printf("call timeout\n");
	std::lock_guard lock(mtx);

	request_uuid = 0;

	notified = false;
	result = std::nullopt;

	cv.notify_all();
}

}
