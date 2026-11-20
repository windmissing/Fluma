"""Audit archived G13 shader closures, dispatch identity and stencil words offline."""
from collections import Counter, defaultdict
import json
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write
from analyze_t741 import COMPILE, LIMIT, NATIVE, SIGNED, compile_role, doc
from analyze_t747 import reference

sys.path.insert(0, str(ROOT / "tools/g13"))
from split_shader import CALL, IDENT, TOKEN, matching, source_units


DISPATCH = "docs/architecture-T607-g13-dispatch-v1.json"
CPP = "entry/src/main/cpp/gpu_resident/g13flow/"
PROBE = "tools/architecture/t747_u32_probe.comp"


def signed_source(manifest):
    receipt = doc(SIGNED)
    package = Path(receipt["signed"]["path"])
    assert receipt["verifyApp"] == "PASS" and sha(package.read_bytes()) == receipt["signed"]["sha256"]
    with ZipFile(package) as archive:
        elf = archive.read(NATIVE)
    role = manifest["roles"]["OP_48"]
    raw = (Path(manifest["project"]) / role["path"]).read_bytes()
    assert sha(elf) == manifest["nativeSHA256"] and build_id(elf) == manifest["buildId"]
    assert (len(raw), sha(raw)) == (role["bytes"], role["sha256"]) and elf.count(raw + b"\0") == 1
    return raw, {"signedReceipt": record(SIGNED), "manifest": record(COMPILE), "signedSHA": receipt["signed"]["sha256"],
                 "buildId": build_id(elf), "nativeSHA": sha(elf), "sourceBytes": len(raw), "sourceSHA": sha(raw)}


def split_group(raw):
    definitions, units = source_units(raw.decode(), hooks=False)
    main, = [text for name, text in units if name == "main"]
    tokens = list(TOKEN.finditer(main))
    pairs = matching([token[0] for token in tokens])
    arms = {}
    at = 5  # void main() { precedes each top-level opcode arm
    while at < len(tokens) - 1:
        head = [token[0] for token in tokens[at:at + 7]]
        assert head[:4] == ["if", "(", "uOp", "=="] and head[5:] == [")", "{"]
        end = pairs[at + 6]
        arms[int(head[4].removesuffix("u"))] = main[tokens[at].start():tokens[end].end()]
        at = end + 1
    assert at == len(tokens) - 1
    bodies = defaultdict(list)
    for name, text in units:
        if name and name != "main":
            bodies[name].append(text)

    def emit(opcodes):
        entry = "\n".join(arms[op] for op in (66, *opcodes))
        work = list(CALL.findall(entry))
        deps = set()
        while work:
            name = work.pop()
            if name in bodies and name not in deps:
                deps.add(name)
                for body in bodies[name]:
                    work.extend(CALL.findall(body))
        chunks = [text for name, text in units if name is None or name in deps]
        chunks.append("void main(){\n" + entry + "\n}")
        uses = set(IDENT.findall("\n".join(chunks)))
        prelude = ["#version 310 es", *(f"#define {key} {value}" for key, value in definitions.items() if key in uses)]
        return ("\n".join(prelude + chunks) + "\n").encode(), sorted(deps)

    return emit, tuple(op for op in arms if op != 66)


def coherent_variant(raw):
    for old, new in ((b"buffer Work {", b"coherent buffer Work {"),
                     (b"buffer Control {", b"coherent buffer Control {")):
        assert raw.count(old) == 1
        raw = raw.replace(old, new)
    return raw


def compile_gles(compiler, raw, directory, name):
    source = directory / (name + "-gles.comp")
    source.write_bytes(raw)
    output = subprocess.run([compiler, "-S", "comp", "-l", "-q", source], check=True,
                            capture_output=True, text=True).stdout
    assert "Local size X: 64" in output
    bindings = dict((key, int(slot)) for key, slot in re.findall(
        r"^([A-Za-z]+): offset -1, type ffffffff, size \d+, index \d+, binding (\d+),", output, re.M))
    uniforms = re.findall(r"^(u\w+): offset -1, type 1405, size \d+, index -1,", output, re.M)
    assert {"uOp", "uScope", "uJ", "uMicro", "uBlock", "uIndex"} <= set(uniforms)
    assert bindings == {"Base": 0, "Candidate": 1, "Work": 2, "Control": 3}
    return {"uniforms": sorted(uniforms), "ssboBindings": bindings, "localSize": [64, 1, 1]}


