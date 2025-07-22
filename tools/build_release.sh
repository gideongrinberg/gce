#!/usr/bin/env bash

PLATFORM=$(echo "$(uname -s)-$(uname -m)" | tr '[:upper:]' '[:lower:]')
BUILD_DIR="build-$PLATFORM"
DIST_DIR="dist-$PLATFORM"
BIN_DIR="gce-$PLATFORM"
mkdir -p "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"
cmake --install "$BUILD_DIR" --prefix "$DIST_DIR"
mv "$DIST_DIR/bin" "$BIN_DIR"
tar -czvf "$BIN_DIR".tar.gz "$BIN_DIR"

rm -rf "$DIST_DIR" "$BIN_DIR"