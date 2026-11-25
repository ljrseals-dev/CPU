#!/bin/sh
set -eu

CXX="${CXX:-c++}"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

# Mutate the MADD classifier, not the execution arithmetic. Overlay the
# altered header ahead of src/ so the production sources remain untouched.
mkdir -p "$TMP/include/silica"
sed 's/multiply_opcode == 0x1B000000u/multiply_opcode == 0x1B004000u/' \
  "$ROOT/src/silica/decode_cache.hpp" > "$TMP/include/silica/decode_cache.hpp"

if ! grep -q 'multiply_opcode == 0x1B004000u' "$TMP/include/silica/decode_cache.hpp"; then
  echo "MADD classifier mutation was not applied; refusing to report a mutation result" >&2
  exit 2
fi

"$CXX" -std=c++20 -O2 -Wall -Wextra -Wpedantic \
  -I"$TMP/include" -I"$ROOT/src" \
  "$ROOT/tests/native_core_test.cpp" "$ROOT/src/silica/cpu.cpp" \
  -o "$TMP/mutated-native-test"

set +e
"$TMP/mutated-native-test" >"$TMP/stdout.log" 2>"$TMP/stderr.log"
status=$?
set -e

if [ "$status" -eq 0 ]; then
  echo "SURVIVED: MADD classifier mutant passed the native regression tests" >&2
  exit 1
fi

echo "KILLED: MADD classifier mutant rejected by native regression tests (exit $status)"
