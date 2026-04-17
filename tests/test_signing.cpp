#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <ingot/signing.hpp>

// Hardhat/Anvil account #0 — DO NOT use in production.
static constexpr auto TEST_PRIVATE_KEY =
    "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
static constexpr auto EXPECTED_ADDRESS =
    "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266";

TEST_CASE("detail::ensure_ok throws on zero, no-ops on nonzero") {
    CHECK_NOTHROW(ingot::detail::ensure_ok(1, "unused"));
    CHECK_THROWS_AS(ingot::detail::ensure_ok(0, "boom"), std::runtime_error);
}

TEST_CASE("detail::ensure_non_null throws on null, no-ops on valid pointer") {
    int x = 0;
    CHECK_NOTHROW(ingot::detail::ensure_non_null(&x, "unused"));
    CHECK_THROWS_AS(ingot::detail::ensure_non_null(nullptr, "boom"), std::runtime_error);
}

TEST_CASE("keccak256 known vector") {
    auto hash = ingot::keccak256(std::string_view(""));
    ingot::Hash h(std::span<const uint8_t>(hash.data(), 32));
    CHECK(h.to_hex() == "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
}

TEST_CASE("keccak256 hello world") {
    auto hash = ingot::keccak256(std::string_view("hello world"));
    ingot::Hash h(std::span<const uint8_t>(hash.data(), 32));
    CHECK(h.to_hex() == "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");
}

TEST_CASE("keccak256 spans a full rate block boundary") {
    // 136 = rate, 137 crosses into a second block, 300 spans two full blocks.
    std::vector<uint8_t> buf_136(136, 0xab);
    std::vector<uint8_t> buf_137(137, 0xcd);
    std::vector<uint8_t> buf_300(300, 0xef);

    auto h136 = ingot::keccak256(std::span<const uint8_t>(buf_136));
    auto h137 = ingot::keccak256(std::span<const uint8_t>(buf_137));
    auto h300 = ingot::keccak256(std::span<const uint8_t>(buf_300));

    CHECK(h136 != h137);
    CHECK(h137 != h300);
    CHECK(h136 != h300);
}

TEST_CASE("FixedBytes hex roundtrip") {
    ingot::Hash h("0xabcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
    CHECK(h.to_hex() == "0xabcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
}

TEST_CASE("FixedBytes accepts uppercase hex and uppercase 0X prefix") {
    ingot::Hash h("0XABCDEF0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
    CHECK(h.to_hex() == "0xabcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");

    ingot::Address a("0XABCDEFabcdef0011223344556677889900112233");
    CHECK(a.to_hex() == "0xabcdefabcdef0011223344556677889900112233");
}

TEST_CASE("FixedBytes accepts hex without 0x prefix") {
    ingot::Hash h(std::string_view(
        "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789"));
    CHECK(h.to_hex() == "0xabcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
}

TEST_CASE("FixedBytes rejects wrong hex length") {
    CHECK_THROWS_AS(ingot::Hash("0x1234"), std::invalid_argument);
}

TEST_CASE("FixedBytes rejects invalid hex character") {
    // '!', ':', 'G', 'z' each fall into a different hex_val reject branch.
    auto make_hash_hex = [](char bad) {
        std::string s = "0x";
        s.append(64, bad);
        return s;
    };
    for (char bad : {'!', ':', 'G', 'z'}) {
        CHECK_THROWS_AS(ingot::Hash(make_hash_hex(bad)), std::invalid_argument);
    }

    auto make_addr_hex = [](char bad) {
        std::string s = "0x";
        s.append(40, bad);
        return s;
    };
    CHECK_THROWS_AS(ingot::Address(make_addr_hex('z')), std::invalid_argument);
    CHECK_THROWS_AS(ingot::Address(make_addr_hex(':')), std::invalid_argument);
}

TEST_CASE("FixedBytes rejects wrong span size") {
    uint8_t buf[10] = {};
    CHECK_THROWS_AS(
        ingot::Hash(std::span<const uint8_t>(buf, 10)),
        std::invalid_argument);
    CHECK_THROWS_AS(
        ingot::Address(std::span<const uint8_t>(buf, 10)),
        std::invalid_argument);
}

TEST_CASE("Address rejects wrong hex length and accepts no-prefix input") {
    CHECK_THROWS_AS(ingot::Address("0x1234"), std::invalid_argument);
    ingot::Address a("f39fd6e51aad88f6f4ce6ab8827279cfffb92266");
    CHECK(a.to_hex() == "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266");
}

TEST_CASE("FixedBytes size / equality / ordering") {
    ingot::Hash a("0x0000000000000000000000000000000000000000000000000000000000000001");
    ingot::Hash b("0x0000000000000000000000000000000000000000000000000000000000000002");
    ingot::Hash a2 = a;
    CHECK(a.size() == 32);
    CHECK(a == a2);
    CHECK(a != b);
    CHECK(a < b);
}

TEST_CASE("Signature::to_hex formats v with +27 offset") {
    ingot::Signature sig;
    sig.r = ingot::FixedBytes<32>(std::string_view(
        "0x1111111111111111111111111111111111111111111111111111111111111111"));
    sig.s = ingot::FixedBytes<32>(std::string_view(
        "0x2222222222222222222222222222222222222222222222222222222222222222"));
    sig.v = 0;
    CHECK(sig.to_hex() ==
          "0x"
          "1111111111111111111111111111111111111111111111111111111111111111"
          "2222222222222222222222222222222222222222222222222222222222222222"
          "1b");

    sig.v = 1;
    CHECK(sig.to_hex().ends_with("1c"));

    auto bytes = sig.to_bytes();
    CHECK(bytes.size() == 65);
    CHECK(bytes[64] == 28);
}

TEST_CASE("address derivation") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);
    CHECK(signer.address().to_hex() == EXPECTED_ADDRESS);
}

TEST_CASE("SigningKey accepts uppercase 0X prefix") {
    ingot::SigningKey<ingot::Secp256k1> signer(
        "0Xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    CHECK(signer.address().to_hex() == EXPECTED_ADDRESS);
}

TEST_CASE("SigningKey accepts raw hex (no prefix)") {
    ingot::SigningKey<ingot::Secp256k1> signer(
        "ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    CHECK(signer.address().to_hex() == EXPECTED_ADDRESS);
}

TEST_CASE("SigningKey rejects wrong hex length") {
    CHECK_THROWS_AS(
        ingot::SigningKey<ingot::Secp256k1>("0xabcd"),
        std::invalid_argument);
}

TEST_CASE("SigningKey rejects invalid hex character") {
    auto make_key = [](char bad) {
        std::string s = "0x";
        s.append(64, bad);
        return s;
    };
    for (char bad : {'!', ':', 'G', 'z'}) {
        CHECK_THROWS_AS(
            ingot::SigningKey<ingot::Secp256k1>(make_key(bad)),
            std::invalid_argument);
    }
}

TEST_CASE("SigningKey accepts uppercase hex digits in the key body") {
    ingot::SigningKey<ingot::Secp256k1> signer(
        "0xAC0974BEC39A17E36BA4A6B4D238FF944BACB478CBED5EFCAE784D7BF4F2FF80");
    CHECK(signer.address().to_hex() == EXPECTED_ADDRESS);
}

TEST_CASE("SigningKey constructs from raw bytes span") {
    std::array<uint8_t, 32> key_bytes = {
        0xac, 0x09, 0x74, 0xbe, 0xc3, 0x9a, 0x17, 0xe3,
        0x6b, 0xa4, 0xa6, 0xb4, 0xd2, 0x38, 0xff, 0x94,
        0x4b, 0xac, 0xb4, 0x78, 0xcb, 0xed, 0x5e, 0xfc,
        0xae, 0x78, 0x4d, 0x7b, 0xf4, 0xf2, 0xff, 0x80,
    };
    ingot::SigningKey<ingot::Secp256k1> signer(
        std::span<const uint8_t, 32>(key_bytes.data(), 32));
    CHECK(signer.address().to_hex() == EXPECTED_ADDRESS);
}

TEST_CASE("SigningKey span ctor rejects invalid key") {
    std::array<uint8_t, 32> zero_key{};
    CHECK_THROWS_AS(
        ingot::SigningKey<ingot::Secp256k1>(
            std::span<const uint8_t, 32>(zero_key.data(), 32)),
        std::invalid_argument);
}

TEST_CASE("invalid private key rejected") {
    CHECK_THROWS_AS(
        ingot::SigningKey<ingot::Secp256k1>(
            "0x0000000000000000000000000000000000000000000000000000000000000000"),
        std::invalid_argument);
}

TEST_CASE("SigningKey is movable") {
    ingot::SigningKey<ingot::Secp256k1> a(TEST_PRIVATE_KEY);
    auto original_address = a.address();

    ingot::SigningKey<ingot::Secp256k1> b(std::move(a));
    CHECK(b.address() == original_address);

    ingot::SigningKey<ingot::Secp256k1> c(
        "0x1111111111111111111111111111111111111111111111111111111111111111");
    c = std::move(b);
    CHECK(c.address() == original_address);

    auto& c_ref = c;
    c = std::move(c_ref);
    CHECK(c.address() == original_address);
}

TEST_CASE("sign raw hash") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);

    auto hash_bytes = ingot::keccak256(std::string_view("test message"));
    ingot::Hash msg_hash(std::span<const uint8_t>(hash_bytes.data(), 32));
    auto sig = signer.sign(msg_hash);

    auto bytes = sig.to_bytes();
    CHECK(bytes.size() == 65);
    CHECK((bytes[64] == 27 || bytes[64] == 28));
    CHECK(sig.to_hex().size() == 132);
}

TEST_CASE("sign byte span hashes then signs") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);
    std::string_view msg = "span message";
    std::span<const uint8_t> bytes(
        reinterpret_cast<const uint8_t*>(msg.data()), msg.size());

    auto sig_via_span = signer.sign(bytes);

    auto hash = ingot::keccak256(msg);
    ingot::Hash h(std::span<const uint8_t>(hash.data(), 32));
    auto sig_via_hash = signer.sign(h);

    CHECK(sig_via_span.r == sig_via_hash.r);
    CHECK(sig_via_span.s == sig_via_hash.s);
    CHECK(sig_via_span.v == sig_via_hash.v);
}

TEST_CASE("sign string_view hashes then signs") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);
    auto sig_sv = signer.sign(std::string_view("hello"));

    auto hash = ingot::keccak256(std::string_view("hello"));
    ingot::Hash h(std::span<const uint8_t>(hash.data(), 32));
    auto sig_hash = signer.sign(h);

    CHECK(sig_sv.r == sig_hash.r);
    CHECK(sig_sv.s == sig_hash.s);
    CHECK(sig_sv.v == sig_hash.v);
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

