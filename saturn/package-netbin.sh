#!/bin/sh
set -e
cd "$(dirname "$0")"

NETBIN_MAX_BYTES=409600

CD_NAME=$(sed -n 's/^[[:space:]]*CD_NAME[[:space:]]*=[[:space:]]*//p' makefile | head -n1 | sed 's/[[:space:]]*$//')
ELF="BuildDrop/${CD_NAME}.elf"
OUT="BuildDrop/coffeemud.netbin"

if [ ! -s "$ELF" ]; then
    echo "package-netbin: no ELF at $ELF -- did the link fail?" >&2
    exit 1
fi

sh2eb-elf-objcopy -O binary \
    -R WORK_AREA* -R COMMAND_BUF* -R SYSTEM_START* -R SYSTEM_END* \
    "$ELF" "$OUT"

sz=$(stat -c%s "$OUT" 2>/dev/null || stat -f%z "$OUT")
echo "coffeemud.netbin: $sz bytes (ceiling $NETBIN_MAX_BYTES)"

if [ "$sz" -gt "$NETBIN_MAX_BYTES" ]; then
    echo "netbin exceeds the PlanetWeb loader ceiling" >&2
    exit 1
fi
