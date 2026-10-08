#pragma once

#include "blossom/detail/geometry.hpp"
#include "blossom/detail/hashing.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// <immintrin.h> ships only on x86-64.
#if defined(__x86_64__)
#define BLOSSOM_HAS_CPU_BACKEND 1

#include <immintrin.h>

// Compile this function for AVX-512 without forcing -mavx512 on everything that is including this.
#define BLOSSOM_AVX512 __attribute__((target("avx512f,avx512dq,avx512vpopcntdq")))

namespace blossom::detail::cpu {

/// Whether the CPU running this program implements every instruction below.
inline bool supported() noexcept {
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512dq") &&
           __builtin_cpu_supports("avx512vpopcntdq");
}

/// Lanes in one register (defines the words handled per instruction).
inline constexpr std::size_t kLanes = Layout::kBlockBits / Layout::kWordBits; // 8

/// Mask returned by a comparison when every lane matched.
inline constexpr __mmask8 kAllLanes = (1u << kLanes) - 1; // 0xFF

static_assert(Layout::kWordsPerBlock % kLanes == 0, "A block must be a whole number of registers");

/// The bits a key claims across a whole block of words.
///
/// Note:
///     (Replaces detail::word_pattern)
BLOSSOM_AVX512 inline __m512i chunk_pattern(std::uint64_t hash) {
    // Broadcast the hash value across all 8 lanes of a 512-bit register
    const __m512i h = _mm512_set1_epi64(static_cast<long long>(hash));

    // Loads this chunk's salts, one per lane
    const __m512i salts = _mm512_load_si512(&kSalts[0]);

    // Multiplies 8 indepdendent 64-bit integers and takes the lower 64 bits of each product
    const __m512i product = _mm512_mullo_epi64(h, salts);

    // Lemire reduction for each number contained in product
    const __m512i positions = _mm512_srli_epi64(product, kHashBits - Layout::kWordBitsLog2);

    // Create a register full of 1s and shift each one of them by their computed position
    return _mm512_sllv_epi64(_mm512_set1_epi64(1), positions);
}

BLOSSOM_AVX512 inline std::vector<std::uint8_t> lookup(
    std::span<const Block> blocks, std::span<const std::uint64_t> keys) {
    std::vector<std::uint8_t> results(keys.size(), 0);
    auto result = results.begin();

    for (const std::uint64_t key : keys) {
        const std::uint64_t hash = splitmix64(key);
        const Block& block = blocks[block_for(hash, blocks.size())];

        // Load the entire block into a register
        const __m512i stored = _mm512_load_si512(&block);

        // Generate the required bitmask for all 8 lanes at once into the pattern register
        const __m512i pattern = chunk_pattern(hash);

        // Compute a bitwise and between the block and the required pattern,
        // then check if it matches the pattern.
        // The result is a mask holding the check result for each lane
        const __mmask8 complete = _mm512_cmpeq_epi64_mask(_mm512_and_si512(stored, pattern), pattern);

        // If each lane returned 1, then the key is found
        *result++ = (complete == kAllLanes);
    }
    return results;
}

BLOSSOM_AVX512 inline void insert(std::span<Block> blocks, std::span<const std::uint64_t> keys) {
    for (const std::uint64_t key : keys) {
        const std::uint64_t hash = splitmix64(key);
        Block& block = blocks[block_for(hash, blocks.size())];

        // Load the entire block into a register
        const __m512i stored = _mm512_load_si512(&block);
        // Generate the required bitmask for all 8 lanes at once into the pattern register
        const __m512i pattern = chunk_pattern(hash);

        // Writes the modified 512-bit vector register back into the block's memory
        _mm512_store_si512(&block, _mm512_or_si512(stored, pattern));
    }
}

BLOSSOM_AVX512 inline void merge(std::span<Block> into, std::span<const Block> from) {
    for (std::size_t block_index = 0; block_index < into.size(); ++block_index) {
        const __m512i lhs_block = _mm512_load_si512(&into[block_index]);
        const __m512i rhs_block = _mm512_load_si512(&from[block_index]);

        // Bitwise OR between lhs and rhs block registers
        const __m512i merged_block = _mm512_or_si512(lhs_block, rhs_block);

        // Store the merged result back to the filter
        _mm512_store_si512(&into[block_index], merged_block);
    }
}

BLOSSOM_AVX512 inline double estimated_fpr(std::span<const Block> blocks) {
    // Compute 1/64 and broadcast it over the lanes
    const __m512d scale = _mm512_set1_pd(1.0 / Layout::kWordBits);

    double total = 0.0;
    for (const Block& block : blocks) {
        // Count the number of bits that are set to 1 in each word of the block
        const __m512i counts = _mm512_popcnt_epi64(_mm512_load_si512(&block));

        // After converting the integers inside of counts,
        // multiply each of them by the scaling factor in order to get a vector of
        // densities for each word
        const __m512d density = _mm512_mul_pd(_mm512_cvtepu64_pd(counts), scale);

        // Compute the product of the values inside of the register
        total += _mm512_reduce_mul_pd(density);
    }
    return total / blocks.size();
}

} // namespace blossom::detail::cpu

#undef BLOSSOM_AVX512

#endif // __x86_64__
