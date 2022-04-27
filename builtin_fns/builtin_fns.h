#pragma once

#include <cstdint>

namespace scs
{

class WasmRuntime;

class BuiltinFns
{
	/* -- logging functions -- */

	static void
	scs_log(
		int32_t log_offset,
		int32_t log_len);

	static void
	scs_print_debug(int32_t value);

	/* -- runtime functions -- */

	static void 
	scs_return(int32_t offset, int32_t len);

	static void
	scs_get_calldata(int32_t offset, int32_t len);

	static void
	scs_invoke(
		int32_t addr_offset, 
		int32_t methodname, 
		int32_t calldata_offset, 
		int32_t calldata_len,
		int32_t return_offset,
		int32_t return_len);

	static void
	scs_get_msg_sender(
		int32_t addr_offset
		/* addr_len = 32 */
		);

	/* -- raw memory storage fns -- */

	static void
	scs_raw_memory_set(
		int32_t key_offset,
		int32_t key_len,
		int32_t mem_offset,
		int32_t mem_len);

	static void
	scs_raw_memory_get(
		int32_t key_offset,
		int32_t key_len,
		int32_t output_offset,
		int32_t output_max_len);

	static int32_t
	scs_raw_memory_get_len(
		int32_t key_offset,
		int32_t key_len);

	BuiltinFns() = delete;

public:

	static void
	link_fns(WasmRuntime& runtime);
};

} /* scs */
