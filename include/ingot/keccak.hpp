#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace ingot {

// Keccak-256 (Ethereum variant, NOT SHA3-256)
// Rate = 1088 bits (136 bytes), capacity = 512 bits, output = 256 bits
// Uses 0x01 domain padding (original Keccak), not 0x06 (NIST SHA3)
std::array<uint8_t, 32> keccak256(std::span<const uint8_t> input);

// Convenience overload for string data
inline std::array<uint8_t, 32> keccak256(std::string_view input) {
    return keccak256(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(input.data()), input.size()));
}

} // namespace ingot
