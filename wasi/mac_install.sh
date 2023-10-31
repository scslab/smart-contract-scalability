set -ex
curl -LO "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-20/wasi-sdk-20.0-macos.tar.gz"
tar -xf wasi-sdk-20.0-macos.tar.gz
mv wasi-sdk-20.0 wasi-sdk

