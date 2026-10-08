#pragma once

#include <cstddef>

namespace blossom {

/// Selects which backend owns the filter's memory and runs its operations.
enum class Backend {
    /// Sectorized bloom filter on CPU, no hardware acceleration.
    Scalar,

    /// Sectorized bloom filter on CPU, with hardware acceleration.
    CPU
};

/// Filter sizing and backend selection.
struct Config {
    /// Number of distinct elements the filter is expected to hold.
    std::size_t expected_elements = 0;

    /// Target false-positive rate at full capacity.
    float false_positive_rate = 0.01f;

    Backend backend = Backend::Scalar;
};

} // namespace blossom
