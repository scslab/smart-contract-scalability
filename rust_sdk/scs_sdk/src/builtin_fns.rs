
#[link(wasm_import_module = "scs")]
extern "C"
{
	// rust has a compiler builtin called "log", so we can't have that smdh
	pub(crate) fn host_log(offset: i32, len: i32);
	pub(crate) fn invoke(addr_offset : i32, methodname : i32, calldata_offset : i32, calldata_len : i32, return_offset : i32, return_len : i32);
	pub(crate) fn get_calldata(offset: i32, len: i32);
}
