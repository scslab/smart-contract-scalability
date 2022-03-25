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

typedef opaque InvariantKey[40]; // 32 bytes for address, 8 bytes for invariant id

typedef opaque Contract<>;

const MAX_STACK_BYTES = 65536;

} /* scs */
