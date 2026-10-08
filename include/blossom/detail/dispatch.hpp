#pragma once

#include "blossom/detail/cpu.hpp"
#include "blossom/detail/geometry.hpp"
#include "blossom/detail/scalar.hpp"
#include "blossom/types.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace blossom::detail {

/// Return nullptr if this machine can run the backend,
/// otherwise return a string explaining what it is missing.
inline const char* unavailable_reason(Backend backend) noexcept {
    switch (backend) {
    case Backend::Scalar:
        return nullptr;
    case Backend::CPU:
#ifdef BLOSSOM_HAS_CPU_BACKEND
        return cpu::supported() ? nullptr : "CPU backend requires AVX-512 support";
#else
        return "CPU backend requires x86-64";
#endif
    }
    return "Unknown backend";
}

inline std::vector<std::uint8_t> lookup(
    [[maybe_unused]] Backend backend, std::span<const Block> blocks, std::span<const std::uint64_t> keys) {
#ifdef BLOSSOM_HAS_CPU_BACKEND
    if (backend == Backend::CPU)
        return cpu::lookup(blocks, keys);
#endif
    return scalar::lookup(blocks, keys);
}

inline void insert([[maybe_unused]] Backend backend, std::span<Block> blocks, std::span<const std::uint64_t> keys) {
#ifdef BLOSSOM_HAS_CPU_BACKEND
    if (backend == Backend::CPU)
        return cpu::insert(blocks, keys);
#endif
    return scalar::insert(blocks, keys);
}

inline void merge([[maybe_unused]] Backend backend, std::span<Block> into, std::span<const Block> from) {
#ifdef BLOSSOM_HAS_CPU_BACKEND
    if (backend == Backend::CPU)
        return cpu::merge(into, from);
#endif
    return scalar::merge(into, from);
}

inline double estimated_fpr([[maybe_unused]] Backend backend, std::span<const Block> blocks) {
#ifdef BLOSSOM_HAS_CPU_BACKEND
    if (backend == Backend::CPU)
        return cpu::estimated_fpr(blocks);
#endif
    return scalar::estimated_fpr(blocks);
}

} // namespace blossom::detail