def audit_closures(compiler, manifest):
    original, parent = signed_source(manifest)
    emit, old = split_group(original)
    assert emit(old)[0] == original
    assert old == tuple(manifest["roles"]["OP_48"]["opcodes"])
    assert old[:2] == (48, 49)
    core = doc("docs/architecture-T747-evidence-v1.json")["probe"]["arithmeticCoreBytes"]
    variants = []
    with TemporaryDirectory(prefix="t750-static-") as scratch:
        directory = Path(scratch)
        isolated, dependencies = emit((48,))
        assert {"viscApply", "fieldRead", "viscBlock", "viscResult", "viscSpectralUpper", "syncAll",
                "reducePair", "checkVelocity", "traceVisc", "coldFP32Probe"} <= set(dependencies)
        assert not {"pressureCycle", "drive", "softExp", "wallLedger"} & set(dependencies)
        for name, ops, raw, deps in (
                ("VISC_48_NATIVE_CONTROL", (48,), isolated, dependencies),
                ("VISC_48_COHERENT_CONTROL", (48,), coherent_variant(isolated), dependencies),
                ("OP_48_REMAINDER", old[1:], *emit(old[1:]))):
            assert len(raw) <= LIMIT
            gles = compile_gles(compiler, raw, directory, name)
            vk = compile_role(compiler, raw, directory, name)
            variants.append({"role": name, "opcodes": list(ops), "privateProbeOpcode": 66,
                             "gles": gles, "sizeAndIdentity": vk, "dependencies": deps,
                             "screeningHeadroomAfterAddingT747CoreOnly": {
                                 "gles": LIMIT - len(raw) - core, "vulkan": LIMIT - vk["vulkanSourceBytes"] - core}})
    return {"parent": parent, "sourceLimitBytes": LIMIT, "compiledVariants": variants,
            "scope": "STATIC_NATIVE_CONTROL_AND_VISIBILITY_CONTROL_ONLY_NO_U32_BLOCK_OR_DEVICE"}


def audit_dispatch(manifest):
    dispatch = doc(DISPATCH)
    quantum, source = dispatch["quantum"], dispatch["source"]
    assert (len(quantum), len(source)) == (820, 16)
    assert dispatch["persistent"]["VISC_BLOCK"] == {"groups": 1, "lanes": 64, "iterations": 16, "blockCount": 8}
    runs = defaultdict(list)
    for p in quantum:
        if p["opcode"] == 48:
            runs[p["scope"], p["J"], p["micro"]].append(p)
    assert len(runs) == 14
    for (scope, j, micro), rows in runs.items():
        assert scope in (1, 2) and j in (1, 2, 4) and 0 <= micro < j
        assert [p["iterationBlock"] for p in rows] == list(range(8))
        start = rows[0]["index"]
        assert rows == quantum[start:start + 8]
        assert quantum[start - 1]["op"] == "VISC_SETUP" and quantum[start + 8]["op"] == "VISC_ENERGY_PARTIAL"

    generated = (ROOT / (CPP + "contract_generated.h")).read_text(encoding="utf-8")
    native_table = re.search(r"k_quantum\[\]=\{(.*?)\};", generated, re.S)[1]
    fields = ("opcode", "scope", "J", "micro", "iterationBlock")
    assert [tuple(map(int, re.findall(r"(\d+)u", line))) for line in native_table.splitlines() if "{" in line] == [
        tuple(row[key] for key in fields) for row in quantum]

    selectors = ("p.scope ? replay_ : candidate_", "glUniform1ui(locations.at[Scope], p.scope);")
    frozen_transport = Path(manifest["project"]) / (CPP + "transport.cpp")
    for p in (ROOT / (CPP + "transport.cpp"), frozen_transport):
        text = p.read_text(encoding="utf-8")
        for selector in selectors:
            assert selector in text
        assert all(snippet in text for snippet in ("size_t lastRole = PipelineRoleCount();", "count < 10", "8000000ull"))
    assert "uScope==0u?2048u:10240u" in (ROOT / (CPP + "trace.glsl")).read_text(encoding="utf-8")
    native, _ = split_group((Path(manifest["project"]) / manifest["roles"]["OP_48"]["path"]).read_bytes())
    split, _ = native((48,))
    assert b"uScope==0u?2048u:10240u" in split and b"c[784u+uScope]++" in split
    assert b"buffer Work {uint w[];}" in split and b"buffer Control {uint c[];}" in split
    assert b"memoryBarrierBuffer();memoryBarrierShared();barrier();" in split

    counts = {}
    for name, rows in (("QUANTUM", quantum), ("SOURCE", source)):
        old_roles = [manifest["opcodeToRole"][p["opcode"]] for p in rows]
        new_roles = ["VISC_48" if p["opcode"] == 48 else role for p, role in zip(rows, old_roles)]
        boundaries = [[a != b for a, b in zip(roles, roles[1:])] for roles in (old_roles, new_roles)]
        assert boundaries[0] == boundaries[1]
        counts[name] = {"singleContinuousSequence": [1 + sum(edges) for edges in boundaries],
                        "fixedBatchesOfTen": [1 + sum(edge or i % 10 == 0 for i, edge in enumerate(edges, 1))
                                              for edges in boundaries],
                        "sameAdjacentRoleBoundariesForAnyEqualPumpPartition": True}
    return {"dispatch": record(DISPATCH), "generatedHeader": record(CPP + "contract_generated.h"),
            "frozenTransport": record(frozen_transport.relative_to(ROOT).as_posix()),
            "totals": {"quantum": 820, "source": 16, "viscBlocksSubmitted": sum(map(len, runs.values())),
                       "microBudget": dict(Counter(scope for scope, _, _ in runs)), "maxIterationSlotsPerRoot": 14 * 8 * 16},
            "blockRanges": [{"scope": scope, "trialJ": j, "micro": micro, "firstIndex": rows[0]["index"],
                             "lastIndex": rows[-1]["index"]} for (scope, j, micro), rows in runs.items()],
            "scopeCollision": [{"tableScope": scope, "nativeBank": "REPLAY", "shaderScope": scope,
                                 "traceBaseWords": 10240, "viscCounterWord": 784 + scope,
                                 "expectedBank": ("CANDIDATE", "REPLAY")[scope - 1],
                                 "expectedShaderIndex": scope - 1, "expectedTraceBaseWords": (2048, 10240)[scope - 1]}
                                for scope in (1, 2)],
            "programUsesOldThenSplit": counts, "activeBlockSyncAllCount": 1 + 16 + 1 + 2 * 7 + 1}


