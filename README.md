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

You need two things from your OS package manager: **clang** (with C++20 support) and **[mise](https://mise.jdx.dev/)**. mise then manages everything else (meson, ninja, conan, bun, rust).

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y clang libstdc++-14-dev pkg-config git curl ca-certificates
curl https://mise.run | sh
```

### Arch

```bash
sudo pacman -S --needed clang mise git
```

### Then, in the repo

```bash
mise trust
mise install
mise run test:all
```

`mise install` fetches the build tools. `mise run test:all` builds the library and runs the C++ unit tests plus the viem and alloy cross-validation suites.

## Tasks

```bash
mise run setup       # conan profile detect + conan install
mise run build       # Build the library
mise run test        # C++ unit tests (doctest)
mise run test:viem   # Cross-validate against viem
mise run test:alloy  # Cross-validate against alloy
mise run test:all    # Everything
```

The cross-validation suites run the same test vectors through ingot and a reference implementation (viem or alloy), then compare the outputs byte-for-byte. This covers address derivation, `personal_sign` across 6 message types, and EIP-1559 transaction signing.

## Manual setup (without mise)

If you prefer not to use mise, install the toolchain yourself: **clang**, **meson**, **ninja**, **conan** (2.x), **bun** (for the viem suite), and a **Rust** toolchain with cargo (for the alloy suite).

```bash
export CC=clang CXX=clang++
conan profile detect --exist-ok
conan install . -of build --build=missing
conan build   . -of build

# C++ unit tests
meson test -C build -v

# viem cross-validation
( cd tests/viem_compare && bun install && bun run compare.ts )

# alloy cross-validation
( cd tests/alloy_compare && cargo run -- ../../build/tests/test_cross_validate )
```

## Docker

Two Dockerfiles reproduce the full build + test pipeline on Ubuntu 24.04 and double as reference setups:

```bash
docker build -t ingot .                    # mise-based (mirrors Quick start)
docker build -t ingot -f Dockerfile.manual . # manual install (no mise)
```

## Documentation

Full docs at [ingot.dahlman.dev](https://ingot.dahlman.dev). See also:

- [`docs/getting-started.md`](docs/getting-started.md) -- setup details and using ingot in your own project
- [`docs/api.md`](docs/api.md) -- API reference
- [`docs/security.md`](docs/security.md) -- secret handling guarantees and limitations
