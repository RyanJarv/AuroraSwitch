#!/bin/sh
# Run host checks or Arm builds in the pinned Linux toolchain environment.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# Use the daemon architecture, which can differ from this shell on remote Docker.
architecture=$(docker info --format '{{.Architecture}}')
case "$architecture" in
    x86_64|amd64) architecture=amd64 ;;
    aarch64|arm64) architecture=arm64 ;;
    *) echo 'Docker builds require an AMD64 or ARM64 daemon.' >&2; exit 1 ;;
esac
platform=linux/$architecture
image=aurora-switch-build:10-2020-q4-major-$architecture

# Rootless Docker maps container root to the daemon's unprivileged host user.
# A numeric host UID inside that namespace maps to a different, subordinate UID.
case "$(docker info --format '{{json .SecurityOptions}}')" in
    *'"name=rootless"'*) container_user=0:0 ;;
    *) container_user="$(id -u):$(id -g)" ;;
esac

if [ "$#" -eq 0 ]; then
    set -- make check
fi

docker build --platform "$platform" -t "$image" -f "$root/docker/Dockerfile" "$root/docker"
docker run --rm --platform "$platform" \
    --user "$container_user" \
    --mount "type=bind,source=$root,target=/workspace" \
    --workdir /workspace "$image" "$@"
