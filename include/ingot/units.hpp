#pragma once

#include "ingot/types.hpp"

#include <intx/intx.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>

namespace ingot {

// Ethereum amount in wei. Runtime-valued wrapper around intx::uint256 — full
// 2^256 range. Implicitly converts to FixedBytes<32> and to uint64_t (throws
// std::overflow_error if high bits are set).
struct Wei {
    intx::uint256 raw = 0;

    constexpr Wei() = default;
    constexpr explicit Wei(intx::uint256 v) : raw(v) {}

    operator FixedBytes<32>() const {
        FixedBytes<32> r;
        intx::be::store(r.bytes, raw);
        return r;
    }

    constexpr operator uint64_t() const {
        if (raw > std::numeric_limits<uint64_t>::max())
            throw std::overflow_error("wei amount exceeds uint64_t");
        return static_cast<uint64_t>(raw);
    }
};

namespace detail {

// std::array<uint64_t, 4> is a structural type, so it's usable as a non-type
// template parameter. intx::uint256 is not (private members). We round-trip
// the value through word storage to keep it in the type system.
constexpr std::array<uint64_t, 4> to_words(const intx::uint256& v) {
    return {v[0], v[1], v[2], v[3]};
}

constexpr intx::uint256 from_words(const std::array<uint64_t, 4>& w) {
    intx::uint256 r;
    r[0] = w[0]; r[1] = w[1]; r[2] = w[2]; r[3] = w[3];
    return r;
}

constexpr intx::uint256 parse_decimal(const char* s) {
    intx::uint256 r = 0;
    for (; *s; ++s) {
        if (*s == '\'') continue;
        r = r * 10 + static_cast<unsigned>(*s - '0');
    }
    return r;
}

} // namespace detail

// Compile-time-typed wei value. The amount lives in the type, so narrowing
// conversions can static_assert on overflow. Produced by the _wei/_gwei/_ether
// literals; convertible to Wei, FixedBytes<32>, or uint64_t at use sites.
template <std::array<uint64_t, 4> Words>
struct CompileTimeWei {
    static constexpr intx::uint256 value = detail::from_words(Words);

    constexpr operator Wei() const { return Wei{value}; }

    operator FixedBytes<32>() const {
        FixedBytes<32> r;
        intx::be::store(r.bytes, value);
        return r;
    }

    consteval operator uint64_t() const {
        static_assert(Words[1] == 0 && Words[2] == 0 && Words[3] == 0,
            "wei amount overflows uint64_t");
        return Words[0];
    }
};

namespace literals {

template <char... Cs>
consteval auto operator""_wei() {
    constexpr char buf[] = {Cs..., '\0'};
    constexpr auto v = detail::parse_decimal(buf);
    return CompileTimeWei<detail::to_words(v)>{};
}

template <char... Cs>
consteval auto operator""_gwei() {
    constexpr char buf[] = {Cs..., '\0'};
    constexpr auto v = detail::parse_decimal(buf) * 1'000'000'000U;
    return CompileTimeWei<detail::to_words(v)>{};
}

template <char... Cs>
consteval auto operator""_ether() {
    constexpr char buf[] = {Cs..., '\0'};
    constexpr auto v = detail::parse_decimal(buf) *
        intx::uint256{1'000'000'000'000'000'000ULL};
    return CompileTimeWei<detail::to_words(v)>{};
}

template <char... Cs>
consteval auto operator""_eth() {
    return operator""_ether<Cs...>();
}

} // namespace literals
} // namespace ingot
