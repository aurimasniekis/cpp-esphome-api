# syntax=docker/dockerfile:1

# ---------------------------------------------------------------------------
# Build stage (Debian bookworm == glibc 2.36, matching the distroless runtime).
# protobuf 21.12 is built + installed from source as STATIC libs. Installing
# from source ships protobuf's CMake config (so the project's find_package and
# ProtoGen.cmake resolve it), and the 3.21 series predates abseil — so the only
# bundled dep is protobuf itself. Zlib support is disabled, leaving the final
# binary with no shared deps beyond libc/libstdc++/libgcc (all in distroless/cc).
# Asio, libsodium, CLI11, spdlog and nlohmann/json are fetched + built from
# source. The whole builder image is discarded.
# ---------------------------------------------------------------------------
FROM debian:bookworm AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ARG PROTOBUF_VERSION=v21.12
RUN git clone --depth 1 --branch "${PROTOBUF_VERSION}" \
        https://github.com/protocolbuffers/protobuf.git /tmp/protobuf \
    && cmake -S /tmp/protobuf -B /tmp/protobuf/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -Dprotobuf_BUILD_TESTS=OFF \
        -Dprotobuf_BUILD_SHARED_LIBS=OFF \
        -Dprotobuf_WITH_ZLIB=OFF \
    && cmake --build /tmp/protobuf/build -j "$(nproc)" \
    && cmake --install /tmp/protobuf/build \
    && rm -rf /tmp/protobuf

WORKDIR /src
COPY . .

RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DESPHOME_API_BUILD_CLI=ON \
        -DESPHOME_API_BUILD_TESTS=OFF \
        -DESPHOME_API_BUILD_EXAMPLES=OFF \
        -DESPHOME_API_INSTALL=OFF \
    && cmake --build build --target esphome-cli -j "$(nproc)" \
    && strip build/bin/esphome-cli

# ---------------------------------------------------------------------------
# Runtime stage — distroless C/C++ (glibc + libstdc++ + libgcc only). The
# statically-linked protobuf means no extra runtime libraries are needed.
# ---------------------------------------------------------------------------
FROM gcr.io/distroless/cc-debian12 AS runtime

COPY --from=build /src/build/bin/esphome-cli /usr/local/bin/esphome-cli

ENTRYPOINT ["/usr/local/bin/esphome-cli"]
