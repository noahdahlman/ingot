#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace ingot {

// RLP (Recursive Length Prefix) encoder for Ethereum serialization.
// All methods append to an output vector — no intermediate allocations.
// Reference: https://ethereum.org/developers/docs/data-structures-and-encoding/rlp
class RlpEncoder {
public:
    // Encode a byte string
    static void encode(std::vector<uint8_t>& out, std::span<const uint8_t> data) {
        if (data.size() == 1 && data[0] < 0x80) {
            out.push_back(data[0]);
        } else if (data.size() <= 55) {
            out.push_back(static_cast<uint8_t>(0x80 + data.size()));
            out.insert(out.end(), data.begin(), data.end());
        } else {
            uint8_t len_buf[8];
            auto n = write_big_endian(len_buf, data.size());
            out.push_back(static_cast<uint8_t>(0xb7 + n));
            out.insert(out.end(), len_buf, len_buf + n);
            out.insert(out.end(), data.begin(), data.end());
        }
    }

    // Encode an empty string (used for zero values and empty fields)
    static void encode_empty(std::vector<uint8_t>& out) {
        out.push_back(0x80);
    }

    // Encode a uint64 as big-endian bytes with no leading zeros
    static void encode_uint(std::vector<uint8_t>& out, uint64_t value) {
        if (value == 0) { encode_empty(out); return; }
        uint8_t buf[8];
        auto n = write_big_endian(buf, value);
        encode(out, std::span<const uint8_t>(buf, n));
    }

    // Encode a uint256 (32-byte big-endian) stripping leading zeros
    static void encode_uint256(std::vector<uint8_t>& out,
                               std::span<const uint8_t, 32> value) {
        std::size_t start = 0;
        while (start < 32 && value[start] == 0) ++start;
        if (start == 32) { encode_empty(out); return; }
        encode(out, std::span<const uint8_t>(value.data() + start, 32 - start));
    }

    // Encode a boolean (0 or 1)
    static void encode_bool(std::vector<uint8_t>& out, bool value) {
        if (value) out.push_back(0x01);
        else encode_empty(out);
    }

    // Wrap already-encoded items as an RLP list
    static void encode_list(std::vector<uint8_t>& out,
                            std::span<const uint8_t> payload) {
        if (payload.size() <= 55) {
            out.push_back(static_cast<uint8_t>(0xc0 + payload.size()));
        } else {
            uint8_t len_buf[8];
            auto n = write_big_endian(len_buf, payload.size());
            out.push_back(static_cast<uint8_t>(0xf7 + n));
            out.insert(out.end(), len_buf, len_buf + n);
        }
        out.insert(out.end(), payload.begin(), payload.end());
    }

private:
    // Write uint64 as big-endian into buf, return byte count.
    // Caller must provide buf[8]. Bytes are written starting at buf[0].
    static std::size_t write_big_endian(uint8_t buf[8], uint64_t value) {
        std::size_t n = 0;
        for (auto tmp = value; tmp > 0; tmp >>= 8) ++n;
        for (std::size_t i = n; i-- > 0; value >>= 8)
            buf[i] = static_cast<uint8_t>(value & 0xff);
        return n;
    }
};

} // namespace ingot
