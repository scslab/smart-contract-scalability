use crate::rt::trap;

#[repr(C)]
pub struct Address
{
	data : [u8 ; 32]
}

impl Address
{
	pub fn new () -> Self
	{
		Self { data : [0 ; 32] }
	}

	pub fn get_ptr(&self) -> *const u8
	{
		let out : *const u8 = unsafe { core::mem::transmute(&self.data) };
		out
	}
}

#[repr(C)]
pub struct FixedLenString<const SZ : usize>
{
	data : [u8; SZ]
}

impl <const SZ : usize>FixedLenString<SZ>
{
	pub fn new(from: &[u8]) -> Self
	{
		if from.len() > SZ
		{
			trap();
		}

		let mut out = Self{data : [0; SZ]};
		unsafe {
			let from_ptr = from.as_ptr();
			let data_ptr : *mut u8 = core::mem::transmute(&out.data);

			data_ptr.copy_from(data_ptr, from.len());
		}
		out
	}
}

