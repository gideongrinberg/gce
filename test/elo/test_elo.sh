#!/usr/bin/env bash
set -euo pipefail

cutechess-cli \
  -engine name=GCE cmd=$(pwd)/cmake-build-release-homebrew/gce-uci tc=inf \
  -engine name=Stockfish cmd=stockfish tc=40/60 option.UCI_LimitStrength=true option.UCI_Elo=1350 \
  -each proto=uci book=$(pwd)/test/elo/Human.bin bookdepth=12 \
  -rounds 8 \
  -draw movenumber=40 movecount=10 score=5 \
  -pgnout tournament.pgn \
  -concurrency 4
