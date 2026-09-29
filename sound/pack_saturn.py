import struct
import sys
from pathlib import Path

SECTOR = 2048
NAME_LEN = 24
ENTRY = struct.Struct(">24sIIHH")
HEADER = struct.Struct(">4sI")


def pad(data):
    return data + b"\0" * (-len(data) % SECTOR)


def main():
    src, out, rate = Path(sys.argv[1]), Path(sys.argv[2]), int(sys.argv[3])
    sounds = sorted(src.glob("*.raw"))
    if not sounds:
        raise SystemExit(f"no .raw sounds in {src}")
    index_bytes = HEADER.size + ENTRY.size * len(sounds)
    sector = -(-index_bytes // SECTOR)
    index, blobs = [], []
    for raw in sounds:
        name = raw.stem.encode("ascii")
        if len(name) > NAME_LEN:
            raise SystemExit(f"sound name too long for the pack: {raw.stem}")
        data = raw.read_bytes()
        index.append(ENTRY.pack(name, sector, len(data), rate, 0))
        blob = pad(data)
        blobs.append(blob)
        sector += len(blob) // SECTOR
    head = pad(HEADER.pack(b"SNDP", len(sounds)) + b"".join(index))
    out.write_bytes(head + b"".join(blobs))
    print(f"{out}: {len(sounds)} sounds, {out.stat().st_size // 1024}KB")


if __name__ == "__main__":
    main()
