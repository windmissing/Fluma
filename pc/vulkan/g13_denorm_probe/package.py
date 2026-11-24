"""Publish the self-contained Windows brief diagnostic."""

import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("out", type=Path)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    exe = (args.build / "G13-FP32-Probe.exe").read_bytes()
    manifest = {"version": "G13-FP32-v2", "exeSHA256": hashlib.sha256(exe).hexdigest()}
    for name in ("probe.spv", "op48-default.spv"):
        shader = (args.build / name).read_bytes()
        if shader not in exe:
            raise ValueError(f"{name} is not embedded in the executable")
        manifest[name] = hashlib.sha256(shader).hexdigest()
    with zipfile.ZipFile(args.out, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("manifest.json", json.dumps(manifest, indent=2) + "\n")
        archive.writestr("G13-FP32-Probe.exe", exe)
        for name in ("Run-Test.cmd", "Run-Test.ps1", "README.md"):
            archive.writestr(name, (here / name).read_bytes())


if __name__ == "__main__":
    main()
