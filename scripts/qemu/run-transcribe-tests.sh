#!/usr/bin/env bash
# Inject three dictation test cases into an active QEMU emery session via pebble transcribe.
set -euo pipefail

if ! command -v pebble >/dev/null 2>&1; then
  echo "ERROR: pebble CLI not found." >&2
  exit 1
fi

pause_between_tests="${PAUSE_SEC:-8}"

run_case() {
  local id="$1"
  local label="$2"
  shift 2
  echo ""
  echo "============================================================"
  echo "TEST $id: $label"
  echo "============================================================"
  echo "Command: pebble transcribe $*"
  echo "Precondition: Merlin session window open, dictation idle or re-tap Select (S)."
  read -r -p "Press Enter when the emulator is ready for this injection..."
  pebble transcribe "$@"
  echo "==> Injected. Observe Merlin UI + run tail-appmessage-logs.sh in another pane."
  if [[ "$id" != "TC3" ]]; then
    echo "Waiting ${pause_between_tests}s before next case..."
    sleep "$pause_between_tests"
  fi
}

echo "Merlin QEMU dictation test runner (emery)"
echo "Ensure:"
echo "  - scripts/qemu/boot-emery.sh completed successfully"
echo "  - scripts/qemu/start-transcribe-server.sh is running OR use one-shot pebble transcribe below"
echo "  - Mock backend running: scripts/qemu/mock-backend.js (optional, for full pipeline)"
echo ""

# TC1 — clean query
run_case "TC1" "Clean recipe query (happy path)" \
  "Give me a quick recipe"

# TC2 — elongated prompt (512-byte dictation buffer boundary in Merlin C)
LONG_PROMPT="Merlin please give me a detailed recipe for mushroom risotto including arborio rice preparation stock reduction wine deglazing butter finish parmesan crisp topping and a side salad pairing and also estimate total cook time prep time and calories per serving for four people with vegetarian substitutions and wine pairing notes for each course and storage instructions for leftovers reheated on the second day without losing texture or flavor profile while keeping the answer concise enough for a watch display yet long enough to stress test buffers"
run_case "TC2" "Elongated prompt (~${#LONG_PROMPT} chars, buffer stress)" \
  "$LONG_PROMPT"

# TC3 — empty / failure path
echo ""
echo "============================================================"
echo "TEST TC3: Empty / failure dictation handling"
echo "============================================================"
echo "Option A (recommended): simulate STT failure via SDK error flag"
read -r -p "Press Enter to inject --error no-speech-detected..."
pebble transcribe --error no-speech-detected

echo ""
echo "Option B (optional): attempt empty-string success path"
read -r -p "Press Enter to inject empty quoted string (may vary by SDK version)..."
pebble transcribe "" || echo "(empty string rejected by pebble transcribe — expected on some SDK builds)"

echo ""
echo "All scripted dictation injections complete."
echo "Expected Merlin C UI for TC3: status shows 'Dictation failed' (no AppMessage outbox)."
