#!/bin/bash

# This script builds the driving style analyzer and runs it from the build directory.

# Get the absolute path of the script's directory
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
# Navigate to the project root directory (assuming the script is in 'launch')
ROOT_DIR="$SCRIPT_DIR/.."

# Define key directories
ANALYZER_SRC_DIR="$ROOT_DIR/src/app/driving_style_analyzer"
BUILD_DIR="$ROOT_DIR/build"
ANALYZER_EXEC_PATH="$BUILD_DIR/analyzer"

# --- 1. Build Step ---
echo "Changing to analyzer source directory: $ANALYZER_SRC_DIR"
cd "$ANALYZER_SRC_DIR" || { echo "Error: Could not change to directory $ANALYZER_SRC_DIR"; exit 1; }

echo "Building the analyzer..."
make

# Check if the build was successful
if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi
echo "Build successful. Executable created at $ANALYZER_EXEC_PATH"

# --- 2. Run Step ---
echo "Changing to build directory: $BUILD_DIR"
cd "$BUILD_DIR" || { echo "Error: Could not change to directory $BUILD_DIR"; exit 1; }

# Check if the executable exists
if [ ! -f "$ANALYZER_EXEC_PATH" ]; then
    # The path from the current (build) directory
    if [ -f "./analyzer" ]; then
        ANALYZER_EXEC_PATH="./analyzer"
    else
        echo "Error: Executable 'analyzer' not found in $BUILD_DIR"
        exit 1
    fi
fi

echo "Running the analyzer..."
$ANALYZER_EXEC_PATH

echo "Analyzer finished."