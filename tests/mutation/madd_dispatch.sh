#!/bin/sh
# Deterministic mutation check: prove the MADD regression test detects a broken decode.
# SPDX-License-Identifier: MIT
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
compiler=${CXX:-c++}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/neuro-silica-madd-mutant.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

mkdir -p "$tmp/src/silica" "$tmp/tests/mutation"
cp "$root/src/silica/cpu.hpp" "$tmp/src/silica/cpu.hpp"
cp "$root/tests/mutation/madd_dispatch_test.cpp" "$tmp/tests/mutation/madd_dispatch_test.cpp"

# First prove the focused regression passes against the unmodified implementation.
"$compiler" -std=c++20 -O0 -g -Wall -Wextra -Wpedantic \
  -I"$root/src" "$root/tests/mutation/madd_dispatch_test.cpp" \
  "$root/src/silica/cpu.cpp" -o "$tmp/baseline"
"$tmp/baseline"

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
  -I"$tmp/src" "$tmp/tests/mutation/madd_dispatch_test.cpp" \
  "$tmp/src/silica/cpu.cpp" -o "$tmp/mutant"

if "$tmp/mutant"; then
  echo "ERROR: MADD decode mutant survived the focused regression" >&2
  exit 1
else
  echo "PASS: MADD decode mutant was killed by the focused regression"
fi
