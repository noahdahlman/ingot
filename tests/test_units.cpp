#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <ingot/signing.hpp>
#include <ingot/units.hpp>

using namespace ingot::literals;

TEST_CASE("wei literal") {
    ingot::Wei w = 42_wei;
    uint64_t as_u64 = w;
    CHECK(as_u64 == 42);
}

TEST_CASE("gwei literal") {
    uint64_t fee = 20_gwei;
    CHECK(fee == 20'000'000'000ULL);
}

TEST_CASE("ether literal") {
    uint64_t one_ether = 1_ether;
    CHECK(one_ether == 1'000'000'000'000'000'000ULL);

    ingot::FixedBytes<32> value = 3_ether;
    CHECK(value.to_hex() ==
          "0x00000000000000000000000000000000000000000000000029a2241af62c0000");
}

TEST_CASE("_eth is an alias for _ether") {
    CHECK(static_cast<uint64_t>(5_eth) == static_cast<uint64_t>(5_ether));
}

TEST_CASE("literal converts to FixedBytes<32> big-endian") {
    ingot::FixedBytes<32> v = 1_ether;
    // 10^18 = 0x0DE0B6B3A7640000
    CHECK(v.to_hex() ==
          "0x0000000000000000000000000000000000000000000000000de0b6b3a7640000");
}

TEST_CASE("literal overflowing uint64 throws on narrow conversion") {
    ingot::Wei big = 100_ether;
    CHECK_THROWS_AS(static_cast<uint64_t>(big), std::overflow_error);
    // Still fits in FixedBytes<32>.
    ingot::FixedBytes<32> fb = big;
    CHECK(fb.to_hex() ==
          "0x0000000000000000000000000000000000000000000000056bc75e2d63100000");
}

TEST_CASE("literals work in Transaction field assignment") {
    ingot::Transaction tx;
    tx.max_priority_fee_per_gas = 1_gwei;
    tx.max_fee_per_gas = 20_gwei;
    tx.value = 1_ether;

    CHECK(tx.max_priority_fee_per_gas == 1'000'000'000ULL);
    CHECK(tx.max_fee_per_gas == 20'000'000'000ULL);
    CHECK(tx.value.to_hex() ==
          "0x0000000000000000000000000000000000000000000000000de0b6b3a7640000");
}
