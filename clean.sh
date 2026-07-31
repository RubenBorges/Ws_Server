#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
DIST_DIR="$PROJECT_DIR/dist"


echo "Removing build directory..."
rm -rf "$BUILD_DIR"
echo "Removing Dist directory..."
rm -rf "$DIST_DIR"
echo "Clean."
