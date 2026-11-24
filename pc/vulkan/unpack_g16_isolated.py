"""Export the v2 probe's raw prefix and endpoint records for the T930 reader."""

import argparse
import json
from pathlib import Path
import struct


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("receipt", type=Path)
    parser.add_argument("out", type=Path)
    args = parser.parse_args()
    raw = args.receipt.read_bytes()
    status, *words = struct.unpack("<I25f", raw)
    args.out.mkdir()
    (args.out / "contact.bin").write_bytes(raw[:36])
    extras = dict(zip((
        "gapStart", "gapEnd", "gapLo", "gapHi", "normalX", "normalY", "wallVx", "wallVy",
        "contactX", "contactY", "gapErrorBound", "loNx", "loNy", "loWallSpeed", "hiNx", "hiNy", "hiWallSpeed",
    ), words[8:], strict=True))
    (args.out / "witness.json").write_text(json.dumps(dict(status=status, **extras), indent=2) + "\n")
    if status == 1 and extras["gapErrorBound"] > 0:
        samples = [
            dict(t=t, gap=extras[f"gap{title}"], normal=[extras[f"{key}Nx"], extras[f"{key}Ny"]],
                 wallSpeed=extras[f"{key}WallSpeed"])
            for t, title, key in zip(words[6:8], ("Lo", "Hi"), ("lo", "hi"), strict=True)
        ]
        (args.out / "near.json").write_text(json.dumps(samples, indent=2) + "\n")


if __name__ == "__main__":
    main()
