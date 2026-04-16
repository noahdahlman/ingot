# ingot

A minimal Ethereum signing library for C++20.

```cpp
#include <ingot/signing.hpp>

ingot::SigningKey<ingot::Secp256k1> key("0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");

auto sig = key.personal_sign("hello");
auto addr = key.address();
```

## Features

- **Simple API** -- `SigningKey`, `Address`, `Signature`, and you're done
- **EIP-191** `personal_sign` out of the box
- **Defensive** -- secret key zeroing on destruction, move, and constructor throw; nonce cleanup after signing; `explicit_bzero` for guaranteed memory wipe
- **Fast** -- keccak-f1600 compiles to fully unrolled straight-line `rolq` instructions, matching tiny_sha3 performance
- **Cross-validated** -- signatures verified identical to [viem](https://viem.sh/) and [alloy](https://github.com/alloy-rs/alloy)

## Dependencies

- [libsecp256k1](https://github.com/bitcoin-core/secp256k1) (fetched automatically as a meson subproject)
- [doctest](https://github.com/doctest/doctest) (via Conan, test only)

## Quick start

Install `clang` and [mise](https://mise.jdx.dev/) from your OS package manager, then:

```bash
mise install        # fetches meson, ninja, conan, bun, rust
mise run test:all   # builds the library and runs all test suites
```

See [Getting Started](getting-started.md) for Ubuntu/Arch setup, manual setup without mise, and integration instructions.
