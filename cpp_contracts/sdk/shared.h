#pragma once

namespace scs
{

namespace detail
{

constexpr static 
void write_uint64_t(uint8_t* out, uint64_t value)
{
	for (size_t i = 0; i < 8; i++)
	{
		out[i] = (value >> (8*i)) & 0xFF;
	}
}

}

template<typename OutputArray>
constexpr static 
OutputArray
make_static_32bytes(uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
	OutputArray out = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t* data = out.data();
	detail::write_uint64_t(data, a);
	detail::write_uint64_t(data + 8, b);
	detail::write_uint64_t(data + 16, c);
	detail::write_uint64_t(data + 24, d);
	return out;
}

}
