#include "sdk/log.h"
#include "sdk/calldata.h"
#include "sdk/invoke.h"

EXPORT("pub00000000")
log_value();

int32_t 
log_value()
{
	uint64_t val = 0xAABBCCDD'EEFF0011;

	sdk::log(val);

	return 0;
}

EXPORT("pub01000000")
log_calldata();

int32_t
log_calldata()
{
	uint64_t calldata = sdk::get_calldata<uint64_t>();
	sdk::log(calldata);
	return 0;
}

EXPORT("pub02000000")
log_data_twice();

int32_t
log_data_twice()
{
	struct calldata_t {
		uint32_t a;
		uint32_t b;
	};

	auto calldata = sdk::get_calldata<calldata_t>();
	sdk::log(calldata.a);
	sdk::log(calldata.b);
	return 0;
}

EXPORT("pub03000000")
log_msg_sender()
{
	auto addr = sdk::get_msg_sender();
	sdk::log(addr);
	return 0;
}
