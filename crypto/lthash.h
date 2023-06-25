#pragma once

#include <array>
#include <cstdint>
#include <compare>

#include "crypto/hash.h"

#include <immintrin.h>

namespace scs
{

// parameter choices based on https://eprint.iacr.org/2019/227.pdf
// Not quite the same as blake2x https://www.blake2.net/blake2x.pdf
// but this should be good enough

template<uint16_t N, uint16_t Q>
struct LtHash {

	static_assert(Q == 16, "unimplemented otherwise");

	static_assert(N % (64 / Q) == 0, "calc error");
	constexpr static uint16_t word_count =  N / (64 / Q);

	constexpr static uint32_t hash_output_bytes = 64;
	constexpr static uint8_t hash_invocations = (word_count * 8) / hash_output_bytes;

	static_assert(hash_output_bytes * hash_invocations == word_count * 8, "mismatch");

	alignas(64) std::array<uint64_t, word_count> words;

	std::strong_ordering operator<=>(const LtHash&) const = default;
	bool operator==(const LtHash&) const = default;

	LtHash() : words()
	{
		words.fill(0);
	}

	template<typename T>
	LtHash(T const& obj)
	{
		std::array<uint8_t, 33> hash_buf;
    	auto serialized = xdr::xdr_to_opaque(obj);

    	if (crypto_generichash(
		    		hash_buf.data(),
		           32,
		           serialized.data(),
		           serialized.size(),
		           NULL,
		           0)
		        != 0) {
		        	throw std::runtime_error("error in crypto_generichash");
    	}

		for (uint8_t i = 0; i < hash_invocations; i++)
		{
			hash_buf[32] = i;
			if (crypto_generichash(
				reinterpret_cast<uint8_t*>(words.data()) + (i * hash_output_bytes),
				hash_output_bytes,
				hash_buf.data(),
				hash_buf.size(),
				NULL,
				0) != 0)
			{
				throw std::runtime_error("crypto generichash error");
			}
		}
	}

	void print() const {
		std::printf("======================\n");
		for (size_t i = 0; i < word_count; i++)
		{
			std::printf("%llx\n", words[i]);
		}
	}

	LtHash& operator+=(const LtHash& other)
	{
		constexpr uint8_t vec_ops = word_count / 4;
		static_assert(word_count % 4 == 0, "mistaken offset otherwise");

		for (uint32_t i = 0; i < vec_ops; i++)
		{
			const __m256i* o = reinterpret_cast<const __m256i*>(other.words.data()) + i;
			__m256i* self = reinterpret_cast<__m256i*>(words.data()) + i;

			alignas (32) const __m256i v1 = _mm256_load_si256(o);
       		alignas (32) __m256i v2 = _mm256_load_si256(self);

       		__m256i out = _mm256_add_epi16(v1, v2);
       		_mm256_store_si256(self,out);
       	}

       	return *this;
	}
};

using LtHash16 = LtHash<1024, 16>;


} // namespace scs

