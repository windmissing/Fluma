"""Build isolated Windows Vulkan DM2 and G11/R1 role and dispatch artifacts."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/glsl"))
from g9_boot import LIMIT, WARM, split_inputs

UNIFORM = re.compile(r"\buniform\s+(uint|int|uvec2)\s+([\w,]+)\s*;")
BINDING = re.compile(r"layout\(std430,\s*binding\s*=\s*([0-3])\)")
PUSH = ("uOp", "uRole", "uJslot", "uMicro", "uPass")
VIS_PUSH = ("uVisPhase", "uVisOldBank", "uVisNewBank", "uVisOrdinal")
VIS_ROOT = ROOT / "entry/src/main/cpp/gpu_resident/g11vis"


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def declarations(source, composite):
    fields = [(name, kind) for kind, names in UNIFORM.findall(source) for name in names.split(",")]
    push = PUSH + (VIS_PUSH if composite else ())
    types = dict(fields)
    if len(types) != len(fields) or any(types.get(name) != "uint" for name in push):
        raise ValueError("Uniform/dispatch ABI differs from the parent")
    offset = 0
    common = []
    for name, kind in fields:
        if name in push:
            continue
        size = 8 if kind == "uvec2" else 4
        offset = (offset + size - 1) // size * size
        common.append((name, kind, offset))
        offset += size
    return common, push


def port(raw, common, push):
    text = UNIFORM.sub("", raw.decode())
    text, count = BINDING.subn(r"layout(std430,set=0,binding=\1)", text)
    if count != 4 or not text.startswith("#version 310 es\n"):
        raise ValueError("Unexpected parent Vulkan port interface")
    common_glsl = "".join(f"layout(offset={offset}) {kind} {name};\n" for name, kind, offset in common)
    push_glsl = "".join(f"layout(offset={i * 4}) uint {name};\n" for i, name in enumerate(push))
    return ("#version 450\nlayout(std140,set=0,binding=4) uniform Common {\n" + common_glsl +
            "};\nlayout(push_constant) uniform Dispatch {\n" + push_glsl + "};\n" +
            text.removeprefix("#version 310 es\n")).encode()


def compile_role(compiler, out, opcode, source, common, push, name):
    row = dict(op=opcode, name=name, sourceBytes=len(source), sourceSHA=sha(source))
    if len(source) > LIMIT:
        return dict(row, state="SOURCE_LIMIT_EXCEEDED")
    shader = out / f"role_{opcode:08x}.comp"
    shader.write_bytes(source)
    binary = shader.with_suffix(".spv")
    command = [str(compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val", "--spirv-dis",
               "-o", str(binary), str(shader)]
    result = subprocess.run(command, capture_output=True, text=True, encoding="utf8", errors="replace")
    asm = result.stdout
    if result.returncode or re.search(r"\berror:|validation.*(?:fail|unavailable|not available)", asm + result.stderr, re.I):
        shader.with_suffix(".log").write_text(asm + result.stderr, encoding="utf8")
        return dict(row, state="STATIC_COMPILER_REJECTED")
    expected = {
        "Common": [offset for _, _, offset in common],
        "Dispatch": [4 * i for i in range(len(push))],
    }
    for block, offsets in expected.items():
        actual = re.findall(rf"OpMemberDecorate %{block} (\d+) Offset (\d+)", asm)
        if actual != [(str(i), str(at)) for i, at in enumerate(offsets)]:
            raise ValueError(f"{opcode}: {block} reflection differs")
    if (not re.search(r"OpExecutionMode %\S+ LocalSize 64 1 1", asm) or
            sorted(re.findall(r"OpDecorate %\S+ Binding (\d+)", asm)) != list("01234") or
            re.findall(r"OpDecorate %\S+ DescriptorSet (\d+)", asm) != ["0"] * 5):
        raise ValueError(f"{opcode}: Vulkan descriptor/local-size reflection differs")
    return dict(row, state="STATIC_SPIRV", spv=binary.name, spvBytes=binary.stat().st_size,
                spvSHA=sha(binary.read_bytes()))


def load_parent(family, directory):
    directory.mkdir(exist_ok=True)
    if family == "dm2":
        subprocess.run([sys.executable, str(ROOT / "tools/g9dm/generate_assets.py"),
                        "--out", str(directory)], check=True)
        return directory / "pipeline.comp", directory / "schema_generated.h"
    subprocess.run(["node", str(ROOT / "test/t566_generate_composite.cjs"), str(directory), "--r1-only"], check=True)
    return directory / "g11vis_r1.comp", directory / "g11vis_pipeline.h"


def dispatches(family, header, ops):
    composite = family == "r1"
    result = {}
    for kind, count in (("Source", 19 if composite else 12), ("Quantum", 2543 if composite else 2404)):
        name = f"kG11R1{kind}Passes" if composite else f"k{kind}Passes"
        body = re.search(rf"{name}\s*=\s*\{{\{{(.*?)\}}\}};", header, re.S)
        if not body:
            raise ValueError(f"Missing frozen {family} {kind} table")
        if composite:
            rows = [[int(word) for word in row] for row in
                    re.findall(r"\{(\d+)u,(\d+)u,(\d+)u,(-?\d+),(\d+)u,(\d+)u\}", body[1])]
        else:
            rows = [[ops["G9_OP_" + op], int(role), int(j), int(micro), 0, 0] for op, role, j, micro in
                    re.findall(r"\{Op::(\w+),\s*(\d+),\s*(\d+),\s*(-?\d+)\}", body[1])]
        if len(rows) != count or any(row[0] not in ops.values() for row in rows):
            raise ValueError(f"Frozen {family} {kind} dispatch table changed")
        result[kind] = rows
    return result


def graphics(compiler, out):
    parent = (VIS_ROOT / "scene_shaders.h").read_text(encoding="utf8")
    glyph = (VIS_ROOT / "scene.cpp").read_text(encoding="utf8")
    varying = {
        "base": (),
        "glyph": ("vCenter", "vOld", "vNew", "vKindFlags", "vBlend", "vGhostMass", "vAppearance"),
        "packet": ("local", "opacity", "mass"),
    }
    blocks = {"base": ("G11SC", "G11SF"), "glyph": ("G11GV", "G11GF"), "packet": ("G11PV", "G11PF")}
    result = {}
    for layer, pair in blocks.items():
        result[layer] = []
        paths = []
        for stage, tag in zip(("vert", "frag"), pair, strict=True):
            source, = re.findall(rf'R"{tag}\((.*?)\){tag}"', glyph if layer=="glyph" else parent, re.S)
            text = re.sub(r"^uniform (?:highp )?(?:vec2|vec4|uint|int) \w+;\n", "", source, flags=re.M)
            text = BINDING.sub(r"layout(std430,set=0,binding=\1)", text)
            text = text.replace("gl_VertexID", "gl_VertexIndex")
            # Negative-height Vulkan viewport preserves the original bottom-up model coordinates.
            text = text.replace("gl_FragCoord.xy", "vec2(gl_FragCoord.x,uResolution.y-gl_FragCoord.y)")
            for i, name in enumerate(varying[layer]):
                text, count = re.subn(rf"((?:flat )?(?:out|in) (?:vec2|float|uint) {name};)",
                                      rf"layout(location={i}) \1", text)
                if count != 1:
                    raise ValueError(f"Missing {layer} varying {name}")
            view = ("layout(push_constant) uniform View {\n"
                    "layout(offset=0) vec2 uResolution;\nlayout(offset=16) vec4 uBox;\n"
                    "layout(offset=32) int uN;\nlayout(offset=36) uint uForeground;\n};\n")
            if not text.startswith("#version 310 es\n"):
                raise ValueError("Unexpected R1 graphics version")
            compiled = ("#version 450\n" + view + text.removeprefix("#version 310 es\n")).encode()
            if len(compiled)>LIMIT:
                raise ValueError("R1 graphics source exceeds limit")
            path = out / f"r1_{layer}.{stage}"
            paths.append(path)
            path.write_bytes(compiled)
            binary = path.with_suffix(f".{stage}.spv")
            args = [str(compiler), "-V", "--target-env", "vulkan1.1", "--spirv-val"]
            disassembly = subprocess.run([*args, "--spirv-dis", "-o", str(binary), str(path)],
                                         check=True, capture_output=True, text=True)
            expected = [("0", "0"), ("1", "16"), ("2", "32"), ("3", "36")]
            if (re.findall(r"OpMemberDecorate %View (\d+) Offset (\d+)", disassembly.stdout)!=expected or
                    re.search(r"\berror:|validation.*(?:fail|unavailable|not available)",
                              disassembly.stdout+disassembly.stderr, re.I)):
                raise ValueError(f"{layer} {stage} graphics push or validation differs")
            result[layer].append(dict(stage=stage, parentSHA=sha(source.encode()), sourceSHA=sha(compiled),
                                      sourceBytes=len(compiled), spv=binary.name,
                                      spvSHA=sha(binary.read_bytes()), spvBytes=binary.stat().st_size))
        subprocess.run([str(compiler), "-V", "--target-env", "vulkan1.1", "-l", "-o",
                        str(out / f"link_{layer}.spv"), *map(str, paths)], check=True, capture_output=True)
        (out / f"link_{layer}.spv").unlink()
    return result


def build(family, compiler, out):
    parent, schema = load_parent(family, out / "parent")
    raw = parent.read_bytes().replace(b"\r\n", b"\n")
    source = raw.decode()
    composite = family == "r1"
    ops, emit = split_inputs(source, composite_r1=composite)
    common, push = declarations(source, composite)
    passes = dispatches(family, schema.read_text(encoding="utf8"), ops)
    names = {op: name for name, op in ops.items()}
    names[WARM] = "WARM_AUDIT"
    jobs = [(op, port(emit((op,))[0], common, push), name) for op, name in sorted(names.items())]
    with ThreadPoolExecutor(max_workers=4) as pool:
        roles = list(pool.map(lambda job: compile_role(compiler, out, job[0], job[1], common, push, job[2]), jobs))
    usable = {r["op"] for r in roles if r["state"] == "STATIC_SPIRV"}
    data = dict(schema="PC_SAND_VULKAN_ROLES_V1", family=family, parentSHA=sha(raw),
                constants=common, commonBytes=(common[-1][2] + (8 if common[-1][1] == "uvec2" else 4) + 15) // 16 * 16,
                push={name: i * 4 for i, name in enumerate(push)}, roles=roles, passes=passes,
                masks={"Source": 0xcffff if composite else 0xffff, "Quantum": 0xcffff if composite else 0x3ffff},
                complete={kind: all(op in usable for op, *_ in rows) for kind, rows in passes.items()},
                graphics=graphics(compiler,out) if composite else {},
                runtimeQualification="NOT_EXECUTED")
    data["roleSetSHA"] = sha(json.dumps(data, sort_keys=True, separators=(",", ":")).encode())
    (out / "roles.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf8")
    with (out / "sand_passes.h").open("w", encoding="ascii") as header:
        header.write("#pragma once\nnamespace grs::pc::sand {\n")
        for kind, rows in passes.items():
            header.write(f"inline constexpr Pass k{kind}[]={{\n")
            for op, role, j, micro, visual, phase in rows:
                header.write(f"  {{{op},{role},{j},{micro},{visual},{phase},{str(op in usable).lower()}}},\n")
            header.write("};\n")
        for name, _, offset in common:
            header.write(f"inline constexpr unsigned k_{name}={offset};\n")
        header.write(f'inline constexpr unsigned kCommonBytes={data["commonBytes"]};\n')
        header.write(f'inline constexpr char kRoleSetSHA[]="{data["roleSetSHA"]}";\n}}\n')
    print(f"{family}: {len(usable)}/{len(roles)} static roles, {data['complete']}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("family", choices=("dm2", "r1"))
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    build(args.family, args.compiler.resolve(), args.out.resolve())
