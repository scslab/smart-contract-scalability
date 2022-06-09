#pragma once

#include <cstdint>

#include <wasm_api/wasm_api.h>

namespace scs
{

class BuiltinFns
{
	/* env */
	static int32_t
	memcmp(uint32_t lhs, uint32_t rhs, uint32_t sz);

	static uint32_t
	memset(uint32_t p, uint32_t val, uint32_t sz);

	static uint32_t
	memcpy(uint32_t dst, uint32_t src, uint32_t sz);

	/* -- logging functions -- */

	static void
	scs_log(
		uint32_t log_offset,
		uint32_t log_len);

	static void
	scs_print_debug(uint32_t addr, uint32_t len);

	/* -- runtime functions -- */

	static void 
	scs_return(uint32_t offset, uint32_t len);

	static void
	scs_get_calldata(uint32_t offset, uint32_t len);

	//returns realized return_len
	static uint32_t
	scs_invoke(
		uint32_t addr_offset, 
		uint32_t methodname, 
		uint32_t calldata_offset, 
		uint32_t calldata_len,
		uint32_t return_offset,
		uint32_t return_len);

	static void
	scs_get_msg_sender(
		uint32_t addr_offset
		/* addr_len = 32 */
		);

	static void
	scs_get_self_addr(
		uint32_t addr_offset
		/* addr_len = 32 */
		);

	/* -- raw memory storage fns -- */

	static void
	scs_raw_memory_set(
		uint32_t key_offset,
		uint32_t mem_offset,
		uint32_t mem_len,
		uint32_t priority);

	static uint32_t
	scs_raw_memory_get(
		uint32_t key_offset,
		uint32_t output_offset,
		uint32_t output_max_len);

	static uint32_t
	scs_raw_memory_get_len(
		uint32_t key_offset);

	/* -- delete storage fns -- */
	
	static void
	scs_delete_key_first(
		uint32_t key_offset,
		/* key_len = 32 */
		uint32_t priority);

	static void
	scs_delete_key_last(
		uint32_t key_offset,
		/* key_len = 32 */
		uint32_t priority);

	/* -- nonnegative int fns -- */

	static void
	scs_nonnegative_int64_set_add(
		uint32_t key_offset,
		/* key_len = 32 */
		int64_t set_value,
		int64_t delta,
		uint32_t priority);

	static uint32_t 
	scs_nonnegative_int64_get(
		uint32_t key_offset,
		/* key_len = 32 */
		uint32_t output_offset
		/* output_len = 8 */);


	BuiltinFns() = delete;

public:

	static void
	link_fns(wasm_api::WasmRuntime& runtime);
};

} /* scs */
