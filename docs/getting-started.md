# Getting Started

## Prerequisites

- **Clang** with C++20 support
- **Meson** and **Ninja**
- **Conan** 2.x
- **mise** (optional, for environment management)

## Setup

### 1. Environment

If using mise, the included `.mise.toml` sets `CC=clang` and `CXX=clang++` automatically:

```bash
mise trust
mise install
```

### 2. Build

A clang profile is included in the repo. Conan handles dependency installation, meson configuration, and compilation:

```bash
conan install . -pr conan_clang_profile -of build --build=missing
conan build . -pr conan_clang_profile -of build
```

libsecp256k1 is fetched and built automatically as a meson subproject on first configure.

### 3. Test

```bash
# Run doctest unit tests
meson test -C build -v

# Run viem cross-validation (requires Bun)
cd tests/viem_compare
bun install
bun run compare.ts
```

## Using ingot in your project

### As a meson subproject

Create `subprojects/ingot.wrap`:

```ini
[wrap-git]
url = https://github.com/noahdahlman/ingot.git
revision = main
depth = 1

[provide]
ingot = ingot_dep
```

Then in your `meson.build`:

```meson
ingot_dep = dependency('ingot',
  fallback: ['ingot', 'ingot_dep'],
)
```

### Manual integration

ingot is a single static library (`libingot.a`) plus headers. Point your include path at `include/` and link against `libingot.a` and `libsecp256k1`.