TEST_CASE("rlp encode single byte >= 0x80 requires length prefix") {
    std::vector<uint8_t> encoded;
    uint8_t data[] = {0x80};
    ingot::RlpEncoder::encode(encoded, data);
    CHECK(encoded == std::vector<uint8_t>{0x81, 0x80});
}

TEST_CASE("rlp encode short string") {
    std::vector<uint8_t> encoded;
    std::string_view dog = "dog";
    auto data = std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(dog.data()), dog.size());
    ingot::RlpEncoder::encode(encoded, data);
    CHECK(encoded == std::vector<uint8_t>{0x83, 0x64, 0x6f, 0x67});
}

TEST_CASE("rlp encode long string (>55 bytes)") {
    std::vector<uint8_t> payload(56, 0x7a);
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode(encoded, std::span<const uint8_t>(payload));
    REQUIRE(encoded.size() == 58);
    CHECK(encoded[0] == 0xb8);
    CHECK(encoded[1] == 56);
    CHECK(encoded[2] == 0x7a);
    CHECK(encoded.back() == 0x7a);
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
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_uint(encoded, 1024);
    CHECK(encoded == std::vector<uint8_t>{0x82, 0x04, 0x00});
}

TEST_CASE("rlp encode empty list") {
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_list(encoded, std::vector<uint8_t>{});
    CHECK(encoded == std::vector<uint8_t>{0xc0});
}

