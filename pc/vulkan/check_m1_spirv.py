"""Port previously assembled M1 commit shader declarations to Vulkan for static review.

Run test/gpu_resident/prepare_t280_commit.py first. No controller or execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

UNIFORMS = re.compile(r"\buniform\s+(uint|int|uvec2)\s+([A-Za-z_]\w*(?:,[A-Za-z_]\w*)*);")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def compile_stage(compiler, source, out):
    raw = source.read_bytes().replace(b"\r\n", b"\n")
    text = raw.decode()
    if text.count("#version 310 es") != 1:
        raise ValueError("M1_ES_VERSION_REQUIRED")
    members = []
    offsets = {}
    offset = 0
    for match in UNIFORMS.finditer(text):
        kind, group = match.groups()
        for name in group.split(","):
            if name in offsets:
                raise ValueError("M1_DUPLICATE_UNIFORM")
            size = 8 if kind == "uvec2" else 4
            offset = (offset + size - 1) // size * size
            offsets[name] = offset
            members.append(f" {kind} {name};")
            offset += size
    if not members:
        raise ValueError("M1_UNIFORM_INTERFACE_MISSING")
    text = UNIFORMS.sub("", text)
    start = text.index("const uint ")
    ubo = "layout(std140,set=0,binding=4) uniform M1Uniforms {\n" + "\n".join(members) + "\n};\n"
    text = text[:start] + ubo + text[start:]
    text = text.replace("#version 310 es", "#version 450", 1)
    ported = out/source.name
    ported.write_bytes(text.encode())
    spirv = ported.with_suffix(".spv")
    result = subprocess.run([str(compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis",
                             "-o", str(spirv), str(ported)], check=True, capture_output=True, text=True)
    asm = result.stdout
    expected = [(str(i), str(at)) for i, at in enumerate(offsets.values())]
    if (re.findall(r"OpMemberDecorate %M1Uniforms (\d+) Offset (\d+)", asm) != expected or
        not re.search(r"OpExecutionMode %\S+ LocalSize 64 1 1", asm) or
        re.search(r"validation.*(fail|unavailable|not available)|\berror:", asm+result.stderr, re.I)):
        raise ValueError("M1_REFLECTION_OR_VALIDATION")
    if sorted(re.findall(r"OpDecorate %\S+ Binding (\d+)", asm)) != list("01234"):
        raise ValueError("M1_DESCRIPTOR_INTERFACE")
    return dict(canonicalSHA=sha(raw), sourceSHA=sha(ported.read_bytes()), spvSHA=sha(spirv.read_bytes()),
                sourceBytes=ported.stat().st_size, spvBytes=spirv.stat().st_size, offsets=offsets,
                blockBytes=(offset+15)//16*16, dispatch="NOT_RUN")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assembled", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    roles = {name: compile_stage(args.compiler, args.assembled/(name+".comp"), args.out)
             for name in ("full_source", "commit")}
    (args.out/"m1_static.json").write_text(json.dumps(roles, indent=2)+"\n", encoding="utf-8")
