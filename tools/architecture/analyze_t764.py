"""Reconcile T760's signed BOOT capture and the isolated G13 probe plan offline."""

import json
from pathlib import Path
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write
from analyze_t741 import doc
from analyze_t753 import binding, header_passes


COMPILE = "docs/receipt-T756-split-v1.json"
SIGN = "test/reports/t257/T760-split.json"
CAPTURE = "test/reports/t491/t760/"
SPLIT_SRC = "entry/src/main/cpp/gpu_resident/g13flow/"
DISPATCH = "docs/architecture-T607-g13-dispatch-v1.json"
LIMIT = 49152


def packaged_identity():
    build = doc(COMPILE)
    signed = doc(SIGN)
    assert signed["verifyApp"] == "PASS"
    assert signed["input"]["sha256"] == build["unsignedSHA256"]
    name = "libs/arm64-v8a/libgrs_g13.so"
    artifacts, files = {}, []
    for key in ("input", "signed"):
        path = Path(signed[key]["path"])
        artifacts[key] = record(path.relative_to(ROOT).as_posix())
        assert artifacts[key]["sha256"] == signed[key]["sha256"]
        with ZipFile(path) as archive:
            files.append(archive.read(name))
    elf = files[1]
    assert files[0] == elf and sha(elf) == build["nativeSHA256"]
    assert build_id(elf) == build["buildId"]
    roles = {}
    for name, details in build["roles"].items():
        if details["kind"] != "compute":
            continue
        input_file = ROOT / build["project"] / details["path"]
        source = input_file.read_bytes()
        assert len(source) == details["bytes"] <= LIMIT
        assert sha(source) == details["sha256"] and elf.count(source + b"\0") == 1
        roles[name] = {"sourceSHA": sha(source), "bytes": len(source), "opcodes": details["opcodes"]}
    assert len(roles) == 6 and max(role["bytes"] for role in roles.values()) == 48755
    return {"receipts": [record(COMPILE), record(SIGN)], "hap": artifacts, "elfBytes": len(elf),
            "elfSHA": sha(elf), "buildId": build["buildId"], "computeRoles": roles}


def diagnostic_nodes(node):
    if isinstance(node, dict):
        for value in node.values():
            yield from diagnostic_nodes(value)
    if isinstance(node, list):
        for value in node:
            yield from diagnostic_nodes(value)
    if isinstance(node, str) and node.startswith('{"schema":"G13InitDiagnosticV1"'):
        yield json.loads(node)


def boot_capture(identity):
    path = CAPTURE + "t760-diag-full.json"
    diagnostic = doc(path)
    assert diagnostic["lastProgress"] == "BOOT_EFFECTIVE_TERMINAL:EXIT"
    assert diagnostic["currentLifecycle"] == "CONFIG_FAILED" and diagnostic["firstConfigFailure"] == "INIT_ERROR_27"
    assert diagnostic["firstFailureAttempt"] == "191" and diagnostic["bootReadyNs"] == "0"
    proof = diagnostic["bootQualification"]
    assert proof["effectiveRejectStage"] == "OP65" and proof["result"] == 27
    scope, program = proof["scope"], proof["program"]
    loaded = program["loadedModule"]
    assert loaded["memoryBuildIdMatched"] and loaded["BuildId"] == scope["mappedBuildId"] == identity["buildId"]
    assert (loaded["sha256"], int(loaded["bytes"])) == (identity["elfSHA"], identity["elfBytes"])
    assert program["sourceSHA"] == identity["computeRoles"]["BOOT"]["sourceSHA"]
    assert (scope["pid"], scope["ownerTid"], scope["processStartTicks"], scope["attempt"]) == (46924, 47218, "8613992", "191")
    assert (scope["fence"], scope["firstGl"], scope["rawEgl"]) == (37146, 0, 12288)
    op65, op66 = proof["op65"], proof["op66"]
    assert (op65["ctrl255"], op65["ctrl7"], op65["mathReadGl"], op65["inputReadGl"]) == (0xC0DE1301, 27, 0, 0)
    assert op65["input744To747"] == [1, 0x33800000, 0x3F800000, 0]
    assert op65["work35_41_47_53_59"] == [0, 0x3F800000, 0x3F800000, 0, 0x3F800000]
    assert op66["state"] == "REJECTED" and op66["presence"] == [True, True] and op66["mismatchMasks"] == [12, 12]
    assert op66["ctrl200To215"] == [0x3F800001, 0x3F800000, 0, 0, 7, 0, 0, 0x46503332] * 2
    prior_path = "test/reports/t491/t730b/v5/l2.json"
    prior_rows = list(diagnostic_nodes(doc(prior_path)))
    assert prior_rows
    for row in prior_rows:
        prior = row["bootReadback"]
        assert prior["attempt"] == "250"
        assert prior["work65_35_41_47_53_59"] == op65["work35_41_47_53_59"]
        assert prior["ctrl200To215"] == op66["ctrl200To215"]
    events_path = CAPTURE + "t756-compute-persisted.jsonl"
    events = [json.loads(line) for line in (ROOT / events_path).read_text(encoding="utf-8-sig").splitlines()]
    assert [e["stage"] for e in events] == [
        "COMPILE_ENTER", "LINK_ENTER", "LINK_OK_REFLECTION_PENDING", "REFLECT_OK",
        "EXPORT_LENGTH_ENTER", "EXPORT_ENTER", "EXPORT_WRITE_SUBMITTED",
    ]
    last = events[-1]
    assert all(e["shaderSHA"] == program["sourceSHA"] for e in events)
    assert all(e["key"] == program["cacheKey"] and e["route"] == "SOURCE_COMPILE" for e in events)
    assert (last["programHandle"], last["contextHandle"]) == (scope["program"], int(scope["eglContext"]))
    assert (last["persistence"], last["binaryBytes"], last["originalRawGL"]) == ("PERSISTED", 301961, 0)
    return {"inputRecords": [record(p) for p in (path, prior_path, events_path)], "scope": scope,
            "bootSourceSHA": program["sourceSHA"], "repeatedFp32Words": {"op65": op65, "op66": op66},
            "bootBinary": last, "otherComputeRoles": "NOT_REACHED", "warmNewProcess": "NOT_OBSERVED",
            "steadyVisibility": "NOT_REACHABLE"}


