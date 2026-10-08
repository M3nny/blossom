#pragma once

#include "blossom/types.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace blossom::detail {

struct Layout {
    // 512 bits is one cache line (and exactly one AVX-512 register).
    static constexpr std::size_t kBlockBits = 512;
    static constexpr std::size_t kWordBits = 64;

    // Using bytes since alignas() needs a byte count.
    static constexpr std::size_t kBlockBytes = kBlockBits / 8; // 64

    static constexpr std::size_t kWordsPerBlock = kBlockBits / kWordBits; // 8

    // To address a specific bit position inside a kWordBits word, we need
    // a binary number that is able to represent the range [0, kWordBits),
    // that is: log_2(kWordBits) by counting the zeroes to the right of lowest set bit
    // (We can use this approach since kWordBits is a power of 2)
    static constexpr unsigned kWordBitsLog2 = std::countr_zero(kWordBits); // 6

    // One bit so that a key's own bits can never coincide
    static constexpr std::size_t kBitsPerWord = 1;
    static constexpr std::size_t kBitsPerKey = kWordsPerBlock * kBitsPerWord; // 8

    static_assert(kBlockBits % kWordBits == 0, "Block must divide evenly into words");
    static_assert(std::has_single_bit(kWordBits), "Word size must be 2^n");
    static_assert(std::has_single_bit(kWordsPerBlock), "s must be 2^n");
};

/// One block defines exactly a cache line.
struct alignas(Layout::kBlockBytes) Block {
    std::uint64_t words[Layout::kWordsPerBlock];
};

struct Geometry {
    std::size_t block_bits = Layout::kBlockBits;          // B: bits per block
    std::size_t word_bits = Layout::kWordBits;            // S: bits per word
    std::size_t words_per_block = Layout::kWordsPerBlock; // s: block_bits / word_bits

    std::size_t k = Layout::kBitsPerKey;              // total bits per key, across the block
    std::size_t bits_per_word = Layout::kBitsPerWord; // k / words_per_block

    std::size_t num_blocks = 0;   // how many blocks the sizing plan chose
    std::size_t num_words = 0;    // num_blocks * words_per_block
    std::size_t size_in_bits = 0; // m: num_words * word_bits
};

/// Size a filter for the requested capacity and error rate.
/// m = -k*n / ln(1 - p^(1/k))
inline Geometry plan(const Config& config) {
    Geometry g;
    const std::size_t n = config.expected_elements;

    if (n == 0) {
        g.num_blocks = 1;
    } else {
        const double target = std::clamp<double>(config.false_positive_rate, 1e-12, 0.999);

        const double k = Layout::kBitsPerKey;
        const double m = -k * n / std::log(1.0 - std::pow(target, 1.0 / k));

        g.num_blocks = std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(m / Layout::kBlockBits)));
    }

    g.num_words = g.num_blocks * g.words_per_block;
    g.size_in_bits = g.num_words * g.word_bits;
    return g;
}

} // namespace blossom::detail
