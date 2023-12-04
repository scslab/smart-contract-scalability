/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

use curve25519_dalek::constants::RISTRETTO_BASEPOINT_COMPRESSED;
use curve25519_dalek::constants::RISTRETTO_BASEPOINT_POINT;
use curve25519_dalek::ristretto::RistrettoPoint;
use curve25519_dalek::scalar::Scalar;
use curve25519_dalek::traits::MultiscalarMul;

use sha3::Sha3_512;

use std::alloc::alloc;
use std::alloc::dealloc;
use std::alloc::Layout;

// u128 doesn't work across rust FFI boundaries
/*

Quoting this github thread: https://github.com/rust-lang/rust/issues/54341
```
Copying from the last thread, since this thread seems to mostly be about long double.

In C and C++, on x86_64 Linux, alignof(__int128) is equal to 16. However, in Rust, align_of::() is equal to 8.

C++: https://gcc.godbolt.org/z/YAq1XC
Rust: https://gcc.godbolt.org/z/QvZeqK

This will cause subtle UB if you ever try to use i128s across FFI boundaries; for example:

C++: https://gcc.godbolt.org/z/PrtHlp
Rust: https://gcc.godbolt.org/z/_SdVqD
```
*/
#[repr(C)]
pub struct PedersenQuery {
    query_low: u64,
    query_high: u64,
    blinding: [u8; 32],
}

pub struct PedersenParams {
    blinding_factor: RistrettoPoint
}

#[repr(C)]
pub struct PedersenResponse {
    serialized: [u8; 32]
}


#[no_mangle]
pub extern "C" fn pedersen_commitment(query : *const PedersenQuery, params : *const PedersenParams) -> PedersenResponse
{
    let blind : Scalar = unsafe { Scalar::from_bytes_mod_order((*query).blinding) };
    let mut q : u128 = unsafe { (*query).query_high.into() };
    q = q << 64;
    q = q + unsafe { (*query).query_low as u128 };

    let value : Scalar = q.into();

    let blinding_base = unsafe { (*params).blinding_factor };

    let out = RistrettoPoint::multiscalar_mul(&[value, blind], &[RISTRETTO_BASEPOINT_POINT, blinding_base]);

    PedersenResponse {
        serialized : *out.compress().as_bytes()
    }
}

#[no_mangle]
pub extern "C" fn gen_pedersen_params() -> *const PedersenParams
{
    unsafe {
        let layout = Layout::new::<RistrettoPoint>();
        let out : *mut PedersenParams =  alloc(layout) as *mut PedersenParams;
        (*out).blinding_factor = RistrettoPoint::hash_from_bytes::<Sha3_512>(RISTRETTO_BASEPOINT_COMPRESSED.as_bytes());

        out
    }
}

#[no_mangle]
pub extern "C" fn free_pedersen_params(ptr : *mut PedersenParams)
{
    unsafe {
        let layout = Layout::new::<RistrettoPoint>();
        dealloc(ptr as *mut u8, layout)
    }
}

