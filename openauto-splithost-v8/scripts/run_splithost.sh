#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP="$ROOT/build/apps/splithost/splithost"
AUTOAPP="${1:-/opt/openauto-ce/openauto/build/autoapp}"
export QT_QPA_PLATFORM=xcb
: "${DISPLAY:=:0}"; export DISPLAY
export HUB_WWW_DIR="${HUB_WWW_DIR:-$ROOT/hub/www}"
export HUB_MODULE_DIR="${HUB_MODULE_DIR:-$ROOT/hub/modules}"
if [[ ! -x "$APP" ]]; then echo "Build d'abord: $ROOT/scripts/build_all.sh"; exit 1; fi
echo "OpenAuto: $AUTOAPP"; echo "Hub WWW: $HUB_WWW_DIR"; echo "Hub Modules: $HUB_MODULE_DIR"
"$APP" --openauto "$AUTOAPP" --title "OpenAuto" --hub-www "$HUB_WWW_DIR" --hub-modules "$HUB_MODULE_DIR"
