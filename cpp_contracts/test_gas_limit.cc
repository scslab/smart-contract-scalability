
#include "sdk/log.h"
#include "sdk/raw_memory.h"
#include "sdk/types.h"
#include "sdk/calldata.h"
#include "sdk/constexpr.h"

EXPORT("pub00000000")
consume_gas()
{
	auto num_loops = sdk::get_calldata<uint64_t>();

	uint64_t out = 0; 

	for (uint64_t i = 0; i < num_loops; i++)
	{
		out += i;
		sdk::log(out);
	}
}
EXPORT("pub01000000")
infinite_loop()
{
	uint64_t out = 0;
	while(true)
	{
		sdk::log(out);
	}
}
