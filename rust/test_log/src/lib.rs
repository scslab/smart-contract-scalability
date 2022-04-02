#![no_std]
use scs_sdk as sdk;
use scs_callable_attr::scs_public_function;

#[scs_public_function]
pub fn test_log()
{
	let i : i32 = 5;
	sdk::log_i32(&i);
}
