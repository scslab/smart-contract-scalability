namespace scs
{

typedef unsigned int uint32;
typedef int int32;

typedef unsigned hyper uint64;
typedef hyper int64;

typedef opaque uint128[16];
typedef opaque uint256[32];

typedef opaque Signature[64]; //ed25519 sig len is 512 bits
typedef opaque PublicKey[32]; //ed25519 key len is 256 bits
typedef opaque Hash[32]; // 256 bit hash, i.e. output of sha256
typedef opaque SecretKey[64]; //ed25519 secret key len is 64 bytes, at least on libsodium

typedef opaque Address[32]; // 256 bit contract addresses

typedef opaque InvariantKey[32]; // arbitrary 32 bytes.

typedef opaque AddressAndKey[64]; // in global storage: [contract address + invariant key] = 64 bytes
// Contracts are free to set their own conventions for keys.

typedef opaque Contract<>;

typedef opaque TransactionLog<>;

const MAX_STACK_BYTES = 65536;

// crypto parameters
const MAX_HASH_LEN = 512;

} /* scs */
