#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/witness.h"
#include "sdk/crypto.h"

namespace sdk
{

namespace detail
{

constexpr static StorageKey pk_storage_key
	= make_static_key(1);

}


void
auth_single_pk_register(const PublicKey& pk)
{
	set_raw_memory(detail::pk_storage_key, pk);
}

void 
auth_single_pk_check_sig(uint64_t wit_idx)
{
	PublicKey pk = get_raw_memory<PublicKey>(detail::pk_storage_key);
	Hash h = get_invoked_hash();
	Signature sig = get_witness<Signature>(wit_idx);

	if (!check_sig_ed25519(pk, sig, h))
	{
		abort();
	}
}


}