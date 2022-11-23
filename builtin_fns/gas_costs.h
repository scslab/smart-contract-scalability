#pragma once

#include <cstdint>

namespace scs
{

#define DECL_COST_LINEAR(fn, cost_static, cost_linear) \
	[[maybe_unused]] \
	constexpr static uint64_t gas_##fn (uint64_t input_len) { \
		return cost_static + input_len * cost_linear ; \
	}
#define DECL_COST_STATIC(fn, cost_static) \
	constexpr static uint64_t gas_##fn =  \
		cost_static ; \
	


// Note that these already include the cost of a function call
// into the host
/* env */

DECL_COST_LINEAR(memcmp, 10, 1)
DECL_COST_LINEAR(memset, 10, 2)
DECL_COST_LINEAR(memcpy, 10, 2)
DECL_COST_LINEAR(strnlen, 10, 1)

/* log */
DECL_COST_LINEAR(log, 50, 1)
// no gas costs on printing debug info

/* runtime */
DECL_COST_LINEAR(return, 50, 2)
DECL_COST_LINEAR(get_calldata, 50, 2)
DECL_COST_STATIC(get_calldata_len, 10)
DECL_COST_LINEAR(invoke, 1000, 2)
DECL_COST_STATIC(get_msg_sender, 50)
DECL_COST_STATIC(get_self_addr, 50)
DECL_COST_STATIC(get_block_number, 10)
DECL_COST_STATIC(get_src_tx_hash, 50)
DECL_COST_STATIC(get_invoked_tx_hash, 50)


}