TEST_CASE("rlp encode long list (>55 bytes payload)") {
    std::vector<uint8_t> payload(56, 0x11);
    std::vector<uint8_t> encoded;
    ingot::RlpEncoder::encode_list(encoded, std::span<const uint8_t>(payload));
    REQUIRE(encoded.size() == 58);
    CHECK(encoded[0] == 0xf8);
    CHECK(encoded[1] == 56);
}

TEST_CASE("rlp encode_bool") {
    std::vector<uint8_t> t, f;
    ingot::RlpEncoder::encode_bool(t, true);
    ingot::RlpEncoder::encode_bool(f, false);
    CHECK(t == std::vector<uint8_t>{0x01});
    CHECK(f == std::vector<uint8_t>{0x80});
}

TEST_CASE("rlp encode_uint256 strips leading zeros and handles zero") {
    std::array<uint8_t, 32> zero{};
    std::vector<uint8_t> enc_zero;
    ingot::RlpEncoder::encode_uint256(enc_zero,
        std::span<const uint8_t, 32>(zero.data(), 32));
    CHECK(enc_zero == std::vector<uint8_t>{0x80});

    std::array<uint8_t, 32> val{};
    val[30] = 0x01;
    val[31] = 0x02;
    std::vector<uint8_t> enc;
    ingot::RlpEncoder::encode_uint256(enc,
        std::span<const uint8_t, 32>(val.data(), 32));
    CHECK(enc == std::vector<uint8_t>{0x82, 0x01, 0x02});
}

