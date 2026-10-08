#include "common/bench.hpp"
#include "common/keys.hpp"

#include <algorithm>
#include <blossom/blossom.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <random>
#include <vector>

namespace {

constexpr std::size_t kKeys = 500'000; // 605 KB filter (fits in 1 MiB L2 cache)
constexpr std::size_t kProbes = 5'000'000;
constexpr float kFprTarget = 0.01f;

// A batch in which `hit_rate` of the probes were inserted and the rest were not,
// they then get shuffled in order to avoid getting an advantage from the branch predictor.
std::vector<std::uint64_t> mix(
    const std::vector<std::uint64_t>& present, const std::vector<std::uint64_t>& absent, double hit_rate) {
    const std::size_t hits = static_cast<std::size_t>(kProbes * hit_rate);

    std::vector<std::uint64_t> probes(kProbes);
    for (std::size_t probe_index = 0; probe_index < kProbes; ++probe_index)
        // Wraps the index when it reaches the end of the vector
        probes[probe_index] =
            probe_index < hits ? present[probe_index % present.size()] : absent[probe_index % absent.size()];

    std::shuffle(probes.begin(), probes.end(), std::mt19937_64(42));
    return probes;
}

} // namespace

int main() {
    std::cout << "backend,hit_rate,keys,filter_bytes,probes,seconds,million_lookups_per_sec\n";

    const auto keys = make_keys(kKeys, 42);
    const auto absent = make_keys(kProbes, 10);

    for (const auto& [id, name] : backends()) {
        blossom::BloomFilter filter({
            .expected_elements = kKeys,
            .false_positive_rate = kFprTarget,
            .backend = id,
        });
        filter.insert(keys);

        const std::size_t bytes = filter.size_in_bits() / 8;

        // The absent keys still come back present with a probability kFprTarget,
        // so the true hit rate is a little above the requested one
        for (int step = 0; step <= 10; ++step) {
            const double hit_rate = step / 10.0;
            const auto probes = mix(keys, absent, hit_rate);

            const double sec = seconds([&] { filter.lookup(probes); });
            std::cout << std::format(
                "{},{:.2f},{},{},{},{:.9f},{:.2f}\n", name, hit_rate, kKeys, bytes, kProbes, sec, kProbes / sec / 1e6);
        }
    }
    return 0;
}
