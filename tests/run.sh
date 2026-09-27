#!/bin/sh
set -e
cd "$(dirname "$0")"

CC=${CC:-cc}
CORE=../core
INC="-I. -Isupport"
for d in "$CORE"/*/; do INC="$INC -I$d"; done
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

LIBS=""
for f in "$CORE"/*/*.c support/*.c; do
    [ -f "$f" ] && LIBS="$LIBS $f"
done

fail=0
for t in test_*.c; do
    name=$(basename "$t" .c)
    if ! $CC -std=c11 -Wall -Wextra -Werror -g $INC "$t" $LIBS -o "$OUT/$name"; then
        echo "BUILD FAIL $name"
        fail=1
        continue
    fi
    "$OUT/$name" || fail=1
done

[ $fail -eq 0 ] && echo "ALL TESTS PASSED"
exit $fail
