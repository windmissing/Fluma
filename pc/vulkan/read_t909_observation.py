"""Extract physical and pixel evidence from a T909 observation directory."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct


def words(raw):
    return struct.unpack("<" + "I" * (len(raw) // 4), raw)


def wide(values, index):
    return values[index] | (values[index + 1] << 32)


def archive(path, packet_key=None):
    raw = path.read_bytes()
    if len(raw) < 72 or raw[:8] != b"M1PROOT1":
        raise ValueError(f"Wrong archive envelope: {path}")
    sizes = words(raw[8:40])
    payload = raw[72:]
    if sum(sizes) != len(payload) or any(sizes[4:]) or sizes[0] != 1024 or sizes[2] % 256 or sizes[3] != 31744:
        raise ValueError(f"Wrong section lengths: {path}")
    if sha256(payload).digest() != raw[40:72]:
        raise ValueError(f"Archive hash mismatch: {path}")
    offsets = [0]
    for size in sizes:
        offsets.append(offsets[-1] + size)
    tail = words(payload[:offsets[1]])
    nonce = wide(tail, 4)
    kind = tail[27]
    if kind not in (1, 2) or tail[28] != 1 or tail[52] > tail[53] or tail[53] * 256 != sizes[2]:
        raise ValueError(f"Invalid accepted root range: {path}")
    events = []
    for i in range(tail[52], tail[53]):
        row = words(payload[offsets[2] + i * 256:offsets[2] + (i + 1) * 256])
        if kind == 2 and (wide(row, 2) != nonce or row[7] != i + 1):
            raise ValueError(f"Wrong event identity: {path} row {i}")
        events.append(row)
    publication = words(payload[offsets[3]:offsets[4]])
    columns = {name: publication[5632 + i * 128:5632 + (i + 1) * 128]
               for i, name in enumerate(("S", "W", "eta", "J", "P"))}
    packet = None
    if packet_key:
        def field(component, i):
            return publication[3072 + component * 128 + i]

        for i in range(min(publication[7680 + 35], 128)):
            if (field(5, i), field(11, i)) == packet_key and field(0, i):
                packet = dict(id=packet_key[0], generation=packet_key[1], mass=field(0, i),
                              xyBits=[field(16, i), field(17, i)])
                break
    return dict(run=wide(tail, 6), nonce=nonce, geometry=wide(tail, 38), rootKind=kind,
                baseCommit=wide(tail, 16), commit=wide(tail, 20), sourceCommit=wide(tail, 18), slot=tail[35],
                generation=wide(tail, 36), eventStart=tail[52], eventEnd=tail[53], eventArena=tail[51],
                acceptedAngle256=struct.unpack("<i", struct.pack("<I", tail[107]))[0],
                columns=columns, events=events, packet=packet, file=path.name, payloadSHA=raw[40:72].hex())


def collect(directory, packet_key=None):
    roots = {}
    for file in sorted(directory.glob("root-*.m1root")):
        row = archive(file, packet_key)
        key = (row["run"], row["geometry"], row["nonce"])
        if key in roots:
            raise ValueError(f"Duplicate archive root: {key}")
        roots[key] = row
    images = []
    for line in (directory / "demo.jsonl").read_text(encoding="utf-8-sig").splitlines():
        row = json.loads(line)
        roi = row.get("m1Observation")
        frame = row.get("windowFrame")
        if row.get("state") != "M1_QUANTUM_PRESENT_QUEUED" or not roi or not frame:
            continue
        key = (int(frame["run"]), int(roi["geometry"]), int(roi["root"]))
        raw = bytes(roi["data"])
        if raw and sha256(raw).hexdigest() != roi["sha256"]:
            raise ValueError(f"ROI hash mismatch: {key}")
        root = roots.get(key)
        same = (root is not None and int(frame["acceptedRoot"]) == key[2]
                and int(frame["generation"]) == int(roi["generation"]) == root["generation"]
                and frame["publicationSlot"] == roi["slot"] == root["slot"]
                and frame["imageIndex"] == roi["imageIndex"]
                and all(frame[field] == 0 for field in ("submit", "fence", "present", "imagePresent")))
        center_in_roi = None
        if same and root["packet"]:
            x, y = (struct.unpack("<f", struct.pack("<I", w))[0] for w in root["packet"]["xyBits"])
            x, y = x / 128 * frame["width"], (1 - y / 256) * frame["height"]
            center_in_roi = any(left <= x < left + w and top <= y < top + h for left, top, w, h in roi["rects"])
        images.append(dict(run=key[0], geometry=key[1], nonce=key[2], sameRoot=bool(same),
                           packetCenterInRoi=center_in_roi,
                           roi=roi, window=frame))
    return dict(roots=list(roots.values()), images=images,
                firstMissingArchive=next((x["nonce"] for x in images if not x["sameRoot"]), None))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--packet", type=int, nargs=2, metavar=("ID", "GENERATION"))
    args = parser.parse_args()
    print(json.dumps(collect(args.directory, tuple(args.packet) if args.packet else None), separators=(",", ":")))
