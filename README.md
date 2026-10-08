# Blossom
Header-only sectorized bloom filter with SIMD support.

## Usage

```cpp
#include <blossom/blossom.hpp>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    blossom::BloomFilter filter({
        .expected_elements = 1000,
        .false_positive_rate = 0.01f,
        .backend = blossom::Backend::Scalar, // or ::CPU for SIMD support (if available)
    });

    std::vector<std::uint64_t> keys = {1, 2, 3};
    filter.insert(keys);

    for (const std::uint8_t hit : filter.lookup(keys))
        std::cout << (hit ? "probably present" : "definitely absent");
}
```

## Developing

```sh
make          # build the test and the benchmarks
make test     # backend consistency: scalar and AVX-512 must agree
make asan     # the same test under AddressSanitizer + LeakSanitizer
make bench    # run every benchmark and write the results into tests/benchmarks/results/
```

## Benchmarks

```sh
# needed by bench_fpr only
./tests/benchmarks/scripts/get_data.sh

make bench
```

### Plots
After creating a virtual environment with `uv venv`, run from the root folder:
```sh
source .venv/bin/activate
uv sync
uv run jupyter lab notebooks/plots.ipynb
```
