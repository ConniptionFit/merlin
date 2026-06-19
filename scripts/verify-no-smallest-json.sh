#!/usr/bin/env bash
# Fail if lowercase legacy memoryFormat values remain in project JSON metadata.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if rg -n '"smallest"|"memory_format"[[:space:]]*:[[:space:]]*"smallest"' "$ROOT" --glob '*.json'; then
  echo "ERROR: legacy memoryFormat value 'smallest' still present" >&2
  exit 1
fi
echo "OK: no legacy 'smallest' memoryFormat in JSON configs"
