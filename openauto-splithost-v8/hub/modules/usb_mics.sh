#!/usr/bin/env bash
set -euo pipefail
list=$(pactl list short sources 2>/dev/null | awk 'tolower($0) ~ /usb|mic|microphone/ {print $2}' | paste -sd, -)
printf '{"module":"usb_mics","data":{"sources":"%s"}}
' "${list:-}"
