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

#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"
#include "transaction_context/method_invocation.h"
#include "transaction_context/transaction_results.h"
#include "transaction_context/transaction_context.h"

#include "threadlocal/threadlocal_context.h"

#include "crypto/hash.h"
#include "crypto/crypto_utils.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "storage_proxy/transaction_rewind.h"

#include "transaction_context/global_context.h"

#include "utils/defer.h"

#include <utils/time.h>

#include "common/syscall_nos.h"
#include "builtin_fns/gas_costs.h"
#include "contract_db/contract_utils.h"

#include <type_traits>
#include <cinttypes>

#define EC_DECL(ret) template<typename TransactionContext_t> ret ExecutionContext<TransactionContext_t>

namespace scs {

template<typename T>
concept TriviallyCopyable
= requires {
    typename std::enable_if<std::is_trivially_copyable<T>::value>::type;
};


wasm_api::HostFnStatus<void> gas_handler(wasm_api::HostCallContext* context, uint64_t gas)
{
    if (context -> runtime -> consume_gas(gas)) {
        return {};
    }
    return wasm_api::HostFnStatus<void>{std::unexpect_t{}, wasm_api::HostFnError::OUT_OF_GAS};
}

template class ExecutionContext<GroundhogTxContext>;
template class ExecutionContext<SisyphusTxContext>;
template class ExecutionContext<TxContext>;

EC_DECL()::ExecutionContext()
    : wasm_context(MAX_STACK_BYTES)
    , active_runtimes()
    , tx_context(nullptr)
    , results_of_last_tx(nullptr)
    , addr_db(nullptr)
{
    wasm_context.link_fn("scs", "syscall", &ExecutionContext<TransactionContext_t>::static_syscall_handler);
    wasm_context.link_fn("scs", "gas", &gas_handler);
}


EC_DECL(wasm_api::MeteredReturn)::invoke_subroutine(MethodInvocation const& invocation, uint64_t gas_limit)
{
    auto iter = active_runtimes.find(invocation.addr);
    if (iter == active_runtimes.end()) {
        CONTRACT_INFO("creating new runtime for contract at %s",
                      debug::array_to_str(invocation.addr).c_str());

        //auto timestamp = utils::init_time_measurement();

        if (!tx_context) {
            throw std::runtime_error("no tx context??");
        }

        auto script = tx_context->get_contract_db_proxy().get_script(invocation.addr);
        wasm_api::Script s {.data = script.data, .len = script.len};

        auto runtime_instance = wasm_context.new_runtime_instance(
            s,
            reinterpret_cast<void*>(this));

        if (!runtime_instance) {
            return wasm_api::MeteredReturn{
                .result = wasm_api::InvokeStatus<uint64_t>(std::unexpect_t{}, wasm_api::InvokeError::OUT_OF_GAS_ERROR),
                .gas_consumed = 0
            };
        }

        //std::printf("launch time: %lf\n", utils::measure_time(timestamp));

        active_runtimes.emplace(invocation.addr, std::move(runtime_instance));

        //std::printf("link time: %lf\n", utils::measure_time(timestamp));
    }

    auto* runtime = active_runtimes.at(invocation.addr).get();

    tx_context->push_invocation_stack(runtime, invocation);

    auto res = runtime->invoke(
        invocation.get_invocable_methodname().c_str(), gas_limit);

    tx_context->pop_invocation_stack();

    return res;
}

template
TransactionStatus
ExecutionContext<TxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    GlobalContext&,
    BlockContext&,
    std::optional<NondeterministicResults>);

template
TransactionStatus
ExecutionContext<GroundhogTxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    GroundhogGlobalContext&,
    GroundhogBlockContext&,
    std::optional<NondeterministicResults>);

template
TransactionStatus
ExecutionContext<SisyphusTxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    SisyphusGlobalContext&,
    SisyphusBlockContext&,
    std::optional<NondeterministicResults>);


