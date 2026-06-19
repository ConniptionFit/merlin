#!/usr/bin/env bash
# Start the Pebble dictation mock server for QEMU (must run before dictation in Merlin).
set -euo pipefail

if ! command -v pebble >/dev/null 2>&1; then
  echo "ERROR: pebble CLI not found." >&2
  exit 1
fi

echo "==> Starting pebble transcribe server for QEMU..."
echo "    Leave this terminal running. Merlin dictation sessions will hit this service."
echo "    Inject a one-shot transcript with: pebble transcribe \"your text here\""
echo "    Inject an error with: pebble transcribe --error no-speech-detected"
echo ""

# Server mode: no preset message; responds per invocation when used interactively.
# For scripted tests, use run-transcribe-tests.sh which calls pebble transcribe with explicit strings.
exec pebble transcribe
