#!/bin/bash

# This script runs the csv_replayer.py with a specified CSV file.

# Get the absolute path of the script's directory
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
# Navigate to the project root directory
ROOT_DIR="$SCRIPT_DIR/.."

# Define the directory where the replayer and CSV files are located
REPLAYER_DIR="$ROOT_DIR/src/app/shm_plotter"

# --- 1. Check for CSV file argument ---
if [ -z "$1" ]; then
    echo "Usage: $0 <csv_filename>"
    echo ""
    echo "Please provide the name of the CSV file to replay."
    echo "Available CSV files in $REPLAYER_DIR:"
    # List only the basename of the csv files
    ls -1 "$REPLAYER_DIR"/*.csv | xargs -n 1 basename
    exit 1
fi

# The python script is run from its own directory, so we pass just the filename.
CSV_FILENAME="$1"
CSV_FILE_PATH="$REPLAYER_DIR/$CSV_FILENAME"

# Check if the provided CSV file exists before changing directory
if [ ! -f "$CSV_FILE_PATH" ]; then
    echo "Error: CSV file not found: $CSV_FILE_PATH"
    echo "Please make sure the file '$CSV_FILENAME' exists in the '$REPLAYER_DIR' directory."
    exit 1
fi

# --- 2. Run the Replayer ---
echo "Changing to replayer directory: $REPLAYER_DIR"
cd "$REPLAYER_DIR" || { echo "Error: Could not change to directory $REPLAYER_DIR"; exit 1; }

echo "Starting the CSV replayer with file: $CSV_FILENAME"
python3 csv_replayer.py "$CSV_FILENAME"

echo "Replayer finished."
