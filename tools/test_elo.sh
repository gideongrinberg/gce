#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <engine directory>"
  exit 1
fi

echo "This requires the rustic chess engine. Please download it."

TEST_PATH="$(cd "$(dirname "$0")" && pwd)/elo"
mkdir -p "$TEST_PATH"

if [ ! -f "$TEST_PATH/baron30.bin" ]; then
  echo "Downloading opening book"
  curl --silent https://maughancdn.s3.amazonaws.com/chess/The%20Baron/baronbook30.zip | funzip > "$TEST_PATH/baron30.bin"
else
  echo "Using downloaded opening book"
fi

# shellcheck disable=SC2086
cutechess-cli \
  -engine cmd="rustic" name=Rustic \
  -engine cmd="$1/gce-uci" name=GCE \
  -each proto=uci tc=40/60+0.1 book="$TEST_PATH/baron30.bin" bookdepth=4 \
  -games 4 -rounds 2 -repeat 2 -maxmoves 200 \
  -concurrency 4 \
  -pgnout "$TEST_PATH/output.pgn"

