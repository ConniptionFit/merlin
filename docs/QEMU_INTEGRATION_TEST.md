# Merlin QEMU Integration Testing (emery / Pebble Time 2)

Executable routines for local QEMU-based verification. Run all commands from the repo root unless noted.

## Prerequisites

```bash
uv tool install pebble-tool --python 3.13
pebble sdk install latest
export PEBBLE_EMULATOR=emery
```

## Terminal layout (recommended)

| Pane | Command |
|------|---------|
| T1 | `./scripts/qemu/boot-emery.sh` |
| T2 | `./scripts/qemu/start-transcribe-server.sh` |
| T3 | `node scripts/qemu/mock-backend.js` |
| T4 | `./scripts/qemu/tail-appmessage-logs.sh` |
| T5 | `./scripts/qemu/run-transcribe-tests.sh` |

---

## 1. Launch & boot emery QEMU

### Failsafe: kill stale QEMU processes

```bash
./scripts/qemu/kill-stale-qemu.sh
```

Manual equivalent:

```bash
pkill -f qemu-system-arm || true
pebble kill || true
pkill -9 -f qemu-system-arm || true
```

### Clean build + boot + install Merlin

```bash
./scripts/qemu/boot-emery.sh
```

Manual equivalent:

```bash
export PEBBLE_EMULATOR=emery
cd app
npm install
pebble clean && pebble build
pebble install --emulator emery
```

The desktop emulator wrapper starts automatically. Default button map:

| Emulator button | Key |
|-----------------|-----|
| Back | `Q` |
| Up | `W` |
| Select | `S` |
| Down | `X` |

---

## 2. Voice & Gemini pipeline via QEMU

### Start dictation mock service (required before Merlin dictation)

```bash
./scripts/qemu/start-transcribe-server.sh
```

### Configure PKJS for mock backend (phone simulator)

```bash
cd app
pebble emu-app-config
```

Set in Clay:

- **Gemini API Key**: any non-empty test string (mock server checks header presence only)
- **Backend URL**: `http://127.0.0.1:9090`

### `pebble transcribe` syntax

One-shot successful injection:

```bash
pebble transcribe "Give me a quick recipe"
```

Inject STT failure (maps to Merlin `Dictation failed` UI):

```bash
pebble transcribe --error no-speech-detected
pebble transcribe --error connectivity
pebble transcribe --error disabled
```

### Automated three-case test runner

```bash
./scripts/qemu/run-transcribe-tests.sh
```

| Case | Injection | Pass criteria |
|------|-----------|---------------|
| **TC1** | `pebble transcribe "Give me a quick recipe"` | Prompt appears in ScrollLayer; STATUS → Thinking; RESPONSE chunks arrive; no crash |
| **TC2** | ~500-char elongated prompt | Transcript truncated at 512B in C; AppMessage still sends; PKJS receives DICTATION_TEXT |
| **TC3** | `pebble transcribe --error no-speech-detected` | C shows `Dictation failed`; **no** DICTATION_TEXT outbox; emulator stable |

**Workflow per case:** open Merlin → wait for dictation or press `S` → run injection command in T5 → observe T4 logs + emulator UI.

---

## 3. Log inspection & network simulation

### AppMessage / PKJS log pipeline

```bash
./scripts/qemu/tail-appmessage-logs.sh
```

Manual filtered pipeline:

```bash
pebble logs 2>&1 | grep --line-buffered -E \
  'DICTATION_TEXT|FEEDBACK_TEXT|RESPONSE|RESPONSE_DONE|STATUS|TIMER_|AppMessage|Merlin PKJS|Thinking|HTTP|Network'
```

Verbose pebble tool logging (when supported):

```bash
pebble logs -vvvv 2>&1 | grep --line-buffered -E 'DICTATION|RESPONSE|STATUS'
```

**What to look for**

1. **Watch → phone (outbox):** C dictation success → PKJS receives payload with `DICTATION_TEXT`
2. **Phone → watch (inbox):** `STATUS: Thinking...` then `RESPONSE` chunks + `RESPONSE_DONE`
3. **Errors:** `STATUS` strings like `HTTP 504: ...`, `Network error`, `Set Gemini key in phone config`

### Mock backend + degradation scenarios

Start normal mock:

```bash
node scripts/qemu/mock-backend.js
```

Run scripted network scenarios:

```bash
./scripts/qemu/run-network-scenarios.sh slow      # 6s latency
./scripts/qemu/run-network-scenarios.sh timeout   # 18s latency
./scripts/qemu/run-network-scenarios.sh 504       # immediate HTTP 504
./scripts/qemu/run-network-scenarios.sh empty      # 200 with empty JSON body
```

Environment overrides:

```bash
MOCK_MODE=504 MOCK_PORT=9090 node scripts/qemu/mock-backend.js
MOCK_MODE=slow MOCK_DELAY_MS=8000 node scripts/qemu/mock-backend.js
```

### Graceful degradation pass criteria (QEMU)

| Scenario | Expected watch behavior | QEMU runtime |
|----------|-------------------------|--------------|
| HTTP 504 | STATUS shows gateway error; session remains interactive | No crash |
| Slow 6–18s | STATUS stays on Thinking until XHR completes/fails | No watchdog reset |
| Empty JSON `{}` | Fallback response text or STATUS error | No fault loop |
| STT `--error` | `Dictation failed` in STATUS; no backend call | Stable |

After each failure scenario, press `S` to confirm dictation can restart.

### Optional screenshot capture

```bash
pebble screenshot /tmp/merlin-qemu-$(date +%H%M%S).png
```

---

## Quick smoke (copy-paste)

```bash
cd ~/Projects/merlin-assistant
./scripts/qemu/kill-stale-qemu.sh
./scripts/qemu/boot-emery.sh
# new terminals:
./scripts/qemu/start-transcribe-server.sh
node scripts/qemu/mock-backend.js
./scripts/qemu/tail-appmessage-logs.sh
pebble transcribe "Give me a quick recipe"
```
