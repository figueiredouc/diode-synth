#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD="$ROOT/build/Diode_artefacts/Release"
VERSION="1.0.0"
PKG_NAME="Diode-${VERSION}-macOS.pkg"
STAGING="$SCRIPT_DIR/pkg_root"

echo "Building Diode installer v${VERSION}..."

if [ ! -d "$BUILD/VST3/Diode.vst3" ]; then
    echo "Error: build first — run cmake --build build --config Release"
    exit 1
fi

rm -rf "$STAGING"
mkdir -p "$STAGING/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGING/Library/Audio/Plug-Ins/Components"

cp -r "$BUILD/VST3/Diode.vst3"       "$STAGING/Library/Audio/Plug-Ins/VST3/"
cp -r "$BUILD/AU/Diode.component"    "$STAGING/Library/Audio/Plug-Ins/Components/"

xattr -r -d com.apple.quarantine "$STAGING/Library/Audio/Plug-Ins/VST3/Diode.vst3"           2>/dev/null || true
xattr -r -d com.apple.quarantine "$STAGING/Library/Audio/Plug-Ins/Components/Diode.component" 2>/dev/null || true

pkgbuild \
    --root "$STAGING" \
    --identifier "pt.fibersight.diode" \
    --version "$VERSION" \
    --install-location "/" \
    "$SCRIPT_DIR/$PKG_NAME"

rm -rf "$STAGING"
echo "Done: installer/$PKG_NAME"
