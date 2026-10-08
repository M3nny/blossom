#pragma once

#include <algorithm>
#include <blossom/blossom.hpp>
#include <chrono>
#include <functional>
#include <vector>

struct BackendInfo {
    blossom::Backend id;
    const char* name;
};

// Seconds taken by the fastest fn() run.
inline double seconds(const std::function<void()>& fn, int runs = 5) {
    std::vector<double> times;
    for (int run = 0; run < runs; ++run) {
        const auto start = std::chrono::steady_clock::now();
        fn();
        times.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
    }
    return *std::min_element(times.begin(), times.end());
}

inline std::vector<BackendInfo> backends() {
    std::vector<BackendInfo> found{{blossom::Backend::Scalar, "scalar"}};
    if (blossom::available(blossom::Backend::CPU))
        found.push_back({blossom::Backend::CPU, "avx512"});
    return found;
}
