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
#include "transaction_context/error.h"
#include "transaction_context/global_context.h"
#include "transaction_context/method_invocation.h"
#include "transaction_context/transaction_context.h"
#include "transaction_context/transaction_results.h"
#include "transaction_context/syscall_enum.h"

#include "threadlocal/threadlocal_context.h"

#include "crypto/hash.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "builtin_fns/builtin_fns.h"

#include "storage_proxy/transaction_rewind.h"

#include "transaction_context/global_context.h"

#include "contract_db/contract_utils.h"

#include "utils/defer.h"

#include <utils/time.h>

#include <utility>

#include <sys/mman.h>

#include "lficpp/signal_handler.h"

#define EC_DECL(ret)                                                           \
    template<typename TransactionContext_t>                                    \
    ret ExecutionContext<TransactionContext_t>

#if __cpp_lib_unreachable > 202202L
void unreachable() {
	std::unreachable();
}
#else
void unreachable() {
	perror("unreachable!");
	std::abort();
}
#endif

namespace scs {

template class ExecutionContext<GroundhogTxContext>;
template class ExecutionContext<SisyphusTxContext>;
template class ExecutionContext<TxContext>;

EC_DECL()::ExecutionContext(LFIGlobalEngine& engine)
    : proc_cache(engine, static_cast<void*>(this))
    , tx_context(nullptr)
    , results_of_last_tx(nullptr)
    , addr_db(nullptr)
{
    signal_init();
}

EC_DECL(void)::invoke_subroutine(MethodInvocation const& invocation)
{
    //std::printf("start invoke_subroutine %lf\n", utils::measure_time_from_basept(basept));
    auto* runtime = tx_context->get_runtime_by_addr(invocation.addr);
    if (runtime == nullptr) {
        CONTRACT_INFO("creating new runtime for contract at %s",
                      debug::array_to_str(invocation.addr).c_str());

        runtime = proc_cache.get_new_proc();

        if (runtime == nullptr) {
            throw HostError("failed to allocate new proc");
        }

        auto script
            = tx_context->get_contract_db_proxy().get_script(invocation.addr);

	auto ts = utils::init_time_measurement();
        if (!runtime->set_program(script.bytes, script.len)) {
            throw HostError("program nexist at target address");
        }

        tx_context->add_active_runtime(invocation.addr, runtime);
    }

    tx_context->push_invocation_stack(runtime, invocation);

    //std::printf("start runtime->run %lf\n", utils::measure_time_from_basept(basept));
    //auto s = utils::init_time_measurement();
    //int code;
    //for (int i = 0; i < 100000; i++) {
    int code = runtime->run(invocation.method_name, invocation.calldata);
    //}
    //std::printf("overall runtime = %lf\n", utils::measure_time_from_basept(s));
    tx_context->pop_invocation_stack();
    if (code != 0) {
        throw HostError("invocation failed");
    }
    //std::printf("end invoke_subroutine %lf\n", utils::measure_time_from_basept(basept));
}

constexpr uint32_t WRITE_MAX = 1024;

EC_DECL(uint64_t)::syscall_handler(uint64_t callno,
                                   uint64_t arg0,
                                   uint64_t arg1,
                                   uint64_t arg2,
                                   uint64_t arg3,
                                   uint64_t arg4,
                                   uint64_t arg5)
{
    int64_t ret = -1;

//    std::printf("syscall handle entry syscall %lu time %lf\n", callno, utils::measure_time_from_basept(basept));
    auto load_storage_key = [this](uint64_t addr) -> AddressAndKey {
        auto* p = tx_context -> get_current_runtime();

        if (!p->is_readable(p->addr(addr), 32)) {
            p->exit(-1);
            unreachable();
        }
        InvariantKey key;
        std::memcpy(key.data(),
                    reinterpret_cast<const uint8_t*>(p->addr(addr)),
                    32);
        return tx_context->get_storage_key(key);
    };

    auto load_hash = [this](uint64_t addr) -> Hash {
        auto* p = tx_context -> get_current_runtime();

        if (!p->is_readable(p->addr(addr), 32)) {
            p->exit(-1);
            unreachable();
        }
        Hash h;
        std::memcpy(h.data(),
                    reinterpret_cast<const uint8_t*>(p->addr(addr)),
                    32);
        return h;
    };

    auto load_bytestring = [this](uint64_t offset, uint32_t len) -> std::vector<uint8_t> {
        auto* p = tx_context -> get_current_runtime();
        if (!p->is_readable(p->addr(offset), len))
        {
            p->exit(-1);
            unreachable();
        }
        std::vector<uint8_t> out;
	//out.resize(len);
	//std::memcpy(out.data(), p->addr(offset), len);
        out.insert(out.end(),
            reinterpret_cast<uint8_t*>(p->addr(offset)), 
	    reinterpret_cast<uint8_t*>(p->addr(offset + len)));
        return out;
    };

    DeClProc* p = tx_context->get_current_runtime();

    // printf("SYSCALL %ld %lx %lx %lx %lx %lx\n", callno, arg0, arg1, arg2, arg3, arg4);

    try {
        switch (callno) {
            case _EXIT:
                p->exit(arg0);
                unreachable();
            case _WRITE:
                arg2 = arg2 > WRITE_MAX ? WRITE_MAX : arg2;
                ret = write(1, (const char*)p->addr(arg1), (size_t)arg2);
                break;
            case SBRK: {
                ret = sandboxaddr(p->sbrk(arg0));
                break;
            }
            case LSEEK:
                break;
            case READ:
                break;
            case CLOSE:
                break;
            case LFIHOG_LOG: {
                // arg0: ptr
                // arg1: len
                uintptr_t ptr = arg0;
                uint32_t len = arg1;

                if (p->is_readable(p->addr(ptr), len)) {
                    TransactionLog log;
                    log.insert(log.end(),
                               reinterpret_cast<uint8_t*>(p->addr(ptr)),
                               reinterpret_cast<uint8_t*>(p->addr(ptr + len)));

                    tx_context->tx_results->add_log(log);
                } else {
                    p->exit(-1);
                    unreachable();
                }
                ret = 0;
                break;
            }
            case LFIHOG_INVOKE: {
                // arg0: address
                // arg1: method
                // arg2: calldata_ptr
                // arg3: calldata_len
                // arg4: return_ptr
                // arg5: return_len
                uintptr_t addr = arg0;
                uint32_t method = arg1;
                uintptr_t calldata_addr = arg2;
                uint32_t calldata_len = arg3;
                uintptr_t return_addr = arg4;
                uint32_t return_len = arg5;

                Address address;
                std::vector<uint8_t> calldata;

                if (p->is_readable(p->addr(addr), 32)) {
                    std::memcpy(address.data(),
                                reinterpret_cast<uint8_t*>(p->addr(addr)),
                                32);
                } else {
                    p->exit(-1);
                    unreachable();
                    break;
                }

                if (calldata_len > 0) {
                    if (p->is_readable(p->addr(calldata_addr), calldata_len)) {
                        calldata.insert(
                            calldata.end(),
                            reinterpret_cast<uint8_t*>(p->addr(calldata_addr)),
                            reinterpret_cast<uint8_t*>(
                                p->addr(calldata_addr + calldata_len)));
                    }
                }

                // printf("call into %s method %lu\n",
                //               debug::array_to_str(address).c_str(),
                //               arg1);

                invoke_subroutine(
                    MethodInvocation(address, method, std::move(calldata)));

                return_len = std::min<uint32_t>(return_len,
                                                tx_context->return_buf.size());

                if (return_len > 0) {
                    if (p->is_writable(p->addr(return_addr), return_len)) {
                        std::memcpy(
                            reinterpret_cast<uint8_t*>(p->addr(return_addr)),
                            reinterpret_cast<uint8_t*>(
                                tx_context->return_buf.data()),
                            return_len);
                    }
                }

                tx_context->return_buf.clear();
                ret = 0;
                break;
            }
            case LFIHOG_SENDER: {
                // arg 0: buffer addr (32 bytes)
                if (!p->is_writable(p->addr(arg0), 32)) {
                    p->exit(-1);
                    unreachable();
                }
                auto const& sender = tx_context->get_msg_sender();

                std::memcpy(reinterpret_cast<uint8_t*>(p->addr(arg0)),
                            sender.data(),
                            sender.size());
                static_assert(sender.size() == 32, "mismatch");

                ret = 0;
                break;
            }
            case LFIHOG_SELF_ADDR: {
                // arg 0: buffer addr (32 bytes)
                if (!p->is_writable(p->addr(arg0), 32)) {
                    p->exit(-1);
                    unreachable();
                }
                auto const& buf = tx_context->get_self_addr();

                std::memcpy(reinterpret_cast<uint8_t*>(p->addr(arg0)),
                            buf.data(),
                            buf.size());
                static_assert(buf.size() == 32, "mismatch");

                ret = 0;
                break;
            }
            case LFIHOG_SRC_TX_HASH: {
                // arg 0: buffer addr (32 bytes)
                if (!p->is_writable(p->addr(arg0), 32)) {
                    p->exit(-1);
                    unreachable();
                }
                auto const& buf = tx_context->get_src_tx_hash();

                std::memcpy(reinterpret_cast<uint8_t*>(p->addr(arg0)),
                            buf.data(),
                            buf.size());
                static_assert(buf.size() == 32, "mismatch");

                ret = 0;
                break;
            }
            case LFIHOG_INVOKED_TX_HASH: {
                // arg 0: buffer addr (32 bytes)
                if (!p->is_writable(p->addr(arg0), 32)) {
                    p->exit(-1);
                    unreachable();
                }
                auto const& buf = tx_context->get_invoked_tx_hash();

                std::memcpy(reinterpret_cast<uint8_t*>(p->addr(arg0)),
                            buf.data(),
                            buf.size());
                static_assert(buf.size() == 32, "mismatch");

                ret = 0;
                break;
            }
            case LFIHOG_BLOCK_NUMBER: {
                ret = tx_context -> get_block_number();
                break;
            }
            case LFIHOG_HAS_KEY: {
                // arg0: key addr (32 bytes)
                auto storage_key = load_storage_key(arg0);
                auto res = tx_context->storage_proxy.get(storage_key);
                ret = (res.has_value())? 1 : 0;
                break;
            }
            case LFIHOG_RAW_MEM_SET: {
                //arg0: key addr
                //arg1: mem buf
                //arg2: mem len
                auto storage_key = load_storage_key(arg0);
                auto write_bytes = load_bytestring(arg1, arg2);

                tx_context->storage_proxy.raw_memory_write(
                    storage_key, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>(write_bytes.begin(), write_bytes.end()));
                ret = 0;
                break;
            }
            case LFIHOG_RAW_MEM_GET:{
                //arg0: key addr
                //arg1: out buf
                //arg2: out max len
                auto storage_key = load_storage_key(arg0);

                auto const& res = tx_context -> storage_proxy.get(storage_key);

                if (!res) {
                    ret = 0;
                    break;
                }

                if (res -> body.type() != ObjectType::RAW_MEMORY){
                    throw HostError("type mismatch in raw mem get");
                }

                uint32_t out_max_len = arg2;
                if (!p->is_writable(p->addr(arg1), out_max_len))
                {
                    throw HostError("invalid write region in raw mem get");
                }

                uint32_t write_len = std::min<uint32_t>(out_max_len, res->body.raw_memory_storage().data.size());

                std::memcpy(
                    reinterpret_cast<uint8_t*>(p->addr(arg1)),
                    res->body.raw_memory_storage().data.data(),
                    write_len);
                ret = write_len;
                break;
            }
            case LFIHOG_RAW_MEM_GET_LEN: {
                //arg0: key addr
                auto storage_key = load_storage_key(arg0);

                auto const& res = tx_context -> storage_proxy.get(storage_key);

                if (!res) {
                    ret = -1;
                    break;
                }

                if (res -> body.type() != ObjectType::RAW_MEMORY){
                    throw HostError("type mismatch in raw mem get");
                }
                ret = res->body.raw_memory_storage().data.size();
                break;
            }
            case LFIHOG_DELETE_KEY_LAST:{
                //arg0 : key addr
                auto storage_key = load_storage_key(arg0);

                tx_context -> storage_proxy.delete_object_last(storage_key);
                ret = 0;
                break;
            }

            case LFIHOG_NNINT_SET_ADD:{
                // arg0: key offset
                // arg1: set value
                // arg2: delta

                int64_t set_value = arg1;
                int64_t delta = arg2;

                auto storage_key = load_storage_key(arg0);

                tx_context -> storage_proxy.nonnegative_int64_set_add(storage_key, set_value, delta);
                ret = 0;
                break;
            }
            case LFIHOG_NNINT_ADD: {
                // arg0: key offset
                // arg1: delta
                int64_t delta = arg1;

                auto storage_key = load_storage_key(arg0);

                tx_context -> storage_proxy.nonnegative_int64_add(storage_key, delta);
                ret = 0;
                break;
            }
            case LFIHOG_NNINT_GET: {
                //arg0: key offset
                auto storage_key = load_storage_key(arg0);
                auto const& res = tx_context -> storage_proxy.get(storage_key);

                if (res -> body.type() != ObjectType::NONNEGATIVE_INT64){
                    throw HostError("type mismatch in int get");
                }

                ret = res -> body.nonnegative_int64();
                break;
            }
            case LFIHOG_HS_INSERT: {
                // arg0: key offset
                // arg1: hash offset
                // arg2: threshold
                auto storage_key = load_storage_key(arg0);
                auto hash = load_hash(arg1);
                uint64_t threshold = arg2;

                tx_context -> storage_proxy.hashset_insert(
                    storage_key, hash, threshold);
                ret = 0;
                break;
            }
            case LFIHOG_HS_INC_LIMIT: {
                // arg0: key offset
                // arg1: inc amount
                uint32_t inc_amount = arg1;
                auto storage_key = load_storage_key(arg0);

                tx_context -> storage_proxy.hashset_increase_limit(storage_key, inc_amount);
                ret = 0;
                break;
            }
            case LFIHOG_HS_CLEAR: {
                // arg0: key offset
                // arg1: threshold
                auto storage_key = load_storage_key(arg0);
                tx_context -> storage_proxy.hashset_clear(storage_key, arg1);
                ret = 0;
                break; 
            }
            case LFIHOG_HS_GET_SIZE: {
                // arg0: key offset
                auto storage_key = load_storage_key(arg0);
                auto const& res = tx_context -> storage_proxy.get(storage_key);

                if (!res) {
                    ret = 0;
                    break;
                }
                if (res -> body.type() != ObjectType::HASH_SET)
                {
                    throw HostError("invalid obj type in hs get size");
                }
                ret = res -> body.hash_set().hashes.size();
                break;
            }
            case LFIHOG_HS_GET_MAX_SIZE: {
                // arg0: key offset
                auto storage_key = load_storage_key(arg0);
                auto const& res = tx_context -> storage_proxy.get(storage_key);

                if (!res) {
                    ret = 0;
                    break;
                }
                if (res -> body.type() != ObjectType::HASH_SET)
                {
                    throw HostError("invalid obj type in hs get size");
                }
                ret = res -> body.hash_set().max_size;
                break;
            }
            case LFIHOG_HS_GET_INDEX_OF: {
                // arg0: key offset
                // arg1: threshold
                auto storage_key = load_storage_key(arg0);
                auto const& res = tx_context -> storage_proxy.get(storage_key);

                if (!res) {
                    ret = -1;
                    break;
                }

                if (res -> body.type() != ObjectType::HASH_SET) {
                    throw HostError("invalid obj type in hs_get_index_of");
                }

                auto const& entries = res->body.hash_set().hashes;
                for (auto i = 0u; i < entries.size(); i++) {
                    if (entries[i].index == arg1) {
                        ret = i;
                        break;
                    }
                }
                ret = -1;
                break;
            }
            case LFIHOG_HS_GET_INDEX: {
                // arg0: key offset
                // arg1: index
                // arg2: output offset
                auto storage_key = load_storage_key(arg0);
                auto const& res = tx_context -> storage_proxy.get(storage_key);

                if (!res) {
                    ret = -1;
                    break;
                }

                if (res -> body.type() != ObjectType::HASH_SET) {
                    throw HostError("invalid obj type in hs_get_index_of");
                }

                auto const& hs = res->body.hash_set().hashes;

                if (hs.size() <= arg1) {
                    throw HostError("invalid hashset index");
                }

                if (!(p->is_writable(p->addr(arg2), 32)))
                {
                    p -> exit(-1);
                    unreachable();
                }

                std::memcpy(
                    reinterpret_cast<uint8_t*>(p->addr(arg2)),
                    hs[arg1].hash.data(),
                    Hash::size());

                ret =  hs[arg1].index;
                break;
            }
            case LFIHOG_CONTRACT_CREATE: {
                // arg0: contract index
                // arg1: hash out
                uint64_t num_contracts = tx_context->get_num_deployable_contracts();

                if (arg0 >= num_contracts) {
                    throw HostError("invalid createable contract addr");
                }

                std::shared_ptr<VerifiedScript> contract = std::make_shared<VerifiedScript>(
                    tx_context -> get_deployable_contract(arg0));

                // Here's where the LFI verifier should run
                std::printf("WARNING: the LFI Verifier is not implemented yet @zyedidia\n");

                Hash h = tx_context -> contract_db_proxy.create_contract(contract);

                // if hash_out is not NULL
                if (arg1 != 0)
                {
                    if (!p->is_writable(p->addr(arg1), h.size()))
                    {
                        p->exit(-1);
                        unreachable();
                    }

                    std::memcpy(
                        reinterpret_cast<uint8_t*>(p->addr(arg1)),
                        h.data(),
                        h.size());
                }
                ret = 0;
                break;
            }
            case LFIHOG_CONTRACT_DEPLOY: {
                // arg0: contract hash
                // arg1: nonce
                // arg2: out addr offset

                auto hash = load_hash(arg0);

                auto deploy_addr = compute_contract_deploy_address(tx_context-> get_self_addr(), hash, arg1);

                if (!tx_context -> contract_db_proxy.deploy_contract(deploy_addr, hash)) {
                    throw HostError("failed to deploy contract");
                }

                // if deployed addr out is not NULL
                if (arg2 != 0)
                {
                    if (!p->is_writable(p->addr(arg2), deploy_addr.size()))
                    {
                        p->exit(-1);
                        unreachable();
                    }

                    std::memcpy(
                        reinterpret_cast<uint8_t*>(p->addr(arg2)),
                        deploy_addr.data(),
                        deploy_addr.size());
                }
                ret = 0;
                break;
            }   
            case LFIHOG_WITNESS_GET: {
                // arg0: witness index
                // arg1: out offset
                // arg2: max write len
                uint32_t max_write_len = arg2;

                auto const& wit = tx_context->get_witness(arg0).value;
                max_write_len = std::min<uint32_t>(max_write_len, wit.size());

                if (!p->is_writable(p->addr(arg1), max_write_len))
                {
                    p->exit(-1);
                    unreachable();
                }
                std::memcpy(
                    reinterpret_cast<uint8_t*>(p->addr(arg1)),
                    wit.data(),
                    max_write_len);
                ret = max_write_len;
                break;
            }
            case LFIHOG_WITNESS_GET_LEN: {
                // arg0: witness index
                ret = tx_context -> get_witness(arg0).value.size();
                break;
            }
            case LFIHOG_RETURN: {
                // arg0: return buf
                // arg1: return len
                tx_context -> return_buf = load_bytestring(arg0, arg1);
                break;
            }
            default:
                std::printf("invalid syscall: %ld\n", callno);
                p->exit(-1);
                unreachable();
        }
    } catch (HostError& e) {
        std::printf("tx failed %s\n", e.what());
        CONTRACT_INFO("Execution error: %s", e.what());
        tx_context->get_current_runtime()->exit(1);
        unreachable();
    } catch (std::exception const& e) {
        std::printf("unrecoverable error! %s\n", e.what());
        std::abort();
    }
    catch (...) {
	    std::printf("impossible!\n");
	    std::abort();
    }
    return (uint64_t)ret;
}

template TransactionStatus
ExecutionContext<TxContext>::execute(Hash const&,
                                     SignedTransaction const&,
                                     BaseGlobalContext&,
                                     BaseBlockContext&,
                                     std::optional<NondeterministicResults>);

template TransactionStatus
ExecutionContext<GroundhogTxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    GroundhogGlobalContext&,
    GroundhogBlockContext&,
    std::optional<NondeterministicResults>);

