#!/usr/bin/env bash
set -euo pipefail
SHORT_HASH=$(git rev-parse --short HEAD)

emcmake cmake -DCMAKE_BUILD_TYPE=Release -S . -B build && cmake --build build --config Release
mkdir -p dist
find ./build -name "index.*" -maxdepth 1 -exec cp {} dist/ \;

cd dist/
touch .nojekyll
git init .
git add .
git commit -m "Deploy ${SHORT_HASH}"
git push --force https://github.com/gideongrinberg/gce.git HEAD:gh-pages
cd ..
rm -rf dist