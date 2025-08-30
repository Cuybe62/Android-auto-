#!/usr/bin/env bash
set -euo pipefail
pretty=$(uptime -p 2>/dev/null || true)
load=$(cut -d ' ' -f1-3 /proc/loadavg)
printf '{"module":"uptime","data":{"pretty":"%s","loadavg":"%s"}}
' "${pretty:-N/A}" "$load"
