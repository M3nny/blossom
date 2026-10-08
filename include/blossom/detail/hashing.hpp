#pragma once

#include "blossom/detail/geometry.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace blossom::detail {

// Width of the hash every step below works in.
inline constexpr unsigned kHashBits = std::numeric_limits<std::uint64_t>::digits; // 64

// The key's hash, and the generator for the salt table.
// See: https://prng.di.unimi.it/splitmix64.c
constexpr std::uint64_t splitmix64(std::uint64_t z) {
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Bit position in [0, kWordBits) derived from a salt value.
// Due to carries cascading upward, the highest bits of the product have the highest entropy,
// we keep the top kWordBitsLog2 of them and drop the rest.
inline std::uint64_t bit_pos(std::uint64_t hash, std::uint64_t salt) {
    return (hash * salt) >> (kHashBits - Layout::kWordBitsLog2);
}

// Block index in [0, num_blocks), by Lemire's multiply-shift reduction.
// See: https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
inline std::size_t block_for(std::uint64_t hash, std::size_t num_blocks) {
    return ((hash >> 32) * num_blocks) >> 32;
}

// Multiplier for the given salt index.
constexpr std::uint64_t salt_at(std::size_t salt_index) {
    const std::uint64_t golden_ratio = 0x9E3779B97F4A7C15ULL;

    // Multiplying by the Golden Ratio constant produces initial states that are well spaced
    // across the 64-bit integer space.
    //
    // Adding 2 to the index avoids the cases where the input would be fixed:
    //  - salt_index = 0 -> 0
    //  - salt_index = 1 -> golden_ratio itself
    const std::uint64_t state = golden_ratio * (salt_index + 2);

    // Since the product could be a 128 bit number, uint64_t would store the lower 64 bits only
    // this is equal to a 2^64 modulo operation.
    // By making the number odd, we ensure the modulo never removes information,
    // before shifting in bit_pos().
    return splitmix64(state) | 1; // odd: required for multiplicative hashing
}

// Generate the salts (one per bit a key sets).
constexpr std::array<std::uint64_t, Layout::kBitsPerKey> make_salts() {
    std::array<std::uint64_t, Layout::kBitsPerKey> salts{};
    for (std::size_t salt_index = 0; salt_index < Layout::kBitsPerKey; ++salt_index)
        salts[salt_index] = salt_at(salt_index);
    return salts;
}

// Precomputed salt table.
alignas(Layout::kBlockBytes) inline constexpr std::array<std::uint64_t, Layout::kBitsPerKey> kSalts = make_salts();

static_assert(Layout::kWordsPerBlock <= kSalts.size(), "Each word needs its own salt");
static_assert(Layout::kBitsPerWord == 1, "A key claims exactly one bit per word");

// The single bit a key claims in one word of its block.
// This creates a bitmask where only one bit is set to 1, and the other (kWordBits-1) bits are set to 0.
inline std::uint64_t word_pattern(std::uint64_t hash, std::size_t word_index) {
    return std::uint64_t{1} << bit_pos(hash, kSalts[word_index]);
}

} // namespace blossom::detail
