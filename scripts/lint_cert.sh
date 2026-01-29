#!/usr/bin/env bash
set -euo pipefail

FILES=$(git ls-files \
  'nnist/src/**/*.cpp' \
  'nnist/include/**/*.h' \
)

clang-tidy-14 \
  --use-color \
  -p="build-linux" \
  -warnings-as-errors='*' \
  $FILES