TEST_CASE("Transaction::encode_unsigned / signing_hash / encode_signed") {
    ingot::Transaction tx;
    tx.chain_id = 1;
    tx.nonce = 0;
    tx.max_priority_fee_per_gas = 1000000000;
    tx.max_fee_per_gas = 20000000000;
    tx.gas_limit = 21000;
    tx.to = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");

    auto unsigned_rlp = tx.encode_unsigned();
    CHECK(unsigned_rlp.size() > 0);
    CHECK((unsigned_rlp[0] & 0xc0) == 0xc0);

    auto h = tx.signing_hash();
    CHECK(h.size() == 32);

    ingot::Signature fake_sig{};
    fake_sig.r = ingot::FixedBytes<32>(std::string_view(
        "0x0101010101010101010101010101010101010101010101010101010101010101"));
    fake_sig.s = ingot::FixedBytes<32>(std::string_view(
        "0x0202020202020202020202020202020202020202020202020202020202020202"));
    fake_sig.v = 0;
    auto signed_tx = tx.encode_signed(fake_sig);
    REQUIRE(signed_tx.size() > 1);
    CHECK(signed_tx[0] == 0x02);
}

TEST_CASE("Transaction contract creation (to = nullopt)") {
    ingot::Transaction tx;
    tx.chain_id = 1;
    tx.nonce = 0;
    tx.max_priority_fee_per_gas = 1;
    tx.max_fee_per_gas = 1;
    tx.gas_limit = 100000;
    tx.to = std::nullopt;
    tx.data = {0x60, 0x80, 0x60, 0x40, 0x52};

    auto rlp = tx.encode_unsigned();
    bool found_empty_to = false;
    for (auto b : rlp) {
        if (b == 0x80) { found_empty_to = true; break; }
    }
    CHECK(found_empty_to);

    auto h = tx.signing_hash();
    CHECK(h.size() == 32);
}

TEST_CASE("sign_transaction with access list") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);

    ingot::Transaction tx;
    tx.chain_id = 1;
    tx.nonce = 5;
    tx.max_priority_fee_per_gas = 2000000000;
    tx.max_fee_per_gas = 30000000000;
    tx.gas_limit = 60000;
    tx.to = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");

    ingot::AccessListEntry e1;
    e1.address = ingot::Address("0x1111111111111111111111111111111111111111");
    e1.storage_keys.push_back(ingot::Hash(
        "0x0000000000000000000000000000000000000000000000000000000000000001"));
    e1.storage_keys.push_back(ingot::Hash(
        "0x0000000000000000000000000000000000000000000000000000000000000002"));
    tx.access_list.push_back(e1);

    ingot::AccessListEntry e2;
    e2.address = ingot::Address("0x2222222222222222222222222222222222222222");
    tx.access_list.push_back(e2);

    auto raw = signer.sign_transaction(tx);
    CHECK(raw[0] == 0x02);
    CHECK(raw.size() > 100);
}

TEST_CASE("sign_transaction produces valid envelope") {
    ingot::SigningKey<ingot::Secp256k1> signer(TEST_PRIVATE_KEY);

    ingot::Transaction tx;
    tx.chain_id = 1;
    tx.nonce = 0;
    tx.max_priority_fee_per_gas = 1000000000;
    tx.max_fee_per_gas = 20000000000;
    tx.gas_limit = 21000;
    tx.to = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");

    auto raw = signer.sign_transaction(tx);

    CHECK(raw[0] == 0x02);
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
