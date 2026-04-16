# API Reference

All types live in the `ingot` namespace. The single include you need:

```cpp
#include <ingot/signing.hpp>
```

## SigningKey\<Secp256k1\>

ECDSA signing key on the secp256k1 curve.

### Construction

```cpp
// From hex string (with or without 0x prefix)
explicit SigningKey(std::string_view hex);

// From raw 32 bytes
explicit SigningKey(std::span<const uint8_t, 32> private_key);
```

Throws `std::invalid_argument` if the key is not a valid secp256k1 secret key.

The key is non-copyable. Move transfers ownership and zeros the source.

### Methods

#### `sign(const Hash& msg_hash) -> Signature`

Signs a raw 32-byte hash. You are responsible for hashing the message first.

```cpp
auto hash_bytes = ingot::keccak256("some data");
ingot::Hash h(std::span<const uint8_t>(hash_bytes.data(), 32));
auto sig = key.sign(h);
```

#### `sign(std::string_view message) -> Signature`

Hashes the message with keccak256, then signs.

```cpp
auto sig = key.sign("some data");
```

#### `sign(std::span<const uint8_t> message) -> Signature`

Hashes raw bytes with keccak256, then signs.

#### `personal_sign(std::string_view message) -> Signature`

EIP-191 `personal_sign`. Prefixes the message with `"\x19Ethereum Signed Message:\n{len}"`, hashes with keccak256, then signs. This is what MetaMask's `personal_sign` and viem's `signMessage` use.

```cpp
auto sig = key.personal_sign("hello");
// Identical to viem: account.signMessage({ message: "hello" })
```

#### `address() -> Address`

Derives the Ethereum address from the public key.

```cpp
auto addr = key.address();
std::cout << addr.to_hex(); // "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266"
```

---

## Address

Alias for `FixedBytes<20>`. A 20-byte Ethereum address.

```cpp
using Address = FixedBytes<20>;
```

---

## Hash

Alias for `FixedBytes<32>`. A 32-byte keccak256 hash.

```cpp
using Hash = FixedBytes<32>;
```

---

## Signature

ECDSA signature with recovery id.

### Fields

- `r` -- `FixedBytes<32>`, first 32 bytes of the signature
- `s` -- `FixedBytes<32>`, second 32 bytes of the signature
- `v` -- `uint8_t`, recovery id (0 or 1)

### Methods

#### `to_hex() -> std::string`

Returns the 65-byte signature as a hex string with `0x` prefix. The `v` value is encoded as `v + 27` (Ethereum legacy format).

```cpp
sig.to_hex(); // "0x..." (132 characters)
```

#### `to_bytes() -> std::array<uint8_t, 65>`

Returns `r || s || v` as 65 raw bytes, with `v` encoded as `v + 27`.

---

## FixedBytes\<N\>

Fixed-size byte array wrapper. The building block for `Address`, `Hash`, and signature components.

### Construction

```cpp
// Default (zero-initialized)
FixedBytes();

// From raw bytes (must be exactly N bytes)
explicit FixedBytes(std::span<const uint8_t> data);

// From hex string (with or without 0x prefix)
explicit FixedBytes(std::string_view hex);
```

### Methods

- `data() -> const uint8_t*` -- pointer to underlying bytes
- `size() -> std::size_t` -- always `N`
- `begin()` / `end()` -- iterators for range-based iteration
- `to_hex() -> std::string` -- hex string with `0x` prefix
- `operator<=>` -- three-way comparison (all comparison operators)

---

## keccak256

```cpp
std::array<uint8_t, 32> keccak256(std::span<const uint8_t> input);
std::array<uint8_t, 32> keccak256(std::string_view input);
```

Ethereum's keccak256 hash (original Keccak with `0x01` padding, not NIST SHA3-256).

```cpp
auto hash = ingot::keccak256("hello world");
// hash == {0x47, 0x17, 0x32, ...}
```
