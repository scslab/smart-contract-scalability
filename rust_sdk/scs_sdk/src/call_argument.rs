use crate::types::Address;
use crate::builtin_fns;
use crate::rt::trap;

use core::marker::PhantomData;

pub struct BackedType<T, const SZ : usize>
{
	backing : [u8; SZ],
	x : PhantomData<T>,
}

impl <T, const SZ : usize> BackedType<T, SZ>
{
	pub fn new() -> Self
	{
		// i despise rust's generic const implementation
		// why does this not work
		// the entire generics/templating system is useless if you
		// can't do something this simple
		// const_assert!(sz ==  {core::mem::size_of::<T>()});
		Self { backing : [0 ; SZ], x : PhantomData}
	}

	pub fn new_from_calldata(calldata_len : i32) -> Self
	{
		let out = Self::new();

		if (calldata_len as usize) > out.get_backing_len()
		{
/*			unsafe
			{
				builtin_fns::print_debug(calldata_len);
				builtin_fns::print_debug(out.get_backing_len() as i32);
			}*/
			trap();
		//	panic!("too much calldata");
		}

		unsafe {
			builtin_fns::get_calldata(out.get_backing_ptr() as i32, calldata_len);
		}
		out
	}

	pub fn get(&self) -> &T
	{
		let out : *const T = unsafe { core::mem::transmute(&self.backing) };
		unsafe {
			&*out
		}
	}

	pub fn get_mut(&mut self) -> &mut T
	{
		let out : *mut T = unsafe { core::mem::transmute(&self.backing) };
		unsafe {
			&mut *out
		}
	}

	pub fn get_backing_ptr(&self) -> *const u8
	{
		let out : *const u8 = unsafe { core::mem::transmute(&self.backing) };
		out
	}
	pub fn get_backing_len(&self) -> usize
	{
		SZ
	}
}

pub struct ContractProxy<'a>
{
	addr : &'a Address
}

impl <'a> ContractProxy<'a>
{
	pub fn new(addr : &'a Address) -> Self
	{
		Self{addr : addr}
	}

	// should be invoked using a macro to translate string to name
	// e.g. call<T, R>(method_id!("methodname"))
	pub fn call<T, const TSIZE : usize, R, const RSIZE : usize>(&self, methodname : i32, arg : &BackedType<T, TSIZE>) -> BackedType<R, RSIZE>
	{
		let out = BackedType::<R, RSIZE>::new();
		if RSIZE != core::mem::size_of::<R>() || TSIZE != core::mem::size_of::<T>()
		{
			trap();
			//panic!("sizeof mismatch");
		}
		unsafe
		{
			builtin_fns::invoke(
				self.addr.get_ptr() as i32, 
				methodname, 
				arg.get_backing_ptr() as i32, 
				arg.get_backing_len() as i32,
				out.get_backing_ptr() as i32,
				out.get_backing_len() as i32);
		}
		out
	}

	pub fn call_noret<T, const TSIZE : usize>(&self, methodname : i32, arg : &BackedType<T, TSIZE>)
	{
		unsafe
		{
			builtin_fns::invoke(
				self.addr.get_ptr() as i32, 
				methodname, 
				arg.get_backing_ptr() as i32, 
				arg.get_backing_len() as i32,
				0,
				0);
		}
	}

	pub fn call_noargs<R, const RSIZE: usize>(&self, methodname : i32) -> BackedType<R, RSIZE>
	{
		let out = BackedType::<R, RSIZE>::new();
		if RSIZE != core::mem::size_of::<R>()
		{
			trap();
		//	panic!("sizeof mismatch");
		}
		unsafe
		{
			builtin_fns::invoke(
				self.addr.get_ptr() as i32, 
				methodname, 
				0,
				0,
				out.get_backing_ptr() as i32,
				out.get_backing_len() as i32);
		}
		out
	}

	pub fn call_noargs_noret(&self, methodname : i32)
	{
		unsafe
		{
			builtin_fns::invoke(
				self.addr.get_ptr() as i32, 
				methodname, 
				0,
				0,
				0,
				0);
		}
	}
}




/*

ABI Design:

Motivation: we pay a lot for computation, not a ton for raw memcpy

Everything will have a fixed size.  That way, we can index freely into any struct element
using a compile-time constant.  

Also, vec types make life a lot harder (alternate option: something like Solidity ABI.  Downside
is that that's more complicated.  Could be done in a later version.  Downside is also that
accessing elts deep inside nested var-length structs requires several indexes).

*/



/*
#[proc_macro_attribute]
pub fn abi_encode_struct(attr: TokenStream, item: TokenStream) -> TokenStream {

	let original_struct : ItemStruct = parse_macro_input!(item as ItemStruct);

	match &original_struct.vis
	{
		Public(_) => (),
		_ => {
			return quote!{compile_error!("abi encoded structs should be public")}
		},
	};

	if &original_struct.attrs.len() != 1
	{
		return quote!{compile_error!("struct cannot have additional attributes")};
	}
} */

/*
pub struct AbiBacking<const max_len : u32>
{
	data : [u8; max_len];
	allocated: u32;
}

impl <const max_len : u32> AbiBacking<max_len>
{
	pub fn alloc(&mut self, mem_required : u32) -> u32
	{
		let alloc = self.allocated;
		self.allocated += mem_required;
		if self.allocated > max_len
		{
			panic!("over allocated AbiBacking");
		}
		alloc
	}
}

trait AbiCompatible
{
	fn head_size() -> u32;
	fn needs_tail() -> bool;
	fn alloc_tail(&self, mem_required : u32);
}

impl AbiCompatible for u8
{
	fn head_size() -> u32
	{
		1
	}
	fn needs_tail() -> bool
	{
		false
	}
	fn alloc_tail(&self, _mem_required : u32) -> u32
	{
		panic!("u8 has no tail")
	}
}

impl AbiCompatible for u32
{
	fn head_size() -> u32
	{
		4
	}
	fn needs_tail() -> bool
	{
		false
	}
	fn alloc_tail(&self, _mem_required : u32) -> u32
	{
		panic!("u32 has no tail")
	}
}

pub struct SeriallyBackedVec<T, const max_len : u32>
{
	backing : *mut AbiBacking<max_len>;
	head_offset : u32;

	// head: {tail_offset : u32, cur_size : u32}
}

impl AbiCompatible for SeriallyBackedVec<T, const max_len : u32>
{
	fn head_size() -> u32
	{
		8
	}
	fn needs_tail() -> bool
	{
		true
	}
	fn alloc_tail(&self, mem_required : u32) -> u32
	{
		unsafe
		{
			(*self.backing).alloc(mem_required)
		}
	}
}

impl SeriallyBackedVec<T, const max_len : u32>
{
	pub fn len(&self) -> u32
	{
		unsafe
		{
			(*backing).get<u32>(self.head_offset + 4)
		}
	}

	fn get_tail_offset(&self) -> u32
	{
		unsafe
		{
			(*backing).get<u32>(self.head_offset)
		}
	}

	fn bounds_check(&self, index: u32)
	{
		if index >= self.len()
		{
			panic!("OOB vector access");
		}
	}

	pub fn new(backing : *mut AbiBacking<max_len>, max_len: u32)
	{
		let out = Self{backing : backing, head_offset : 0};
		out.head_offset = out.alloc_tail(max_len * Self::head_size());
		out
	}

	pub fn get_mut(&mut self, index : u32) -> mut T
	{
		self.bounds_check(index);
		let out : &mut T = unsafe {

		}
	}
} */
