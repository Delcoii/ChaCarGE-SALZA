#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GUI_DIR="$ROOT_DIR/src/app/infotainment_gui"

mkdir -p ~/Log
cd "$GUI_DIR"
make
./app > ~/Log/app.log 2>&1 &
