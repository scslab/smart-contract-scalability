
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

