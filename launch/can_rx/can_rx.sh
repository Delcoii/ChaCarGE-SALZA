#!/bin/bash

# Determine paths relative to this script
SCRIPT_DIR=$(cd "$(dirname "$(readlink -f "$0")")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/../.." && pwd)
BIN="$ROOT_DIR/src/bsw/ipc/src/can_rx_process/can_rx_process"

# Validate executable
if [ ! -x "$BIN" ]; then
    echo "[ERROR] Executable not found or not executable:"
    echo "        $BIN"
    exit 1
fi

# Move to binary directory (mirrors the csv replayer script style)
BIN_DIR=$(dirname "$BIN")
echo "Changing to executable directory: $BIN_DIR"
cd "$BIN_DIR" || { echo "[ERROR] Could not change to directory $BIN_DIR"; exit 1; }

echo "[INFO] Starting CAN RX process"
echo "[INFO] $(date)"

# Launch process
exec "$BIN"