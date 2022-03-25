CLOCAL_AMFLAGS = -I m4

SUBDIRS = $(PKGCONFIG_SUBDIRS)

CLEANFILES = *~ .*~

X_FILES = \
	xdr/types.x \
	xdr/transaction.x

XH_FILES = $(X_FILES:.x=.h)

#building x files
$(XH_FILES) : $(XDRC)

SUFFIXES = .x .h .wat .wasm

.x.h:
	$(XDRC)  -hh -o $@ $<

TRANSACTION_CONTEXT_SRCS = \
	transaction_context/execution_context.cc

WASM_API_SRCS = \
	wasm_api/wasm_api.cc

WASM3_SRCS = \
	wasm3/source/m3_api_libc.c \
	wasm3/source/m3_api_meta_wasi.c \
	wasm3/source/m3_api_tracer.c \
	wasm3/source/m3_api_uvwasi.c \
	wasm3/source/m3_api_wasi.c \
	wasm3/source/m3_bind.c \
	wasm3/source/m3_code.c \
	wasm3/source/m3_compile.c \
	wasm3/source/m3_core.c \
	wasm3/source/m3_env.c \
	wasm3/source/m3_exec.c \
	wasm3/source/m3_function.c \
	wasm3/source/m3_info.c \
	wasm3/source/m3_module.c \
	wasm3/source/m3_parse.c

SRCS = \
	$(WASM_API_SRCS) \
	$(WASM3_SRCS)

TEST_SRCS = \
	wasm_api/tests/wasm3_test.h

MAIN_CCS = \
	main/simple.cc

TEST_WATS = \
	$(wildcard */tests/wat/*.wat)

TEST_WASMS = ${TEST_WATS:.wat=.wasm}

.wat.wasm:
	wat2wasm -o $@ $<

AM_CPPFLAGS = $(xdrpp_CFLAGS) $(libsodium_CFLAGS) $(LIBFYAML_CFLAGS) $(tbb_CFLAGS) 
LDADD = $(xdrpp_LIBS) $(libsodium_LIBS) $(LIBFYAML_LIBS) $(tbb_LIBS)

$(SRCS:.cc=.o) : $(XH_FILES)
$(MAIN_CCS:.cc=.o) : $(XH_FILES)

test_runner.cc : $(TEST_SRCS) $(XH_FILES) $(TEST_WASMS)
	cxxtestgen --error-printer -o test_runner.cc $(TEST_SRCS)

bin_PROGRAMS = \
	simple \
	test

simple_SOURCES = $(SRCS) main/simple.cc
test_SOURCES = $(SRCS) test_runner.cc



