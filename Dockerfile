# syntax=docker/dockerfile:1

# ---------------------------------------------------------------------------
# Build stage — Alpine (musl). The build needs only a C++17 compiler and CMake;
# Asio, CLI11, spdlog and nlohmann/json are fetched + built from source. The CLI
# is linked fully static against musl, so it has no runtime library dependencies
# and runs on a distroless-static / scratch base. The whole builder image is
# discarded.
# ---------------------------------------------------------------------------
FROM alpine:3.21 AS build

RUN apk add --no-cache \
        build-base \
        cmake \
        ninja \
        git \
        linux-headers \
        ca-certificates

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DESPHOME_API_BUILD_CLI=ON \
        -DESPHOME_API_BUILD_TESTS=OFF \
        -DESPHOME_API_BUILD_EXAMPLES=OFF \
        -DESPHOME_API_INSTALL=OFF \
        -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++" \
    && cmake --build build --target esphome-cli -j "$(nproc)" \
    && strip build/bin/esphome-cli

# ---------------------------------------------------------------------------
# Runtime stage — distroless static (just ca-certificates, tzdata and a nonroot
# user). The fully static musl binary needs nothing else.
# ---------------------------------------------------------------------------
FROM gcr.io/distroless/static-debian12 AS runtime

COPY --from=build /src/build/bin/esphome-cli /usr/local/bin/esphome-cli

ENTRYPOINT ["/usr/local/bin/esphome-cli"]
