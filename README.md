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
meson setup build
meson compile -C build
meson test -C build -v
```

## API

### `SigningKey<Secp256k1>`

```cpp
// Construct from a 32-byte hex private key
SigningKey<Secp256k1> key("0xac09...");
SigningKey<Secp256k1> key(std::span<const uint8_t, 32> bytes);

// Derive the Ethereum address
Address addr = key.address();

// Sign a raw 32-byte hash
Signature sig = key.sign(hash);

// Sign arbitrary bytes (keccak256 hashed first)
Signature sig = key.sign(std::span<const uint8_t> message);
Signature sig = key.sign(std::string_view message);

// EIP-191 personal_sign
Signature sig = key.personal_sign("hello");

// EIP-1559 transaction signing -- returns the signed envelope bytes
std::vector<uint8_t> raw = key.sign_transaction(tx);
```

### `Transaction`

EIP-1559 (type 2) transaction:

```cpp
ingot::Transaction tx;
tx.chain_id              = 1;
tx.nonce                 = 0;
tx.max_priority_fee_per_gas = 1'000'000'000;   // 1 gwei
tx.max_fee_per_gas          = 20'000'000'000;  // 20 gwei
tx.gas_limit             = 21000;
tx.to                    = ingot::Address("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045");
tx.value                 = ingot::FixedBytes<32>(...); // uint256, big-endian
tx.data                  = {};                         // calldata
tx.access_list           = {};                         // EIP-2930

auto raw = signer.sign_transaction(tx);
```

### Types

| Type | Description |
|------|-------------|
| `Address` | `FixedBytes<20>` -- Ethereum address |
| `Hash` | `FixedBytes<32>` -- Keccak-256 hash |
| `Signature` | `r` (32B) + `s` (32B) + `v` (recovery id) |
| `FixedBytes<N>` | Fixed-size byte array with hex parsing/serialization |

## Testing

```bash
mise run test        # C++ unit tests (doctest)
mise run test:viem   # Cross-validate against viem
mise run test:alloy  # Cross-validate against alloy
mise run test:all    # Run everything
```

The cross-validation suites run the same test vectors through ingot and a reference implementation (viem/alloy), then compare the outputs byte-for-byte. This covers address derivation, `personal_sign` across 6 message types, and EIP-1559 transaction signing.

## Project structure

```
include/ingot/
  signing.hpp       SigningKey -- key management, signing, address derivation
  transaction.hpp   Transaction -- EIP-1559 tx struct, encoding, signing hash
  rlp.hpp           RlpEncoder -- RLP serialization
  types.hpp         FixedBytes, Address, Hash, Signature
  keccak.hpp        keccak256
  secure_zero.hpp   explicit_bzero wrapper
src/
  keccak.cpp        keccak-f1600 implementation
tests/
  test_signing.cpp        Unit tests (doctest)
  test_cross_validate.cpp Outputs test vectors for cross-validation
  viem_compare/           viem (TypeScript) cross-validation
  alloy_compare/          alloy (Rust) cross-validation
```
