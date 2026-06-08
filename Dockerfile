FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git curl zip unzip tar pkg-config \
        autoconf autoconf-archive automake libtool m4 python3 bison flex perl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /app
COPY . .

RUN cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    && cmake --build build --target minesweeper --parallel "$(nproc)"

FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/minesweeper build/minesweeper
COPY --from=builder /app/web web

EXPOSE 8080
CMD ["./build/minesweeper"]
