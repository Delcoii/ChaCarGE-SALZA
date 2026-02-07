#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)"
BIN="$ROOT_DIR/src/bsw/ipc/src/can_rx_process/can_rx_process"

exec "$BIN"