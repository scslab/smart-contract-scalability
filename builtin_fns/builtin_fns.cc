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

#include "builtin_fns/builtin_fns.h"

#include "wasm_api/wasm_api.h"

namespace scs
{

BUILTIN_INSTANTIATE;

template<typename TransactionContext_t>
void
BuiltinFns<TransactionContext_t>::link_fns(wasm_api::WasmRuntime& runtime)
{
	/* -- env -- */

	runtime.template link_env<&BuiltinFns<TransactionContext_t>::memcmp>(
		"memcmp");
	runtime.template link_env<&BuiltinFns<TransactionContext_t>::memset>(
		"memset");
	runtime.template link_env<&BuiltinFns<TransactionContext_t>::memcpy>(
		"memcpy");
	runtime.template link_env<&BuiltinFns<TransactionContext_t>::strnlen>(
		"strnlen");

	/* -- logging functions -- */

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_log>(
		"scs",
		"log");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_print_debug>(
		"scs",
		"print_debug");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_print_c_str>(
		"scs",
		"print_c_str");

	/* -- runtime functions -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_invoke>(
		"scs", 
		"invoke");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_return>(
		"scs",
		"return");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_calldata>(
		"scs",
		"get_calldata");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_calldata_len>(
		"scs",
		"get_calldata_len");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_msg_sender>(
		"scs",
		"get_msg_sender");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_self_addr>(
		"scs",
		"get_self");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_block_number>(
		"scs",
		"get_block_number");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_src_tx_hash>(
		"scs",
		"get_tx_hash");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_invoked_tx_hash>(
		"scs",
		"get_invoked_tx_hash");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_gas>(
		"scs",
		"gas");

	/* -- general storage -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_has_key>(
		"scs",
		"has_key");

	/* -- raw memory storage -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_raw_memory_set>(
		"scs",
		"raw_mem_set");
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_raw_memory_get>(
		"scs",
		"raw_mem_get");
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_raw_memory_get_len>(
		"scs",
		"raw_mem_len");

	/* -- deletions -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_delete_key_last>(
		"scs",
		"delete_key_last");

	/* -- nonnegative int64 -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_nonnegative_int64_set_add>(
		"scs",
		"nn_int64_set_add");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_nonnegative_int64_add>(
		"scs",
		"nn_int64_add");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_nonnegative_int64_get>(
		"scs",
		"nn_int64_get");

	/* -- hashset -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hashset_insert>(
		"scs",
		"hashset_insert");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hashset_increase_limit>(
		"scs",
		"hashset_increase_limit");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hashset_clear>(
		"scs",
		"hashset_clear");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hashset_get_size>(
		"scs",
		"hashset_get_size");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hashset_get_max_size>(
		"scs",
		"hashset_get_max_size");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hashset_get_index_of>(
		"scs",
		"hashset_get_index_of");
	
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hashset_get_index>(
		"scs",
		"hashset_get_index");

	/* -- asset -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_asset_add>(
		"scs",
		"asset_add");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_asset_get>(
		"scs",
		"asset_get");

	/* -- crypto -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_hash>(
		"scs",
		"hash");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_check_sig_ed25519>(
		"scs",
		"check_sig_ed25519");

	/* -- contracts -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_create_contract>(
		"scs",
		"create_contract");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_deploy_contract>(
		"scs",
		"deploy_contract");

	/* -- witnesses -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_witness>(
		"scs",
		"get_witness");

	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_get_witness_len>(
		"scs",
		"get_witness_len");

	/* -- rpc -- */
	runtime.template link_fn<&BuiltinFns<TransactionContext_t>::scs_external_call>(
		"scs",
		"external_call");
}

} /* scs */
