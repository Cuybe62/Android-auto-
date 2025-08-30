#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/hub/web-src"
DST="$ROOT/hub/www"
if ! command -v npm >/dev/null 2>&1; then echo "npm introuvable (Node 18+)."; exit 1; fi
cd "$SRC"
if [[ ! -d node_modules ]]; then npm ci || npm i; fi
npm run build
mkdir -p "$DST"
cp -r dist/* "$DST/"
echo "Hub web → $DST"
