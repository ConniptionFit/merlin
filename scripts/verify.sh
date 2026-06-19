#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FAIL=0

check() {
  if [[ -e "$1" ]]; then
    echo "OK  $1"
  else
    echo "MISSING  $1"
    FAIL=1
  fi
}

echo "Merlin integration verification"
echo "==============================="

for f in \
  app/package.json \
  app/wscript \
  app/src/c/merlin.c \
  app/src/c/session_window.c \
  app/src/c/wizard_layer.c \
  app/src/c/timer_manager.c \
  app/src/c/app_message.c \
  app/src/pkjs/index.js \
  app/src/pkjs/api.js \
  app/src/pkjs/timeline.js \
  app/src/pkjs/lib/message_queue.js \
  app/resources/images/wizard.png \
  app/resources/icons/menu_icon.png \
  service/main.go \
  service/handler/query.go \
  service/handler/feedback.go \
  service/gemini/client.go \
  service/geo/redact.go
do
  check "$ROOT/$f"
done

echo
echo "PKJS syntax check"
node --check "$ROOT/app/src/pkjs/index.js"
node --check "$ROOT/app/src/pkjs/api.js"
node --check "$ROOT/app/src/pkjs/timeline.js"
node --check "$ROOT/app/src/pkjs/lib/message_queue.js"

if command -v go >/dev/null 2>&1; then
  echo
  echo "Go build"
  (cd "$ROOT/service" && go mod tidy && go build -o merlin-service .)
else
  echo
  echo "SKIP Go build (go not installed)"
fi

if command -v pebble >/dev/null 2>&1; then
  echo
  echo "Pebble build"
  (cd "$ROOT/app" && pebble build)
else
  echo
  echo "SKIP Pebble build (pebble SDK not installed)"
fi

if [[ "$FAIL" -ne 0 ]]; then
  exit 1
fi

echo
echo "Verification complete"
