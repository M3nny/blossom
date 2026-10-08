#pragma once

#include "blossom/detail/geometry.hpp"
#include "blossom/detail/hashing.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace blossom::detail::scalar {

inline std::vector<std::uint8_t> lookup(std::span<const Block> blocks, std::span<const std::uint64_t> keys) {
    std::vector<std::uint8_t> results(keys.size(), 0);
    auto result = results.begin();

    for (const std::uint64_t key : keys) {
        const std::uint64_t hash = splitmix64(key);
        const Block& block = blocks[block_for(hash, blocks.size())];

        bool present = true;
        for (std::size_t word_index = 0; word_index < Layout::kWordsPerBlock; ++word_index) {
            const std::uint64_t pattern = word_pattern(hash, word_index);
            if ((block.words[word_index] & pattern) != pattern) {
                present = false;
                break;
            }
        }

        *result++ = present;
    }
    return results;
}

inline void insert(std::span<Block> blocks, std::span<const std::uint64_t> keys) {
    for (const std::uint64_t key : keys) {
        const std::uint64_t hash = splitmix64(key);
        Block& block = blocks[block_for(hash, blocks.size())];

        for (std::size_t word_index = 0; word_index < Layout::kWordsPerBlock; ++word_index) {
            block.words[word_index] |= word_pattern(hash, word_index);
        }
    }
}

inline void merge(std::span<Block> into, std::span<const Block> from) {
    for (std::size_t block_index = 0; block_index < into.size(); ++block_index)
        for (std::size_t word_index = 0; word_index < Layout::kWordsPerBlock; ++word_index)
            into[block_index].words[word_index] |= from[block_index].words[word_index];
}

inline double estimated_fpr(std::span<const Block> blocks) {
    double total = 0.0;

    for (const Block& block : blocks) {
        double block_rate = 1.0;
        for (const std::uint64_t word : block.words)
            block_rate *= static_cast<double>(std::popcount(word)) / Layout::kWordBits;

        total += block_rate;
    }

    return total / blocks.size();
}

} // namespace blossom::detail::scalar
