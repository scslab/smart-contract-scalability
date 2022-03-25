CLOCAL_AMFLAGS = -I m4

SUBDIRS = $(PKGCONFIG_SUBDIRS)

CLEANFILES = *~ .*~

X_FILES = \
	xdr/types.x \
	xdr/transaction.x

XH_FILES = $(X_FILES:.x=.h)

#building x files
$(XH_FILES) : $(XDRC)

SUFFIXES = .x .h

.x.h:
	$(XDRC)  -hh -o $@ $<

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

MAIN_CCS = \
	main/simple.cc

AM_CPPFLAGS = $(xdrpp_CFLAGS) $(libsodium_CFLAGS) $(LIBFYAML_CFLAGS) $(tbb_CFLAGS) 
LDADD = $(xdrpp_LIBS) $(libsodium_LIBS) $(LIBFYAML_LIBS) $(tbb_LIBS)

$(SRCS:.cc=.o) : $(XH_FILES)
$(MAIN_CCS:.cc=.o) : $(XH_FILES)

bin_PROGRAMS = \
	simple

simple_SOURCES = $(SRCS) main/simple.cc



