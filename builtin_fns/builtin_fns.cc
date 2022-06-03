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
		"host_log");

/*	runtime. link_fn(
		"scs",
		"print_debug",
		&BuiltinFns::scs_print_debug); */

	/* -- runtime functions -- */
	runtime.link_fn<&BuiltinFns::scs_invoke>(
		"scs", 
		"invoke");

	runtime.link_fn<&BuiltinFns::scs_return>(
		"scs",
		"return");

	runtime.link_fn<&BuiltinFns::scs_get_calldata>(
		"scs",
		"get_calldata");

	runtime.link_fn<&BuiltinFns::scs_get_msg_sender>(
		"scs",
		"get_msg_sender");
}


} /* scs */
