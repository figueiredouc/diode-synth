#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD="$ROOT/build/Andre_artefacts/Release"
VERSION="1.0.0"
PKG_NAME="Andre-${VERSION}-macOS.pkg"
STAGING="$SCRIPT_DIR/pkg_root"

echo "Building Andre installer v${VERSION}..."

# Ensure plugins are built
if [ ! -d "$BUILD/VST3/Andre.vst3" ]; then
    echo "Error: build first — run cmake --build build --config Release"
    exit 1
fi

# Stage files into macOS system paths
rm -rf "$STAGING"
mkdir -p "$STAGING/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGING/Library/Audio/Plug-Ins/Components"

cp -r "$BUILD/VST3/Andre.vst3"          "$STAGING/Library/Audio/Plug-Ins/VST3/"
cp -r "$BUILD/AU/Andre.component"        "$STAGING/Library/Audio/Plug-Ins/Components/"

# Remove quarantine flags
xattr -r -d com.apple.quarantine "$STAGING/Library/Audio/Plug-Ins/VST3/Andre.vst3"      2>/dev/null || true
xattr -r -d com.apple.quarantine "$STAGING/Library/Audio/Plug-Ins/Components/Andre.component" 2>/dev/null || true

# Build pkg
pkgbuild \
    --root "$STAGING" \
    --identifier "pt.fibersight.andre" \
    --version "$VERSION" \
    --install-location "/" \
    "$SCRIPT_DIR/$PKG_NAME"

rm -rf "$STAGING"
echo "Done: installer/$PKG_NAME"
