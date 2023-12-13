RocksDB relies on a bunch of compression libraries.  Install those first before building (prior to building rocksdb)

Install process is

`sudo make static_lib install-static -j ROCKSDB_CXX_STANDARD=gnu++2b` or whatever cpp version; this version gets put into the `ROCKSDB_INCLUDES`
probably weird to have a different version of the stdlib in the header (potential for confusion later).  Rocksdb just relies on cpp17,
for its main features anyways.


