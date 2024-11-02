OBJS = syscall.S.o \
	   syscall.c.o \
	   crt0.c.o

all: libsyscalls.a build/install/lib/libc.a

libsyscalls.a: $(OBJS)
	ar rcs $@ $^

syscall.S.o: syscall.S
	$(LFICC) -c $< -o $@ $(CFLAGS)
syscall.c.o: syscall.c
	$(LFICC) -c $< -o $@ $(CFLAGS)
crt0.c.o: crt0.c
	$(LFICC) -c $< -o $@ $(CFLAGS)

build:
	mkdir -p build
	CC=$(LFICC) CFLAGS="-DBUFSIZ=8192 $(CFLAGS)" cd build && meson setup --cross-file ../lfi.txt \
		-Dtls-model=local-exec \
		-Dmultilib=false \
		-Dpicolib=false \
		-Dpicocrt=false \
		-Dpicocrt-lib=false \
		-Dsemihost=false \
		-Dposix-console=true \
		-Dnewlib-global-atexit=true \
		-Dprefix=$(shell pwd)/build/install \
		-Dincludedir=include \
		-Dlibdir=lib \
		-Dthread-local-storage=false \
		-Datomic-ungetc=false \
		-Dspecsdir=none \
		-Dfast-bufio=true \
		../picolibc

build/install/lib/libc.a: build
	ninja -C build install

clean:
	rm -f libsyscalls.a *.o
	rm -rf build

.PHONY: all clean
