#!/usr/bin/env bash
set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/game24-timer.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

"${CC:-cc}" -std=c99 -Wall -Wextra -Wpedantic -O2 \
  "$script_dir/game24.c" -o "$build_dir/game24" -lm
python3 "$script_dir/timer.py" "$build_dir/game24"
