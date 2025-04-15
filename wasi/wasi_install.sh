set -ex
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-25/wasi-sysroot-25.0.tar.gz
tar -xf wasi-sysroot-25.0.tar.gz
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-25/libclang_rt.builtins-wasm32-wasi-25.0.tar.gz
tar -xf libclang_rt.builtins-wasm32-wasi-25.0.tar.gz
