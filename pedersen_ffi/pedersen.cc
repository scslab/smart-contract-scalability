#include "pedersen_ffi/pedersen.h"

namespace scs
{

static const void* pedersen_params = nullptr;


struct PedersenQuery {
    uint64_t query_low;
    uint64_t query_high;
    uint8_t blinding[32];
};

struct PedersenResponse {
	uint8_t serialized[32];
};


namespace detail
{

extern "C"
{
void* gen_pedersen_params();
void free_pedersen_params(void*);
PedersenResponse pedersen_commitment(const PedersenQuery*, const void*);
}

} // detail

std::array<uint8_t, 32>
pedersen_commitment(unsigned __int128 value, Hash const& blinding)
{
	PedersenQuery query;
	query.query_low = value & UINT64_MAX;
	query.query_high = value >> 64;
	std::memcpy(query.blinding, blinding.data(), 32);

	PedersenResponse response = detail::pedersen_commitment(&query, pedersen_params);

	std::array<uint8_t, 32> out;
	std::memcpy(out.data(), response.serialized, 32);
	return out;
}


void init_pedersen() 
{
	pedersen_params = detail::gen_pedersen_params();
}



}