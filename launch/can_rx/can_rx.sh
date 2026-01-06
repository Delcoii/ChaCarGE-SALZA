#!/bin/bash

# Determine paths relative to this script
SCRIPT_DIR=$(cd "$(dirname "$(readlink -f "$0")")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/../.." && pwd)
BIN="$ROOT_DIR/src/bsw/ipc/src/can_rx_process/can_rx_process"

# Load aliases/functions from bashrc so canstart alias is available
if [ -f "$HOME/.bashrc" ]; then
    # shellcheck disable=SC1090
    . "$HOME/.bashrc"
fi

# Bring up CAN interface via canstart alias/command
if command -v canstart >/dev/null 2>&1; then
    echo "[INFO] Bringing up CAN (500000)"
    canstart 500000
else
    echo "[WARN] canstart command not found; skipping CAN bring-up"
fi

# Validate executable
if [ ! -x "$BIN" ]; then
    echo "[ERROR] Executable not found or not executable:"
    echo "        $BIN"
    exit 1
fi

# Move to binary directory 
BIN_DIR=$(dirname "$BIN")
echo "Changing to executable directory: $BIN_DIR"
cd "$BIN_DIR" || { echo "[ERROR] Could not change to directory $BIN_DIR"; exit 1; }

echo "[INFO] Starting CAN RX process"
echo "[INFO] $(date)"

# Launch process
exec "$BIN"
