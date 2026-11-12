#!/usr/bin/env bash
# Build & run the host tests (no hardware needed).
#   ./tests/run.sh            # run every test
#   CC=clang ./tests/run.sh   # alternate compiler
set -euo pipefail
cd "$(dirname "$0")/.."

CC=${CC:-gcc}
LIB=(firmware/main/scanner_common.c
     firmware/main/scanner_oui.c
     firmware/main/scanner_classify.c
     firmware/main/scanner_table.c
     firmware/main/scanner_proto.c)

OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

failed=0
for src in tests/test_*.c; do
  name=$(basename "$src" .c)
  if ! "$CC" -std=c11 -Wall -I firmware/main "$src" "${LIB[@]}" -o "$OUT/$name"; then
    echo "BUILD FAIL  $name"
    failed=1
    continue
  fi
  if ! "$OUT/$name" > "$OUT/$name.out" 2>&1; then
    echo "TEST FAIL   $name"
    cat "$OUT/$name.out"
    failed=1
    continue
  fi
  if grep -q '^FAIL' "$OUT/$name.out"; then
    echo "TEST FAIL   $name (exit 0 but FAIL lines present)"
    cat "$OUT/$name.out"
    failed=1
    continue
  fi
  echo "OK          $name  ($(grep -c '^PASS' "$OUT/$name.out") checks)"
done

if [ "$failed" -eq 0 ]; then
  echo "ALL TESTS OK"
else
  exit 1
fi
