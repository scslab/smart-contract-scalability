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

	static uint32_t
	strnlen(uint32_t str, uint32_t max_len);

	/* -- logging functions -- */

	static void
	scs_log(
		uint32_t log_offset,
		uint32_t log_len);

	static void
	scs_print_debug(uint32_t addr, uint32_t len);

	static void
	scs_print_c_str(uint32_t addr, uint32_t len);

	/* -- runtime functions -- */

	static void 
	scs_return(uint32_t offset, uint32_t len);

	static void
	scs_get_calldata(
		uint32_t output_offset,
		uint32_t calldata_slice_start,
		uint32_t calldata_slice_end);

	static uint32_t
	scs_get_calldata_len();

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

	static uint64_t
	scs_get_block_number();

	static void
	scs_get_src_tx_hash(
		uint32_t hash_offset
		/* hash_len = 32 */);

	static void
	scs_get_invoked_tx_hash(
		uint32_t hash_offset
		/* hash_len = 32 */);

	static void
	scs_gas(uint64_t consumed_gas);

	/* -- general storage -- */
	static uint32_t
	scs_has_key(uint32_t key_offset
		/* key_len = 32 */);

	/* -- raw memory storage fns -- */

	static void
	scs_raw_memory_set(
		uint32_t key_offset,
		uint32_t mem_offset,
		uint32_t mem_len);

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
	scs_delete_key_last(
		uint32_t key_offset
		/* key_len = 32 */);

	/* -- nonnegative int fns -- */

	static void
	scs_nonnegative_int64_set_add(
		uint32_t key_offset,
		/* key_len = 32 */
		int64_t set_value,
		int64_t delta);

	static void
	scs_nonnegative_int64_add(
		uint32_t key_offset,
		/* key_len = 32 */
		int64_t delta);

	static uint32_t 
	scs_nonnegative_int64_get(
		uint32_t key_offset,
		/* key_len = 32 */
		uint32_t output_offset
		/* output_len = 8 */);

	/* -- hashset -- */

	static void
	scs_hashset_insert(
		uint32_t key_offset,
		/* key_len = 32 */
		uint32_t hash_offset
		/* hash_len = 32 */,
		uint64_t threshold);

	static void
	scs_hashset_increase_limit(
		uint32_t key_offset,
		/* key_len = 32 */
		uint32_t limit_increase);

	static void
	scs_hashset_clear(
		uint32_t key_offset,
		/* key_len = 32 */
		uint64_t threshold);

	static uint32_t
	scs_hashset_get_size(
		uint32_t key_offset
		/* key_len = 32 */);

	static uint32_t
	scs_hashset_get_max_size(
		uint32_t key_offset
		/* key_len = 32 */);

	/**
	 *  if two things with same threshold, 
	 * returns lowest index.
	 * If none, returns UINT32_MAX
	 **/
	static uint32_t
	scs_hashset_get_index_of(
		uint32_t key_offset,
		/* key_len = 32 */
		uint64_t threshold);

	// returns threshold
	static 
	uint64_t
	scs_hashset_get_index(
		uint32_t key_offset,
		/* key_len = 32 */
		uint32_t output_offset,
		/* output_len = 32 */
		uint32_t index);

	/* -- crypto -- */
	static void
	scs_hash(
		uint32_t input_offset,
		uint32_t input_len,
		uint32_t output_offset
		/* output_len = 32 */);

	static uint32_t
	scs_check_sig_ed25519(
		uint32_t pk_offset,
		/* pk len = 32 */
		uint32_t sig_offset,
		/* sig_len = 64 */
		uint32_t msg_offset,
		uint32_t msg_len);

	/* -- contracts -- */
	static void
	scs_create_contract(
		uint32_t contract_index, 
		uint32_t hash_out
        /* out_len = 32 */);

	static void
	scs_deploy_contract(
		uint32_t hash_offset, 
		/* hash_len = 32 */
		uint64_t nonce,
        uint32_t out_address_offset 
        /* addr_len = 32 */);

    /* -- witnesses -- */
    static void
    scs_get_witness(uint64_t wit_idx, uint32_t out_offset, uint32_t max_len);

    static uint32_t
    scs_get_witness_len(uint64_t wit_idx);

    /* -- rpc -- */
    // returns rpc result length
    static void
    scs_external_call(
    	uint32_t target_addr,
    	/* addr_len = 32 */
    	uint32_t call_data_offset,
    	uint32_t call_data_len,
    	uint32_t response_offset,
    	uint32_t response_max_len);

	BuiltinFns() = delete;

public:

	static void
	link_fns(wasm_api::WasmRuntime& runtime);
};

} /* scs */
