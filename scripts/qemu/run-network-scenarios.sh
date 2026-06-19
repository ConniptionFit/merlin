#!/usr/bin/env bash
# Run network-degradation scenarios against Merlin in QEMU using the mock backend.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
MOCK="$ROOT/scripts/qemu/mock-backend.js"

if ! command -v node >/dev/null 2>&1; then
  echo "ERROR: node required." >&2
  exit 1
fi

echo "Merlin QEMU network simulation scenarios"
echo "Configure Clay Backend URL -> http://127.0.0.1:9090 via: cd app && pebble emu-app-config"
echo ""

run_scenario() {
  local name="$1"
  local mode="$2"
  local delay="${3:-6000}"
  echo ""
  echo "---- $name (MOCK_MODE=$mode) ----"
  echo "1. Start this scenario (blocks until Ctrl-C)"
  echo "2. In emulator: trigger dictation (Select / S) with pebble transcribe injection"
  echo "3. Watch scripts/qemu/tail-appmessage-logs.sh and emulator window for graceful STATUS errors"
  echo ""
  MOCK_MODE="$mode" MOCK_DELAY_MS="$delay" node "$MOCK"
}

case "${1:-menu}" in
  slow)
    run_scenario "Slow network (6s latency)" slow 6000
    ;;
  timeout)
    run_scenario "Gateway timeout (18s then 200 — PKJS XHR onerror/timeout)" timeout 6000
    ;;
  504)
    run_scenario "HTTP 504 from API gateway" 504 0
    ;;
  empty)
    run_scenario "Empty JSON body (200 with {})" empty 0
    ;;
  *)
    cat <<'EOF'
Usage: scripts/qemu/run-network-scenarios.sh {slow|timeout|504|empty}

Scenarios
  slow     MOCK_MODE=slow     — 6s delayed /query response; UI should stay on "Thinking..."
  timeout  MOCK_MODE=timeout  — 18s delay; PKJS should surface network error via STATUS
  504      MOCK_MODE=504      — immediate HTTP 504; expect STATUS "HTTP 504: ..."
  empty    MOCK_MODE=empty    — 200 with {}; PKJS should enqueue fallback response text

Pass criteria (QEMU must NOT crash / kernel panic / app fault loop):
  - Watch stays responsive; Back (Q) still pops session window
  - STATUS layer shows error string; no hard fault in pebble logs
  - After error, Select (S) can start a new dictation session

Run each scenario in a dedicated terminal, with boot-emery.sh already completed.
EOF
    ;;
esac
