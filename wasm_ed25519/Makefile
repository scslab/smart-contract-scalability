
.c.o: Makefile
	clang -O3 --target=wasm32-unknown-wasi -c $< -o $@ \
		--sysroot=../wasi/wasi-sysroot \
		-nostdlib \
		 -fno-exceptions

libed25519.a: ed25519/src/add_scalar.o ed25519/src/fe.o ed25519/src/ge.o ed25519/src/key_exchange.o ed25519/src/keypair.o ed25519/src/sc.o ed25519/src/sha512.o ed25519/src/sign.o ed25519/src/verify.o
	llvm-ar-14 rcs libed25519.a \
		ed25519/src/add_scalar.o \
		ed25519/src/fe.o \
		ed25519/src/ge.o \
		ed25519/src/key_exchange.o \
		ed25519/src/keypair.o \
		ed25519/src/sc.o \
		ed25519/src/sha512.o \
		ed25519/src/sign.o \
		ed25519/src/verify.o
	llvm-ranlib-14 libed25519.a

clean:
	rm ed25519/src/*.o
	rm libed25519.a
