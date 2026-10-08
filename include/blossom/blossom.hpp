#pragma once

#include "blossom/detail/dispatch.hpp"
#include "blossom/detail/geometry.hpp"
#include "blossom/types.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace blossom {

inline bool available(Backend backend) noexcept {
    return detail::unavailable_reason(backend) == nullptr;
}

// Requires that std::hash is defined for T and its result is
// convertible to std::size_t
template <typename T>
concept StdHashable = requires(const T& t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

class BloomFilter {
  public:
    /// Construct a filter sized for the given configuration.
    ///
    /// @throws std::runtime_error if this machine cannot run the requested backend.
    BloomFilter(Config config) : backend_(config.backend), geom_(detail::plan(config)), blocks_(geom_.num_blocks) {
        if (const char* reason = detail::unavailable_reason(backend_))
            throw std::runtime_error(reason);
    }

    /// Test a batch of keys.
    ///
    /// @param  keys Contiguous sequence of 64-bit keys to test.
    /// @return A vector where result[i] is nonzero if keys[i] is probably present,
    ///         and zero if it is definitely absent.
    std::vector<std::uint8_t> lookup(std::span<const std::uint64_t> keys) const {
        return detail::lookup(backend_, blocks_, keys);
    }

    /// lookup overload for accepting arbitrary hashable ranges.
    /// If the elements are already uint64_t, they take use the original method directly.
    template <std::ranges::input_range R>
        requires StdHashable<std::ranges::range_value_t<R>> &&
                 (!std::same_as<std::ranges::range_value_t<R>, std::uint64_t>)
    std::vector<std::uint8_t> lookup(const R& keys) const {
        return lookup(to_uint64(keys));
    }

    /// Insert a batch of keys into the filter.
    ///
    /// @param keys Contiguous sequence of 64-bit keys to insert.
    void insert(std::span<const std::uint64_t> keys) {
        detail::insert(backend_, blocks_, keys);
    }

    /// insert overload for accepting arbitrary hashable ranges.
    /// If the elements are already uint64_t, they take use the original method directly.
    template <std::ranges::input_range R>
        requires StdHashable<std::ranges::range_value_t<R>> &&
                 (!std::same_as<std::ranges::range_value_t<R>, std::uint64_t>)
    void insert(const R& keys) {
        insert(to_uint64(keys));
    }

    /// Merge another filter into this one.
    ///
    /// @param  other BloomFilter to be merged.
    /// @throws std::invalid_argument if the backend or the geometry of the
    ///         bloom filters does not match.
    void merge(const BloomFilter& other) {
        if (backend_ != other.backend_) {
            throw std::invalid_argument("Cannot merge filters with different backends");
        }

        if (size_in_bits() != other.size_in_bits() || num_hashes() != other.num_hashes()) {
            throw std::invalid_argument("Cannot merge filters with different geometry");
        }

        detail::merge(backend_, blocks_, other.blocks_);
    }

    /// The currently used Backend.
    Backend backend() const noexcept {
        return backend_;
    }

    /// Total number of bits allocated for the filter.
    std::size_t size_in_bits() const noexcept {
        return geom_.size_in_bits;
    }

    /// Number of hash functions applied per key.
    std::size_t num_hashes() const noexcept {
        return geom_.k;
    }

    /// Current false-positive rate, computed from the bits actually stored.
    double estimated_fpr() const {
        return detail::estimated_fpr(backend_, blocks_);
    }

  private:
    // Hash each element of keys with std::hash<T> to uint64_t.
    template <std::ranges::input_range R>
    static std::vector<std::uint64_t> to_uint64(const R& keys) {
        using T = std::ranges::range_value_t<R>;
        std::vector<std::uint64_t> out;

        // Pre-allocate the memory for the output vector if R is a sized range
        if constexpr (std::ranges::sized_range<R>)
            out.reserve(std::ranges::size(keys));

        // Construct the hashed elements vector
        for (const auto& key : keys)
            out.push_back(std::hash<T>{}(key));

        return out;
    }

    Backend backend_;
    detail::Geometry geom_;
    std::vector<detail::Block> blocks_;
};

} // namespace blossom
