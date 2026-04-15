#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ingot {

// Fixed-size byte array wrapper
template <std::size_t N>
struct FixedBytes {
    std::array<uint8_t, N> bytes{};

    constexpr FixedBytes() = default;

    explicit FixedBytes(std::span<const uint8_t> data) {
        if (data.size() != N)
            throw std::invalid_argument("invalid byte length");
        std::copy(data.begin(), data.end(), bytes.begin());
    }

    // Construct from hex string (with or without 0x prefix)
    explicit FixedBytes(std::string_view hex) {
        if (hex.starts_with("0x") || hex.starts_with("0X"))
            hex.remove_prefix(2);
        if (hex.size() != N * 2)
            throw std::invalid_argument("invalid hex length");
        for (std::size_t i = 0; i < N; ++i) {
            bytes[i] = static_cast<uint8_t>(
                (hex_val(hex[i * 2]) << 4) | hex_val(hex[i * 2 + 1]));
        }
    }

    constexpr auto data() const noexcept { return bytes.data(); }
    constexpr auto data() noexcept { return bytes.data(); }
    constexpr auto size() const noexcept { return N; }
    constexpr auto begin() const noexcept { return bytes.begin(); }
    constexpr auto end() const noexcept { return bytes.end(); }

    auto operator<=>(const FixedBytes&) const = default;

    [[nodiscard]] std::string to_hex() const {
        static constexpr char hex_chars[] = "0123456789abcdef";
        std::string result;
        result.reserve(2 + N * 2);
        result += "0x";
        for (auto b : bytes) {
            result += hex_chars[b >> 4];
            result += hex_chars[b & 0x0f];
        }
        return result;
    }

private:
    static constexpr uint8_t hex_val(char c) {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        throw std::invalid_argument("invalid hex character");
    }
};

// Ethereum address: 20 bytes
using Address = FixedBytes<20>;

// Keccak-256 hash: 32 bytes
using Hash = FixedBytes<32>;

// ECDSA signature with recovery id
struct Signature {
    FixedBytes<32> r;
    FixedBytes<32> s;
    uint8_t v; // recovery id (0 or 1), add 27 for Ethereum legacy

    [[nodiscard]] std::string to_hex() const {
        std::string result = "0x";
        // Strip "0x" from r and s
        result += r.to_hex().substr(2);
        result += s.to_hex().substr(2);
        static constexpr char hex_chars[] = "0123456789abcdef";
        uint8_t vv = v + 27;
        result += hex_chars[vv >> 4];
        result += hex_chars[vv & 0x0f];
        return result;
    }

    // Compact 65-byte serialization: r(32) || s(32) || v(1)
    [[nodiscard]] std::array<uint8_t, 65> to_bytes() const {
        std::array<uint8_t, 65> out{};
        std::copy(r.begin(), r.end(), out.begin());
        std::copy(s.begin(), s.end(), out.begin() + 32);
        out[64] = v + 27;
        return out;
    }
};

} // namespace ingot