template<typename TransactionContext_t>
template<typename BlockContext_t, typename GlobalContext_t>
TransactionStatus
ExecutionContext<TransactionContext_t>::execute(Hash const& tx_hash,
                          SignedTransaction const& tx,
                          GlobalContext_t& scs_data_structures,
                          BlockContext_t& block_context,
                          std::optional<NondeterministicResults> nondeterministic_res)
{
    if (tx_context) {
        throw std::runtime_error("one execution at one time");
    }

    addr_db = &scs_data_structures.address_db;

    MethodInvocation invocation(tx.tx.invocation);

    tx_context = std::make_unique<TransactionContext_t>(
        tx, tx_hash, scs_data_structures, block_context.block_number, nondeterministic_res);

    defer d{ [this]() {
        extract_results();
        reset();
    } };

    auto invoke_res = invoke_subroutine(invocation, tx.tx.gas_limit);

    if (!invoke_res.result) {
        if (invoke_res.result.error() == wasm_api::InvokeError::UNRECOVERABLE) {
            std::printf("unrecoverable error\n");
            std::terminate();
        }

        return TransactionStatus::FAILURE;
    }

    auto storage_commitment = tx_context -> push_storage_deltas();

    if (!storage_commitment)
    {
        return TransactionStatus::FAILURE;
    }

    if (!tx_context -> tx_results -> validating_check_all_rpc_results_used())
    {
        return TransactionStatus::FAILURE;
    }

    // cannot be rewound -- this forms the threshold for commit
    if (!block_context.tx_set.try_add_transaction(tx_hash, tx, tx_context -> tx_results->get_results().ndeterministic_results))
    {
        return TransactionStatus::FAILURE;
    }

    storage_commitment -> commit(block_context.modified_keys_list);

    return TransactionStatus::SUCCESS;
}

EC_DECL(void)::extract_results()
{
    results_of_last_tx = tx_context->extract_results();
}

EC_DECL(void)::reset()
{
    // nothing to do to clear wasm_context
    active_runtimes.clear();
    tx_context.reset();
    addr_db = nullptr;
}

EC_DECL(std::vector<TransactionLog> const&)::get_logs()
{
    if (!results_of_last_tx) {
        throw std::runtime_error("can't get logs before execution");
    }
    return results_of_last_tx->logs;
}

EC_DECL()::~ExecutionContext()
{
  if (tx_context)
  {
    std::printf("cannot destroy without unwinding inflight tx\n");
    std::fflush(stdout);
    std::terminate();
  }
}

/*
EC_DECL(int32_t)::env_memcmp(uint32_t lhs, uint32_t rhs, uint32_t sz)
{
    tx_context -> consume_gas(gas_memcmp(sz));
    return tx_context -> get_current_runtime()->memcmp(lhs, rhs, sz);
}


EC_DECL(uint32_t)::env_memset(uint32_t ptr, uint32_t val, uint32_t len)
{
    tx_context -> consume_gas(gas_memset(len));
    return tx_context -> get_current_runtime()->memset(ptr, val, len);
}


EC_DECL(uint32_t)::env_memcpy( uint32_t dst, uint32_t src, uint32_t len)
{
    tx_context -> consume_gas(gas_memcpy(len));
    return tx_context -> get_current_runtime()->safe_memcpy(dst, src, len);
}

EC_DECL(uint32_t)::env_strnlen(uint32_t ptr, uint32_t max_len)
{
    tx_context -> consume_gas(gas_strnlen(max_len));
    return tx_context -> get_current_runtime()->safe_strlen(ptr, max_len);
}
*/

