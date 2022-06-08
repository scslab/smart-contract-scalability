#include "sdk/calldata.h"
#include "sdk/log.h"
#include "sdk/invoke.h"
#include "sdk/alloc.h"

static uint32_t flag = 0;

EXPORT("pub00000000")
redirect_call_to_proxy()
{
	struct calldata_t {
		sdk::Address callee;
		uint32_t method;
	};

	auto calldata = sdk::get_calldata<calldata_t>();

	sdk::print_debug(calldata);

	flag = 1;
	sdk::invoke(calldata.callee, calldata.method, sdk::EmptyStruct{});
	flag = 0;
}

EXPORT("pub01000000")
self_call()
{
	sdk::log((uint8_t)0xFF);
}

EXPORT("pub02000000")
self_call_reentrant_guard()
{
	if (flag) {
		abort();
	}
}
