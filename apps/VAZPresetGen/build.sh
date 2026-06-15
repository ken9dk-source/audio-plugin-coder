#!/usr/bin/env bash
# Build the VAZ AI Preset Generator on macOS / Linux.
#   chmod +x build.sh && ./build.sh
# macOS note: use python.org Python 3 or `brew install python-tk` (system Python lacks a usable Tk).
set -euo pipefail
cd "$(dirname "$0")"

PY="${PYTHON:-python3}"

echo "[1/3] Installing dependencies..."
"$PY" -m pip install --upgrade pip
"$PY" -m pip install --upgrade customtkinter pyinstaller

echo "[2/3] Running engine self-test..."
"$PY" tests/test_engine.py

echo "[3/3] Building with PyInstaller..."
"$PY" -m PyInstaller --noconfirm --clean VAZPresetGen.spec

if [[ "$(uname)" == "Darwin" ]]; then
    echo ""
    echo "BUILD OK -> dist/VAZ Preset Generator.app   (drag to /Applications; right-click > Open the first time)"
else
    echo ""
    echo "BUILD OK -> dist/VAZ Preset Generator   (chmod +x and run)"
fi
