/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

// TODO invoke cost linear in invoked wasm size?
DECL_COST_LINEAR(invoke, 10000, 2)
DECL_COST_STATIC(get_msg_sender, 50)
DECL_COST_STATIC(get_self_addr, 50)
DECL_COST_STATIC(get_block_number, 10)
DECL_COST_STATIC(get_src_tx_hash, 50)
DECL_COST_STATIC(get_invoked_tx_hash, 50)

/* general storage */
DECL_COST_STATIC(has_key, 10);

/* raw memory */
DECL_COST_LINEAR(raw_memory_set, 50, 2)
DECL_COST_LINEAR(raw_memory_get, 50, 2)
DECL_COST_STATIC(raw_memory_get_len, 10)

/* delete */
DECL_COST_STATIC(delete_key_last, 50)

/* nonnegative int64 */
DECL_COST_STATIC(nonnegative_int64_set_add, 50)
DECL_COST_STATIC(nonnegative_int64_add, 50)
DECL_COST_STATIC(nonnegative_int64_get, 10)

/* hashset */
// TODO: make some of these costs linear in hashset size
DECL_COST_STATIC(hashset_insert, 100)
DECL_COST_STATIC(hashset_increase_limit, 50)
DECL_COST_STATIC(hashset_clear, 100)
DECL_COST_STATIC(hashset_get_size, 50)
DECL_COST_STATIC(hashset_get_max_size, 50)
DECL_COST_STATIC(hashset_get_index_of, 100)
DECL_COST_STATIC(hashset_get_index, 50)

/* asset */
DECL_COST_STATIC(asset_add, 200);

/* crypto */
DECL_COST_STATIC(hash, 10000);
DECL_COST_STATIC(check_sig_ed25519, 10000);

/* contracts */
DECL_COST_LINEAR(create_contract, 10000, 100);
DECL_COST_STATIC(deploy_contract, 100);

/* witness */
DECL_COST_LINEAR(get_witness, 50, 2);
DECL_COST_STATIC(get_witness_len, 10);

} // namespace scs
