#!/bin/sh
# Run host checks or Arm builds in the pinned Linux toolchain environment.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
image=aurora-switch-build:10-2020-q4-major

if [ "$#" -eq 0 ]; then
    set -- make check
fi

docker build --platform linux/amd64 -t "$image" -f "$root/docker/Dockerfile" "$root/docker"
docker run --rm --platform linux/amd64 \
    --user "$(id -u):$(id -g)" \
    --mount "type=bind,source=$root,target=/workspace" \
    --workdir /workspace "$image" "$@"
