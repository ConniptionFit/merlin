#!/usr/bin/env bash
# Forcefully terminate stale Pebble/QEMU emulator processes before a fresh session.
set -euo pipefail

echo "==> Scanning for stale emulator processes..."

PATTERNS=(
  'qemu-system-arm'
  'qemu_emery'
  'pebble-emulator'
  'pebble.*qemu'
)

FOUND=0
for pattern in "${PATTERNS[@]}"; do
  if pgrep -fl "$pattern" >/dev/null 2>&1; then
    FOUND=1
    echo "    matched: $pattern"
    pgrep -fl "$pattern" || true
  fi
done

if [[ "$FOUND" -eq 0 ]]; then
  echo "==> No stale QEMU/emulator processes found."
  exit 0
fi

echo "==> Sending SIGTERM..."
pkill -f 'qemu-system-arm' 2>/dev/null || true
pkill -f 'qemu_emery' 2>/dev/null || true
pkill -f 'pebble-emulator' 2>/dev/null || true
sleep 1

# Escalate if anything survives.
if pgrep -f 'qemu-system-arm' >/dev/null 2>&1; then
  echo "==> Escalating to SIGKILL on qemu-system-arm"
  pkill -9 -f 'qemu-system-arm' 2>/dev/null || true
fi

# pebble kill also stops the bundled phone simulator + emulator wrapper.
if command -v pebble >/dev/null 2>&1; then
  echo "==> Running: pebble kill"
  pebble kill 2>/dev/null || true
fi

sleep 1
if pgrep -f 'qemu-system-arm' >/dev/null 2>&1; then
  echo "ERROR: qemu-system-arm still running:" >&2
  pgrep -fl 'qemu-system-arm' >&2 || true
  exit 1
fi

echo "==> Emulator environment is clean."
