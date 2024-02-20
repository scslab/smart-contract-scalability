set -ex
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-20/wasi-sysroot-20.0.tar.gz
tar -xf wasi-sysroot-20.0.tar.gz
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-20/libclang_rt.builtins-wasm32-wasi-20.0.tar.gz
tar -xf libclang_rt.builtins-wasm32-wasi-20.0.tar.gz
