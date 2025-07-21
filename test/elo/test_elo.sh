#!/usr/bin/env bash
set -euo pipefail

SCRIPT_PATH="$(cd "$(dirname "$0")" && pwd)"
GCE_PATH="$SCRIPT_PATH/../../cmake-build-release-homebrew/gce-uci"
if [ ! -f "/path/to/file" ]; then
  curl --silent https://digilander.libero.it/taioscacchi/archivio/Titans.zip | funzip > Titans.bin
fi

cutechess-cli \
  -engine name=GCE cmd="$GCE_PATH" \
  -engine name=Sunfish cmd="$SCRIPT_PATH/sunfish.sh" \
  -each proto=uci book=$SCRIPT_PATH/Titans.bin bookdepth=12 tc=inf \
  -rounds 8 \
  -draw movenumber=40 movecount=10 score=5 \
  -pgnout tournament.pgn \
  -concurrency 4 \
  -debug
