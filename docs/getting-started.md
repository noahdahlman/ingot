# Getting Started

## Prerequisites

- **clang** with C++20 support and libstdc++
- **[mise](https://mise.jdx.dev/)**, which manages the rest of the toolchain (meson, ninja, conan, bun, rust)

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

## Build and test

From the repo root:

```bash
mise trust
mise install
mise run test:all
```

`mise install` fetches the build tools. `mise run test:all` runs the `setup`, `build`, `test`, `test:viem`, and `test:alloy` tasks in order.

```bash
mise run setup       # conan profile detect + conan install
mise run build       # Compile the library
mise run test        # C++ unit tests (doctest)
mise run test:viem   # Cross-validate against viem
mise run test:alloy  # Cross-validate against alloy
```

libsecp256k1 is fetched and built automatically as a meson subproject on the first configure.

## Manual setup (without mise)

If you prefer to install the toolchain yourself, you need:
- clang (C++20)
- meson, ninja
- conan 2

# for cross validation tests
- bun
- Rust + cargo

Then, from the repo root:

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
