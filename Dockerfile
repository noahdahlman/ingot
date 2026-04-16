FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# System toolchain: clang + libstdc++ headers. Everything else (meson,
# ninja, conan, bun, rust) is managed by mise below.
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
ENV PATH="/root/.local/bin:/root/.local/share/mise/shims:${PATH}"

WORKDIR /src

# Install tools first so this layer caches across source changes.
COPY .mise.toml .
RUN mise trust && mise install

COPY . .

RUN mise run test:all
