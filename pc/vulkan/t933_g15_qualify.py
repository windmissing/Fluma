"""Prepare the G15 opcode 23 single-role and fixed-prefix qualification pairs."""

import argparse
import hashlib
import json
from pathlib import Path
import struct


ORIGINAL = "29e0d466a922536acce6727cb4a768a974f333ecaa7faa1875a77202a0ee3e24"
STRIPPED = "5f8fcc775e1c1bd0fae0078ec979a011655531e7b708891c94f9085a77d34c01"
PREFIXES = {
    "dm2": "d5367cd29c885e797fe350c0ea76fa358772983f9c8c3ec3a33a80d27fc03ff8",
    "g15": "bf03a518742740996c7b8741e9dc54865a7ba212147b809a66b67c78b4d7c880",
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def strip_source_extension(blob):
    words = struct.unpack(f"<{len(blob)//4}I", blob)
    if words[0] != 0x07230203:
        raise ValueError("Invalid SPIR-V")
    cursor, removed, output = 5, 0, bytearray(blob[:20])
    while cursor < len(words):
        count, opcode = words[cursor] >> 16, words[cursor] & 65535
        if not count or cursor + count > len(words):
            raise ValueError("Invalid SPIR-V instruction")
        instruction = blob[4*cursor:4*(cursor+count)]
        if opcode == 4 and instruction[4:].rstrip(b"\0") == b"GL_EXT_shader_explicit_arithmetic_types_int64":
            removed += 1
        else:
            output.extend(instruction)
        cursor += count
    if removed != 1 or len(blob)-len(output) != 52 or sha(output) != STRIPPED:
        raise ValueError("OP23 extension delta changed")
    return bytes(output)


def prepare(args):
    out = args.out.resolve()
    exe, dm2, g15 = (path.resolve(strict=True) for path in (args.exe,args.dm2,args.g15))
    original = (g15 / "role_00000017.spv").read_bytes()
    if sha(original) != ORIGINAL or sha((dm2 / "role_00000017.spv").read_bytes()) != STRIPPED:
        raise ValueError("OP23 inputs do not match T931")
    stripped = strip_source_extension(original)
    for name, folder in (("dm2", dm2), ("g15", g15)):
        if sha(b"".join((folder / f"role_{i:08x}.spv").read_bytes() for i in range(23))) != PREFIXES[name]:
            raise ValueError(f"Prefix differs: {name}")
    out.mkdir()
    original_path, stripped_path = out / "op23-original.spv", out / "op23-stripped.spv"
    original_path.write_bytes(original)
    stripped_path.write_bytes(stripped)
    arms = {
        "source-extension": {"A": [str(original_path)], "B": [str(stripped_path)]},
        "prefix": {"A": [str(stripped_path), str(dm2)], "B": [str(stripped_path), str(g15)]},
    }
    (out / "plan.json").write_text(json.dumps(dict(exe=str(exe), exeSHA=sha(exe.read_bytes()),
        targetSHA=dict(original=ORIGINAL, stripped=STRIPPED), prefixSHA=PREFIXES,
        arms=arms, order="ABBA"), indent=2)+"\n", encoding="utf-8")
    if args.run:
        from qualify_role import supervise
        from t916_g13_compare import LoadedImages
        for comparison, sides in arms.items():
            for sequence, arm in enumerate("ABBA", 1):
                folder = out / f"{comparison}-{sequence}-{arm}"
                folder.mkdir()
                supervise(exe, "OP_23", folder, False, True,
                          argv=["--qualify-g15-op23", *sides[arm]], cwd=out,
                          observe_modules=LoadedImages(), watched_role="OP_23")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "dm2", "g15", "out"):
        parser.add_argument(name, type=Path)
    parser.add_argument("--run", action="store_true")
    prepare(parser.parse_args())
