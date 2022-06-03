#include "sdk/log.h"
#include "sdk/calldata.h"

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
