
#[link(wasm_import_module = "scs")]
extern "C"
{
	// rust has a compiler builtin called "log", so we can't have that smdh
	pub(crate) fn host_log(offset: i32, len: i32);
}
