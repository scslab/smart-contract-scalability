#![no_std]

use scs_sdk as sdk;
use scs_callable_attr::{scs_public_function, method_id, scs_interface_method};

use erc20;

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

	let arg = sdk::call_argument::BackedType::<i32, {core::mem::size_of::<i32>()}>::new();

	let res : sdk::call_argument::BackedType::<(), 0> = proxy.call(method_id!("foo"), &arg);
} 

#[repr(C)]
struct FancyCall
{
	addr : sdk::types::Address,
	val : i32,
	long_val : u64,
}

#[scs_public_function]
pub fn try_fancy_call(calldata_len : i32)
{
	let arg = sdk::call_argument::BackedType::<FancyCall, {core::mem::size_of::<FancyCall>()}>::new_from_calldata(calldata_len);

	sdk::log_generic_reprc(&arg.get().addr);
	sdk::log_generic_reprc(&arg.get().val);
	sdk::log_generic_reprc(&arg.get().long_val);
}

#[scs_public_function]
pub fn try_call_erc20(calldata_len : i32)
{
	let arg = sdk::call_argument::BackedType::<FancyCall, {core::mem::size_of::<FancyCall>()}>::new_from_calldata(calldata_len);
	let proxy = sdk::call_argument::ContractProxy::new(&arg.get().addr);

	let erc20_proxy = erc20::InterfaceERC20::new(proxy);

	let erc20_arg = sdk::call_argument::BackedType::<(), 0>::new();

	erc20_proxy.name(&erc20_arg);
}


