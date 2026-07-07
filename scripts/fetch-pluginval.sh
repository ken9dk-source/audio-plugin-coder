#!/usr/bin/env bash
# Download the version-pinned pluginval binary into _tools/pluginval/.
# The binary is gitignored; run this once per machine.
set -euo pipefail

VERSION="${1:-v1.0.4}"
INSTALL_PATH="_tools/pluginval"

if [[ "$(uname)" == "Darwin" ]]; then
    ASSET="pluginval_macOS.zip"
    BIN="$INSTALL_PATH/pluginval.app/Contents/MacOS/pluginval"
else
    ASSET="pluginval_Linux.zip"
    BIN="$INSTALL_PATH/pluginval"
fi

if [[ -e "$BIN" ]]; then
    echo "pluginval already present at $BIN"
    exit 0
fi

URL="https://github.com/Tracktion/pluginval/releases/download/$VERSION/$ASSET"
TMP_ZIP="$(mktemp -t pluginval.XXXXXX).zip"

echo "Fetching pluginval $VERSION ..."
curl -fsSL "$URL" -o "$TMP_ZIP"
mkdir -p "$INSTALL_PATH"
unzip -o -q "$TMP_ZIP" -d "$INSTALL_PATH"
rm -f "$TMP_ZIP"

[[ -e "$BIN" ]] || { echo "pluginval binary not found after extraction" >&2; exit 1; }
echo "pluginval $VERSION installed at $BIN"
