#!/usr/bin/env bash
# Tail Pebble emulator logs with filters for Merlin AppMessage / PKJS traffic.
set -euo pipefail

if ! command -v pebble >/dev/null 2>&1; then
  echo "ERROR: pebble CLI not found." >&2
  exit 1
fi

# Message keys defined in app/package.json
FILTER='DICTATION_TEXT|FEEDBACK_TEXT|FEEDBACK_FLAG|RESPONSE|RESPONSE_DONE|STATUS|TIMER_DURATION|TIMER_NAME|AppMessage|appmessage|sendAppMessage|Merlin PKJS|Thinking|Dictation|outbox|inbox|HTTP|Network|Gemini|timeline'

echo "==> Tailing pebble logs (emulator) filtered for Merlin AppMessage / PKJS traffic"
echo "    Filter regex: $FILTER"
echo "    Tip: add pebble tool verbosity if needed: pebble -vvvv logs"
echo ""

# pebble logs follows the active emulator when no --phone is supplied.
# Use --no-color if your terminal mishandles ANSI.
pebble logs 2>&1 | grep --line-buffered -E "$FILTER" || true
