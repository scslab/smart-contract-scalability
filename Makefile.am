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

SUFFIXES = .x .h

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

CONFIG_SRCS = \
	config/print_configs.cc

CONTRACT_DB_SRCS = \
	contract_db/contract_db.cc \
	contract_db/contract_db_proxy.cc \
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

LFICPP_SRCS = \
	lficpp/lficpp.cc \
	lficpp/signal_handler.cc

MEMPOOL_SRCS = \
	mempool/mempool.cc

OBJECT_SRCS = \
	object/comparators.cc \
	object/make_delta.cc \
	object/object_defaults.cc \
	object/revertable_object.cc

PEDERSEN_FFI_SRCS = \
	pedersen_ffi/pedersen.cc

PERSISTENCE_SRCS =
#	persistence/rocksdb_wrapper.cc

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
	$(LFICPP_SRCS) \
	$(MEMPOOL_SRCS) \
	$(OBJECT_SRCS) \
	$(PEDERSEN_FFI_SRCS) \
	$(PERSISTENCE_SRCS) \
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
	$(WASM3_SRCS) \
	$(WASM_API_SRCS) \
	$(utility_CCS)

LFI_CONTRACTS = \
	lfi_contracts/deployer.c \
	lfi_contracts/erc20.c \
	lfi_contracts/payment.c \
	lfi_contracts/test_invoke.c \
        lfi_contracts/test_log.c \
	lfi_contracts/test_redirect_call.c \
	lfi_contracts/test_calldata.c \
	lfi_contracts/test_gas_limit.c

#disable some builtin rule
lfi_contracts/libc/syscall.S:
	echo ""

LFI_BUILTS = $(LFI_CONTRACTS:.c=.lfi)

%.lfi:%.c
	cd lfi_contracts && \
	knit $@

main/test.o: $(LFI_CONTRACTS:.c=.lfi)

$(LFI_BUILTS) : $(wildcard lfi_contracts/libc/*.c) $(wildcard lfi_contracts/libc/*.h) $(wildcard lfi_contracts/libc/*.S)

TEST_UTILS_SRCS = \
	test_utils/deploy_and_commit_contractdb.cc

TESTS_CCS = \
	tests/builtin_fns_log_tests.cc \
	tests/builtin_fns_invoke_tests.cc \
	tests/gas_tests.cc
# tests/hashset_tests.cc \
# tests/storage_tests.cc \
# tests/sdk_tests.cc

#	tests/test_rpc.cc \
#	storage_proxy/tests/test_proxy_applicator.cc \
#	object/tests/test_revertable_object.cc \
#	tx_block/tests/test_unique_txset.cc \
#	$(HASH_SET_TEST_SRCS) \
#	$(EXPERIMENTS_TEST_SRCS) \
#	$(STATE_DB_TEST_SRCS)

MAIN_CCS = \
	main/blockstm_comparison.cc \
	main/test.cc \
	main/sisyphus_payment_sim.cc \
	main/groundhog_payment_sim.cc

wasm_ed25519/libed25519.a:
	cd wasm_ed25519 &&\
	make libed25519.a

#TODO parity-wasm serialization doesn't support bulk memory ops
# This ruins the metering (it outputs wasm in an invalid section ordering)
# Could fix if I care
# https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md
# https://github.com/paritytech/parity-wasm/blob/b760a74d4b533adb01c52fb5a91d59ab87587097/src/elements/module.rs#L635
# add this flag to compile flags to enable (for wasm)
#		-mbulk-memory

grpc_DEDUP_CFLAGS = $(shell echo $(grpc_CFLAGS) | ./dedup.sh | sort | uniq | grep -v -e "-X.*" | grep -v -e "-march.*")
grpcpp_DEDUP_CFLAGS = $(shell echo $(grpcpp_CFLAGS) | ./dedup.sh | sort | uniq | grep -v -e "-X.*" | grep -v -e "-march.*")

AM_CPPFLAGS = \
	$(xdrpp_CFLAGS) \
	$(libsodium_CFLAGS) \
	$(LIBFYAML_CFLAGS) \
	$(tbb_CFLAGS) \
	$(utility_CFLAGS) \
	$(lmdb_CFLAGS) \
	$(mtt_CFLAGS) \
	$(wasm_api_CFLAGS) \
	$(Catch2_CFLAGS) \
	$(grpcpp_DEDUP_CFLAGS) \
	$(protobuf_CFLAGS) \
	-I./lib/lfi/lfi-verify \
	-I./lib/lfi/liblfi/

LDADD = $(xdrpp_LIBS) \
	$(libsodium_LIBS) \
	$(LIBFYAML_LIBS) \
	$(tbb_LIBS) \
	$(utility_LIBS) \
	$(lmdb_LIBS) \
	$(mtt_LIBS) \
	$(rocksdb_LIBS) \
	$(wasm_api_LIBS) \
	$(Catch2_LIBS) \
	./lib/lfi/build/liblfi/liblfi.a \
	$(grpcpp_LIBS) \
	$(protobuf_LIBS)

lib/lfi/build/liblfi/liblfi.a: lib/lfi/liblfi/*.c lib/lfi/liblfi/*.h
	cd lib/lfi && \
	meson setup build && \
	ninja -C build liblfi/liblfi.a

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

test_SOURCES = main/test.cc $(COMPLETE_SRCS) $(utility_TEST_CCS) $(mtt_TEST_CCS)
blockstm_comparison_SOURCES = main/blockstm_comparison.cc $(SRCS)
sisyphus_payment_sim_SOURCES = main/sisyphus_payment_sim.cc $(SRCS)
groundhog_payment_sim_SOURCES = main/groundhog_payment_sim.cc $(SRCS)
basic_SOURCES = main/basic.cc $(COMPLETE_SRCS)
sisyphus_proof_size_exp_SOURCES = main/sisyphus_proof_size_exp.cc $(SRCS)

clean-local:
	cd metering && \
	cargo clean
	rm -f $(CC_WASMS:.cc=.wasm)

