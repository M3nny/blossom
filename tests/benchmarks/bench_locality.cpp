#include "common/bench.hpp"
#include "common/keys.hpp"

#include <blossom/detail/geometry.hpp>
#include <blossom/detail/hashing.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

namespace {

using blossom::detail::Block;
using blossom::detail::block_for;
using blossom::detail::kSalts;
using blossom::detail::splitmix64;

// Tested with:
// L1d: 288 KiB (6 instances), 48 KiB (1 instance per core)
// L2:  6 MiB   (6 instances), 1 MiB  (1 instance per core)
// L3:  32 MiB  (1 instance shared)
//
// Shifting by 10 gives us KiB, whilst shifting by 20 gives us MiB
constexpr std::size_t kArrayBytes[] = {
    32u << 10,  // L1d
    128u << 10, // L2
    512u << 10, // L2
    2u << 20,   // L3
    8u << 20,   // L3
    32u << 20,  // L3
    64u << 20,  // DRAM
    128u << 20, // DRAM
};

constexpr std::size_t kKeys = 5'000'000;

// One block per key
std::uint64_t blocked(const std::vector<Block>& array, const std::vector<std::uint64_t>& keys) {
    const std::size_t num_blocks = array.size();
    std::uint64_t acc = 0;

    for (const std::uint64_t key : keys) {
        const std::uint64_t hash = splitmix64(key);

        const std::size_t block_index = block_for(hash, num_blocks);
        const Block& block = array[block_index];

        for (const std::uint64_t word : block.words)
            acc ^= word;
    }
    return acc;
}

// Eight blocks per key (8 words spread across the whole array)
std::uint64_t scattered(const std::vector<Block>& array, const std::vector<std::uint64_t>& keys) {
    const std::size_t num_blocks = array.size();
    std::uint64_t acc = 0;

    for (const std::uint64_t key : keys) {
        const std::uint64_t base_hash = splitmix64(key);

        for (std::size_t word_index = 0; word_index < 8; ++word_index) {
            const std::uint64_t word_salt = kSalts[word_index];
            const std::uint64_t salted_hash = base_hash * word_salt;
            const std::size_t block_index = block_for(salted_hash, num_blocks);
            const Block& block = array[block_index];
            acc ^= block.words[word_index];
        }
    }
    return acc;
}

} // namespace

int main() {
    std::cout << "pattern,array_bytes,keys,seconds,ns_per_key\n";

    const auto keys = make_keys(kKeys, 42);

    // Variable used to make the compiler actually compute the loops,
    // instead of deleting them (since they only side-effect to stdout)
    volatile std::uint64_t sink = 0;

    for (const std::size_t bytes : kArrayBytes) {
        std::vector<Block> array(bytes / sizeof(Block)); // 2^x / 2^9 (divides evenly)

        // For every block set some data on a word
        // This is done in order to avoid memory allocation during time measurement (avoid lazy page allocations)
        for (std::size_t block_index = 0; block_index < array.size(); ++block_index)
            array[block_index].words[block_index % 8] = block_index;

        const double blocked_sec = seconds([&] { sink += blocked(array, keys); });
        std::cout << std::format("blocked,{},{},{:.9f},{:.3f}\n", bytes, kKeys, blocked_sec, blocked_sec / kKeys * 1e9);

        const double scattered_sec = seconds([&] { sink += scattered(array, keys); });
        std::cout << std::format(
            "scattered,{},{},{:.9f},{:.3f}\n", bytes, kKeys, scattered_sec, scattered_sec / kKeys * 1e9);
    }

    return 0;
}
