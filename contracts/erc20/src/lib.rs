#![no_std]
#![feature(generic_const_exprs)]
#![allow(incomplete_features)]

use scs_sdk as sdk;
use scs_callable_attr::{scs_public_function, scs_interface_method, create_interface};

create_interface!{InterfaceERC20}

#[scs_interface_method(InterfaceERC20)]
#[scs_public_function]
pub fn noret_args(calldata_len : &i32)
{
	// do nothing
}

#[scs_interface_method(InterfaceERC20)]
#[scs_public_function]
pub fn ret_args(calldata_len : &i32) -> i64
{
	0
	// do nothing
}


#[scs_interface_method(InterfaceERC20)]
#[scs_public_function]
pub fn ret_noargs() -> i64
{
	0
	// do nothing
}

#[scs_interface_method(InterfaceERC20)]
#[scs_public_function]
pub fn noret_noargs()
{
	// do nothing
}


