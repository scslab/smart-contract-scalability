#include "builtin_fns/builtin_fns.h"

#include "wasm_api/wasm_api.h"

namespace scs
{

void
BuiltinFns::link_fns(WasmRuntime& runtime)
{
	/* -- logging functions -- */

	runtime.link_fn(
		"scs",
		"host_log",
		&BuiltinFns::scs_log);

	runtime.link_fn(
		"scs",
		"print_debug",
		&BuiltinFns::scs_print_debug);

	/* -- runtime functions -- */
	runtime.link_fn(
		"scs", 
		"invoke", 
		&BuiltinFns::scs_invoke);

	runtime.link_fn(
		"scs",
		"return",
		&BuiltinFns::scs_return);

	runtime.link_fn(
		"scs",
		"get_calldata",
		&BuiltinFns::scs_get_calldata);

	runtime.link_fn(
		"scs",
		"get_msg_sender",
		&BuiltinFns::scs_get_msg_sender);
}


} /* scs */
