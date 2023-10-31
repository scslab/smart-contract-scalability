# Snapshot Status

These contracts were created from commit b940cb19b35b3e2949004ccac00a090f7298b98d

#Caveats

GMP Wasm I have yet to figure out how to build correctly - the gmp-wasm library requires 
various system calls/other features that are not supported in wasm,
and it doesn't really compile correctly end to end, as it is.
So vdf_rpc.wasm might not work out of the box (and should just be ignored).