template TransactionStatus
ExecutionContext<SisyphusTxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    SisyphusGlobalContext&,
    SisyphusBlockContext&,
    std::optional<NondeterministicResults>);

template<typename TransactionContext_t>
template<typename BlockContext_t, typename GlobalContext_t>
TransactionStatus
ExecutionContext<TransactionContext_t>::execute(
    Hash const& tx_hash,
    SignedTransaction const& tx,
    GlobalContext_t& scs_data_structures,
    BlockContext_t& block_context,
    std::optional<NondeterministicResults> nondeterministic_res)
{
    if (tx_context) {
        throw std::runtime_error("one execution at one time");
    }

    addr_db = &scs_data_structures.address_db;

    basept = utils::init_time_measurement();

    MethodInvocation invocation(tx.tx.invocation);

    tx_context
        = std::make_unique<TransactionContext_t>(tx,
                                                 tx_hash,
                                                 scs_data_structures,
                                                 block_context.block_number,
                                                 nondeterministic_res);

    defer d{ [this]() {
        extract_results();
        reset();
    } };

    try {
        invoke_subroutine(invocation);
    } catch (HostError& e) {
        std::printf("tx failed %s\n", e.what());
        CONTRACT_INFO("Execution error: %s", e.what());
        return TransactionStatus::FAILURE;
    } catch (...) {
        std::printf("unrecoverable error!\n");
        std::abort();
    }

    auto storage_commitment = tx_context->push_storage_deltas();

    if (!storage_commitment) {
        return TransactionStatus::FAILURE;
    }

    if (!tx_context->tx_results->validating_check_all_rpc_results_used()) {
	    return TransactionStatus::FAILURE;
    }

    // cannot be rewound -- this forms the threshold for commit
    if (!block_context.tx_set.try_add_transaction(
            tx_hash,
            tx,
            tx_context->tx_results->get_results().ndeterministic_results)) {
        
	    std::printf("c\n");
	    return TransactionStatus::FAILURE;
    }

    storage_commitment->commit(block_context.modified_keys_list);

    //std::printf("commit time %lf\n", utils::measure_time_from_basept(basept));
    return TransactionStatus::SUCCESS;
}

EC_DECL(void)::extract_results()
{
    results_of_last_tx = tx_context->extract_results();
}

EC_DECL(void)::reset()
{
    proc_cache.reset();
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
    if (tx_context) {
        std::printf("cannot destroy without unwinding inflight tx\n");
        std::fflush(stdout);
        std::terminate();
    }
}

} // namespace scs