def published_plan(identity):
    path = Path(doc(COMPILE)["project"]) / SPLIT_SRC
    transport_path, bridge_path = path / "transport.cpp", path / "bridge.cpp"
    transport = (ROOT / transport_path).read_text(encoding="utf-8")
    bridge = (ROOT / bridge_path).read_text(encoding="utf-8")
    boot_tail = transport.split("const bool auditFailed =", 1)[1].split("void Transport::Abort", 1)[0]
    assert boot_tail.index('return finishBoot("OP65"') < boot_tail.index('return finishBoot("OP66"') < boot_tail.index("ready_ = true")
    steady = bridge.split("controller_.Gpu().BootReady() && !controller_.Ready()", 1)[1]
    assert steady.index("PrepareSteady(false)") < steady.index('State("COMPUTE_READY")') < steady.index('"INIT_ERROR_"')
    pump = transport.split("uint32_t Transport::PumpBounded", 1)[1].split("bool Transport::CopyBuffer", 1)[0]
    assert pump.index("if (!Bind(") < pump.index("glDispatchCompute(") < pump.index("GL_SHADER_STORAGE_BARRIER_BIT")
    assert pump.index("GL_BUFFER_UPDATE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT") < pump.index("glFenceSync(")
    rows = doc(DISPATCH)["quantum"]
    frozen_rows = header_passes(ROOT / path / "contract_generated.h", "quantum")
    probes = []
    expected = {5: 12, 26: 33, 27: 48, 28: 48, 410: 12, 431: 33, 432: 48, 433: 48, 814: 6, 817: 9, 818: 10}
    for ordinal, opcode in expected.items():
        p = rows[ordinal]
        assert (p["index"], p["opcode"]) == (ordinal, opcode)
        fields = tuple(p.get(key, 0) for key in ("opcode", "scope", "J", "micro", "iterationBlock"))
        assert fields == frozen_rows[ordinal]
        route = binding("QUANTUM", p)
        role, = [name for name, data in identity["computeRoles"].items() if opcode in data["opcodes"]]
        probes.append({"ordinal": ordinal, "opcode": opcode, "logicalScope": fields[1], "J": fields[2],
                       "micro": fields[3], "block": fields[4], "route": vars(route), "role": role})
    by_ordinal = {p["ordinal"]: p for p in probes}
    assert [by_ordinal[i]["role"] for i in (26, 27, 431, 432)] == ["OP_18", "OP_48"] * 2
    assert [by_ordinal[i]["route"]["ssbo1"] for i in (5, 410, 814, 817)] == [
        "CANDIDATE", "REPLAY", "REPLAY", "PENDING_PACK"]
    trace = [(2048, 10240)[by_ordinal[i]["route"]["shader_scope"]] + 1024 + 384 for i in (27, 432)]
    assert trace == [3456, 11648]
    assert all(0 < face % 65 < 64 for face in (127, 128)) and (127 % 64, 128 % 64) == (63, 0)
    return {"authorities": [record(str(p.as_posix())) for p in (transport_path, bridge_path, Path(DISPATCH))],
            "rows": probes, "firstBlockWordOffsets": trace, "neighborFaces": [127, 128],
            "rootTailBytes": {"offset": 73728, "length": 1024}, "packVertexStride": 64, "tracerVertexStride": 32}


def main():
    identity = packaged_identity()
    write("docs/architecture-T764-evidence-v1.json", {
        "schema": "T764_G13_PREQUALIFICATION_VISIBILITY_ARCHITECTURE_V1",
        "deviceUsed": False, "productImplementationChanged": False, "artifact": identity,
        "t760Capture": boot_capture(identity), "pinnedProbePlan": published_plan(identity),
    })
    print("T764 verified signed BOOT, original FTZ words and frozen T756 probe rows")


if __name__ == "__main__":
    main()
