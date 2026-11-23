"""T733 dependency closure for the G9 family, independent of package tooling."""
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "g13"))
from split_shader import CALL, IDENT, TOKEN, matching, source_units, specialize

LIMIT = 48 * 1024
WARM = 0x7FFFFF00

def constant_names(text):
    tokens = TOKEN.findall(text)
    if tokens[:1] != ["const"]:
        return []
    matched = matching(tokens)
    boundaries = [2]
    at = 2
    while at < len(tokens) - 1:
        if tokens[at] == ",":
            boundaries.append(at + 1)
        at = matched.get(at, at) + 1
    names = []
    for lo, hi in zip(boundaries, boundaries[1:] + [len(tokens)], strict=True):
        if not IDENT.fullmatch(tokens[lo]) or "=" not in tokens[lo:hi]:
            raise ValueError("Unexpected global constant declaration: " + text)
        names.append(tokens[lo])
    return names


def split_inputs(source, *, composite_r1=False):
    if composite_r1:
        guard = "#ifndef G11VIS_STAGE_GLSL\n#define G11VIS_STAGE_GLSL\n"
        if source.count(guard) != 1 or source.count("#endif\n") != 1:
            raise ValueError("Unexpected R1 visual include guard")
        source = source.replace(guard, "").replace("#endif\n", "")
    definitions, units = source_units(source, False)
    if definitions:
        raise ValueError("New preprocessor definitions require dependency review")
    functions, constants = defaultdict(list), {}
    retained_globals = set()
    for index, (name, text) in enumerate(units):
        if name:
            functions[name].append(index)
            continue
        names = constant_names(text)
        if not names:
            retained_globals.add(index)
        for symbol in names:
            if symbol in constants:
                raise ValueError("Duplicate declared constant: " + symbol)
            constants[symbol] = index
    run, = functions.pop("g9Run")
    main, = functions.pop("main")
    if re.sub(r"\s+", "", units[main][1]) != "voidmain(){g9Run();}":
        raise ValueError("Unexpected G9 shader entry")
    ops = {name: int(value) for name, value in
           re.findall(r"const uint (G9_OP_\w+)\s*=\s*(\d+)u;", source)}
    if composite_r1:
        if sorted(ops.values()) != list(range(42)) or ops.get("G9_OP_ROOT_INIT") != 0:
            raise ValueError("Unexpected R1 parent opcode table")
        visual = {name: int(value, 0) for name, value in
                  re.findall(r"const uint (G11_OP_\w+|G11_COLD_INIT)\s*=\s*(0x[\da-f]+|\d+)u;", source)}
        if sorted(visual.values()) != [512, 513, 514, 515, 516, 0x7fffff01]:
            raise ValueError(f"Unexpected R1 visual opcode table: {visual}")
        ops.update(visual)
    elif sorted(ops.values()) != list(range(44)) or ops.get("G9_OP_ROOT_INIT") != 12:
        raise ValueError("Unexpected G9 opcode table")
    entry = units[run][1].replace("void g9Run(", "void main(", 1)
    literals = dict(ops, **{"0x7fffff00u": WARM})
    bodies = {op: specialize(entry, op, literals) for op in (*ops.values(), WARM)}

    def build(opcodes):
        body = "\n".join(f"if(uOp=={op}u){{\n{bodies[op]}\nreturn;\n}}" for op in opcodes)
        needed = retained_globals | {-1}
        pending = list(needed)
        while pending:
            index = pending.pop()
            text = units[index][1] if index >= 0 else body
            dependencies = {constants[symbol] for symbol in IDENT.findall(text) if symbol in constants}
            for call in CALL.findall(text):
                dependencies.update(functions.get(call, []))
            pending.extend(dependencies - needed)
            needed.update(dependencies)
        chunks = [text for i, (_, text) in enumerate(units) if i in needed]
        output = "\n".join(["#version 310 es", *chunks, "void main(){", body, "}", ""]).encode()
        return output, sorted({name for i, (name, _) in enumerate(units) if i in needed and name})

    return ops, build



