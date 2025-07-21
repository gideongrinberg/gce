#!/usr/bin/env bash
set -euo pipefail

SCRIPT_PATH="$(cd "$(dirname "$0")" && pwd)"

if ! command -v uv >/dev/null 2>&1; then
  curl -LsSf https://astral.sh/uv/install.sh | sh
fi

if [ ! -d "$SCRIPT_PATH/sunfish/" ]; then
  cd "$SCRIPT_PATH" && git clone https://github.com/thomasahle/sunfish
fi

uv run --python pypy3.11 --with "tqdm==4.57.0,chess==1.9.4" "$SCRIPT_PATH/sunfish/sunfish.py"