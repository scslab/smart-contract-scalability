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

use wasm_instrument;

use parity_wasm::elements::Module;

mod cost_rules;

use crate::cost_rules::DefaultRules;

#[repr(C)]
pub struct metered_contract {
    data: *mut u8,
    len: u32,
    capacity: u32,
}

impl metered_contract {
    pub fn null() -> Self {
        Self {
            data: std::ptr::null_mut(),
            len: 0,
            capacity: 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn add_metering_ext(data_ptr: *const u8, len: u32) -> metered_contract {
    if data_ptr == std::ptr::null_mut() {
        return metered_contract::null();
    }

    let mut vec = match match std::panic::catch_unwind(|| {
        let data = unsafe { std::slice::from_raw_parts(data_ptr, len as usize) };
        add_metering(&data)
    }) {
        Ok(v) => v,
        Err(_) => {
            return metered_contract::null();
        }
    } {
        // https://doc.rust-lang.org/std/mem/fn.forget.html
        Some(v) => {
            if v.capacity() <= u32::MAX as usize {
                std::mem::ManuallyDrop::new(v)
            } else {
                return metered_contract::null();
            }
        },
        None => {
            return metered_contract::null();
        }
    };

    let ptr = vec.as_mut_ptr();
    let len = vec.len();
    let capacity = vec.capacity();

    metered_contract {
        data: ptr,
        len: len as u32,
        capacity: capacity as u32,
    }
}

#[no_mangle]
pub extern "C" fn free_metered_contract(contract: metered_contract) {
    if contract.data == std::ptr::null_mut() {
        return;
    }

    unsafe {
        drop(Vec::from_raw_parts(
            contract.data,
            contract.len as usize,
            contract.capacity as usize,
        ));
    }
}

pub fn add_metering(buf: &[u8]) -> Option<Vec<u8>> {
    let module = match Module::from_bytes(buf) {
        Ok(module) => module,
        Err(_) => return None,
    };

    let rules = DefaultRules::default();

    let out_mod = match wasm_instrument::gas_metering::inject(module, &rules, "scs") {
        Ok(metered_module) => metered_module,
        Err(_) => return None,
    };

    return out_mod.into_bytes().ok();
}
