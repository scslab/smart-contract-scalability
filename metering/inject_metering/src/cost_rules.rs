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

use parity_wasm::elements;
/**
 *
 * Loosely based on https://github.com/paritytech/substrate/blob/master/frame/contracts/src/schedule.rs
 *
 * Not using their actual parameter values.
 *
 **/
use wasm_instrument::gas_metering;

pub struct DefaultRules {
    pub i64const: u32,
    pub i64load: u32,
    pub i64store: u32,
    pub select: u32,
    pub r#if: u32,
    pub br: u32,
    pub br_if: u32,
    pub br_table: u32,
    pub br_table_per_entry: u32,
    pub call: u32,
    pub call_indirect: u32,
    pub call_indirect_per_param: u32,
    pub local_get: u32,
    pub local_set: u32,
    pub local_tee: u32,
    pub global_get: u32,
    pub global_set: u32,
    pub memory_current: u32,
    pub memory_grow: u32,
    pub i64clz: u32,
    pub i64ctz: u32,
    pub i64popcnt: u32,
    pub i64eqz: u32,
    pub i64extendsi32: u32,
    pub i64extendui32: u32,
    pub i32wrapi64: u32,
    pub i64eq: u32,
    pub i64ne: u32,
    pub i64lts: u32,
    pub i64ltu: u32,
    pub i64gts: u32,
    pub i64gtu: u32,
    pub i64les: u32,
    pub i64leu: u32,
    pub i64ges: u32,
    pub i64geu: u32,
    pub i64add: u32,
    pub i64sub: u32,
    pub i64mul: u32,
    pub i64divs: u32,
    pub i64divu: u32,
    pub i64rems: u32,
    pub i64remu: u32,
    pub i64and: u32,
    pub i64or: u32,
    pub i64xor: u32,
    pub i64shl: u32,
    pub i64shrs: u32,
    pub i64shru: u32,
    pub i64rotl: u32,
    pub i64rotr: u32,
}

impl DefaultRules {
    pub fn default() -> Self {
        Self {
            // these are not at all well benchmarked
            i64const: 1,
            i64load: 1,
            i64store: 1,
            select: 1,
            r#if: 1,
            br: 1,
            br_if: 2,
            br_table: 2,
            br_table_per_entry: 1,
            call: 5,
            call_indirect: 5,
            call_indirect_per_param: 1,
            local_get: 1,
            local_set: 1,
            local_tee: 1,
            global_get: 1,
            global_set: 1,
            memory_current: 1,
            memory_grow: 10,
            i64clz: 1,
            i64ctz: 1,
            i64popcnt: 3,
            i64eqz: 2,
            i64extendsi32: 1,
            i64extendui32: 1,
            i32wrapi64: 1,
            i64eq: 2,
            i64ne: 2,
            i64lts: 2,
            i64ltu: 2,
            i64gts: 2,
            i64gtu: 2,
            i64les: 2,
            i64leu: 2,
            i64ges: 2,
            i64geu: 2,
            i64add: 2,
            i64sub: 2,
            i64mul: 3,
            i64divs: 3,
            i64divu: 3,
            i64rems: 3,
            i64remu: 3,
            i64and: 2,
            i64or: 2,
            i64xor: 2,
            i64shl: 2,
            i64shrs: 2,
            i64shru: 2,
            i64rotl: 2,
            i64rotr: 2,
        }
    }
}

impl gas_metering::Rules for DefaultRules {
    fn instruction_cost(&self, instruction: &elements::Instruction) -> Option<u32> {
        use self::elements::Instruction::*;

        let w = self;

        let weight = match *instruction {
            End | Unreachable | Return | Else => 0,
            I32Const(_) | I64Const(_) | Block(_) | Loop(_) | Nop | Drop => w.i64const,
            I32Load(_, _)
            | I32Load8S(_, _)
            | I32Load8U(_, _)
            | I32Load16S(_, _)
            | I32Load16U(_, _)
            | I64Load(_, _)
            | I64Load8S(_, _)
            | I64Load8U(_, _)
            | I64Load16S(_, _)
            | I64Load16U(_, _)
            | I64Load32S(_, _)
            | I64Load32U(_, _) => w.i64load,
            I32Store(_, _)
            | I32Store8(_, _)
            | I32Store16(_, _)
            | I64Store(_, _)
            | I64Store8(_, _)
            | I64Store16(_, _)
            | I64Store32(_, _) => w.i64store,
            Select => w.select,
            If(_) => w.r#if,
            Br(_) => w.br,
            BrIf(_) => w.br_if,
            Call(_) => w.call,
            GetLocal(_) => w.local_get,
            SetLocal(_) => w.local_set,
            TeeLocal(_) => w.local_tee,
            GetGlobal(_) => w.global_get,
            SetGlobal(_) => w.global_set,
            CurrentMemory(_) => w.memory_current,
            GrowMemory(_) => w.memory_grow,
            CallIndirect(_idx, _) => 100, // nothing in our sdk requires call_indirect, so just make it kind of expensive
            BrTable(ref data) => w
                .br_table
                .saturating_add(w.br_table_per_entry.saturating_mul(data.table.len() as u32)),
            I32Clz | I64Clz => w.i64clz,
            I32Ctz | I64Ctz => w.i64ctz,
            I32Popcnt | I64Popcnt => w.i64popcnt,
            I32Eqz | I64Eqz => w.i64eqz,
            I64ExtendSI32 => w.i64extendsi32,
            I64ExtendUI32 => w.i64extendui32,
            I32WrapI64 => w.i32wrapi64,
            I32Eq | I64Eq => w.i64eq,
            I32Ne | I64Ne => w.i64ne,
            I32LtS | I64LtS => w.i64lts,
            I32LtU | I64LtU => w.i64ltu,
            I32GtS | I64GtS => w.i64gts,
            I32GtU | I64GtU => w.i64gtu,
            I32LeS | I64LeS => w.i64les,
            I32LeU | I64LeU => w.i64leu,
            I32GeS | I64GeS => w.i64ges,
            I32GeU | I64GeU => w.i64geu,
            I32Add | I64Add => w.i64add,
            I32Sub | I64Sub => w.i64sub,
            I32Mul | I64Mul => w.i64mul,
            I32DivS | I64DivS => w.i64divs,
            I32DivU | I64DivU => w.i64divu,
            I32RemS | I64RemS => w.i64rems,
            I32RemU | I64RemU => w.i64remu,
            I32And | I64And => w.i64and,
            I32Or | I64Or => w.i64or,
            I32Xor | I64Xor => w.i64xor,
            I32Shl | I64Shl => w.i64shl,
            I32ShrS | I64ShrS => w.i64shrs,
            I32ShrU | I64ShrU => w.i64shru,
            I32Rotl | I64Rotl => w.i64rotl,
            I32Rotr | I64Rotr => w.i64rotr,

            // Returning None makes the gas instrumentation fail which we intend for
            // unsupported or unknown instructions.
            _ => return None,
        };
        Some(weight)
    }

    fn memory_grow_cost(&self) -> gas_metering::MemoryGrowCost {
        // TODO: benchmark the memory.grow instruction with the maximum allowed pages.
        // This would make the cost for growing already included in the instruction cost.
        gas_metering::MemoryGrowCost::Free
    }
}