EC_DECL(wasm_api::HostFnStatus<uint64_t>)::
syscall_handler(wasm_api::WasmRuntime* runtime, uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) noexcept
{
    auto ret = wasm_api::HostFnStatus<uint64_t>(std::unexpect_t{}, wasm_api::HostFnError::UNRECOVERABLE);
    uint64_t required_gas = UINT64_MAX;

    auto& tx_ctx = *tx_context;

    auto load_from_memory = [&runtime]<TriviallyCopyable T>(T& arg, uint32_t offset) -> bool {
        auto mem = runtime-> get_memory();

        if (((uint64_t)offset) + sizeof(T) > mem.size()) {
            return false;
        }

        T* in = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(mem.data()) + offset);
        arg = *in;

        return true;
    };

    auto load_vec_from_memory = [&runtime]<typename T>(T& arg, uint32_t offset, uint32_t len) -> bool {
        auto mem = runtime-> get_memory();

        if (((uint64_t)offset) + len > mem.size()) {
            return false;
        }

        arg.insert(
            arg.end(),
            reinterpret_cast<uint8_t*>(mem.data()) + offset,
            reinterpret_cast<uint8_t*>(mem.data()) + offset + len);

        return true;
    };

    auto load_storage_key = [&] (AddressAndKey& load_arg, uint32_t offset) -> bool {
        InvariantKey key;
        if (!load_from_memory(key, offset)) {
            return false;
        }
        load_arg = tx_ctx.get_storage_key(key);
        return true;
    };

    auto write_to_memory = [&runtime]<TriviallyCopyable T>(T const& arg, uint32_t offset) -> bool
    {
        auto mem = runtime-> get_memory();

        if (((uint64_t)offset) + sizeof(T) > mem.size()) {
            return false;
        }

        T* out = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(mem.data()) + offset);
        *out = arg;

        return true;
    };

    auto write_vec_to_memory = [&runtime]<typename T>(T const& arg, uint32_t offset, uint32_t len) -> bool
    {
        auto mem = runtime-> get_memory();

        if (((uint64_t)offset) + len > mem.size()) {
            return false;
        }

        std::memcpy(
            reinterpret_cast<uint8_t*>(mem.data()) + offset,
            arg.data(), 
            len);

        return true;
    };

    auto write_slice_to_memory = [&runtime]<typename T>(T const& arg, uint32_t offset, uint32_t slice_start, uint32_t slice_end) -> bool
    {
        auto mem = runtime-> get_memory();

        if (((uint64_t)offset) + slice_end > mem.size()) {
            return false;
        }
        
        if (slice_end < slice_start) {
            return false;
        }

        if (slice_end == slice_start) {
            return true;
        }

        std::memcpy(
            reinterpret_cast<uint8_t*>(mem.data()) + offset,
            arg.data() + slice_start, 
            slice_end - slice_start);

        return true;
    };

    auto deterministic_error = [] {
        return wasm_api::HostFnStatus<uint64_t>(std::unexpect_t{}, wasm_api::HostFnError::DETERMINISTIC_ERROR);
    };

    auto unrecoverable_error = [] {
        return wasm_api::HostFnStatus<uint64_t>(std::unexpect_t{}, wasm_api::HostFnError::UNRECOVERABLE);
    };

    using enum SYSCALLS;

    switch(static_cast<SYSCALLS>(callno))
    {
    case EXIT:
        // unimpl for wasm
        return unrecoverable_error();
        break;
    case WRITE:
    {
        // arg0: str offset
        // arg1: str max len

        // disable if not in debug mode

        required_gas = 0;

        std::vector<uint8_t> log;
        if (!load_vec_from_memory(log, arg0, arg1)) {
            ret = deterministic_error();
            break;
        }

        log.push_back('\0');

        std::printf("print: %s\n", log.data());
        ret = 0;
        break;
    }
    case WRITE_BYTES:
    {
        // arg0: str offset
        // arg1: str max len

        // disable if not in debug mode

        required_gas = 0;

        std::vector<uint8_t> log;
        if (!load_vec_from_memory(log, arg0, arg1)) {
            ret = deterministic_error();
            break;
        }

        std::printf("print: %s len %" PRIu64 "\n", debug::array_to_str(log).c_str(), arg1);
        ret = 0;
        break;
    }
    case LOG:
    {
        // arg0: log offset
        // arg1: log len
        uint32_t log_offset = arg0;
        uint32_t log_len = arg1;
        
        required_gas = gas_log(log_len);

        CONTRACT_INFO("Logging offset=%lu len=%lu", log_offset, log_len);
        std::vector<uint8_t> log;
        if (!load_vec_from_memory(log, log_offset, log_len)) {
            ret = deterministic_error();
            break;
        }
        tx_ctx.tx_results->add_log(TransactionLog(log.begin(), log.end()));
        ret = 0;
        break;
    }
    case INVOKE:
    {
        // arg0: invocation addr
        // arg1: method
        // arg2: calldata
        // arg3: calldata_len
        // arg4: return addr
        // arg5: return max len

        uint32_t methodname = arg1;

        uint32_t calldata = arg2;
        uint32_t calldata_len = arg3;

        uint32_t return_addr = arg4;
        uint32_t return_len = arg5;

        required_gas = gas_invoke(calldata_len + return_len);

        Address invoked_addr;
        if (!load_from_memory(invoked_addr, arg0)) {
            ret = deterministic_error();
            break;
        }

        std::vector<uint8_t> calldata_buf;
        if (!load_vec_from_memory(calldata_buf, calldata, calldata_len)) {
            ret = deterministic_error();
            break;
        }

        MethodInvocation invocation(
            invoked_addr,
            methodname,
            std::move(calldata_buf));

        CONTRACT_INFO("call into %s method %lu",
                      debug::array_to_str(invocation.addr).c_str(),
                      methodname);

        auto invoke_res = invoke_subroutine(invocation, runtime-> get_available_gas());

        if (__builtin_add_overflow_p(required_gas, invoke_res.gas_consumed, static_cast<uint64_t>(0)))
        {
            required_gas = UINT64_MAX;
        } else
        {
            required_gas += invoke_res.gas_consumed;
        }

        // This version of groundhog doesn't implement a try-catch mechanism on state/storageproxy
        if (!invoke_res.result) {
            using enum wasm_api::InvokeError;
            switch(invoke_res.result.error())
            {
            case DETERMINISTIC_ERROR:
                ret = deterministic_error();
                break;
            case OUT_OF_GAS_ERROR:
                ret = wasm_api::HostFnStatus<uint64_t>(std::unexpect_t{}, wasm_api::HostFnError::OUT_OF_GAS);
                break;
            case UNRECOVERABLE:
                return unrecoverable_error();
                break;
            case RETURN:
                ret = 0;
                break;
            default:
                return unrecoverable_error();
            }

            if (!ret) {
                break;
            }
        } else {
            if (*invoke_res.result != 0) {
                ret = deterministic_error();
                break;
            } else {
                ret = 0;
            }
        }

        return_len = std::min<uint32_t>(return_len, tx_ctx.return_buf.size());
    	if (return_len > 0)
    	{
            if (!write_vec_to_memory(tx_ctx.return_buf, return_addr, return_len)) {
                ret = deterministic_error();
                break;
            }
    	}
    	tx_ctx.return_buf.clear();
        ret = return_len;
        break;
    }
    case GET_SENDER:
    {
        // arg0: addr offset
        required_gas = gas_get_msg_sender;

        auto sender = tx_ctx.get_msg_sender();
        if (!sender) {
            ret = deterministic_error();
            break;
        }
        if (!write_to_memory(*sender, arg0)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case GET_SELF_ADDR:
    {
        // arg0: addr offset
        required_gas = gas_get_self_addr;

        Address const& addr = tx_ctx.get_current_method_invocation().addr;
        if (!write_to_memory(addr, arg0)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case GET_SRC_TX_HASH:
    {
        // arg0: hash offset
        required_gas = gas_get_src_tx_hash;
        if (!write_to_memory(tx_ctx.get_src_tx_hash(), arg0)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case GET_INVOKED_TX_HASH:
    {
        // arg0: hash offset
        required_gas = gas_get_invoked_tx_hash;
        if (!write_to_memory(tx_ctx.get_invoked_tx_hash(), arg0)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case GET_BLOCK_NUMBER:
    {
        required_gas = gas_get_block_number;
        ret = tx_ctx.get_block_number();
        break;
    }
    case HAS_KEY:
    {
        // arg0: key offset
        required_gas = gas_has_key;

        AddressAndKey addr_and_key;

        if (!load_storage_key(addr_and_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto res = tx_ctx.storage_proxy.get(addr_and_key);
        ret = res.has_value() ? 1 : 0;
        break;
    }
    case RAW_MEM_SET:
    {
        // arg0: key offset
        // arg1: mem offset
        // arg2: mem len
        uint32_t mem_offset = arg1;
        uint32_t mem_len = arg2;
        
        required_gas = gas_raw_memory_set(mem_len);

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        if (mem_len > RAW_MEMORY_MAX_LEN) {
            ret = deterministic_error();
            break;
        }

        xdr::opaque_vec<RAW_MEMORY_MAX_LEN> data;

        if (!load_vec_from_memory(data, mem_offset, mem_len)) {
            ret = deterministic_error();
            break;
        }

        if (!tx_ctx.storage_proxy.raw_memory_write(
            storage_key, std::move(data)))
        {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case RAW_MEM_GET:
    {
        // arg0: key offset
        // arg1: mem offset
        // arg2: mem max len
        uint32_t output_max_len = arg2;

        required_gas = gas_raw_memory_get(output_max_len);

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto const& res = tx_ctx.storage_proxy.get(storage_key);

        if (!res) {
            ret = -1;
            break;
        }

        if (res->body.type() != ObjectType::RAW_MEMORY) {
            ret = deterministic_error();
            break;
        }

        uint32_t max_write_len = std::min<uint32_t>(res->body.raw_memory_storage().data.size(), output_max_len);

        if (!write_vec_to_memory(res->body.raw_memory_storage().data, arg1, max_write_len)) {
            ret = deterministic_error();
            break;
        }

        ret = max_write_len;
        break;

    }
    case RAW_MEM_GET_LEN:
    {
        // arg0: key offset
        required_gas = gas_raw_memory_get_len;
        
        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto const& res = tx_ctx.storage_proxy.get(storage_key);

        if (!res) {
            ret = -1;
            break;
        }

        if (res->body.type() != ObjectType::RAW_MEMORY) {
            ret = deterministic_error();
            break;
        }

        ret = res->body.raw_memory_storage().data.size();
        break;
    }
    case DELETE_KEY_LAST:
    {
        // arg0: key offset
        required_gas = gas_delete_key_last;

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        if (!tx_ctx.storage_proxy.delete_object_last(storage_key)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case NNINT_SET_ADD:
    {
        // arg0: storage key
        // arg1: set value
        // arg2: delta

        int64_t set_value = arg1;
        int64_t delta = arg2;

        required_gas = gas_nonnegative_int64_set_add;

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        if (!tx_ctx.storage_proxy.nonnegative_int64_set_add(
            storage_key, set_value, delta)) {
            ret = deterministic_error();
            break;
        }

        ret = 0;
        break;
    }
    case NNINT_ADD:
    {
        // arg0: storage key
        // arg1: delta
        int64_t delta = arg1;

        required_gas = gas_nonnegative_int64_add;
        
        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        EXEC_TRACE("int64 add %lld to key %s",
                   delta,
                   debug::array_to_str(storage_key).c_str());

        if (!tx_ctx.storage_proxy.nonnegative_int64_add(
            storage_key, delta)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case NNINT_GET:
    {
        // arg0: key addr
        required_gas = gas_nonnegative_int64_get;
        
        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto const& res = tx_ctx.storage_proxy.get(storage_key);

        if (!res) {
            ret = 0;
            break;
        }

        if (res->body.type() != ObjectType::NONNEGATIVE_INT64) {
            ret = deterministic_error();
            break;
        }
        ret = res->body.nonnegative_int64();
        break;
    }
    case HS_INSERT:
    {
        // arg0: key addr
        // arg1: hash addr
        // arg2: hash threshold
        required_gas = gas_hashset_insert;

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        Hash to_insert;
        if (!load_from_memory(to_insert, arg1)) {
            ret = deterministic_error();
            break;
        }
        if (!tx_ctx.storage_proxy.hashset_insert(
            storage_key, to_insert, arg2)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case HS_INC_LIMIT:
    {
        // arg0: key addr
        // arg1: increase
        required_gas = gas_hashset_increase_limit;

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }
        uint32_t limit_increase = arg1;

        if (!tx_ctx.storage_proxy.hashset_increase_limit(
            storage_key, limit_increase)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case HS_CLEAR:
    {
        // arg0: key addr
        // arg1: threshold
        required_gas = gas_hashset_clear;

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        if (!tx_ctx.storage_proxy.hashset_clear(
            storage_key, arg1)) {
            ret = deterministic_error();
            break;
        }

        ret = 0;
        break;
    }
    case HS_GET_SIZE:
    {
        // arg0: key offset
        required_gas = gas_hashset_get_size;

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (res.has_value()) {
            if (res->body.type() != ObjectType::HASH_SET) {
                ret = deterministic_error();
                break;
            }
            ret = res -> body.hash_set().hashes.size();
            break;
        }
        ret = 0;
        break;
    }
    case HS_GET_MAX_SIZE:
    {
        // arg0: key offset
        required_gas = gas_hashset_get_max_size;

        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (res.has_value()) {
            if (res->body.type() != ObjectType::HASH_SET) {
                ret = deterministic_error();
                break;
            }
            ret = res -> body.hash_set().max_size;
            break;
        }
        ret = 0;
        break;
    }
    case HS_GET_INDEX_OF:
    {
        // arg0: key offset
        // arg1: query threshold
        required_gas = gas_hashset_get_index_of;
        
        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }        

        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (!res.has_value()) {
            ret = deterministic_error();
            break;
        }

        if (res->body.type() != ObjectType::HASH_SET) {
            ret = deterministic_error();
            break;
        }

        auto const& entries = res->body.hash_set().hashes;

        // TODO reimplement with a binary search if need be
        bool found = false;
        for (auto i = 0u; i < entries.size(); i++)
        {
            if (entries[i].index == arg1) {
                ret = i;
                found = true;
                break;
            }
        }
        if (!found) {
            ret = deterministic_error();
            break;
        }
        break;
    }
    case HS_GET_INDEX:
    {
        // arg0: key offset
        // arg1: query index
        // arg2: output offset
        required_gas = gas_hashset_get_index;


        AddressAndKey storage_key;
        if (!load_storage_key(storage_key, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (!res.has_value()) {
            ret = deterministic_error();
            break;        
        }

        if (res->body.type() != ObjectType::HASH_SET) {
            ret = deterministic_error();
            break;
        }

        if (res->body.hash_set().hashes.size() <= arg1) {
            ret = deterministic_error();
            break;
        }

        if (!write_to_memory(
            res->body.hash_set().hashes[arg1].hash, arg2)) {
            ret = deterministic_error();
            break;
        }

        ret = res->body.hash_set().hashes[arg1].index;
        break;
    }
    case CONTRACT_CREATE:
    {
        // arg0: contract index
        // arg1: hash offset out
        uint32_t contract_idx = arg0;

        uint32_t num_contracts = tx_ctx.get_num_deployable_contracts();
        required_gas = gas_create_contract(0);

        if (num_contracts <= contract_idx) {
            ret = deterministic_error();
            break;
        }

        std::shared_ptr<const Contract> contract = std::make_shared<const Contract>(
            tx_ctx.get_deployable_contract(contract_idx));

        required_gas = gas_create_contract(contract -> size());

        Hash h = tx_ctx.contract_db_proxy.create_contract(contract);

        if (!write_to_memory(h, arg1)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    } 
    case CONTRACT_DEPLOY:
    {
        // arg0: contract hash in offset
        // arg1: nonce
        // arg2: out addr offset
        required_gas = gas_deploy_contract;

        Hash contract_hash;
        if (!load_from_memory(contract_hash, arg0)) {
            ret = deterministic_error();
            break;
        }

        auto deploy_addr_opt = tx_ctx.contract_db_proxy.deploy_contract(tx_ctx.get_self_addr(), contract_hash, arg1);

        if (!deploy_addr_opt.has_value()) {
            // failed to deploy contract
            ret = deterministic_error();
            break;
        }

        if (!write_to_memory(*deploy_addr_opt, arg2)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case WITNESS_GET:
    {
        // arg0: wit idx
        // arg1: wit offset out
        // arg2: wit max len out

        required_gas = gas_get_witness(arg2);

        auto wit = tx_ctx.get_witness(arg0);

        if (!wit) {
            ret = deterministic_error();
            break;
        }

        uint32_t write_len = std::min<uint32_t>(wit->value.size(), arg2);

        if (!write_vec_to_memory((*wit).value, arg1, write_len)) {
            ret = deterministic_error();
            break;
        }

        ret = wit->value.size();
        break;
    }
    case WITNESS_GET_LEN:
    {
        // arg0: wit idx
        required_gas = gas_get_witness_len;

        auto wit = tx_ctx.get_witness(arg0);
        if (!wit) {
            ret = deterministic_error();
            break;
        }
        ret = wit -> value.size();
        break;
    }
    case GET_CALLDATA:
    {
        // arg0: calldata offset
        // arg1: slice start
        // arg2: slice end

        uint32_t offset = arg0;

        uint32_t slice_start = arg1;
        uint32_t slice_end = arg2;

        auto& calldata = tx_ctx.get_current_method_invocation().calldata;

        required_gas = gas_get_calldata(0);

        if (slice_end <= slice_start || slice_end > calldata.size()) {
            ret = deterministic_error();
            break;
        }

        required_gas = 
            gas_get_calldata(slice_end - slice_start);

        if (!write_slice_to_memory(calldata, offset, slice_start, slice_end)) {
            ret = deterministic_error();
            break;
        }
        
        ret = 0;
        break;
    }
    case GET_CALLDATA_LEN:
    {
        required_gas = gas_get_calldata_len;
        ret = tx_ctx.get_current_method_invocation().calldata.size();
        break;
    }
    case RETURN:
    {
        // arg0: offset
        // arg1: read len

        uint32_t len = arg1;

        required_gas = gas_return(len);

        if (!load_vec_from_memory(tx_ctx.return_buf, arg0, len)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case GAS:
    {
        // arg0: gas
        required_gas = arg0;
        ret = 0;
        break;   
    }
    case HASH:
    {
        // arg0: input offset
        // arg1: input len
        // arg2: output offset
        required_gas = gas_hash;

        if (arg1 > MAX_HASH_LEN) {
            ret = deterministic_error();
            break;
        }

        std::vector<uint8_t> hash_buf;
        if (!load_vec_from_memory(hash_buf, arg0, arg1)) {
            ret = deterministic_error();
            break;
        }

        Hash out = hash_vec(hash_buf);

        if (!write_to_memory(out, arg2)) {
            ret = deterministic_error();
            break;
        }
        ret = 0;
        break;
    }
    case VERIFY_ED25519:
    {
        // arg0: pk offset
        // arg1: sig offset
        // arg2: msg_offset
        // arg3: msg len

        static_assert(sizeof(PublicKey) == 32, "expected 32 byte pk");
        static_assert(sizeof(Signature) == 64, "expected 64 byte sig");

        required_gas = gas_check_sig_ed25519;

        PublicKey pk;
        Signature sig;
        if (!load_from_memory(pk, arg0)) {
            ret = deterministic_error();
            break;
        }
        if (!load_from_memory(sig, arg1)) {
            ret = deterministic_error();
            break;
        }

        std::vector<uint8_t> msg;
        if (!load_vec_from_memory(msg, arg2, arg3)) {
            ret = deterministic_error();
            break;
        }
        
        bool res = check_sig_ed25519(pk, sig, msg);

        auto s = debug::array_to_str(msg);
        EXEC_TRACE("sig check: %lu on msg %s", res, s.c_str());

        ret = res;
        break;
    }

    default:
        // unknown syscall
        ret = deterministic_error();
        break;
    }

    // should not be at unrecoverable here

    auto consume_gas = [&runtime] (uint64_t gas_amount) -> bool {
       return runtime -> consume_gas(gas_amount);
    };

    if (!consume_gas(required_gas)) {
        return wasm_api::HostFnStatus<uint64_t>(std::unexpect_t{}, wasm_api::HostFnError::OUT_OF_GAS);
    }
    return ret;
}


} // namespace scs
