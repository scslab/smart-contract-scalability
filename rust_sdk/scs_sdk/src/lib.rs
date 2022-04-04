#![no_std]
#![allow(dead_code)]
#![allow(incomplete_features)]
#![feature(generic_const_exprs)]

mod builtin_fns;
mod rt;
pub mod call_argument;
pub mod types;

pub fn log_i32(val: &i32)
{
	let addr : *const i32 = val as *const i32;
	unsafe {
		builtin_fns::host_log(addr as i32, 4);
	}
}