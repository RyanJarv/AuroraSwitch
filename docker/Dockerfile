FROM ubuntu:24.04

# Match the toolchain archive and checksum in firmware-release.yml.
RUN apt-get update && apt-get install -y --no-install-recommends \
    bzip2 ca-certificates cmake curl g++ gdb git make python3 \
    && rm -rf /var/lib/apt/lists/*

ARG TOOLCHAIN_URL=https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2
ARG TOOLCHAIN_SHA256=21134caa478bbf5352e239fbc6e2da3038f8d2207e089efc96c3b55f1edcd618
RUN curl --fail --location --retry 3 --connect-timeout 15 --max-time 300 \
    "$TOOLCHAIN_URL" --output /tmp/toolchain.tar.bz2 \
    && echo "$TOOLCHAIN_SHA256  /tmp/toolchain.tar.bz2" | sha256sum --check \
    && mkdir -p /opt/arm \
    && tar -xjf /tmp/toolchain.tar.bz2 -C /opt/arm \
    && rm /tmp/toolchain.tar.bz2 \
    && /opt/arm/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-gcc --version | grep -q '10-2020-q4-major'

ENV PATH="/opt/arm/gcc-arm-none-eabi-10-2020-q4-major/bin:${PATH}"
WORKDIR /workspace
