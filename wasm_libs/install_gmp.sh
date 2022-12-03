#!/bin/bash

set -ex

basedir=$(git rev-parse --show-toplevel)

export RANLIB="${basedir}/wasi/wasi-sdk/bin/ranlib"
export AR="${basedir}/wasi/wasi-sdk/bin/ar"

cd ${basedir}/wasm_libs/srcs/gmp-wasm/

pwd

./configure \
	--prefix=${basedir}/wasm_libs/libs/gmp/ \
	CC="${basedir}/wasi/wasi-sdk/bin/clang --sysroot="${basedir}"/wasi/wasi-sdk/share/wasi-sysroot" \
	--host=wasm32-wasi \
	--enable-alloca=alloca \
	ABI=longlong \
	CFLAGS="-O3"

make -j 8

which ranlib

make install


