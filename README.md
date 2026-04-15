# ingot

[![Docs](https://img.shields.io/badge/docs-ingot.dahlman.dev-7c3aed)](https://ingot.dahlman.dev)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

Minimal Ethereum signing library for C++20.

```cpp
#include <ingot/signing.hpp>

ingot::SigningKey<ingot::Secp256k1> key("0xac09...");

auto addr = key.address();                    // Address derivation
auto sig  = key.personal_sign("hello");       // EIP-191 personal_sign
auto raw  = key.sign_transaction(tx);         // EIP-1559 transaction signing
```

## Features

- **Simple API** -- `SigningKey`, `Address`, `Signature`, `Transaction`
- **EIP-191** `personal_sign`
- **EIP-1559** transaction signing (type 2 envelopes)
- **RLP encoding** for Ethereum serialization
- **Defensive** -- secret key zeroing on destruction, move, and constructor throw; nonce cleanup; `explicit_bzero`
- **Fast** -- keccak-f1600 fully unrolls to straight-line `rolq` instructions
- **Cross-validated** against [viem](https://viem.sh/) and [alloy](https://github.com/alloy-rs/alloy)

## Quick start

[mise](https://mise.jdx.dev/) manages all toolchain dependencies (meson, ninja, node, rust):

```bash
mise install
mise run test:all
```

### Manual setup

```bash
conan install . -pr conan_clang_profile -of build --build=missing
conan build . -of build
```

## Testing

```bash
mise run test        # C++ unit tests (doctest)
mise run test:viem   # Cross-validate against viem
mise run test:alloy  # Cross-validate against alloy
mise run test:all    # Run everything
```

The cross-validation suites run the same test vectors through ingot and a reference implementation (viem/alloy), then compare the outputs byte-for-byte. This covers address derivation, `personal_sign` across 6 message types, and EIP-1559 transaction signing.