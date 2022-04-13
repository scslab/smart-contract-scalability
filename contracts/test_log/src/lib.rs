#![no_std]

use scs_sdk as sdk;
use scs_callable_attr::{scs_public_function, method_id};

use erc20;


#[repr(C)]
pub struct FancyCall
{
	addr : sdk::types::Address,
	val : i32,
	long_val : u64,
}

#[scs_public_function(test_log)]
pub fn test_log()
{
	let i : i32 = core::mem::size_of::<FancyCall>() as i32;
	sdk::log_i32(&i);
}

#[scs_public_function(test_log)]
pub fn try_macros(calldata_len : &i32)
{
	let addr = sdk::types::Address::new();

	let proxy = sdk::call_argument::ContractProxy::new(&addr);

	let arg = sdk::call_argument::BackedType::<i32, {core::mem::size_of::<i32>()}>::new();

	let res : sdk::call_argument::BackedType::<(), 0> = proxy.call(method_id!("foo"), &arg);
} 

#[scs_public_function(test_log)]
pub fn try_fancy_call(arg : &FancyCall)
{
	//let arg = sdk::call_argument::BackedType::<FancyCall, {core::mem::size_of::<FancyCall>()}>::new_from_calldata(calldata_len);

	sdk::log_generic_reprc(&arg.addr);
	sdk::log_generic_reprc(&arg.val);
	sdk::log_generic_reprc(&arg.long_val);
}

#[scs_public_function(test_log)]
pub fn try_call_erc20(arg : &FancyCall)
{
	//let arg = sdk::call_argument::BackedType::<FancyCall, {core::mem::size_of::<FancyCall>()}>::new_from_calldata(calldata_len);
	let proxy = sdk::call_argument::ContractProxy::new(&arg.addr);

	let erc20_proxy = erc20::InterfaceERC20::new(proxy);

	erc20_proxy.name();
}


