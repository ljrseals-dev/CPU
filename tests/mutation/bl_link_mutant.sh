#!/bin/sh
set -eu

CXX="${CXX:-c++}"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

# Remove the BL link-register side effect while leaving the branch itself intact.
sed 's/0x80000000u) != 0/0x00000000u) != 0/' \
  "$ROOT/src/silica/cpu.cpp" > "$TMP/cpu-mutated.cpp"

if ! grep -q '0x00000000u) != 0' "$TMP/cpu-mutated.cpp"; then
  echo "BL mutation was not applied; refusing to report a mutation result" >&2
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
  echo "SURVIVED: missing BL link-register write passed the native suite" >&2
  exit 1
fi

echo "KILLED: BL link-register mutant rejected by native regression tests (exit $status)"
