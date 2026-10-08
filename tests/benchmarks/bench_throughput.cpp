#include "common/bench.hpp"
#include "common/keys.hpp"

#include <blossom/blossom.hpp>
#include <cstddef>
#include <format>
#include <iostream>

namespace {

// Tested with:
// L1d: 288 KiB (6 instances), 48 KiB (1 instance per core)
// L2:  6 MiB   (6 instances), 1 MiB  (1 instance per core)
// L3:  32 MiB  (1 instance shared)
//
// At the 1% fpr rate target below the filter costs ~9.7 bits per key.
constexpr std::size_t kKeyCounts[] = {
    25'000,      // 30 KB      L1d
    100'000,     // 121 KB     L2
    500'000,     // 605 KB     L2
    1'000'000,   // 1.21 MB    L3
    5'000'000,   // 6.05 MB    L3
    25'000'000,  // 30.25 MB   L3
    50'000'000,  // 60.51 MB   DRAM
    100'000'000, // 121.02 MB  DRAM
};

constexpr float kFprTarget = 0.01f;
constexpr std::size_t kProbes = 4'000'000;

void row(const char* backend,
    const char* operation,
    std::size_t keys,
    std::size_t filter_bytes,
    std::size_t ops,
    double sec) {
    std::cout << std::format(
        "{},{},{},{},{},{:.9f},{:.2f}\n", backend, operation, keys, filter_bytes, ops, sec, ops / sec / 1e6);
}

} // namespace

int main() {
    std::cout << "backend,operation,keys,filter_bytes,ops,seconds,million_ops_per_sec\n";

    const auto probes = make_keys(kProbes, 10);

    for (const std::size_t n : kKeyCounts) {
        const auto keys = make_keys(n, 42);

        for (const auto& [id, name] : backends()) {
            const blossom::Config config{
                .expected_elements = n,
                .false_positive_rate = kFprTarget,
                .backend = id,
            };

            blossom::BloomFilter filter(config);
            const std::size_t bytes = filter.size_in_bits() / 8;

            row(name, "insert", n, bytes, n, seconds([&] { filter.insert(keys); }));
            row(name, "lookup", n, bytes, kProbes, seconds([&] { filter.lookup(probes); }));

            // A second filter of identical geometry
            blossom::BloomFilter other(config);
            other.insert(probes);

            // Define an operation as a 64-byte block processed
            row(name, "merge", n, bytes, bytes / 64, seconds([&] { filter.merge(other); }));
        }
    }
    return 0;
}
