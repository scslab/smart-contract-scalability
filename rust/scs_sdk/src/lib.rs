#![no_std]
#![allow(dead_code)]

mod builtin_fns;
mod rt;

pub fn log_i32(val: &i32)
{
	let addr : *const i32 = val as *const i32;
	unsafe {
		builtin_fns::host_log(addr as i32, 4);
	}
}
