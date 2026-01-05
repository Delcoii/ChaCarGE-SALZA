#!/bin/bash

# Execute file
BIN="$HOME/git/CHACARGE-SALZA/src/bsw/ipc/src/can_rx_process/can_rx_process"

# Execute file check
if [ ! -x "$BIN" ]; then
    echo "[ERROR] Executable not found or not executable:"
    echo "        $BIN"
    exit 1
fi

echo "[INFO] CAN RX Process Start!!"
echo "[INFO] $(date)"

# process Execute
exec "$BIN" 