def audit_words():
    def op(name, a, b):
        return reference(name, a, b, 0, 0)

    def stencil(q, left, right, bottom, top):
        a = op("ADD", left, right)
        b = op("ADD", a, bottom)
        c = op("ADD", b, top)
        m = op("MUL", 0x40800000, q)
        return [a, b, c, m, op("SUB", c, m)]

    def ghost(inside, wall):
        return op("ADD", op("MUL", 0xBF800000, inside), op("MUL", 0x40000000, wall))

    lower = ghost(0x3E000000, 0x3E800000)
    upper = ghost(0x3E000000, 0xBE800000)
    assert (lower, upper, ghost(0x00000001, 0), ghost(0, 0)) == (0x3EC00000, 0xBF200000, 0x80000001, 0)
    vectors = [
        ("reassociation negative control", (0, 0x3F800000, 0x33800000, 0xBF800000, 0), None, 0),
        ("normal cancellation at the lower edge", (0, 0x00800001, 0x80800000, 0, 0), None, 1),
        ("subnormal center produces negative subnormal", (1, 0, 0, 0, 0), None, 0x80000004),
        ("subnormal carry into normal", (0, 0x007FFFFF, 1, 0, 0), None, 0x00800000),
        ("lower channel ghost and homogenization", (0x3E000000, 0x3E000000, 0x3E000000, lower, 0x3E000000),
         0x3E800000, 0xBE800000),
        ("upper channel ghost and homogenization", (0x3E000000, 0x3E000000, 0x3E000000, 0x3E000000, upper),
         0xBE800000, 0xBE800000),
    ]
    rows = []
    for name, inputs, wall, expected in vectors:
        words = stencil(*inputs)
        final = words[-1] if wall is None else op("SUB", words[-1], op("MUL", 0x40000000, wall))
        assert final == expected, name
        rows.append({"case": name, "qLeftRightBottomTop": [f"{v:08x}" for v in inputs],
                     "addAddAddMulSub": [f"{v:08x}" for v in words],
                     "wall": None if wall is None else f"{wall:08x}", "afterHomogenize": f"{final:08x}"})
    reassociated = op("ADD", op("ADD", 0x3F800000, 0xBF800000), 0x33800000)
    assert reassociated == 0x33800000
    nx = ny = 64
    faces = (nx + 1) * ny + nx * (ny + 1)
    return {"operatorVectors": rows, "reassociatedWrongWord": f"{reassociated:08x}",
            "profile64": {"faces": faces, "loopSlotsPerActiveBlock": 16 * faces,
                          "activeFaceCounts": {"closed": 8064, "periodicXY": 8192, "periodicXChannel": 8128}},
            "sources": [record(p) for p in (CPP + "viscosity.glsl", CPP + "mac_access.glsl", CPP + "trace.glsl", PROBE)]}


if __name__ == "__main__":
    compiler = Path(sys.argv[1]).resolve()
    manifest = doc(COMPILE)
    write("docs/architecture-T750-evidence-v1.json", {
        "schema": "T750_G13_VISC_SLICE_ARCHITECTURE_V1", "deviceUsed": False, "backendSelected": False,
        "compilerSHA": sha(compiler.read_bytes()), "closure": audit_closures(compiler, manifest),
        "dispatch": audit_dispatch(manifest), "oracle": audit_words()})
    print("T750 checked signed parent, static split controls, scope mapping and exact stencil words")
