# Security

ingot is designed to handle private key material safely. This page documents the measures in place and their limitations.

## Secret key lifecycle

### Zeroing on destruction

The destructor calls `explicit_bzero` via `secure_zero()`, which is guaranteed by the OS not to be optimized away. This is more robust than the common `volatile` pointer trick, which the C++ standard does not guarantee will prevent elision.

### Zeroing on move

Moving a `SigningKey` copies the secret to the destination, then immediately zeros the source. The default compiler-generated move would leave the key in both locations.

### Zeroing on failed construction

If key validation fails during construction (invalid secp256k1 key), the secret is zeroed before the exception propagates. This matters because C++ does not call destructors for partially-constructed objects.

### No intermediate copies

The hex constructor parses directly into the internal secret buffer. Earlier versions used a `Hash` temporary that was never zeroed -- this was identified and fixed in the security audit.

## Nonce protection

After every `sign()` call, the raw secp256k1 signature struct (which contains the nonce `k`) and the serialized output buffer are both zeroed. Recovering the nonce from a single signature allows full private key recovery.

## What ingot does NOT protect against

- **Heap/stack inspection by a privileged process** -- if an attacker can read your process memory, zeroing helps but is not a complete defense (the key exists in memory while in use)
- **Swap/core dump leakage** -- consider `mlock()` and disabling core dumps in your application
- **Side-channel attacks on hex parsing** -- the `hex_val()` function uses branches, which have input-dependent timing. This is exploitable only in specific co-residency scenarios (shared CPU, hyperthreading)
- **Big-endian platforms** -- the keccak256 implementation assumes little-endian byte order

## Testing and analysis

!!! warning
    This library has **not** been formally audited. Use at your own risk.

The following tools have been run against the codebase:

- **cppcheck** -- static analysis (0 errors)
- **Valgrind** -- runtime memory analysis (0 errors, 0 leaks)
- **viem cross-validation** -- signature output verified identical to viem for multiple test vectors including edge cases (empty string, unicode, special characters)
