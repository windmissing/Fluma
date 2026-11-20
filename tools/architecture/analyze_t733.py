"""Measure and statically compile G9 dependency candidates without loading the product."""
import json
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write

sys.path.insert(0, str(ROOT / "tools/glsl"))
from g9_boot import LIMIT, WARM, split_inputs
GROUPS = {
    "BOOT": (12, WARM),
    "SOURCE": tuple(range(12)),
    "ROOT_SETUP": tuple(range(13, 19)),
    "TOUCH": (19, 20),
    "DM_TRANSFER": (21, 22),
    "FLIGHT_FOLD": (23, 24),
    "FACE_FLOW": tuple(range(25, 31)),
    "CAPTURE_AUDIT": (31, 32),
    "ROOT_FINAL": tuple(range(33, 44)),
}
COST_COMPARISON_GROUPS = {
    **{key: GROUPS[key] for key in ("BOOT", "SOURCE", "ROOT_SETUP", "TOUCH", "DM_TRANSFER")},
    "FLIGHT_FACE_HEAD": tuple(range(23, 28)),
    "FACE_TAIL_CAPTURE_AUDIT": tuple(range(28, 33)),
    "ROOT_FINAL": GROUPS["ROOT_FINAL"],
}
FAMILIES = {
    "RX": (
        "T720-rx", "libgrs_g9rx.so",
        ".codex/local-test/t720-rx/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/g9rx/rx_assets",
        "2cb9a84813833027f68ee7434f98f833dae01e310d9ea053e57e2870d133778e",
    ),
    "DM": (
        "T718-dm", "libgrs_dm_t647.so",
        ".codex/local-test/t718/entry/.cxx/default/default/debug/arm64-v8a/dm_native/dm_assets",
        "c10b15ee3d7ba93bde10d393b5d8ebf0cbcec02df4ae6615f90b196007d5b0c4",
    ),
}


def actual_source(name, library, source_sha):
    receipt_path = f"test/reports/t257/{name}.json"
    receipt = json.loads((ROOT / receipt_path).read_bytes())
    package = Path(receipt["signed"]["path"])
    package_sha = sha(package.read_bytes())
    assert package_sha == receipt["signed"]["sha256"]
    with ZipFile(package) as archive:
        elf = archive.read("libs/arm64-v8a/" + library)
    sources = [elf[start.start():elf.index(b"\0", start.start())]
               for start in re.finditer(rb"#version 310 es", elf)]
    shader, = [raw for raw in sources if sha(raw) == source_sha]
    return shader, {"signedReceipt": record(receipt_path), "signedSHA": package_sha,
                    "librarySHA": sha(elf), "buildId": build_id(elf),
                    "shaderSHA": source_sha, "shaderBytes": len(shader)}


def dispatch_counts(header, ops, groups):
    text = (ROOT / header).read_text(encoding="utf-8")
    mapping = {op: role for role, values in groups.items() for op in values}
    result = {}
    for key, expected in (("Source", 12), ("Quantum", 2404)):
        body = re.search(rf"k{key}Passes\s*=\s*\{{\{{(.*?)\}}\}};", text, re.S)[1]
        passes = [[ops["G9_OP_" + name], int(role), int(j), int(micro)] for name, role, j, micro in
                  re.findall(r"\{Op::(\w+),\s*(\d+),\s*(\d+),\s*(-?\d+)\}", body)]
        assert len(passes) == expected
        groups = [mapping[op] for op, _, _, _ in passes]
        result[key] = {"dispatches": len(passes), "roles": sorted(set(groups)),
                       "contiguousRoleTransitions": sum(a != b for a, b in zip(groups, groups[1:])),
                       "passesSHA": sha(json.dumps(passes, separators=(",", ":")).encode())}
    return result


def compile_groups(family, label, groups, build, compiler, scratch):
    rows = []
    for role, opcodes in groups.items():
        binary, dependencies = build(opcodes)
        assert len(binary) <= LIMIT, (family, role, len(binary))
        candidate = scratch / f"{family}-{label}-{role}.comp"
        candidate.write_bytes(binary)
        proc = subprocess.run([compiler, "-S", "comp", candidate], capture_output=True)
        if proc.returncode:
            raise RuntimeError(f"{family}/{role}: compiler rejected {proc.stdout!r} {proc.stderr!r}")
        rows.append({"role": role, "opcodes": opcodes, "sourceBytes": len(binary), "sourceSHA": sha(binary),
                     "functions": dependencies, "staticCompileExit": proc.returncode})
    return rows


def audit(family, compiler, scratch):
    name, library, assets, shader_sha = FAMILIES[family]
    shader, baseline = actual_source(name, library, shader_sha)
    assert (ROOT / assets / "pipeline.comp").read_bytes().replace(b"\r\n", b"\n") == shader
    ops, build = split_inputs(shader.decode())
    rows = compile_groups(family, "main", GROUPS, build, compiler, scratch)
    comparison = compile_groups(family, "cost", COST_COMPARISON_GROUPS, build, compiler, scratch)
    header = assets + "/schema_generated.h"
    widest = sorted(((op, len(build([op])[0])) for op in range(44)), key=lambda item: -item[1])[:4]
    return {"baseline": baseline, "roles": rows, "widestSingleOpcode": widest,
            "oversizedBroadGroups": {role: len(build(ops)[0]) for role, ops in
                                      {"ROOT_SETUP_AND_TOUCH": range(13, 21), "TRANSFER_FLIGHT": range(21, 25)}.items()},
            "schedule": dispatch_counts(header, ops, GROUPS),
            "costComparison": {"roles": comparison,
                               "schedule": dispatch_counts(header, ops, COST_COMPARISON_GROUPS)}}


def main(compiler):
    for groups in (GROUPS, COST_COMPARISON_GROUPS):
        assert sorted(x for ops in groups.values() for x in ops) == [*range(44), WARM]
    compiler = compiler.resolve()
    with TemporaryDirectory(prefix="t733-static-") as tmp:
        families = {family: audit(family, compiler, Path(tmp)) for family in FAMILIES}
    assert families["RX"]["schedule"] == families["DM"]["schedule"]
    assert families["RX"]["costComparison"]["schedule"] == families["DM"]["costComparison"]["schedule"]
    for root in ("Source", "Quantum"):
        assert families["RX"]["schedule"][root]["passesSHA"] == (
            families["RX"]["costComparison"]["schedule"][root]["passesSHA"]
        )
    out = {"schema": "T733_DEPENDENCY_CANDIDATE_V1", "deviceUsed": False,
           "scope": "OFFLINE_SOURCE_CLOSURE_AND_STATIC_GLSLANG_ONLY", "sourceByteLimit": LIMIT,
           "compiler": str(compiler), "compilerSHA": sha(compiler.read_bytes()),
           "families": families, "rootCorrectness": "NOT_MEASURED", "firstValidSwap": "NOT_MEASURED"}
    write("docs/architecture-T733-closure-v1.json", out)
    for family, data in families.items():
        print(family, [(r["role"], r["sourceBytes"]) for r in data["roles"]], data["schedule"])


if __name__ == "__main__":
    main(Path(sys.argv[1]))
