// Outputs signing results as pipe-delimited lines for cross-validation with viem.
// Format: name|address|signature_hex

#include <ingot/signing.hpp>
#include <iostream>
#include <string_view>
#include <utility>

static constexpr auto PRIVATE_KEY =
    "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";

int main() {
    ingot::SigningKey<ingot::Secp256k1> signer(PRIVATE_KEY);
    auto address = signer.address().to_hex();

    const std::pair<std::string_view, std::string_view> vectors[] = {
        {"empty", ""},
        {"hello", "hello"},
        {"hello_world", "hello world"},
        {"long_message",
         "The quick brown fox jumps over the lazy dog and then does it again"},
        {"special_chars", "!@#$%^&*()_+-=[]{}|;':\",./<>?"},
        {"unicode", "ethereum \xe2\x89\x88 \xc3\xa9th\xc3\xa9reum"},
    };

    for (const auto& [name, message] : vectors) {
        auto sig = signer.personal_sign(message);
        std::cout << name << "|" << address << "|" << sig.to_hex() << "\n";
    }

    // Transaction signing test vectors
    // Simple ETH transfer: 0 value, nonce 0, chain 1
    {
        ingot::Transaction tx;
        tx.chain_id = 1;
        tx.nonce = 0;
        tx.max_priority_fee_per_gas = 1000000000;  // 1 gwei
        tx.max_fee_per_gas = 20000000000;           // 20 gwei
        tx.gas_limit = 21000;
        tx.to = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");

        auto raw = signer.sign_transaction(tx);
        std::cout << "tx_simple|";
        for (auto b : raw) {
            static constexpr char hex[] = "0123456789abcdef";
            std::cout << hex[b >> 4] << hex[b & 0xf];
        }
        std::cout << "\n";
    }

    // Transfer with value and nonce
    {
        ingot::Transaction tx;
        tx.chain_id = 1;
        tx.nonce = 7;
        tx.max_priority_fee_per_gas = 2000000000;   // 2 gwei
        tx.max_fee_per_gas = 50000000000;            // 50 gwei
        tx.gas_limit = 21000;
        tx.to = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");
        // 1 ETH = 1e18 wei = 0x0de0b6b3a7640000
        uint8_t one_eth[32] = {};
        one_eth[24] = 0x0d; one_eth[25] = 0xe0; one_eth[26] = 0xb6;
        one_eth[27] = 0xb3; one_eth[28] = 0xa7; one_eth[29] = 0x64;
        one_eth[30] = 0x00; one_eth[31] = 0x00;
        tx.value = ingot::FixedBytes<32>(std::span<const uint8_t>(one_eth, 32));

        auto raw = signer.sign_transaction(tx);
        std::cout << "tx_with_value|";
        for (auto b : raw) {
            static constexpr char hex[] = "0123456789abcdef";
            std::cout << hex[b >> 4] << hex[b & 0xf];
        }
        std::cout << "\n";
    }

    return 0;
}
