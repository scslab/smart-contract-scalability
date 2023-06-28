#pragma once

#include <array>
#include <compare>
#include <cstdint>

#include "crypto/hash.h"

#include <immintrin.h>

namespace scs {

namespace detail {

enum ArithmeticEngine
{
    Basic,
    AVX
};

template<uint16_t Q, ArithmeticEngine engine>
struct EngineImpl
{};

template<>
struct EngineImpl<16, ArithmeticEngine::Basic>
{
    template<size_t word_count>
    static void add(std::array<uint64_t, word_count>& s_words,
                    std::array<uint64_t, word_count> const& o_words)
    {
        constexpr uint64_t maskA = 0xFFFF'0000'FFFF'0000;
        constexpr uint64_t maskB = 0x0000'FFFF'0000'FFFF;

        static_assert(word_count % 4 == 0, "mistaken offset otherwise");

        // using strategy from folly implementation
        for (size_t i = 0; i < word_count; i++) {
            uint64_t out
                = ((s_words[i] & maskA) + (o_words[i] & maskA)) & maskA;
            out |= ((s_words[i] & maskB) + (o_words[i] & maskB)) & maskB;
            s_words[i] = out;
        }
    }
};

#ifdef __AVX2__

template<>
struct EngineImpl<16, ArithmeticEngine::AVX>
{
    template<size_t word_count>
    static void add(std::array<uint64_t, word_count>& s_words,
                    std::array<uint64_t, word_count> const& o_words)
    {
        constexpr uint8_t vec_ops = word_count / 4;
        static_assert(word_count % 4 == 0, "mistaken offset otherwise");

        for (uint32_t i = 0; i < vec_ops; i++) {
            const __m256i* o
                = reinterpret_cast<const __m256i*>(o_words.data()) + i;
            __m256i* self = reinterpret_cast<__m256i*>(s_words.data()) + i;

            alignas(32) const __m256i v1 = _mm256_load_si256(o);
            alignas(32) __m256i v2 = _mm256_load_si256(self);

            __m256i out = _mm256_add_epi16(v1, v2);
            _mm256_store_si256(self, out);
        }
    }
};

#else // ifndef __AVX2__

template<>

struct EngineImpl<16, ArithmeticEngine::AVX> : public EngineImpl<16, ArithmeticEngine::Basic>
{
    #warning "Attempted to use AVX2 but AVX2 not enabled on current target"
};

#endif

} // namespace detail

// parameter choices based on https://eprint.iacr.org/2019/227.pdf
// Not quite the same as blake2x https://www.blake2.net/blake2x.pdf
// but this should be good enough

template<uint16_t N, uint16_t Q, detail::ArithmeticEngine engine>
struct LtHash
{
    using Engine = detail::EngineImpl<Q, engine>;

    static_assert(N % (64 / Q) == 0, "calc error");
    constexpr static uint16_t word_count = N / (64 / Q);

    constexpr static uint32_t hash_output_bytes = 64;
    constexpr static uint8_t hash_invocations
        = (word_count * 8) / hash_output_bytes;

    static_assert(hash_output_bytes * hash_invocations == word_count * 8,
                  "mismatch");

    alignas(64) std::array<uint64_t, word_count> words;

    std::strong_ordering operator<=>(const LtHash&) const = default;
    bool operator==(const LtHash&) const = default;

    LtHash()
        : words()
    {
        words.fill(0);
    }

    template<typename T>
    LtHash(T const& obj)
    {
        std::array<uint8_t, 33> hash_buf;
        auto serialized = xdr::xdr_to_opaque(obj);

        if (crypto_generichash(hash_buf.data(),
                               32,
                               serialized.data(),
                               serialized.size(),
                               NULL,
                               0)
            != 0) {
            throw std::runtime_error("error in crypto_generichash");
        }

        for (uint8_t i = 0; i < hash_invocations; i++) {
            hash_buf[32] = i;
            if (crypto_generichash(reinterpret_cast<uint8_t*>(words.data())
                                       + (i * hash_output_bytes),
                                   hash_output_bytes,
                                   hash_buf.data(),
                                   hash_buf.size(),
                                   NULL,
                                   0)
                != 0) {
                throw std::runtime_error("crypto generichash error");
            }
        }
    }

    void print() const
    {
        std::printf("======================\n");
        for (size_t i = 0; i < word_count; i++) {
            std::printf("%llx\n", words[i]);
        }
    }

    LtHash& operator+=(const LtHash& other)
    {
        Engine::add(words, other.words);
        return *this;
    }

    const std::array<uint64_t, word_count>& get_hash() const { return words; }
};

using LtHash16 = LtHash<1024, 16, detail::ArithmeticEngine::AVX>;

} // namespace scs
