#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/build"
cd "$ROOT/build"
cmake ..
make -j"$(nproc)"
echo "Binaire: $ROOT/build/apps/splithost/splithost"
