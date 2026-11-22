"""Compile the Windows BOOT diagnostic presentation shaders."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--compiler", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parent
lines = ["#pragma once", "#include <cstdint>", "namespace grs::pc {"]
receipt = {}
for stage, name in (("vert", "kBootViewVertex"), ("frag", "kBootViewFragment")):
    source = root / f"boot_view.{stage}"
    spv = args.out / f"boot_view.{stage}.spv"
    subprocess.run([str(args.compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val",
                    "-o", str(spv), str(source)], check=True)
    raw = spv.read_bytes()
    words = struct.unpack(f"<{len(raw) // 4}I", raw)
    lines.append(f"inline constexpr uint32_t {name}[] = {{")
    for i in range(0, len(words), 8):
        lines.append("  " + ",".join(hex(value) for value in words[i:i+8]) + ",")
    lines.append("};")
    receipt[stage] = {"sourceSHA": hashlib.sha256(source.read_bytes()).hexdigest(),
                      "moduleSHA": hashlib.sha256(raw).hexdigest(), "bytes": len(raw)}
lines.append("}")
(args.out / "pc_view_spirv.h").write_text("\n".join(lines)+"\n", encoding="utf-8")
(args.out / "pc_view_assets.json").write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf-8")
