#include "builtin_fns/builtin_fns.h"

#include "wasm_api/wasm_api.h"

namespace scs
{

void
BuiltinFns::link_fns(wasm_api::WasmRuntime& runtime)
{
	/* -- env -- */

	runtime.template link_env<&BuiltinFns::memcmp>(
		"memcmp");
	runtime.template link_env<&BuiltinFns::memset>(
		"memset");
	runtime.template link_env<&BuiltinFns::memcpy>(
		"memcpy");

	/* -- logging functions -- */

	runtime.template link_fn<&BuiltinFns::scs_log>(
		"scs",
		"log");

	runtime.template link_fn<&BuiltinFns::scs_print_debug>(
		"scs",
		"print_debug");

	/* -- runtime functions -- */
	runtime.template link_fn<&BuiltinFns::scs_invoke>(
		"scs", 
		"invoke");

	runtime.template link_fn<&BuiltinFns::scs_return>(
		"scs",
		"return");

	runtime.template link_fn<&BuiltinFns::scs_get_calldata>(
		"scs",
		"get_calldata");

	runtime.template link_fn<&BuiltinFns::scs_get_msg_sender>(
		"scs",
		"get_msg_sender");
}


} /* scs */
