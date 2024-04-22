#
# Copyright 2023 Geoffrey Ramseyer
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

CLOCAL_AMFLAGS = -I m4

SUBDIRS = $(PKGCONFIG_SUBDIRS)

CLEANFILES = *~ .*~

include $(srcdir)/lib/utility/Makefile.am.fragment
include $(srcdir)/lib/merkle_trie_toolkit/Makefile.am.fragment

X_FILES = \
	xdr/block.x \
	xdr/rpc.x \
	xdr/storage.x \
	xdr/storage_delta.x \
	xdr/types.x \
	xdr/transaction.x

XH_FILES = $(X_FILES:.x=.h)

#building x files
$(XH_FILES) : $(XDRC)

SUFFIXES = .x .h .wat .wasm

.x.h:
	$(XDRC)  -hh -o $@ $<

proto/external_call.pb.cc proto/external_call.pb.h : proto/external_call.proto
	$(PROTOC) --cpp_out=. proto/external_call.proto

proto/external_call.grpc.pb.h proto/external_call.grpc.pb.cc : proto/external_call.proto
	$(PROTOC) --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` proto/external_call.proto

PROTO_SRCS = \
	proto/external_call.pb.cc \
	proto/external_call.grpc.pb.cc

BLOCK_ASSEMBLY_SRCS = \
	block_assembly/assembly_worker.cc \
	block_assembly/limits.cc

BUILTIN_FNS_SRCS = \
	builtin_fns/asset.cc \
	builtin_fns/builtin_fns.cc \
	builtin_fns/contract.cc \
	builtin_fns/crypto.cc \
	builtin_fns/delete.cc \
	builtin_fns/env.cc \
	builtin_fns/general_storage.cc \
	builtin_fns/hashset.cc \
	builtin_fns/int64.cc \
	builtin_fns/logging.cc \
	builtin_fns/raw_memory.cc \
	builtin_fns/rpc.cc \
	builtin_fns/runtime.cc \
	builtin_fns/witness.cc

CONFIG_SRCS = \
	config/print_configs.cc

CONTRACT_DB_SRCS = \
	contract_db/contract_db.cc \
	contract_db/contract_db_proxy.cc \
	contract_db/contract_persistence.cc \
	contract_db/contract_utils.cc \
	contract_db/uncommitted_contracts.cc

CRYPTO_SRCS = \
	crypto/crypto_utils.cc

DEBUG_SRCS = \
	debug/debug_utils.cc

EXPERIMENTS_SRCS = \
	experiments/payment_experiment.cc

EXPERIMENTS_TEST_SRCS = \
	experiments/tests/test_payment_experiment.cc

HASH_SET_SRCS = \
	hash_set/atomic_set.cc

HASH_SET_TEST_SRCS = \
	hash_set/tests/test_atomic_set.cc \
	hash_set/tests/test_hs_utils.cc

LFI_SRCS = \
	lficpp/lficpp.cc \
	lficpp/signal_handler.cc

MEMPOOL_SRCS = \
	mempool/mempool.cc

OBJECT_SRCS = \
	object/comparators.cc \
	object/make_delta.cc \
	object/object_defaults.cc \
	object/revertable_object.cc

PHASE_SRCS = \
	phase/phases.cc

RPC_SRCS = \
	rpc/rpc_address_db.cc

RPC_SERVER_SRCS = \
	rpc_server/server_runner.cc

SISYPHUS_VM_SRCS = \
	sisyphus_vm/vm.cc

STATE_DB_SRCS = \
	state_db/groundhog_persistent_state_db.cc \
	state_db/modified_keys_list.cc \
	state_db/new_key_cache.cc \
	state_db/sisyphus_state_db.cc \
	state_db/state_db.cc \
	state_db/state_db_v2.cc \
	state_db/typed_modification_index.cc

STATE_DB_TEST_SRCS = \
	state_db/tests/test_typed_modification_index.cc

STORAGE_PROXY_SRCS = \
	storage_proxy/proxy_applicator.cc \
	storage_proxy/storage_proxy.cc

THREADLOCAL_SRCS = \
	threadlocal/cancellable_rpc.cc \
	threadlocal/rate_limiter.cc \
	threadlocal/threadlocal_context.cc

TRANSACTION_CONTEXT_SRCS = \
	transaction_context/execution_context.cc \
	transaction_context/global_context.cc \
	transaction_context/method_invocation.cc \
	transaction_context/transaction_context.cc \
	transaction_context/transaction_results.cc

# this file also depends on syscall.S but including that explicit dependency makes Make try and rebuild it incorrectly
transaction_context/syscall_enum.h : lfi_contracts/libc/enum_import_gen.sh
	./lfi_contracts/libc/enum_import_gen.sh > transaction_context/syscall_enum.h

transaction_context/execution_context.cc : transaction_context/syscall_enum.h

TX_BLOCK_SRCS = \
	tx_block/tx_set.cc \
	tx_block/unique_tx_set.cc

VM_SRCS = \
	vm/genesis.cc \
	vm/vm.cc \
	vm/groundhog_vm.cc \
	vm/base_vm.cc 

SRCS = \
	$(BLOCK_ASSEMBLY_SRCS) \
	$(CONFIG_SRCS) \
	$(CONTRACT_DB_SRCS) \
	$(CRYPTO_SRCS) \
	$(DEBUG_SRCS) \
	$(EXPERIMENTS_SRCS) \
	$(HASH_SET_SRCS) \
	$(LFI_SRCS) \
	$(MEMPOOL_SRCS) \
	$(OBJECT_SRCS) \
	$(PHASE_SRCS) \
	$(RPC_SRCS) \
	$(RPC_SERVER_SRCS) \
	$(SISYPHUS_VM_SRCS) \
	$(STATE_DB_SRCS) \
	$(STORAGE_PROXY_SRCS) \
	$(THREADLOCAL_SRCS) \
	$(TRANSACTION_CONTEXT_SRCS) \
	$(TX_BLOCK_SRCS) \
	$(VM_SRCS) \
	$(utility_CCS) \
	lib/lfi_verify/verify.c

TEST_UTILS_SRCS = \
	test_utils/deploy_and_commit_contractdb.cc

TESTS_CCS = \
	tests/builtin_fns_invoke_tests.cc \
	tests/gas_tests.cc \
	tests/invoke_tests.cc \
	$(HASH_SET_TEST_SRCS) \
	$(STATE_DB_TEST_SRCS) \
	storage_proxy/tests/test_proxy_applicator.cc

MAIN_CCS = \
	main/blockstm_comparison.cc \
	main/test.cc \
	main/sisyphus_payment_sim.cc \
	main/groundhog_payment_sim.cc

.wat.wasm:
	wat2wasm -o $@ $<

CC_WASMS = \
	cpp_contracts/erc20.cc \
	cpp_contracts/erc721.cc \
	cpp_contracts/test_hashset.cc \
	cpp_contracts/test_hashset_manipulation.cc \
	cpp_contracts/test_gas_limit.cc \
	cpp_contracts/test_log.cc \
	cpp_contracts/test_nn_int64.cc \
	cpp_contracts/test_redirect_call.cc \
	cpp_contracts/test_raw_memory.cc \
	cpp_contracts/test_rpc.cc \
	cpp_contracts/test_sdk.cc \
	cpp_contracts/genesis/deploy.cc \
	cpp_contracts/atomic_swap.cc \
	cpp_contracts/payment_experiment/payment.cc \
	cpp_contracts/payment_channel.cc \
	cpp_contracts/public_auction.cc \
	cpp_contracts/0x.cc \
	cpp_contracts/cfmm.cc \
	cpp_contracts/sisyphus_erc20.cc

.cc.wasm: Makefile.am Makefile
	clang -Os -target wasm32-unknown-wasi -I./cpp_contracts/ $< -o $@ \
		--sysroot=wasi/wasi-sysroot \
		-std=gnu++2a \
		-nostdlib++ \
		-nostdlib \
		-Wl,--no-entry -fno-exceptions -Wl,--allow-undefined \
		wasi/lib/wasi/libclang_rt.builtins-wasm32.a

#TODO parity-wasm serialization doesn't support bulk memory ops
# This ruins the metering (it outputs wasm in an invalid section ordering)
# Could fix if I care
# https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md
# https://github.com/paritytech/parity-wasm/blob/b760a74d4b533adb01c52fb5a91d59ab87587097/src/elements/module.rs#L635
# add this flag to compile flags to enable (for wasm)
#		-mbulk-memory

$(CC_WASMS:.cc=.wasm) : $(wildcard cpp_contracts/sdk/*.h)

grpc_DEDUP_CFLAGS = $(shell echo $(grpc_CFLAGS) | ./dedup.sh | sort | uniq | grep -v -e "-X.*" | grep -v -e "-march.*")
grpcpp_DEDUP_CFLAGS = $(shell echo $(grpcpp_CFLAGS) | ./dedup.sh | sort | uniq | grep -v -e "-X.*" | grep -v -e "-march.*")

LFI_CFLAGS = -I./lib/lfi/liblfi/

LFI_LDADD = lib/lfi/liblfi/liblfi.a

lib/lfi/liblfi/liblfi.a: lib/lfi/liblfi/*.c lib/lfi/liblfi/*.h lib/lfi/liblfi/*.S
	cd lib/lfi/liblfi && \
	knit liblfi.a

AM_CPPFLAGS = \
	$(xdrpp_CFLAGS) \
	$(libsodium_CFLAGS) \
	$(LIBFYAML_CFLAGS) \
	$(tbb_CFLAGS) \
	$(utility_CFLAGS) \
	$(lmdb_CFLAGS) \
	$(mtt_CFLAGS) \
	$(Catch2_CFLAGS) \
	$(grpcpp_DEDUP_CFLAGS) \
	$(protobuf_CFLAGS) \
	$(LFI_CFLAGS)

LDADD = $(xdrpp_LIBS) \
	$(libsodium_LIBS) \
	$(LIBFYAML_LIBS) \
	$(tbb_LIBS) \
	$(utility_LIBS) \
	$(lmdb_LIBS) \
	$(mtt_LIBS) \
	$(rocksdb_LIBS) \
	$(Catch2_LIBS) \
	$(grpcpp_LIBS) \
	$(protobuf_LIBS) \
	$(LFI_LDADD)

$(SRCS:.cc=.o) : $(XH_FILES)
$(TESTS_CCS:.cc=.o) : $(XH_FILES)
$(MAIN_CCS:.cc=.o) : $(XH_FILES)

bin_PROGRAMS = \
	test \
	basic \
	blockstm_comparison \
	sisyphus_payment_sim \
	groundhog_payment_sim \
	sisyphus_proof_size_exp

COMPLETE_SRCS = $(SRCS) $(TESTS_CCS) $(TEST_UTILS_SRCS)

test_SOURCES = main/test.cc $(COMPLETE_SRCS) $(utility_TEST_CCS) $(mtt_TEST_CCS) $(WASM_API_TEST_CCS)
blockstm_comparison_SOURCES = main/blockstm_comparison.cc $(SRCS)
sisyphus_payment_sim_SOURCES = main/sisyphus_payment_sim.cc $(SRCS)
groundhog_payment_sim_SOURCES = main/groundhog_payment_sim.cc $(SRCS)
basic_SOURCES = main/basic.cc $(COMPLETE_SRCS)
sisyphus_proof_size_exp_SOURCES = main/sisyphus_proof_size_exp.cc $(SRCS)


