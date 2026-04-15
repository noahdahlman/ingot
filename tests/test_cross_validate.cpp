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

    return 0;
}
