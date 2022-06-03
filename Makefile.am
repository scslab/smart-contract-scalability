CLOCAL_AMFLAGS = -I m4

SUBDIRS = $(PKGCONFIG_SUBDIRS)

CLEANFILES = *~ .*~

X_FILES = \
	xdr/storage.x \
	xdr/types.x \
	xdr/transaction.x

XH_FILES = $(X_FILES:.x=.h)

#building x files
$(XH_FILES) : $(XDRC)

SUFFIXES = .x .h .wat .wasm

.x.h:
	$(XDRC)  -hh -o $@ $<

BUILTIN_FNS_SRCS = \
	builtin_fns/builtin_fns.cc \
	builtin_fns/env.cc \
	builtin_fns/logging.cc \
	builtin_fns/runtime.cc

CONTRACT_DB_SRCS = \
	contract_db/contract_db.cc

CRYPTO_SRCS = \
	crypto/crypto_utils.cc

STATE_DB_SRCS = \
	state_db/delta_batch.cc \
	state_db/delta_vec.cc \
	state_db/object_mutator.cc \
	state_db/serial_delta_batch.cc \
	state_db/state_db.cc

TRANSACTION_CONTEXT_SRCS = \
	transaction_context/execution_context.cc \
	transaction_context/global_context.cc \
	transaction_context/method_invocation.cc \
	transaction_context/storage_cache.cc \
	transaction_context/threadlocal_context.cc \
	transaction_context/transaction_context.cc

TX_BLOCK_SRCS = \
	tx_block/tx_block.cc

SRCS = \
	$(BUILTIN_FNS_SRCS) \
	$(CONTRACT_DB_SRCS) \
	$(CRYPTO_SRCS) \
	$(STATE_DB_SRCS) \
	$(TRANSACTION_CONTEXT_SRCS) \
	$(TX_BLOCK_SRCS)

MAIN_CCS = \
	main/simple.cc

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
	$(wildcard cpp_contracts/*.cc)

.cc.wasm: $(wildcard cpp_contracts/sdk/*.h) Makefile.am
	wasi/wasi-sdk/bin/clang -Os -mtune=native -march=native -I./cpp_contracts/ $< -o $@ \
		--sysroot=wasi/wasi-sdk/share/wasi-sysroot \
		-nostdlib \
		-std=gnu++2a \
		-Wl,--no-entry -fno-exceptions -Wl,--allow-undefined

AM_CPPFLAGS = $(xdrpp_CFLAGS) $(libsodium_CFLAGS) $(LIBFYAML_CFLAGS) $(tbb_CFLAGS) $(merkle_trie_toolkit_CFLAGS) $(wasm_api_CFLAGS) $(Catch2_CFLAGS)
LDADD = $(xdrpp_LIBS) $(libsodium_LIBS) $(LIBFYAML_LIBS) $(tbb_LIBS) $(merkle_trie_toolkit_LIBS) $(wasm_api_LIBS) $(Catch2_LIBS)

$(SRCS:.cc=.o) : $(XH_FILES)
$(MAIN_CCS:.cc=.o) : $(XH_FILES)

bin_PROGRAMS = \
	simple \
	test

main/test.o : $(CC_WASMS:.cc=.wasm)

#test: $(TEST_WASMS) $(TEST_SRCS) $(SRCS) test_runner.cc

simple_SOURCES = $(SRCS) main/simple.cc
test_SOURCES = $(SRCS) main/test.cc




