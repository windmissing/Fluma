"""Check the T736 SOURCE handoff against the same signed inputs as T733."""
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory

from analyze_t733 import FAMILIES, GROUPS, LIMIT, ROOT, actual_source, split_inputs
from analyze_t660_t662 import record, sha, write


def document(path):
    return json.loads((ROOT / path).read_bytes())


def source_reflection(compiler, candidate):
    output = subprocess.run([compiler, "-S", "comp", "-l", "-q", candidate],
                            check=True, capture_output=True, text=True).stdout
    uniforms = {
        name: {"glType": "0x" + kind.upper(), "size": int(count)}
        for name, kind, count in re.findall(
            r"^(u\w+): offset -1, type ([0-9a-f]+), size (\d+), index -1,", output, re.M)
    }
    blocks = {
        name: int(binding) for name, binding in re.findall(
            r"^(\w+): offset -1, type ffffffff, size \d+, index \d+, binding (\d+), stages 32, numMembers 1",
            output, re.M)
    }
    assert len(uniforms) == 40 and all(u["size"] == 1 for u in uniforms.values())
    assert blocks == {"ReadState": 0, "WriteState": 1, "Workspace": 2, "EventPath": 3}
    assert "Local size X: 64" in output
    return {"activeUniforms": dict(sorted(uniforms.items())),
            "blocks": dict(sorted(blocks.items(), key=lambda item: item[1])),
            "localSize": [64, 1, 1], "groupsPerPass": [1, 1, 1]}


