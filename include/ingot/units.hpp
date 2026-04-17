#pragma once

#include "ingot/types.hpp"

#include <cstdint>
#include <stdexcept>

// __int128 is a GCC/clang extension used here to represent wei amounts up to
// ~1.7e38 without pulling in a full uint256 library. Silence the pedantic
// warning for this header only.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

namespace ingot {

// Ethereum amount in wei. Stored as unsigned __int128 so values up to
// ~1.7e38 wei are representable — well beyond any real-world amount.
// Implicitly convertible to FixedBytes<32> (Transaction::value) and to
// uint64_t (gas price fields). The uint64_t conversion throws if the high
// bits are set, which makes over-large literal assignments ill-formed.
struct Wei {
    unsigned __int128 raw = 0;

    constexpr Wei() = default;
    constexpr explicit Wei(unsigned __int128 v) : raw(v) {}

    constexpr operator FixedBytes<32>() const {
        FixedBytes<32> r;
        for (int i = 0; i < 16; ++i)
            r.bytes[31 - i] = static_cast<uint8_t>(raw >> (i * 8));
        return r;
    }

    constexpr operator uint64_t() const {
        if (raw >> 64)
            throw std::overflow_error("wei amount exceeds uint64_t");
        return static_cast<uint64_t>(raw);
    }
};

namespace literals {

constexpr Wei operator""_wei(unsigned long long v) {
    return Wei{v};
}

constexpr Wei operator""_gwei(unsigned long long v) {
    return Wei{static_cast<unsigned __int128>(v) * 1'000'000'000ULL};
}

constexpr Wei operator""_ether(unsigned long long v) {
    return Wei{static_cast<unsigned __int128>(v) * 1'000'000'000'000'000'000ULL};
}

constexpr Wei operator""_eth(unsigned long long v) {
    return operator""_ether(v);
}

} // namespace literals
} // namespace ingot

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
