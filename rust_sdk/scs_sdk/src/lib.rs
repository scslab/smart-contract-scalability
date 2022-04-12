#![no_std]
#![allow(dead_code)]
#![allow(incomplete_features)]
#![feature(generic_const_exprs)]

pub mod builtin_fns;
pub mod rt;
pub mod call_argument;
pub mod types;

pub fn log_i32(val: &i32)
{
	let addr : *const i32 = val as *const i32;
	unsafe {
		builtin_fns::host_log(addr as i32, 4);
	}
}

pub fn log_generic_reprc<T>(val : *const T)
{
	let addr : *const u8 = unsafe { core::mem::transmute(val) };
	unsafe
	{
		builtin_fns::host_log(addr as i32, core::mem::size_of::<T>() as i32);
	}
}