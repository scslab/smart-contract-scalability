#include "sdk/log.h"

EXPORT("pub00000000")
log_value();

int32_t log_value()
{
	uint64_t val = 0xAABBCCDD'EEFF0011;

	sdk::log(val);

	return 0;
}
