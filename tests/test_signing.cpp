#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <ingot/signing.hpp>

// Well-known test private key (DO NOT use in production)
// This is the first Hardhat/Anvil test account private key
static constexpr auto TEST_PRIVATE_KEY =
    "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";

// Expected address for the above key (Hardhat account #0)
static constexpr auto EXPECTED_ADDRESS =
    "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266";

TEST_CASE("keccak256 known vector") {
    // keccak256("") = c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
    auto hash = ingot::keccak256(std::string_view(""));
    ingot::Hash h(std::span<const uint8_t>(hash.data(), 32));
    CHECK(h.to_hex() == "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
}

TEST_CASE("keccak256 hello world") {
    auto hash = ingot::keccak256(std::string_view("hello world"));
    ingot::Hash h(std::span<const uint8_t>(hash.data(), 32));
    CHECK(h.to_hex() == "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");
}

TEST_CASE("address derivation") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);
    auto addr = signer.address();
    CHECK(addr.to_hex() == EXPECTED_ADDRESS);
}

TEST_CASE("FixedBytes hex roundtrip") {
    ingot::Hash h("0xabcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
    CHECK(h.to_hex() == "0xabcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
}

TEST_CASE("sign raw hash") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);

    // Sign a known hash
    auto hash_bytes = ingot::keccak256(std::string_view("test message"));
    ingot::Hash msg_hash(std::span<const uint8_t>(hash_bytes.data(), 32));
    auto sig = signer.sign(msg_hash);

    // Signature should be 65 bytes (r=32 + s=32 + v=1)
    auto bytes = sig.to_bytes();
    CHECK(bytes.size() == 65);

    // v should be 27 or 28
    CHECK((bytes[64] == 27 || bytes[64] == 28));

    // Hex representation should be 132 chars (0x + 128 hex + 2 hex for v)
    CHECK(sig.to_hex().size() == 132);
}

TEST_CASE("personal_sign") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);
    auto sig = signer.personal_sign("hello");

    auto bytes = sig.to_bytes();
    CHECK(bytes.size() == 65);
    CHECK((bytes[64] == 27 || bytes[64] == 28));
}

TEST_CASE("deterministic signatures") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);

    auto hash_bytes = ingot::keccak256(std::string_view("determinism"));
    ingot::Hash msg_hash(std::span<const uint8_t>(hash_bytes.data(), 32));

    auto sig1 = signer.sign(msg_hash);
    auto sig2 = signer.sign(msg_hash);

    CHECK(sig1.r == sig2.r);
    CHECK(sig1.s == sig2.s);
    CHECK(sig1.v == sig2.v);
}

TEST_CASE("invalid private key rejected") {
    // All zeros is not a valid secp256k1 private key
    CHECK_THROWS_AS(
        ingot::SigningKey<ingot::Secp256k1>("0x0000000000000000000000000000000000000000000000000000000000000000"),
        std::invalid_argument);
}

// --- RLP encoding tests ---

TEST_CASE("rlp encode empty string") {
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_empty(encoded);
    CHECK(encoded == std::vector<uint8_t>{0x80});
}

TEST_CASE("rlp encode single byte") {
    std::vector<uint8_t> encoded;
    uint8_t data[] = {0x42};
    ingot::RlpEncoder::encode(encoded, data);
    CHECK(encoded == std::vector<uint8_t>{0x42});
}

TEST_CASE("rlp encode short string") {
    // "dog" = 0x83, 'd', 'o', 'g'
    std::vector<uint8_t> encoded;
    std::string_view dog = "dog";
    auto data = std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(dog.data()), dog.size());
    ingot::RlpEncoder::encode(encoded, data);
    CHECK(encoded == std::vector<uint8_t>{0x83, 0x64, 0x6f, 0x67});
}

TEST_CASE("rlp encode uint zero") {
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_uint(encoded, 0);
    CHECK(encoded == std::vector<uint8_t>{0x80});
}

TEST_CASE("rlp encode uint small") {
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_uint(encoded, 127);
    CHECK(encoded == std::vector<uint8_t>{0x7f});
}

TEST_CASE("rlp encode uint 1024") {
    // 1024 = 0x0400, big-endian = [0x04, 0x00]
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_uint(encoded, 1024);
    CHECK(encoded == std::vector<uint8_t>{0x82, 0x04, 0x00});
}

TEST_CASE("rlp encode empty list") {
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_list(encoded, std::vector<uint8_t>{});
    CHECK(encoded == std::vector<uint8_t>{0xc0});
}

// --- Transaction signing tests ---

TEST_CASE("sign_transaction produces valid envelope") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);

    ingot::Transaction tx;
    tx.chain_id = 1;
    tx.nonce = 0;
    tx.max_priority_fee_per_gas = 1000000000;  // 1 gwei
    tx.max_fee_per_gas = 20000000000;           // 20 gwei
    tx.gas_limit = 21000;
    tx.to = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");

    auto raw = signer.sign_transaction(tx);

    // Must start with type byte 0x02
    CHECK(raw[0] == 0x02);
    // Must be longer than just the type byte
    CHECK(raw.size() > 100);
}

TEST_CASE("sign_transaction is deterministic") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);

    ingot::Transaction tx;
    tx.chain_id = 1;
    tx.nonce = 7;
    tx.max_priority_fee_per_gas = 1000000000;
    tx.max_fee_per_gas = 20000000000;
    tx.gas_limit = 21000;
    tx.to = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");

    auto raw1 = signer.sign_transaction(tx);
    auto raw2 = signer.sign_transaction(tx);
    CHECK(raw1 == raw2);
}
