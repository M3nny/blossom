#include "common/keys.hpp"

#include <blossom/blossom.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

constexpr std::size_t kKeys = 200000;

blossom::BloomFilter filled_filter(blossom::Backend backend, const std::vector<std::uint64_t>& keys) {
    blossom::BloomFilter filter({
        .expected_elements = keys.size(),
        .false_positive_rate = 0.01f,
        .backend = backend,
    });
    filter.insert(keys);
    return filter;
}

} // namespace

int main() {
    std::size_t failures = 0;
    const auto keys = make_keys(kKeys, 42);
    const auto probes = make_keys(kKeys, 10);

    blossom::BloomFilter reference = filled_filter(blossom::Backend::Scalar, keys);

    if (blossom::available(blossom::Backend::CPU)) {
        const blossom::BloomFilter candidate = filled_filter(blossom::Backend::CPU, keys);

        // Check keys that were never inserted
        const auto expected = reference.lookup(probes);
        const auto actual = candidate.lookup(probes);

        if (expected != actual) {
            std::cout << "lookups differ\n";
            ++failures;
        }

        // Check if the implementations estimated_fpr is almost equal
        const double ref_fpr = reference.estimated_fpr();
        const double cand_fpr = candidate.estimated_fpr();
        if (std::abs(ref_fpr - cand_fpr) > 1e-12 * ref_fpr) {
            std::cout << "estimated_fpr " << ref_fpr << " vs " << cand_fpr << "\n";
            ++failures;
        }

        std::cout << "scalar vs avx512 over " << kKeys << " keys: " << (failures == 0 ? "identical" : "differ") << "\n";
    } else {
        std::cout << "no AVX-512 available, comparison skipped\n";
    }

    // Test the merge path, so that it is inspectable by a memory checker.
    reference.merge(filled_filter(blossom::Backend::Scalar, probes));
    return failures != 0;
}