def write_boot(out, source, schema, family):
    """Use the exact T733-reviewed parent, role and dispatch contract."""
    root = Path(__file__).resolve().parents[2]
    contract = json.loads((root / "docs/architecture-T733-closure-v1.json").read_bytes())
    review = contract["families"][family]
    digest = lambda data: hashlib.sha256(data).hexdigest()
    parent = digest(source)
    if parent != review["baseline"]["shaderSHA"]:
        raise ValueError(f"{family} BOOT parent differs from approved T733 source")
    ops, emit = split_inputs(source.decode("utf8"))
    opcodes = (ops["G9_OP_ROOT_INIT"], WARM)
    shader, functions = emit(opcodes)
    expected = next(row for row in review["roles"] if row["role"] == "BOOT")
    if not shader or len(shader) > LIMIT or len(shader) != expected["sourceBytes"] or digest(shader) != expected["sourceSHA"]:
        raise ValueError(f"{family} BOOT bytes differ from approved closure or exceed {LIMIT}")
    for kind in ("Source", "Quantum"):
        body = re.search(rf"k{kind}Passes\s*=\s*\{{\{{(.*?)\}}\}};", schema, re.S)
        if not body:
            raise ValueError(f"{family} {kind} dispatch contract unavailable")
        passes = [[ops["G9_OP_"+name], int(role), int(j), int(micro)]
                  for name, role, j, micro in
                  re.findall(r"\{Op::(\w+),\s*(\d+),\s*(\d+),\s*(-?\d+)\}", body[1])]
        baseline = review["schedule"][kind]
        if len(passes) != baseline["dispatches"] or digest(
                json.dumps(passes,separators=(",",":")).encode()) != baseline["passesSHA"]:
            raise ValueError(f"{family} {kind} dispatch contract changed")
        if kind == "Quantum" and passes[0] != [opcodes[0],0,0,-1]:
            raise ValueError("BOOT ROOT_INIT first pass changed")
    role = {
        "schema":"G9_BOOT_ONLY_V1", "family":family, "variant":"BOOT_ONLY",
        "backend":"GLES", "parentShaderSHA":parent, "role":"BOOT",
        "opcodes":opcodes, "sourceBytes":len(shader), "sourceSHA":digest(shader),
        "functions":functions, "executableRoots":[],
        "schedule":review["schedule"], "layoutSHA":digest(schema.encode("utf8")),
        "localSize":[64,1,1],
        "bindings":{"ReadState":{"binding":0,"access":"readonly","required":True},
                    "WriteState":{"binding":1,"access":"read_write","required":True},
                    "Workspace":{"binding":2,"access":"read_write","required":True},
                    "EventPath":{"binding":3,"access":"read_write","required":False}},
        "workTail":{"range":7,"offsetUnit":"bytes","words":2,"seal":0x641647},
    }
    identity = digest(json.dumps(role,sort_keys=True,separators=(",",":")).encode("utf8"))
    role["roleSetSHA"] = identity
    (out/"boot.comp").write_bytes(shader)
    (out/"boot_manifest.json").write_bytes((json.dumps(role,indent=2)+"\n").encode("utf8"))
    (out/"boot_source.h").write_bytes((
        '#pragma once\n'
        'inline constexpr const char kBootPipeline[]=R"G9BOOT('+shader.decode("utf8")+')G9BOOT";\n'
        'inline constexpr const char kBootPipelineSHA[]="'+role["sourceSHA"]+'";\n'
        'inline constexpr const char kBootRoleSetSHA[]="'+identity+'";\n'
    ).encode("utf8"))


def write_dm_source(out, parent):
    write_source(out, parent, "DM")


def write_source(out, parent, family):
    root = Path(__file__).resolve().parents[2]
    raw = (root / "docs/architecture-T736-source-contract-v1.json").read_bytes()
    contract = json.loads(raw)
    expected = contract["families"][family]["sourceRole"]
    boot = json.loads((out / "boot_manifest.json").read_bytes())
    digest = lambda data: hashlib.sha256(data).hexdigest()
    _, emit = split_inputs(parent.decode("utf8"))
    shader, _ = emit(range(12))
    if len(shader) != expected["bytes"] or len(shader) > LIMIT or digest(shader) != expected["sha256"]:
        raise ValueError(f"{family} SOURCE differs from approved T736 closure or exceeds cap")
    if expected["passSHA"] != boot["schedule"]["Source"]["passesSHA"]:
        raise ValueError(f"{family} SOURCE pass table differs from T736")
    reflection = contract["reflection"]
    manifest = {
        "schema": "G9_BOOT_SOURCE_V1", "family": family, "backend": "GLES",
        "variant": "BOOT_SOURCE", "supportsRoots": ["SOURCE"],
        "parentShaderSHA": boot["parentShaderSHA"], "t736ContractSHA": digest(raw),
        "boot": {"sourceSHA": boot["sourceSHA"], "sourceBytes": boot["sourceBytes"],
                 "opcodes": boot["opcodes"], "roleSetSHA": boot["roleSetSHA"]},
        "source": {**expected, "opcodes": list(range(12))},
        "reflection": reflection, "passTable": contract["passTable"],
    }
    manifest["roleSetSHA"] = digest(json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode())
    uniforms = ",\n".join('{'+json.dumps(name)+','+desc["glType"]+'u}'
                         for name, desc in reflection["activeUniforms"].items())
    if len(reflection["activeUniforms"]) != 40 or any(v["size"] != 1 for v in reflection["activeUniforms"].values()):
        raise ValueError("Invalid SOURCE reflection contract")
    (out / "source.comp").write_bytes(shader)
    (out / "source_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf8")
    (out / "source_shader.h").write_bytes((
        '#pragma once\n#include <cstdint>\n'
        'inline constexpr char kSourcePipeline[]=R"G9SRC('+shader.decode() + ')G9SRC";\n'
        'inline constexpr char kSourcePipelineSHA[]="'+expected["sha256"]+'";\n'
        'inline constexpr char kSourceRoleSetSHA[]="'+manifest["roleSetSHA"]+'";\n'
        'inline constexpr struct { const char *name; uint32_t type; } kSourceUniforms[]{\n'
        + uniforms + '\n};\n').encode())
