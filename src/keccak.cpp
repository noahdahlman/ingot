// Keccak-256 implementation (Ethereum variant)
// Based on the Keccak reference specification.
// Uses 0x01 padding (original Keccak), NOT 0x06 (NIST SHA3-256).
//
// The chained rho+pi traversal and constant tables are derived from tiny_sha3
// by Markku-Juhani O. Saarinen (CC0/MIT): https://github.com/mjosaarinen/tiny_sha3

#include "ingot/keccak.hpp"
#include <cstring>

namespace ingot {
namespace {

constexpr int ROUNDS = 24;
constexpr std::size_t RATE = 136; // 1088 bits / 8

constexpr uint64_t RC[ROUNDS] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808AULL,
    0x8000000080008000ULL, 0x000000000000808BULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008AULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000AULL,
    0x000000008000808BULL, 0x800000000000008BULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800AULL, 0x800000008000000AULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL,
};

// Chained rho+pi traversal: rotation amounts and destination lanes
// for the 24-step cycle starting from lane 1 (from tiny_sha3).
constexpr int ROTC[24] = {
     1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
    27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44,
};

constexpr int PILN[24] = {
    10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4,
    15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1,
};

// Uses (64 - n) rather than (-n & 63) so clang fully unrolls the rho+pi loop
// into straight-line rolq instructions. Safe here because n is always in [1, 62].
inline constexpr uint64_t rotl(uint64_t x, int n) {
    return (x << n) | (x >> (64 - n));
}

void keccak_f1600(uint64_t state[25]) {
    for (int round = 0; round < ROUNDS; ++round) {
        // Theta
        uint64_t C[5], t;
        for (int x = 0; x < 5; ++x)
            C[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
        for (int x = 0; x < 5; ++x) {
            t = C[(x + 4) % 5] ^ rotl(C[(x + 1) % 5], 1);
            for (int y = 0; y < 25; y += 5)
                state[y + x] ^= t;
        }

        // Rho + Pi (chained single-temp traversal from tiny_sha3)
        t = state[1];
        for (int i = 0; i < 24; ++i) {
            int j = PILN[i];
            C[0] = state[j];
            state[j] = rotl(t, ROTC[i]);
            t = C[0];
        }

        // Chi
        for (int y = 0; y < 25; y += 5) {
            for (int x = 0; x < 5; ++x) C[x] = state[y + x];
            for (int x = 0; x < 5; ++x)
                state[y + x] ^= (~C[(x + 1) % 5]) & C[(x + 2) % 5];
        }

        // Iota
        state[0] ^= RC[round];
    }
}

} // namespace

std::array<uint8_t, 32> keccak256(std::span<const uint8_t> input) {
    uint64_t state[25] = {};
    const auto* data = input.data();
    auto remaining = input.size();

    // Absorb full blocks
    while (remaining >= RATE) {
        for (std::size_t i = 0; i < RATE / 8; ++i) {
            uint64_t lane;
            std::memcpy(&lane, data + i * 8, 8);
            state[i] ^= lane;
        }
        keccak_f1600(state);
        data += RATE;
        remaining -= RATE;
    }

    // Absorb final partial block with padding
    uint8_t block[RATE] = {};
    std::memcpy(block, data, remaining);
    block[remaining] = 0x01;      // Keccak domain (NOT SHA3's 0x06)
    block[RATE - 1] |= 0x80;     // Final bit of multi-rate padding

    for (std::size_t i = 0; i < RATE / 8; ++i) {
        uint64_t lane;
        std::memcpy(&lane, block + i * 8, 8);
        state[i] ^= lane;
    }
    keccak_f1600(state);

    // Squeeze output
    std::array<uint8_t, 32> hash;
    std::memcpy(hash.data(), state, 32);
    return hash;
}

} // namespace ingot
