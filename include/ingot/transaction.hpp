#pragma once

#include "ingot/keccak.hpp"
#include "ingot/rlp.hpp"
#include "ingot/types.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ingot {

// EIP-2930 access list entry: [address, [storage_keys]]
struct AccessListEntry {
    Address address;
    std::vector<Hash> storage_keys;
};

// EIP-1559 (type 2) transaction
struct Transaction {
    uint64_t chain_id = 1;
    uint64_t nonce = 0;
    uint64_t max_priority_fee_per_gas = 0;
    uint64_t max_fee_per_gas = 0;
    uint64_t gas_limit = 21000;
    std::optional<Address> to;  // nullopt for contract creation
    FixedBytes<32> value;       // wei amount (uint256, big-endian)
    std::vector<uint8_t> data;
    std::vector<AccessListEntry> access_list;

    // Encode the 9 unsigned transaction fields into out.
    // Shared by signing_hash(), encode_unsigned(), and sign_transaction().
    void encode_fields(std::vector<uint8_t>& out) const {
        RlpEncoder::encode_uint(out, chain_id);
        RlpEncoder::encode_uint(out, nonce);
        RlpEncoder::encode_uint(out, max_priority_fee_per_gas);
        RlpEncoder::encode_uint(out, max_fee_per_gas);
        RlpEncoder::encode_uint(out, gas_limit);

        if (to) {
            RlpEncoder::encode(out,
                std::span<const uint8_t>(to->data(), 20));
        } else {
            RlpEncoder::encode_empty(out);
        }

        RlpEncoder::encode_uint256(out,
            std::span<const uint8_t, 32>(value.data(), 32));
        RlpEncoder::encode(out, data);
        encode_access_list(out);
    }

    // RLP-encode the unsigned transaction payload.
    // Fields: [chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
    //          gas_limit, to, value, data, access_list]
    [[nodiscard]] std::vector<uint8_t> encode_unsigned() const {
        std::vector<uint8_t> fields;
        encode_fields(fields);

        std::vector<uint8_t> result;
        RlpEncoder::encode_list(result, fields);
        return result;
    }

    // The signing hash: keccak256(0x02 || rlp([unsigned fields]))
    [[nodiscard]] Hash signing_hash() const {
        auto rlp = encode_unsigned();

        std::vector<uint8_t> preimage;
        preimage.reserve(1 + rlp.size());
        preimage.push_back(0x02);
        preimage.insert(preimage.end(), rlp.begin(), rlp.end());

        auto hash_bytes = keccak256(std::span<const uint8_t>(preimage));
        return Hash(std::span<const uint8_t>(hash_bytes.data(), 32));
    }

    // Encode the signed transaction envelope:
    // 0x02 || rlp([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
    //              gas_limit, to, value, data, access_list,
    //              signature_y_parity, signature_r, signature_s])
    [[nodiscard]] std::vector<uint8_t> encode_signed(const Signature& sig) const {
        std::vector<uint8_t> fields;
        encode_fields(fields);

        RlpEncoder::encode_bool(fields, sig.v != 0);
        RlpEncoder::encode_uint256(fields,
            std::span<const uint8_t, 32>(sig.r.data(), 32));
        RlpEncoder::encode_uint256(fields,
            std::span<const uint8_t, 32>(sig.s.data(), 32));

        std::vector<uint8_t> result;
        result.push_back(0x02);
        RlpEncoder::encode_list(result, fields);
        return result;
    }

private:
    void encode_access_list(std::vector<uint8_t>& out) const {
        std::vector<uint8_t> list_payload;
        for (const auto& entry : access_list) {
            std::vector<uint8_t> entry_payload;
            RlpEncoder::encode(entry_payload,
                std::span<const uint8_t>(entry.address.data(), 20));

            std::vector<uint8_t> keys_payload;
            for (const auto& key : entry.storage_keys) {
                RlpEncoder::encode(keys_payload,
                    std::span<const uint8_t>(key.data(), 32));
            }
            RlpEncoder::encode_list(entry_payload, keys_payload);
            RlpEncoder::encode_list(list_payload, entry_payload);
        }
        RlpEncoder::encode_list(out, list_payload);
    }
};

} // namespace ingot
