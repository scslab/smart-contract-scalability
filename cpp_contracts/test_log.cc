#include "sdk/log.h"

void
EXPORT("pub00000000")
log_value();

void log_value()
{
	uint64_t val = 0xAABBCCDD'EEFF0011;

	sdk::log(val);
}
