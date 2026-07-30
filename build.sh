#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
INSTALL_DIR="${1:-$PROJECT_DIR/dist}"

echo "Building p2pServe..."
cmake -GNinja "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
ninja -C "$BUILD_DIR"

echo "Installing to $INSTALL_DIR..."
cmake --install "$BUILD_DIR"

echo ""
echo "Done. Run with:"
echo "  $INSTALL_DIR/bin/p2pServe"
echo ""
echo "Config file installed to:"
echo "  $INSTALL_DIR/etc/data.jsonl"
