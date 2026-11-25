#!/bin/sh
set -eu

CXX="${CXX:-c++}"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

# Break only the W/X MADD dispatch comparison. The production mask cannot
# produce this deliberately impossible masked opcode, so MADD must fault.
sed 's/multiply_opcode == 0x1B000000u/multiply_opcode == 0x1B004000u/' \
  "$ROOT/src/silica/cpu.cpp" > "$TMP/cpu-mutated.cpp"

if ! grep -q 'multiply_opcode == 0x1B004000u' "$TMP/cpu-mutated.cpp"; then
  echo "MADD mutation was not applied; refusing to report a mutation result" >&2
  exit 2
fi

"$CXX" -std=c++20 -O2 -Wall -Wextra -Wpedantic -I"$ROOT/src" \
  "$ROOT/tests/native_core_test.cpp" "$TMP/cpu-mutated.cpp" \
  -o "$TMP/mutated-native-test"

set +e
"$TMP/mutated-native-test" >"$TMP/stdout.log" 2>"$TMP/stderr.log"
status=$?
set -e

if [ "$status" -eq 0 ]; then
  echo "SURVIVED: MADD decode mutant passed the native regression suite" >&2
  exit 1
fi

echo "KILLED: MADD decode mutant rejected by native regression tests (exit $status)"
