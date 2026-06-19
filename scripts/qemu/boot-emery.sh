#!/usr/bin/env bash
# Build Merlin and boot the Pebble QEMU desktop emulator for the emery (Time 2) profile.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APP="$ROOT/app"
export PEBBLE_EMULATOR="${PEBBLE_EMULATOR:-emery}"

if ! command -v pebble >/dev/null 2>&1; then
  echo "ERROR: pebble CLI not found. Install pebble-tool:" >&2
  echo "  uv tool install pebble-tool --python 3.13" >&2
  echo "  pebble sdk install latest" >&2
  exit 1
fi

echo "==> Using emulator platform: $PEBBLE_EMULATOR"
echo "==> Killing stale emulator processes..."
"$ROOT/scripts/qemu/kill-stale-qemu.sh"

echo "==> Building Merlin (emery)..."
cd "$APP"
npm install --silent 2>/dev/null || npm install
pebble clean
pebble build

echo "==> Launching QEMU emulator wrapper and installing Merlin..."
echo "    (A desktop emulator window should appear. Buttons: Q=Back, S=Select, W/X=Up/Down)"
pebble install --emulator "$PEBBLE_EMULATOR"

echo ""
echo "==> Merlin is installed on emery QEMU."
echo "    Next steps:"
echo "      1. In another terminal: scripts/qemu/start-transcribe-server.sh"
echo "      2. Launch Merlin on the emulator (app launcher)."
echo "      3. scripts/qemu/run-transcribe-tests.sh   # inject voice prompts"
echo "      4. scripts/qemu/tail-appmessage-logs.sh   # watch AppMessage traffic"
