#!/bin/bash

# Determine project root
SCRIPT_DIR=$(cd "$(dirname "$(readlink -f "$0")")" && pwd)
# Navigate to the project root directory 
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)

# Define key directories 
APP_DIR="$ROOT_DIR/src/app/traffic_light_detection"
ENTRY="$APP_DIR/main.py"
VENV_DIR="$ROOT_DIR/yoloenv"                      # project-local venv (fallback)
VENV_ACTIVATE="$VENV_DIR/bin/activate"
HOME_ACTIVATE="$HOME/yolo_env/yolo_env/bin/activate"  # alias target from bashrc

# Ensure entrypoint exists
if [ ! -f "$ENTRY" ]; then
    echo "[ERROR] Entrypoint not found:"
    echo "        $ENTRY"
    exit 1
fi

# Load bashrc so yoloenv alias is available
if [ -f "$HOME/.bashrc" ]; then
    # shellcheck disable=SC1090
    . "$HOME/.bashrc"
fi

# Activate virtual environment (prefer yoloenv alias from bashrc)
if command -v yoloenv >/dev/null 2>&1; then
    echo "[INFO] Activating virtualenv via alias: yoloenv"
    yoloenv
elif [ -f "$HOME_ACTIVATE" ]; then
    echo "[INFO] Activating virtualenv: $HOME_ACTIVATE"
    # shellcheck disable=SC1090
    . "$HOME_ACTIVATE"
elif [ -f "$VENV_ACTIVATE" ]; then
    echo "[INFO] Activating project-local virtualenv: $VENV_DIR"
    # shellcheck disable=SC1090
    . "$VENV_ACTIVATE"
elif command -v python3 >/dev/null 2>&1; then
    echo "[WARN] Virtualenv not found; using system python3"
else
    echo "[ERROR] No python3 or virtualenv available"
    exit 1
fi

# Move to app directory for relative imports
echo "Changing to app directory: $APP_DIR"
cd "$APP_DIR" || { echo "[ERROR] Could not change to $APP_DIR"; exit 1; }

echo "[INFO] Starting traffic light detection"
exec python3 "$ENTRY"
