To install the version of clang with wasm supported:

1. Install a copy of wasi-sdk-* under wasi/

2. Rename that folder to wasi-sdk/

In the future, clang might ship with this wasi built in, or
you could adjust Makefile.am to point to a different install location.

