#!/bin/bash

# This script runs the shm_plotter_noqt.py with a specified CSV file.

# Get the absolute path of the script's directory
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
# Navigate to the project root directory
ROOT_DIR="$SCRIPT_DIR/.."

# Define the directory where the replayer and CSV files are located
LOGGER_DIR="$ROOT_DIR/src/app/shm_plotter"

# --- 2. Run the Replayer ---
echo "Changing to logger directory: $LOGGER_DIR"
cd "$LOGGER_DIR" || { echo "Error: Could not change to directory $LOGGER_DIR"; exit 1; }
python3 shm_plotter_noqt.py

echo "Logger finished."