def audit(compiler):
    t733 = document("docs/architecture-T733-closure-v1.json")
    rx = document("docs/architecture-T641-seed-config-v1.json")["profiles"][0]
    dm = document("docs/architecture-T554-g9-dm-manifest-v2.json")["parameterProfiles"][0]
    profiles = {
        "RX": (rx, 827477575, 1481788999, 3, "templateSHA", "parameterSHA", 63701),
        "DM": (dm, 843924551, 843662407, 2, "seedTemplateSha256", "parameterSha256", 0),
    }
    old_command = "test/reports/t491/t712b/dm20-input/commands.u32"
    raw = struct.unpack("<192I", (ROOT / old_command).read_bytes())
    assert raw[138] == 2 and raw[40] == 0 and not any(raw[64:77])
    q20 = 1 << 20
    values = (0.2, 64.0, 200.0, 0.0, 0.0)
    body_words = struct.unpack("<10I", struct.pack("<5d", *values))
    admitted = int(Fraction.from_float(values[0]) * q20)
    assert admitted == 209715 and 2 * 40 * q20 + admitted <= 400 * q20 <= 512 * q20 - admitted
    seed = document("docs/architecture-T552-g9-seed-manifest-v2.json")["seed"]["canonicalAssignments"]
    assert seed["header"]["18"] == seed["header"]["19"] == 1
    results, reflection, rows = {}, {}, []
    with TemporaryDirectory(prefix="t736-reflect-") as directory:
        for family, (receipt, library, assets, shader_sha) in FAMILIES.items():
            raw_source, baseline = actual_source(receipt, library, shader_sha)
            assert (ROOT / assets / "pipeline.comp").read_bytes().replace(b"\r\n", b"\n") == raw_source
            ops, build = split_inputs(raw_source.decode())
            header = (ROOT / assets / "schema_generated.h").read_text(encoding="utf-8")
            body = re.search(r"kSourcePasses\s*=\s*\{\{(.*?)\}\};", header, re.S)[1]
            table = [(name, ops["G9_OP_" + name], int(role), int(j), int(micro))
                     for name, role, j, micro in re.findall(
                         r"\{Op::(\w+),\s*(\d+),\s*(\d+),\s*(-?\d+)\}", body)]
            assert [t[1:] for t in table] == [(i, 0, 0, -1) for i in range(12)]
            assert not rows or rows == table
            rows = table
            previous = t733["families"][family]
            role = next(r for r in previous["roles"] if r["role"] == "SOURCE")
            shader, _ = build(GROUPS["SOURCE"])
            assert len(shader) == role["sourceBytes"] <= LIMIT and sha(shader) == role["sourceSHA"]
            assert re.findall(r"layout\(std430,binding=(\d)\)(readonly|coherent) buffer (\w+)",
                              shader.decode()) == [
                                  ("0", "readonly", "ReadState"), ("1", "coherent", "WriteState"),
                                  ("2", "coherent", "Workspace"), ("3", "coherent", "EventPath")]
            candidate = Path(directory) / (family + ".comp")
            candidate.write_bytes(shader)
            found = source_reflection(compiler, candidate)
            assert not reflection or found == reflection
            reflection = found
            config, profile, magic, revision, seed_key, param_key, tag = profiles[family]
            assert hashlib.sha256(struct.pack("<64I", *config["config64"])).hexdigest() == config[param_key]
            results[family] = {
                "baseline": baseline, "profileCode": profile, "k": 192,
                "inputMagic": magic, "inputRevision": revision, "inputBody64To76": [*body_words, 2, 1, tag],
                "seedTemplateSHA": config[seed_key], "parameterSHA": config[param_key],
                "sourceRole": {"bytes": len(shader), "sha256": sha(shader),
                               "passSHA": previous["schedule"]["Source"]["passesSHA"]},
            }
    cpp = "entry/src/main/cpp/gpu_resident/"
    for family in ("g9rx", "g9dm"):
        transport = (ROOT / cpp / family / "transport.cpp").read_text(encoding="utf-8")
        assert "permit_.kind == 1 && index == 0" in transport and "pass.op == Op::SOURCE_COMPARE" in transport
        assert "work[8].begin" in transport and "glClientWaitSync(fence_, 0, 0)" in transport
    controller_path, clock_path = (cpp + "g9dm/" + name
                                   for name in ("controller.cpp", "progress_clock_v3.cpp"))
    controller = (ROOT / controller_path).read_text(encoding="utf-8")
    commit = controller.index("transport_.CommitAccepted")
    assert controller.index("RetireSourcePlanLocked") < commit < controller.index("run_.workspaceRetired=true", commit)
    clock = (ROOT / clock_path).read_text(encoding="utf-8")
    assert "||!run.workspaceRetired)return ControlResult::STALE_REJECTED" in clock
    passes = [
        {"ordinal": op, "op": name, "programRole": "SOURCE", "uRole": role, "uJslot": j, "uMicro": micro,
         "bufferIndices": [0 if op in (1, 8) else 2 if op == 9 else 1, 4 if op in (8, 9) else 2, 5, 6],
         **({"beforeCopies": [[0, 1, "stateBytes"]]} if op == 0 else
            {"afterCopies": [[6, 7, "arenaBytes"], [6, 8, "arenaBytes"]]} if op == 9 else {})}
        for name, op, role, j, micro in rows
    ]
    base_mass = 128 * 40 * q20
    return {
        "schema": "T736_SOURCE_CONTINUATION_STATIC_V1", "deviceUsed": False, "runtimeResult": "NOT_RUN",
        "sourceByteLimit": LIMIT, "compilerSHA": sha(compiler.read_bytes()), "families": results,
        "reflection": reflection, "passTable": passes,
        "passTableScope": "Fresh Controller bank 0/candidate 2/arena 0; use live frozen permit at runtime.",
        "firstRootExpected": {
            "kind": 1, "ordinal": 0, "angleIndex": 0, "positiveBodyValues": values,
            "baseMassQ": base_mass, "admittedQ": admitted, "finalMassQ": base_mass + admitted,
            "packetSlot": 0, "packetId": 1, "packetGeneration": 1, "origin": 2,
            "xQ20": 64 * q20, "y2Q21": 400 * q20, "column": 64,
            "eventKinds": [1, 3, 14], "pathCount": 0, "audits": 65535, "outcome": 1,
            "commitDelta": 1, "quantaDelta": 0, "gpuDispatches": 12, "nativeDispatchBarriers": 12,
            "hostCopyBarriersSeparate": 3,
        },
        "historicalDmNotSource": {"file": record(old_command), "rootKindWord138": raw[138], "inputKindWord40": raw[40]},
        "dmControllerFinding": {
            "status": "DETERMINISTIC_BY_SOURCE_INSPECTION", "when": "SOURCE, activeRemaining == 0",
            "reason": "The caller sets workspaceRetired only after CommitAccepted; the earlier retirement preflight requires it.",
            "controller": record(controller_path), "clock": record(clock_path),
        },
    }


if __name__ == "__main__":
    write("docs/architecture-T736-source-contract-v1.json", audit(Path(sys.argv[1]).resolve()))
    print("T736 checked signed shader bytes, two SOURCE reflections, all 12 passes and the K192 inputs")
