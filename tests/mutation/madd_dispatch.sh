#!/bin/sh
# Deterministic mutation check: prove the MADD regression test detects a broken decode.
# SPDX-License-Identifier: MIT
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
compiler=${CXX:-c++}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/neuro-silica-madd-mutant.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

mkdir -p "$tmp/src/silica" "$tmp/tests"
cp "$root/src/silica/cpu.hpp" "$tmp/src/silica/cpu.hpp"
cp "$root/tests/native_core_test.cpp" "$tmp/tests/native_core_test.cpp"

# Mutate only the valid MADD opcode match value into an impossible value.
if ! grep -Fq '0x9B000000u' "$root/src/silica/cpu.cpp"; then
  echo "mutation precondition failed: expected MADD match value not found" >&2
  exit 2
fi
sed 's/0x9B000000u/0x9B007000u/' "$root/src/silica/cpu.cpp" > "$tmp/src/silica/cpu.cpp"
if cmp -s "$root/src/silica/cpu.cpp" "$tmp/src/silica/cpu.cpp"; then
  echo "mutation failed to change the source" >&2
  exit 2
fi

"$compiler" -std=c++20 -O0 -g -Wall -Wextra -Wpedantic \
  -I"$tmp/src" "$tmp/tests/native_core_test.cpp" "$tmp/src/silica/cpu.cpp" \
  -o "$tmp/mutant"

if "$tmp/mutant"; then
  echo "ERROR: MADD decode mutant survived the native regression suite" >&2
  exit 1
else
  echo "PASS: MADD decode mutant was killed by the native regression suite"
fi
