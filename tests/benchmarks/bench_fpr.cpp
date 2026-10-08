#include <blossom/blossom.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

constexpr const char* kDatasetPath = "tests/benchmarks/data/fb_200M_uint64";

constexpr float kFprTargets[] = {
    0.1f,
    0.01f,
    0.001f,
    0.0001f,
};

// Load the SOSD binary format (a uint64 count, followed by that many uint64 keys).
// istream::read is a byte API, so everything has to be reinterpreted as chars.
std::vector<std::uint64_t> load() {
    std::ifstream in(kDatasetPath, std::ios::binary);
    if (!in) {
        std::cerr << "cannot open " << kDatasetPath << "\n";
        std::exit(1);
    }

    std::uint64_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    std::vector<std::uint64_t> keys(count);
    in.read(reinterpret_cast<char*>(keys.data()), count * sizeof(std::uint64_t));

    return keys;
}

} // namespace

int main() {
    std::vector<std::uint64_t> keys = load();

    // Estract two disjoint halves from the dataset
    std::vector<std::uint64_t> to_insert, probes;
    to_insert.reserve(keys.size() / 2);
    probes.reserve(keys.size() / 2);
    for (std::size_t key_index = 0; key_index < keys.size(); ++key_index)
        (key_index % 2 == 0 ? to_insert : probes).push_back(keys[key_index]);

    std::cout << "target_fpr,keys,probes,filter_bits,bits_per_key,measured_fpr,estimated_fpr\n";

    for (const float fpr_target : kFprTargets) {
        blossom::BloomFilter filter({
            .expected_elements = to_insert.size(),
            .false_positive_rate = fpr_target,
            .backend = blossom::Backend::Scalar,
        });
        filter.insert(to_insert);

        std::size_t false_positives = 0;
        for (const std::uint8_t hit : filter.lookup(probes))
            if (hit)
                ++false_positives;

        const double measured = static_cast<double>(false_positives) / probes.size();
        const double bits_per_key = static_cast<double>(filter.size_in_bits()) / to_insert.size();

        std::cout << std::format("{:g},{},{},{},{:.3f},{:.8f},{:.8f}\n",
            fpr_target,
            to_insert.size(),
            probes.size(),
            filter.size_in_bits(),
            bits_per_key,
            measured,
            filter.estimated_fpr());
    }
    return 0;
}
