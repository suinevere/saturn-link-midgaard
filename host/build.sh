#!/bin/sh
set -e
cd "$(dirname "$0")"
CC=${CC:-cc}
CORE=../core
ADP=../adapters

LIBS=""
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) LIBS="-lws2_32" ;;
esac

INC="-I. -I$ADP/tcp"
for d in "$CORE"/*/; do INC="$INC -I$d"; done

$CC -std=c11 -Wall -Wextra -g $INC \
    main_host.c surface_term.c \
    "$CORE"/*/*.c $ADP/tcp/transport_tcp.c \
    $LIBS -o cmud-host
echo "built host/cmud-host"
