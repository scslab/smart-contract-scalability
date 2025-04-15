set -ex
curl -LO "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-25/wasi-sdk-25.0-macos.tar.gz"
tar -xf wasi-sdk-25.0-macos.tar.gz
mv wasi-sdk-25.0 wasi-sdk

