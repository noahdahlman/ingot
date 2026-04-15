# ingot

[![Docs](https://img.shields.io/badge/docs-ingot.dahlman.dev-7c3aed)](https://ingot.dahlman.dev)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

Minimal Ethereum signing library for C++20.

```cpp
#include <ingot/signing.hpp>

ingot::SigningKey<ingot::Secp256k1> key("0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");

auto sig = key.personal_sign("hello");
auto addr = key.address();
```

## Features

- **Simple API** -- `SigningKey`, `Address`, `Signature`
- **EIP-191** `personal_sign`
- **Defensive** -- secret key zeroing on destruction, move, and constructor throw; nonce cleanup; `explicit_bzero`
- **Fast** -- keccak-f1600 fully unrolls to straight-line `rolq` instructions
- **Cross-validated** against [viem](https://viem.sh/)

## Quick start

```bash
conan install . --profile=conan_clang_profile --output-folder=build --build=missing

CC=clang CXX=clang++ meson setup build \
  --native-file build/conan_meson_native.ini \
  --pkg-config-path build \
  --wrap-mode=default

ninja -C build
ninja -C build test
```

