#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

// Generate pseudo-random keys.
inline std::vector<std::uint64_t> make_keys(std::size_t n, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<std::uint64_t> keys(n);
    for (std::uint64_t& key : keys)
        key = rng();
    return keys;
}
