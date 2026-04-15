FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    clang \
    libstdc++-14-dev \
    pkg-config \
    curl \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# mise
RUN curl https://mise.run | sh
ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /src

# Install tools first for better layer caching (skip clang, using system)
COPY .mise.toml .
RUN sed -i '/^clang/d' .mise.toml && mise trust && mise install
ENV PATH="/root/.local/share/mise/shims:${PATH}"

COPY . .

# Patch conan profile to match system clang version
RUN conan profile detect --force && \
    CLANG_VER=$(clang --version | head -1 | grep -oP '\d+' | head -1) && \
    sed -i "s/compiler.version=22/compiler.version=${CLANG_VER}/" conan_clang_profile

# Build
RUN conan install . -pr conan_clang_profile -of build --build=missing
RUN conan build . -pr conan_clang_profile -of build

# Tests
RUN meson test -C build -v
RUN cd tests/viem_compare && bun install && bun run compare.ts
RUN cd tests/alloy_compare && cargo run -- ../../build/tests/test_cross_validate
