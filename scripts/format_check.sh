#!/usr/bin/env bash
set -euo pipefail

FILES=$(git ls-files \
  'nnist/src/**/*.cpp' \
  'nnist/include/**/*.h' \
  'nnist/tests/**/*.cpp' \
)

if [ -z "$FILES" ]; then
  echo "No files to format-check."
  exit 0
fi

clang-format --dry-run --Werror $FILES
