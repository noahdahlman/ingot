#pragma once

#include "ingot/keccak.hpp"
#include "ingot/secure_zero.hpp"
#include "ingot/types.hpp"

#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ingot {

struct Secp256k1 {};

template <typename Curve>
class SigningKey;

template <>
class SigningKey<Secp256k1> {
public:
    // [C2 fix] Parse hex directly into secret_ — no intermediate Hash temp
    explicit SigningKey(std::string_view hex) {
        if (hex.starts_with("0x") || hex.starts_with("0X"))
            hex.remove_prefix(2);
        if (hex.size() != 64)
            throw std::invalid_argument("private key must be 32 bytes hex");
        for (std::size_t i = 0; i < 32; ++i) {
            secret_[i] = static_cast<uint8_t>(
                (hex_val(hex[i * 2]) << 4) | hex_val(hex[i * 2 + 1]));
        }
        // [H2 fix] If init() throws, zero secret_ since destructor won't run
        try {
            init();
        } catch (...) {
            secure_zero(secret_.data(), 32);
            throw;
        }
    }

    explicit SigningKey(std::span<const uint8_t, 32> private_key) {
        std::memcpy(secret_.data(), private_key.data(), 32);
        try {
            init();
        } catch (...) {
            secure_zero(secret_.data(), 32);
            throw;
        }
    }

    ~SigningKey() {
        // [H1 fix] Use explicit_bzero — guaranteed not optimized away
        secure_zero(secret_.data(), 32);
    }

    SigningKey(const SigningKey&) = delete;
    SigningKey& operator=(const SigningKey&) = delete;

    // [C1 fix] Explicit move that zeros the source key
    SigningKey(SigningKey&& other) noexcept
        : secret_(other.secret_), ctx_(std::move(other.ctx_)) {
        secure_zero(other.secret_.data(), 32);
    }

    SigningKey& operator=(SigningKey&& other) noexcept {
        if (this != &other) {
            secure_zero(secret_.data(), 32);
            secret_ = other.secret_;
            ctx_ = std::move(other.ctx_);
            secure_zero(other.secret_.data(), 32);
        }
        return *this;
    }

    // Sign a raw 32-byte message hash
    [[nodiscard]] Signature sign(const Hash& msg_hash) const {
        secp256k1_ecdsa_recoverable_signature raw_sig;
        if (!secp256k1_ecdsa_sign_recoverable(
                ctx_.get(), &raw_sig, msg_hash.data(), secret_.data(),
                nullptr, nullptr)) {
            throw std::runtime_error("secp256k1 signing failed");
        }

        uint8_t out[64];
        int recid;
        secp256k1_ecdsa_recoverable_signature_serialize_compact(
            ctx_.get(), out, &recid, &raw_sig);

        Signature sig;
        sig.r = FixedBytes<32>(std::span<const uint8_t>(out, 32));
        sig.s = FixedBytes<32>(std::span<const uint8_t>(out + 32, 32));
        sig.v = static_cast<uint8_t>(recid);

        // [H3 fix] Zero nonce-bearing intermediates
        secure_zero(&raw_sig, sizeof(raw_sig));
        secure_zero(out, sizeof(out));

        return sig;
    }

    // Sign arbitrary message bytes (hashes with keccak256 first)
    [[nodiscard]] Signature sign(std::span<const uint8_t> message) const {
        auto hash_bytes = keccak256(message);
        Hash h(std::span<const uint8_t>(hash_bytes.data(), 32));
        return sign(h);
    }

    // Sign a string message (hashes with keccak256 first)
    [[nodiscard]] Signature sign(std::string_view message) const {
        auto hash_bytes = keccak256(message);
        Hash h(std::span<const uint8_t>(hash_bytes.data(), 32));
        return sign(h);
    }

    // EIP-191 personal_sign: "\x19Ethereum Signed Message:\n{len}{msg}"
    [[nodiscard]] Signature personal_sign(std::string_view message) const {
        std::string prefix = "\x19"
                             "Ethereum Signed Message:\n"
                           + std::to_string(message.size());
        std::string prefixed = prefix + std::string(message);
        auto hash_bytes = keccak256(std::string_view(prefixed));
        Hash h(std::span<const uint8_t>(hash_bytes.data(), 32));
        return sign(h);
    }

    // Derive the Ethereum address from this key's public key
    [[nodiscard]] Address address() const {
        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_create(ctx_.get(), &pubkey, secret_.data()))
            throw std::runtime_error("failed to derive public key");

        uint8_t serialized[65];
        std::size_t len = 65;
        secp256k1_ec_pubkey_serialize(
            ctx_.get(), serialized, &len, &pubkey,
            SECP256K1_EC_UNCOMPRESSED);

        // Ethereum address = last 20 bytes of keccak256(pubkey[1..65])
        auto hash = keccak256(std::span<const uint8_t>(serialized + 1, 64));
        return Address(std::span<const uint8_t>(hash.data() + 12, 20));
    }

private:
    struct CtxDeleter {
        void operator()(secp256k1_context* c) const {
            secp256k1_context_destroy(c);
        }
    };

    std::array<uint8_t, 32> secret_{};
    std::unique_ptr<secp256k1_context, CtxDeleter> ctx_;

    // [M3 fix] Null-check context creation
    void init() {
        auto* raw = secp256k1_context_create(
            SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        if (!raw)
            throw std::runtime_error("failed to create secp256k1 context");
        ctx_.reset(raw);
        if (!secp256k1_ec_seckey_verify(ctx_.get(), secret_.data()))
            throw std::invalid_argument("invalid private key");
    }

    static constexpr uint8_t hex_val(char c) {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        throw std::invalid_argument("invalid hex character");
    }
};

} // namespace ingot
