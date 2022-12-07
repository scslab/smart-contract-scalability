CLOCAL_AMFLAGS = -I m4

SUBDIRS = $(PKGCONFIG_SUBDIRS)

CLEANFILES = *~ .*~

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

FILTER_SRCS = \
	filter/apply_context.cc \
	filter/filter_context.cc

HASH_SET_SRCS = \
	hash_set/atomic_set.cc

HASH_SET_TEST_SRCS = \
	hash_set/tests/test_atomic_set.cc

MEMPOOL_SRCS = \
	mempool/mempool.cc

METERING_FFI_SRCS = \
	metering_ffi/metered_contract.cc

OBJECT_SRCS = \
	object/comparators.cc \
	object/make_delta.cc \
	object/object_defaults.cc \
	object/revertable_object.cc

PHASE_SRCS = \
	phase/phases.cc

RPC_SRCS = \
	rpc/rpc_address_db.cc

STATE_DB_SRCS = \
	state_db/delta_vec.cc \
	state_db/modified_keys_list.cc \
	state_db/new_key_cache.cc \
	state_db/state_db.cc

STORAGE_PROXY_SRCS = \
	storage_proxy/proxy_applicator.cc \
	storage_proxy/storage_proxy.cc

THREADLOCAL_SRCS = \
	threadlocal/cancellable_rpc.cc \
	threadlocal/rate_limiter.cc \
	threadlocal/threadlocal_context.cc \
	threadlocal/timeout.cc

TRANSACTION_CONTEXT_SRCS = \
	transaction_context/execution_context.cc \
	transaction_context/global_context.cc \
	transaction_context/method_invocation.cc \
	transaction_context/transaction_context.cc \
	transaction_context/transaction_results.cc

TX_BLOCK_SRCS = \
	tx_block/tx_block.cc \
	tx_block/tx_set.cc

VM_SRCS = \
	vm/genesis.cc \
	vm/vm.cc

SRCS = \
	$(PROTO_SRCS) \
	$(BLOCK_ASSEMBLY_SRCS) \
	$(BUILTIN_FNS_SRCS) \
	$(CONTRACT_DB_SRCS) \
	$(CRYPTO_SRCS) \
	$(DEBUG_SRCS) \
	$(EXPERIMENTS_SRCS) \
	$(HASH_SET_SRCS) \
	$(MEMPOOL_SRCS) \
	$(METERING_FFI_SRCS) \
	$(OBJECT_SRCS) \
	$(PHASE_SRCS) \
	$(RPC_SRCS) \
	$(STATE_DB_SRCS) \
	$(STORAGE_PROXY_SRCS) \
	$(THREADLOCAL_SRCS) \
	$(TRANSACTION_CONTEXT_SRCS) \
	$(TX_BLOCK_SRCS) \
	$(VM_SRCS)

TEST_UTILS_SRCS = \
	test_utils/deploy_and_commit_contractdb.cc

TESTS_CCS = \
	tests/builtin_fns_log_tests.cc \
	tests/builtin_fns_invoke_tests.cc \
	tests/gas_tests.cc \
	tests/hashset_tests.cc \
	tests/storage_tests.cc \
	tests/sdk_tests.cc \
	tests/test_rpc.cc \
	storage_proxy/tests/test_proxy_applicator.cc \
	object/tests/test_revertable_object.cc \
	$(HASH_SET_TEST_SRCS) \
	$(EXPERIMENTS_TEST_SRCS)

MAIN_CCS = \
	main/filter_perf.cc

TEST_WATS = \
	$(wildcard */tests/wat/*.wat)

TEST_WASMS = ${TEST_WATS:.wat=.wasm}

TEST_RUST_WASMS = \
	contracts/built_wasms/test_log.wasm

contracts/built_wasms/%.wasm : $(wildcard contracts/*/src/*.rs) $(wildcard rust_sdk/*/src/*.rs) contracts/Makefile
	cd contracts/ && \
	make $(notdir $@)

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
	cpp_contracts/public_auction.cc

.cc.wasm: Makefile.am Makefile
	wasi/wasi-sdk/bin/clang -Os -mtune=native -march=native -I./cpp_contracts/ $< -o $@ \
		--sysroot=wasi/wasi-sdk/share/wasi-sysroot \
		-std=gnu++2a \
		-nostdlib++ \
		-nostdlib \
		-Wl,--no-entry -fno-exceptions -Wl,--allow-undefined \
		-I./wasm_libs/libs/gmp/include/ \
		-Wl,-lgmp,-L./wasm_libs/libs/gmp/lib \
		wasi/wasi-sdk/lib/clang/13.0.0/lib/wasi/libclang_rt.builtins-wasm32.a

#	-nostdlib 
#	-nostdlib++ 

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

AM_CPPFLAGS = \
	$(xdrpp_CFLAGS) \
	$(libsodium_CFLAGS) \
	$(LIBFYAML_CFLAGS) \
	$(tbb_CFLAGS) \
	$(utility_CFLAGS) \
	$(lmdb_CFLAGS) \
	$(merkle_trie_toolkit_CFLAGS) \
	$(wasm_api_CFLAGS) \
	$(Catch2_CFLAGS) \
	$(grpcpp_DEDUP_CFLAGS) \
	$(protobuf_CFLAGS)

LDADD = $(xdrpp_LIBS) \
	$(libsodium_LIBS) \
	$(LIBFYAML_LIBS) \
	$(tbb_LIBS) \
	$(utility_LIBS) \
	$(lmdb_LIBS) \
	$(merkle_trie_toolkit_LIBS) \
	$(wasm_api_LIBS) \
	$(Catch2_LIBS) \
	./metering/target/release/libinject_metering.a \
	$(grpcpp_LIBS) \
	$(protobuf_LIBS)

metering/target/release/libinject_metering.a : metering/inject_metering/src/*.rs
	cd metering && \
	cargo build --release --package inject_metering

$(SRCS:.cc=.o) : $(XH_FILES)
$(TESTS_CCS:.cc=.o) : $(XH_FILES)
$(MAIN_CCS:.cc=.o) : $(XH_FILES)

bin_PROGRAMS = \
	test

vm/genesis.o: $(CC_WASMS:.cc=.wasm)
main/test.o : $(CC_WASMS:.cc=.wasm)

COMPLETE_SRCS = $(SRCS) $(TESTS_CCS) $(TEST_UTILS_SRCS)

#filter_perf_SOURCES = $(SRCS) main/filter_perf.cc
test_SOURCES = main/test.cc $(COMPLETE_SRCS)

test_LDFLAGS= -Wl,-force_load $(merkle_trie_toolkit_LIBS)

clean-local:
	cd metering && \
	cargo clean

