FROM ubuntu:24.04

# Include CMake/GDB for CLion as well as the Make-based command-line build.
# The AMD64 toolchain matches firmware-release.yml; ARM64 uses the same release.
RUN apt-get update && apt-get install -y --no-install-recommends \
    bzip2 ca-certificates cmake curl g++ gdb git make python3 \
    && rm -rf /var/lib/apt/lists/*

# BuildKit sets TARGETARCH from the requested image platform.
ARG TARGETARCH
RUN case "$TARGETARCH" in \
        amd64) toolchain_host=x86_64; \
            toolchain_sha256=21134caa478bbf5352e239fbc6e2da3038f8d2207e089efc96c3b55f1edcd618 ;; \
        arm64) toolchain_host=aarch64; \
            toolchain_sha256=343d8c812934fe5a904c73583a91edd812b1ac20636eb52de04135bb0f5cf36a ;; \
        *) echo "Unsupported build architecture: $TARGETARCH" >&2; exit 1 ;; \
    esac \
    && curl --fail --location --retry 3 --connect-timeout 15 --max-time 300 \
        "https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-${toolchain_host}-linux.tar.bz2" \
        --output /tmp/toolchain.tar.bz2 \
    && echo "$toolchain_sha256  /tmp/toolchain.tar.bz2" | sha256sum --check \
    && mkdir -p /opt/arm \
    && tar -xjf /tmp/toolchain.tar.bz2 -C /opt/arm \
    && rm /tmp/toolchain.tar.bz2 \
    && /opt/arm/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-gcc --version | grep -q '10-2020-q4-major'

ENV PATH="/opt/arm/gcc-arm-none-eabi-10-2020-q4-major/bin:${PATH}"
WORKDIR /workspace
