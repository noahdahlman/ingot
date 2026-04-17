#pragma once

#include "ingot/keccak.hpp"
#include "ingot/secure_zero.hpp"
#include "ingot/transaction.hpp"
#include "ingot/types.hpp"

#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ingot {

namespace detail {

[[noreturn]] inline void throw_secp256k1(const char* what) {
    throw std::runtime_error(what);
}

inline void ensure_ok(int result, const char* what) {
    if (!result) throw_secp256k1(what);
}

inline void ensure_non_null(const void* p, const char* what) {
    if (!p) throw_secp256k1(what);
}

} // namespace detail

struct Secp256k1 {};

template <typename Curve>
class SigningKey;

template <>
class SigningKey<Secp256k1> {
public:
    explicit SigningKey(std::string_view hex) {
        if (hex.starts_with("0x") || hex.starts_with("0X"))
            hex.remove_prefix(2);
        if (hex.size() != 64)
            throw std::invalid_argument("private key must be 32 bytes hex");
        for (std::size_t i = 0; i < 32; ++i) {
            secret_[i] = static_cast<uint8_t>(
                (hex_val(hex[i * 2]) << 4) | hex_val(hex[i * 2 + 1]));
        }
        try {
            verify_key();
        } catch (...) {
            secure_zero(secret_.data(), 32);
            throw;
        }
    }

    explicit SigningKey(std::span<const uint8_t, 32> private_key) {
        std::memcpy(secret_.data(), private_key.data(), 32);
        try {
            verify_key();
        } catch (...) {
            secure_zero(secret_.data(), 32);
            throw;
        }
    }

    ~SigningKey() {
        secure_zero(secret_.data(), 32);
    }

    SigningKey(const SigningKey&) = delete;
    SigningKey& operator=(const SigningKey&) = delete;

    SigningKey(SigningKey&& other) noexcept : secret_(other.secret_) {
        secure_zero(other.secret_.data(), 32);
    }

    SigningKey& operator=(SigningKey&& other) noexcept {
        if (this != &other) {
            secure_zero(secret_.data(), 32);
            secret_ = other.secret_;
            secure_zero(other.secret_.data(), 32);
        }
        return *this;
    }

    // Sign a raw 32-byte message hash
    [[nodiscard]] Signature sign(const Hash& msg_hash) const {
        secp256k1_ecdsa_recoverable_signature raw_sig;
        detail::ensure_ok(
            secp256k1_ecdsa_sign_recoverable(
                ctx(), &raw_sig, msg_hash.data(), secret_.data(),
                nullptr, nullptr),
            "secp256k1 signing failed");

        uint8_t out[64];
        int recid;
        secp256k1_ecdsa_recoverable_signature_serialize_compact(
            ctx(), out, &recid, &raw_sig);

        Signature sig;
        sig.r = FixedBytes<32>(std::span<const uint8_t>(out, 32));
        sig.s = FixedBytes<32>(std::span<const uint8_t>(out + 32, 32));
        sig.v = static_cast<uint8_t>(recid);

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

    // Sign an EIP-1559 transaction. Encodes fields once for both
    // the signing hash and the final signed envelope.
    [[nodiscard]] std::vector<uint8_t> sign_transaction(const Transaction& tx) const {
        // Encode the 9 unsigned fields once
        std::vector<uint8_t> fields;
        tx.encode_fields(fields);

        // Signing hash: keccak256(0x02 || rlp_list(fields))
        std::vector<uint8_t> preimage;
        preimage.push_back(0x02);
        RlpEncoder::encode_list(preimage, fields);
        auto hash_bytes = keccak256(std::span<const uint8_t>(preimage));
        Hash hash(std::span<const uint8_t>(hash_bytes.data(), 32));

        auto sig = sign(hash);

        // Append signature to the same fields buffer
        RlpEncoder::encode_bool(fields, sig.v != 0);
        RlpEncoder::encode_uint256(fields,
            std::span<const uint8_t, 32>(sig.r.data(), 32));
        RlpEncoder::encode_uint256(fields,
            std::span<const uint8_t, 32>(sig.s.data(), 32));

        // Final envelope: 0x02 || rlp_list(all_fields)
        std::vector<uint8_t> result;
        result.push_back(0x02);
        RlpEncoder::encode_list(result, fields);
        return result;
    }

    // Derive the Ethereum address from this key's public key
    [[nodiscard]] Address address() const {
        secp256k1_pubkey pubkey;
        detail::ensure_ok(
            secp256k1_ec_pubkey_create(ctx(), &pubkey, secret_.data()),
            "failed to derive public key");

        uint8_t serialized[65];
        std::size_t len = 65;
        secp256k1_ec_pubkey_serialize(
            ctx(), serialized, &len, &pubkey,
            SECP256K1_EC_UNCOMPRESSED);

        auto hash = keccak256(std::span<const uint8_t>(serialized + 1, 64));
        return Address(std::span<const uint8_t>(hash.data() + 12, 20));
    }

private:
    std::array<uint8_t, 32> secret_{};

    // Shared secp256k1 context — created once, thread-safe for signing.
    // Sign-only: no verify precomputation tables (~130KB saving).
    static secp256k1_context* ctx() {
        static secp256k1_context* c = [] {
            auto* p = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
            detail::ensure_non_null(p, "failed to create secp256k1 context");
            return p;
        }();
        return c;
    }

    void verify_key() {
        if (!secp256k1_ec_seckey_verify(ctx(), secret_.data()))
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
