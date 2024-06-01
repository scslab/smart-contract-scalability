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
#include "transaction_context/error.h"

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

#define EC_DECL(ret) template<typename TransactionContext_t> ret ExecutionContext<TransactionContext_t>

namespace scs {

template class ExecutionContext<SisyphusTxContext>;

EC_DECL()::ExecutionContext()
    : wasm_context(MAX_STACK_BYTES)
    , active_runtimes()
    , tx_context(nullptr)
    , results_of_last_tx(nullptr)
    , addr_db(nullptr)
{}


EC_DECL(void)::invoke_subroutine(MethodInvocation const& invocation)
{
    auto iter = active_runtimes.find(invocation.addr);
    if (iter == active_runtimes.end()) {
        CONTRACT_INFO("creating new runtime for contract at %s",
                      debug::array_to_str(invocation.addr).c_str());

        //auto timestamp = utils::init_time_measurement();

        auto script = tx_context->get_contract_db_proxy().get_script(invocation.addr);
        wasm_api::Script s {.data = script.data, .len = script.len};

        auto runtime_instance = wasm_context.new_runtime_instance(
            s,
            reinterpret_cast<void*>(this));

        if (!runtime_instance) {
            throw HostError("cannot find target address");
        }

        runtime_instance -> template link_fn<&ExecutionContext<TransactionContext_t>::static_syscall_handler>("scs", "syscall");
        runtime_instance -> template link_fn<&ExecutionContext<TransactionContext_t>::static_gas_handler>("scs", "gas");

        runtime_instance -> template link_env<&ExecutionContext<TransactionContext_t>::static_env_memcmp>("memcmp");
        runtime_instance -> template link_env<&ExecutionContext<TransactionContext_t>::static_env_memset>("memset");
        runtime_instance -> template link_env<&ExecutionContext<TransactionContext_t>::static_env_memcpy>("memcpy");
        runtime_instance -> template link_env<&ExecutionContext<TransactionContext_t>::static_env_strnlen>("strnlen");

        //std::printf("launch time: %lf\n", utils::measure_time(timestamp));

        active_runtimes.emplace(invocation.addr, std::move(runtime_instance));

        //std::printf("link time: %lf\n", utils::measure_time(timestamp));
    }

    auto* runtime = active_runtimes.at(invocation.addr).get();

    tx_context->push_invocation_stack(runtime, invocation);

    runtime->template invoke<void>(
        invocation.get_invocable_methodname().c_str());

    tx_context->pop_invocation_stack();
}

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

