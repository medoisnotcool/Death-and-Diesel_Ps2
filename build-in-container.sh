#!/usr/bin/env sh
# Build the PS2 ELF locally without installing PS2SDK on the host.
# Requires Docker or a compatible container runtime.
set -eu

docker run --rm \
  -v "$PWD:/src" \
  -w /src \
  ps2dev/ps2dev:latest \
  sh -lc 'make clean && make'
