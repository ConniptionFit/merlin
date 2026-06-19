# Merlin

Decentralized BYOK AI assistant for Core Devices Pebble Time 2 (Emery).

Merlin uses native watch dictation (Core Devices Android STT router), a PKJS phone middleware layer, and a stateless Go backend that accepts your Gemini API key per request.

## Project layout

- `app/` — Pebble C app + PKJS companion JavaScript
- `service/` — Stateless Go backend

## Prerequisites

- [Pebble SDK](https://developer.repebble.com/sdk) (`pebble-tool`)
- Node.js + npm
- Go 1.22+
- Core Devices Android app with STT set to **Local (with Cloud Fallback)**

## Backend

```bash
cd service
go mod tidy
go run .
```

The server listens on `:8080`. For phone testing, bind is already `0.0.0.0` via default ListenAndServe and set Clay **Backend URL** to your laptop LAN IP, e.g. `http://192.168.1.42:8080`.

Endpoints:

- `GET /health`
- `POST /query` — headers: `X-Gemini-Key`, optional `X-Latitude`, `X-Longitude`
- `POST /feedback`

## Watch app

```bash
cd app
npm install
pebble build
pebble install --phone <phone-ip>   # or sideload .pbw via Core Devices app
```

Open Merlin settings on the phone to enter:

1. **Gemini API Key** (stored in phone localStorage only)
2. **Backend URL** (defaults to `http://localhost:8080`)

## Usage

- **Select** — start dictation
- **Select long-press** — feedback dictation
- **Back** — exit session

Timers use the Pebble Wakeup API. Timeline pins are created via `timeline-api.rebble.io` using the phone timeline token.

## QEMU integration testing

See [docs/QEMU_INTEGRATION_TEST.md](docs/QEMU_INTEGRATION_TEST.md) for emery emulator boot, `pebble transcribe` injection cases, AppMessage log pipelines, and network-degradation scenarios.

Quick start:

```bash
./scripts/qemu/kill-stale-qemu.sh
./scripts/qemu/boot-emery.sh
./scripts/qemu/start-transcribe-server.sh   # separate terminal
node scripts/qemu/mock-backend.js           # separate terminal
./scripts/qemu/tail-appmessage-logs.sh      # separate terminal
pebble transcribe "Give me a quick recipe"
```

## License

Apache 2.0
