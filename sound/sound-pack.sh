#!/bin/sh
set -eu

TARGET=${1:?usage: sound-pack.sh dreamcast OUT_DIR | saturn OUT_PAK}
OUT=${2:?usage: sound-pack.sh dreamcast OUT_DIR | saturn OUT_PAK}

PACK_URL="https://www.coffeemud.net/sounds.zip"
PACK_SHA256="5dba010173e615cde3307fb574b1fd5916bb4afd1a3d20b657ab3a556eac1904"
MAX_SAMPLES=65534
DC_TOP_RATE=22050
SATURN_RATE=11025
HERE=$(cd "$(dirname "$0")" && pwd)

case "$TARGET" in
    dreamcast|saturn) ;;
    *) echo "unknown target $TARGET" >&2; exit 2 ;;
esac

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

if [ -n "${SOUND_PACK_ZIP:-}" ]; then
    cp "$SOUND_PACK_ZIP" "$WORK/sounds.zip"
else
    curl -fsSL -o "$WORK/sounds.zip" "$PACK_URL"
fi
GOT=$(sha256sum "$WORK/sounds.zip" | cut -d' ' -f1)
if [ "$GOT" != "$PACK_SHA256" ]; then
    echo "sounds.zip is $GOT, expected $PACK_SHA256" >&2
    exit 1
fi
unzip -q "$WORK/sounds.zip" -d "$WORK/wav"
find "$WORK/wav" -iname '*.wav' | sort > "$WORK/list"
[ -s "$WORK/list" ] || { echo "sounds.zip holds no WAV files" >&2; exit 1; }

if [ "$TARGET" = dreamcast ]; then DEST=$OUT; else DEST=$WORK/raw; fi
mkdir -p "$DEST"

while read -r wav; do
    name=$(basename "$wav")
    name=$(printf '%s' "${name%.*}" | tr '[:upper:]' '[:lower:]')
    if [ "$TARGET" = dreamcast ]; then
        secs=$(ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 "$wav")
        rate=$(awk -v d="$secs" -v m="$MAX_SAMPLES" -v t="$DC_TOP_RATE" \
            'BEGIN { x = (d > 0) ? int(m / d) : t; if (x > t) x = t; if (x < 4000) x = 4000; print x }')
        ffmpeg -nostdin -loglevel error -y -i "$wav" \
            -af "aresample=$rate,atrim=end_sample=$MAX_SAMPLES" -ac 1 -ar "$rate" -c:a pcm_s16le \
            -map_metadata -1 -fflags +bitexact -flags:a +bitexact "$DEST/$name.wav"
    else
        ffmpeg -nostdin -loglevel error -y -i "$wav" \
            -af "aresample=$SATURN_RATE,atrim=end_sample=$MAX_SAMPLES" -ac 1 -ar "$SATURN_RATE" \
            -f s8 "$DEST/$name.raw"
    fi
done < "$WORK/list"

want=$(wc -l < "$WORK/list")
count=$(find "$DEST" -type f | wc -l)
[ "$count" -eq "$want" ] || { echo "converted $count of $want sounds" >&2; exit 1; }

if [ "$TARGET" = saturn ]; then
    mkdir -p "$(dirname "$OUT")"
    python3 "$HERE/pack_saturn.py" "$DEST" "$OUT" "$SATURN_RATE"
else
    echo "$count dreamcast sounds, $(du -sk "$OUT" | cut -f1)KB"
fi
echo "from sounds.zip sha256 $GOT"
