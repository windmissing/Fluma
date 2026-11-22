"""Port previously assembled M1 commit shader declarations to Vulkan for static review.

Run test/gpu_resident/prepare_t280_commit.py first. No controller or execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "g13"))
from split_shader import CALL, source_units

UNIFORMS = re.compile(r"\buniform\s+(uint|int|uvec2)\s+([A-Za-z_]\w*(?:,[A-Za-z_]\w*)*);")
SOURCE_LIMIT = 49152


def sha(data):
    return hashlib.sha256(data).hexdigest()


def quantum_units(raw):
    definitions, units = source_units(raw.decode(), hooks=False)
    if definitions:
        raise ValueError("M1_UNEXPECTED_QUANTUM_MACRO")
    functions = {}
    for name, body in units:
        if name is not None:
            functions.setdefault(name, []).append(body)
    main, = functions.pop("main")
    first = "if(uPass<=20u){candidateMain();return;}"
    if main.count(first) != 1:
        raise ValueError("M1_QUANTUM_DISPATCH_BOUNDARY")
    candidate = "void main(){candidateMain();}"
    tail = main.replace(first, "")
    for name, passes, entry in (("quantum_candidate", range(21), candidate),
                                ("quantum_commit", range(21, 29), tail)):
        pending = CALL.findall(entry)
        used = set()
        while pending:
            fn = pending.pop()
            if fn in functions and fn not in used:
                used.add(fn)
                for body in functions[fn]:
                    pending.extend(CALL.findall(body))
        chunks = [body for fn, body in units if fn is None or fn in used]
        yield name, list(passes), ("\n".join(["#version 310 es", *chunks, entry])+"\n").encode()


def compile_stage(compiler, source, out, *, raw=None, bounded=False):
    if raw is None:
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
    if bounded and ported.stat().st_size > SOURCE_LIMIT:
        raise ValueError(f"{source.name}: source exceeds {SOURCE_LIMIT} bytes")
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
    parser.add_argument("--split-quantum", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    roles = {"full_source": compile_stage(args.compiler, args.assembled/"full_source.comp", args.out,
                                          bounded=True)}
    if args.split_quantum:
        raw = (args.assembled/"commit.comp").read_bytes().replace(b"\r\n", b"\n")
        for name, passes, part in quantum_units(raw):
            record = compile_stage(args.compiler, Path(name+".comp"), args.out, raw=part, bounded=True)
            roles[name] = dict(record, parentSHA=sha(raw), passes=passes)
        if roles["quantum_candidate"]["offsets"] != roles["quantum_commit"]["offsets"]:
            raise ValueError("M1_QUANTUM_INCONSISTENT_OFFSETS")
    else:
        roles["commit"] = compile_stage(args.compiler, args.assembled/"commit.comp", args.out)
    (args.out/"m1_static.json").write_text(json.dumps(roles, indent=2)+"\n", encoding="utf-8")