    try {
        invoke_subroutine(invocation);
    } catch (wasm_api::HostError& e) {
	    std::printf("tx failed %s\n", e.what());
	    CONTRACT_INFO("Execution error: %s", e.what());

        // Doesn't check return -- duplicate transactions just get added again.
        // In case of nondeterministic results -- whichever one gets added first wins (any duplicates
        // are then assumed to just repeat the same results
        block_context.tx_set.try_add_transaction(tx_hash, tx, tx_context -> tx_results->get_results().ndeterministic_results);

        return TransactionStatus::FAILURE;
    } catch (...) {
        std::printf("unrecoverable error!\n");
        std::abort();
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

EC_DECL(void)::gas_handler(uint64_t gas)
{
    syscall_handler(SYSCALLS::GAS, gas, 0, 0, 0, 0, 0);
}

EC_DECL(int64_t)::
syscall_handler(uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    uint64_t ret = -1;
    auto& tx_ctx = *tx_context;
    auto& runtime = *tx_ctx.get_current_runtime();

    auto load_from_memory = [&runtime]<typename T>(uint32_t offset, uint32_t len) -> T {
        return runtime.template load_from_memory<T>(offset, len);
    };

    auto load_from_memory_constsize = [&runtime]<typename T>(uint32_t offset) -> T {
        return runtime.template load_from_memory_to_const_size_buf<T>(offset);
    };

    auto load_storage_key = [&] (uint32_t offset) -> AddressAndKey {
        auto key = load_from_memory_constsize.template operator()<InvariantKey>(arg0);
        return tx_ctx.get_storage_key(key);
    };

    auto write_to_memory = [&runtime]<typename T>(T const& buf, uint32_t offset, uint32_t len)
    {
        runtime.template write_to_memory(buf, offset, len);
    };

    auto consume_gas = [&tx_ctx] (uint64_t gas_amount) {
	tx_ctx.consume_gas(gas_amount);
    };

    switch(static_cast<SYSCALLS>(callno))
    {
    case EXIT:
        throw HostError("sys exit unimplemented in Wasm3");
    case WRITE:
    {
        // arg0: str offset
        // arg1: str max len

        // disable if not in debug mode

        auto log = load_from_memory.template operator()<std::vector<uint8_t>>(arg0, arg1);
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
        auto log = load_from_memory.template operator()<std::vector<uint8_t>>(arg0, arg1);

        std::printf("print: %s\n", debug::array_to_str(log).c_str());
        ret = 0;
        break;
    }
    case LOG:
    {
        // arg0: log offset
        // arg1: log len
        uint32_t log_offset = arg0;
        uint32_t log_len = arg1;
        
	    consume_gas(gas_log(log_len));

        CONTRACT_INFO("Logging offset=%lu len=%lu", log_offset, log_len);
        auto log = load_from_memory.template operator()<std::vector<uint8_t>>(log_offset, log_len);
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

        consume_gas(gas_invoke(calldata_len + return_len));

        MethodInvocation invocation(
            load_from_memory_constsize.template operator()<Address>(arg0),
            methodname,
            load_from_memory.template operator()<std::vector<uint8_t>>(calldata, calldata_len));

        CONTRACT_INFO("call into %s method %lu",
                      debug::array_to_str(invocation.addr).c_str(),
                      methodname);

        invoke_subroutine(invocation);

        return_len = std::min<uint32_t>(return_len, tx_ctx.return_buf.size());
    	if (return_len > 0)
    	{
            	write_to_memory(tx_ctx.return_buf, return_addr, return_len);
    	}
    	tx_ctx.return_buf.clear();
        ret = return_len;
        break;
    }
    case GET_SENDER:
    {
        // arg0: addr offset
        consume_gas(gas_get_msg_sender);
        Address const& sender = tx_ctx.get_msg_sender();
        write_to_memory(sender, arg0, sender.size());
        ret = 0;
        break;
    }
    case GET_SELF_ADDR:
    {
        // arg0: addr offset
        consume_gas(gas_get_self_addr);
        Address const& addr = tx_ctx.get_current_method_invocation().addr;
        write_to_memory(addr, arg0, addr.size());
        ret = 0;
        break;
    }
    case GET_SRC_TX_HASH:
    {
        // arg0: hash offset
        consume_gas(gas_get_src_tx_hash);
        write_to_memory(tx_ctx.get_src_tx_hash(), arg0, sizeof(Hash));
        ret = 0;
        break;
    }
    case GET_INVOKED_TX_HASH:
    {
        // arg0: hash offset
        consume_gas(gas_get_invoked_tx_hash);
        write_to_memory(tx_ctx.get_invoked_tx_hash(), arg0, sizeof(Hash));
        ret = 0;
        break;
    }
    case GET_BLOCK_NUMBER:
    {
        consume_gas(gas_get_block_number);
        ret = tx_ctx.get_block_number();
        break;
    }
    case HAS_KEY:
    {
        // arg0: key offset
        consume_gas(gas_has_key);

        auto addr_and_key = load_storage_key(arg0);
        auto res = tx_ctx.storage_proxy.get(addr_and_key);
        ret = res.has_value() ? 1 : 0;
        break;
    }
    case RAW_MEM_SET:
    {
        // arg0: key offset
        // arg1: mem offset
        // arg2: mem len
        // arg3: priority
        uint32_t mem_offset = arg1;
        uint32_t mem_len = arg2;
        
        consume_gas(gas_raw_memory_set(mem_len));
        auto storage_key = load_storage_key(arg0);

        if (mem_len > RAW_MEMORY_MAX_LEN) {
            throw HostError("mem write too long");
        }

        auto data
            = load_from_memory.template operator()<xdr::opaque_vec<RAW_MEMORY_MAX_LEN>>(
                      mem_offset, mem_len);

        tx_ctx.storage_proxy.raw_memory_write(
            storage_key, std::move(data), arg3);
        ret = 0;
        break;
    }
    case RAW_MEM_GET:
    {
        // arg0: key offset
        // arg1: mem offset
        // arg2: mem max len
        uint32_t output_max_len = arg2;

        consume_gas(gas_raw_memory_get(output_max_len));
        auto storage_key = load_storage_key(arg0);

        auto const& res = tx_ctx.storage_proxy.get(storage_key);

        if (!res) {
            ret = -1;
            break;
        }

        if (res->body.type() != ObjectType::RAW_MEMORY) {
            throw HostError("type mismatch in raw mem get");
        }

        uint32_t max_write_len = std::min<uint32_t>(res->body.raw_memory_storage().data.size(), output_max_len);

        write_to_memory(res->body.raw_memory_storage().data, arg1, max_write_len);

        ret = max_write_len;
        break;

    }
    case RAW_MEM_GET_LEN:
    {
        // arg0: key offset
        consume_gas(gas_raw_memory_get_len);
        auto storage_key = load_storage_key(arg0);

        auto const& res = tx_ctx.storage_proxy.get(storage_key);

        if (!res) {
            ret = -1;
            break;
        }

        if (res->body.type() != ObjectType::RAW_MEMORY) {
            throw HostError("type mismatch in raw mem get");
        }

        ret = res->body.raw_memory_storage().data.size();
        break;
    }
    case DELETE_KEY_LAST:
    {
        // arg0: key offset
        consume_gas(gas_delete_key_last);
        auto storage_key = load_storage_key(arg0);
        tx_ctx.storage_proxy.delete_object_last(storage_key);
        ret = 0;
        break;
    }
    case NNINT_SET_ADD:
    {
        // arg0: storage key
        // arg1: set value
        // arg2: delta
        // arg3: priority

        int64_t set_value = arg1;
        int64_t delta = arg2;

        consume_gas(gas_nonnegative_int64_set_add);
        auto storage_key = load_storage_key(arg0);

        tx_ctx.storage_proxy.nonnegative_int64_set_add(
            storage_key, set_value, delta, arg3);

        ret = 0;
        break;
    }
    case NNINT_ADD:
    {
        // arg0: storage key
        // arg1: delta
        // arg2: priority
        int64_t delta = arg1;

        consume_gas(gas_nonnegative_int64_add);
        auto storage_key = load_storage_key(arg0);

        EXEC_TRACE("int64 add %lld to key %s",
                   delta,
                   debug::array_to_str(storage_key).c_str());

        tx_ctx.storage_proxy.nonnegative_int64_add(
            storage_key, delta, arg2);
        ret = 0;
        break;
    }
    case NNINT_GET:
    {
        // arg0: key addr
        consume_gas(gas_nonnegative_int64_get);
        auto storage_key = load_storage_key(arg0);

        auto const& res = tx_ctx.storage_proxy.get(storage_key);

        if (!res) {
            ret = 0;
            break;
        }

        if (res->body.type() != ObjectType::NONNEGATIVE_INT64) {
            throw HostError("type mismatch in raw mem get");
        }
        ret = res->body.nonnegative_int64();
        break;
    }
    case HS_INSERT:
    {
        // arg0: key addr
        // arg1: hash addr
        // arg2: hash threshold
        // arg3: priority
        consume_gas(gas_hashset_insert);
        auto storage_key = load_storage_key(arg0);

        auto hash = load_from_memory_constsize.template operator()<Hash>(arg1);
        tx_ctx.storage_proxy.hashset_insert(
            storage_key, hash, arg2, arg3);
        ret = 0;
        break;
    }
    case HS_INC_LIMIT:
    {
        // arg0: key addr
        // arg1: increase
        consume_gas(gas_hashset_increase_limit);
        auto storage_key = load_storage_key(arg0);

        uint32_t limit_increase = arg1;

        tx_ctx.storage_proxy.hashset_increase_limit(
            storage_key, limit_increase);
        ret = 0;
        break;
    }
    case HS_CLEAR:
    {
        // arg0: key addr
        // arg1: threshold
        consume_gas(gas_hashset_clear);
        auto storage_key = load_storage_key(arg0);

        tx_ctx.storage_proxy.hashset_clear(
            storage_key, arg1);
        ret = 0;
        break;
    }
    case HS_GET_SIZE:
    {
        // arg0: key offset
        consume_gas(gas_hashset_get_size);
        auto storage_key = load_storage_key(arg0);
        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (res.has_value()) {
            if (res->body.type() != ObjectType::HASH_SET) {
                throw HostError("type mismatch in hashset get_size");
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
        consume_gas(gas_hashset_get_max_size);
        auto storage_key = load_storage_key(arg0);
        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (res.has_value()) {
            if (res->body.type() != ObjectType::HASH_SET) {
                throw HostError("type mismatch in hashset get_max_size");
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
        consume_gas(gas_hashset_get_index_of);
        auto storage_key = load_storage_key(arg0);
        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (!res.has_value()) {
            throw HostError("key nexist (no hashset)");
        }

        if (res->body.type() != ObjectType::HASH_SET) {
            throw HostError("type mismatch in hashset get_index_of");
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
            throw HostError("key nexist (not found)");
        }
        break;
    }
    case HS_GET_INDEX:
    {
        // arg0: key offset
        // arg1: query index
        // arg2: output offset
        consume_gas(gas_hashset_get_index);
        auto storage_key = load_storage_key(arg0);
        auto res = tx_ctx.storage_proxy.get(storage_key);

        if (!res.has_value()) {
            throw HostError("key nexist (no hashset)");
        }

        if (res->body.type() != ObjectType::HASH_SET) {
            throw HostError("type mismatch in hashset get_index_of_index");
        }

        if (res->body.hash_set().hashes.size() <= arg1) {
            throw HostError("invalid hashset index");
        }

        write_to_memory(
            res->body.hash_set().hashes[arg1].hash, arg2, sizeof(Hash));

        ret = res->body.hash_set().hashes[arg1].index;
        break;
    }
    case CONTRACT_CREATE:
    {
        // arg0: contract index
        // arg1: hash offset out
        uint32_t contract_idx = arg0;

        uint32_t num_contracts = tx_ctx.get_num_deployable_contracts();
        if (num_contracts <= contract_idx) {
            throw HostError("Invalid deployable contract access");
        }

        std::shared_ptr<const Contract> contract = std::make_shared<const Contract>(
            tx_ctx.get_deployable_contract(contract_idx));

        consume_gas(gas_create_contract(contract -> size()));

        Hash h = tx_ctx.contract_db_proxy.create_contract(contract);

        write_to_memory(h, arg1, h.size());
        ret = 0;
        break;
    } 
    case CONTRACT_DEPLOY:
    {
        // arg0: contract hash in offset
        // arg1: nonce
        // arg2: out addr offset
        consume_gas(gas_deploy_contract);

        auto contract_hash = load_from_memory_constsize.template operator()<Hash>(arg0);

        auto deploy_addr_opt = tx_ctx.contract_db_proxy.deploy_contract(tx_ctx.get_self_addr(), contract_hash, arg1);

        if (!deploy_addr_opt.has_value()) {
            throw HostError("failed to deploy contract");
        }

        write_to_memory(*deploy_addr_opt, arg2, deploy_addr_opt->size());
        ret = 0;
        break;
    }
    case WITNESS_GET:
    {
        // arg0: wit idx
        // arg1: wit offset out
        // arg2: wit max len out

        consume_gas(gas_get_witness(arg2));

        auto const& wit = tx_ctx.get_witness(arg0);

        uint32_t write_len = std::min<uint32_t>(wit.value.size(), arg2);

        runtime.write_to_memory(wit.value, arg1, write_len);
        ret = wit.value.size();
        break;
    }
    case WITNESS_GET_LEN:
    {
        // arg0: wit idx
        consume_gas(gas_get_witness_len);
        ret = tx_ctx.get_witness(arg0).value.size();
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
        slice_end = std::min<uint32_t>(slice_end, calldata.size());

        if (slice_end <= slice_start) {
            throw HostError("invalid calldata params");
        }

        consume_gas(
            gas_get_calldata(slice_end - slice_start));
        
        tx_ctx.get_current_runtime()->write_slice_to_memory(
            calldata, offset, slice_start, slice_end);
        ret = 0;
        break;
    }
    case GET_CALLDATA_LEN:
    {
        consume_gas(gas_get_calldata_len);
        ret = tx_ctx.get_current_method_invocation().calldata.size();
        break;
    }
    case RETURN:
    {
        // arg0: offset
        // arg1: read len

        uint32_t len = arg1;
        consume_gas(gas_return(len));

        tx_ctx.return_buf = 
            load_from_memory.template operator()<std::vector<uint8_t>>(arg0, len);
        ret = 0;
        break;
    }
    case GAS:
    {
        // arg0: gas
        consume_gas(arg0);
        break;   
    }
    case HASH:
    {
        // arg0: input offset
        // arg1: input len
        // arg2: output offset
        consume_gas(gas_hash);

        auto hash_buf = load_from_memory.template operator()<xdr::opaque_vec<MAX_HASH_LEN>>(arg0, arg1);

        Hash out = hash_xdr(hash_buf);

        write_to_memory(out, arg2, out.size());
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

        consume_gas(gas_check_sig_ed25519);
        
        auto pk = load_from_memory_constsize.template operator()<PublicKey>(arg0);
        auto sig = load_from_memory_constsize.template operator()<Signature>(arg1);

        auto msg = load_from_memory.template operator()<std::vector<uint8_t>>(arg2, arg3);

        bool res = check_sig_ed25519(pk, sig, msg);

        auto s = debug::array_to_str(msg);
        EXEC_TRACE("sig check: %lu on msg %s", res, s.c_str());

        ret = res;
        break;
    }

    default:
        throw HostError(std::string("invalid syscall no: ") + std::to_string(callno));
    }
    return ret;
}


} // namespace scs
