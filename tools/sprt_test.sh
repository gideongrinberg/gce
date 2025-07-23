#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 2 ]; then
  echo "Usage: $0 <new engine directory> <predicted elo gain>"
  exit 1
fi

BASELINE_VERSION="v0.2.0"
BASE_URL="https://github.com/gideongrinberg/gce/releases/download/$BASELINE_VERSION"

TEST_PATH="$(cd "$(dirname "$0")" && pwd)/sprt"
mkdir -p "$TEST_PATH"

if [ ! -f "$TEST_PATH/gce-uci" ]; then
  echo "Downloading version $BASELINE_VERSION for baseline"
  FILE=""
  OS="$(uname -s)"
  ARCH="$(uname -m)"
  if [ "$OS" = "Darwin" ]; then
      if [ "$ARCH" = "arm64" ]; then
          FILE="gce-darwin-arm64.tar.gz"
      elif [ "$ARCH" = "x86_64" ]; then
          FILE="gce-darwin-x86_64.tar.gz"
      else
          echo "Unsupported architecture: $ARCH"
          exit 1
      fi
  elif [ "$OS" = "Linux" ]; then
      if [ "$ARCH" = "x86_64" ]; then
          FILE="gce-linux-x86_64.tar.gz"
      else
          echo "Unsupported architecture: $ARCH"
          exit 1
      fi
  else
      echo "Unsupported operating system: $OS"
      exit 1
  fi

  curl -JLO --silent "$BASE_URL/$FILE"
  NAME="${FILE%.tar.gz}"
  tar -xzf "$FILE" -C "$TEST_PATH" --strip-components=1 "$NAME/gce-uci"
  chmod +x "$TEST_PATH/gce-uci"
  rm -f "$FILE"
else
  echo "Using downloaded $BASELINE_VERSION executable as baseline"
fi

if [ ! -f "$TEST_PATH/baron30.bin" ]; then
  echo "Downloading opening book"
  curl --silent https://maughancdn.s3.amazonaws.com/chess/The%20Baron/baronbook30.zip | funzip > "$TEST_PATH/baron30.bin"
else
  echo "Using downloaded opening book"
fi

# shellcheck disable=SC2086
cutechess-cli \
  -engine cmd="$TEST_PATH/gce-uci" name=Base \
  -engine cmd="$1/gce-uci" name=New \
  -each proto=uci tc=inf/10+0.1 book="$TEST_PATH/baron30.bin" bookdepth=4 \
  -sprt elo0=0 elo1=$2 alpha=0.05 beta=0.05 \
  -games 2 -rounds 2500 -repeat 2 -maxmoves 200 \
  -concurrency 4 \
  -ratinginterval 10 \
  -pgnout "$TEST_PATH/output.pgn"

