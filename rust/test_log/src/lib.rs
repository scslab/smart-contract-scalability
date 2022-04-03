#![no_std]
use scs_sdk as sdk;
use scs_callable_attr::{scs_public_function, method_id};

#[scs_public_function]
pub fn test_log(calldata_len : i32)
{
	let i : i32 = 5;
	sdk::log_i32(&i);
}

#[scs_public_function]
pub fn try_macros(calldata_len : i32)
{

	let addr = sdk::types::Address::new();

	let proxy = sdk::call_argument::ContractProxy::new(&addr);

	let arg = sdk::call_argument::BackedType::<i32, 4>::new();

	let res : sdk::call_argument::BackedType::<(), 0> = proxy.call(method_id!("foo"), &arg);
}
