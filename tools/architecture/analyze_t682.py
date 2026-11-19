"""Audit saved T682 bytes, HAPs and source without running product or GL."""
import json
import re
import zipfile
from pathlib import Path

from analyze_t660_t662 import ROOT, artifact, record, sha, ui_texts, write

RAW = "test/reports/t491/t590c/"
PREFIX = RAW + "t468-create-1-1795092162916-"
BASE = ".codex/local-test/t663-delivery/entry/src/main/"
CPP = BASE + "cpp/gpu_resident/"


def read(path):
    return json.loads((ROOT / path).read_bytes())


def native_module(package):
    return next(m for m in package["modules"] if m["entry"].endswith("libgrsm1.so"))


def audit():
    native = read(PREFIX + "native.json")
    manifest = read(PREFIX + "manifest.json")
    receipts = []
    for item in [manifest["native"], *manifest["rawLogs"]]:
        path = RAW + item["name"]
        if not (ROOT / path).is_file():
            receipts.append({"path": path, "manifestBytes": item["bytes"],
                             "manifestSha256": item["sha256"], "status": "MANIFEST_ONLY"})
            continue
        saved = record(path)
        assert saved["bytes"] == item["bytes"] and saved["sha256"] == item["sha256"], path
        receipts.append({**saved, "status": "VERIFIED_BYTES"})

    latest = native["apiCursor"]
    current = native["current"]
    for cursor_key, snapshot_key in (("epoch", "epoch"), ("window", "window"),
                                     ("context", "eglContext"), ("ownerTid", "ownerTid"),
                                     ("bootSerial", "bootSerial")):
        assert latest[cursor_key] == native[snapshot_key], cursor_key
    assert (latest["api"], latest["edge"], latest["object"], latest["handle"]) == (6, 1, 2, 4)
    assert latest["reservedZero"] == [0] * 32
    assert latest["endNs"] == current["exitNs"] == native["nativeEndNs"] == "0"
    assert (latest["returned"], latest["rawGl"], native["sealed"]) == (-1, -1, False)
    assert current["object"] == latest["object"] and current["handle"] == latest["handle"]
    sample_ns = int(current["enterNs"]) + int(native["inCallDurationNs"])
    timing = {
        "derivedSerializerNowNs": str(sample_ns),
        "apiEnterToSerializerObservationNs": str(sample_ns - int(latest["beginNs"])),
        "attemptBeginToObservationNs": str(sample_ns - int(native["nativeBeginNs"])),
        "currentStageEnterBeforeApiEnterNs": str(int(latest["beginNs"]) - int(current["enterNs"])),
    }
    completed = []
    for row in native["events"]:
        if row["stage"] in ("COMPILE", "LINK", "ATTACH"):
            completed.append({**row, "durationNs": str(int(row["exitNs"]) - int(row["enterNs"]))})
    final_compile = next(e for e in completed if e["object"] == 2 and e["stage"] == "COMPILE")
    attach = next(e for e in completed if e["object"] == 2 and e["stage"] == "ATTACH")
    assert final_compile["success"] and final_compile["rawGl"] == 0
    assert attach["handle"] == latest["handle"] and attach["a"] == str(final_compile["handle"])

    old_path = "test/reports/t491/t591c/collector.ndjson"
    rows = [json.loads(line) for line in (ROOT / old_path).read_bytes().splitlines()]
    attempts = [json.loads(row["body"]) for row in rows if row["tag"] == "LOCKED_TICK"]
    old = attempts[-1]
    assert old["current"]["object"] == 0 and old["current"]["stage"] == "LINK"
    assert "apiCursor" not in old and old["nativeEndNs"] == "0" and not old["sealed"]
    assert old["programs"][0]["sha256"] == native["programs"][0]["sha256"]
    assert not old["programs"][2]["sha256"]
    assert old["layoutSha"] == native["layoutSha"] and old["combinedShaderSha"] == native["combinedShaderSha"]
    old_fields = ("bootSerial", "epoch", "window", "eglContext", "ownerTid", "nativeBeginNs",
                  "nativeEndNs", "sealed", "inCall", "inCallDurationNs", "current", "programs", "driver")

    packages = {t: artifact(t) for t in ("T590-cursor2", "T591-collector", "T590-a2")}
    assert all(p["signedHashMatches"] for p in packages.values())
    assert len({native_module(p)["buildId"] for p in packages.values()}) == 3
    delivered = read("test/reports/t663/compile-receipt.json")
    binary = native_module(packages["T590-cursor2"])
    assert binary["sha256"] == delivered["packagedELF"]["sha256"]
    assert binary["buildId"] == delivered["packagedELF"]["buildId"]
    sign = read("test/reports/t257/T590-cursor2.json")
    p = Path(sign["input"]["path"])
    assert sha(p.read_bytes()) == sign["input"]["sha256"]
    with zipfile.ZipFile(p) as z:
        assert sha(z.read(binary["entry"])) == binary["sha256"]

    full_source = (ROOT / (CPP + "m2route/full_kernel.cpp")).read_bytes()
    blobs = re.findall(rb'R"GRS_M2\((#version .*?)\)GRS_M2"', full_source.replace(b"\r\n", b"\n"), re.S)
    assert len(blobs) == 2
    sources = []
    for program in native["programs"]:
        ident = (program["sha256"], program["sourceBytes"])
        sources.append({"program": program, "packagesContainingExactBytes": [
            t for t, package in packages.items() if any(
                (s["sha256"], s["bytes"]) == ident for s in native_module(package)["embeddedGLSLSources"])]})
        assert len(sources[-1]["packagesContainingExactBytes"]) == len(packages)
    for obj, source in zip((0, 2), blobs):
        assert sha(source) == native["programs"][obj]["sha256"]
        sources[obj]["generatedLiteralMatchesBytes"] = len(source) == native["programs"][obj]["sourceBytes"]
        sources[obj]["storageBlockDeclarations"] = len(re.findall(rb"\bstd430\b", source))
    src_paths = ["m2/create_diagnostics.h", "m2/create_diagnostics_abi.cpp", "m2route/full_transport.cpp",
                 "m2route/full_kernel.cpp", "m2route/source_transport.cpp", "m2route/quantum_candidate.cpp",
                 "m2route/quantum_kernel.cpp", "m2route/create_shader_identity.h", "app_bridge.cpp"]
    contract_paths = ["test/reports/t663/formal-contract.json", "test/reports/t663/compile-receipt.json",
                      "test/reports/t663/generated-sources.json", "test/t468/generate.cjs",
                      "docs/test-report-T590-v3.md", "docs/test-report-T591-v3.md",
                      "docs/architecture-T678-contract-v1.json", "docs/test-report-T491-scene-cache-device-v1.md",
                      "docs/architecture-T462-program-binary-persistence-v1.md", RAW + "t590-boot.json",
                      BASE + "ets/pages/T590Cursor.ets", BASE + "ets/pages/GpuSandWaterExperience.ets",
                      BASE + "ets/entryability/EntryAbility.ets", BASE + "resources/base/profile/main_pages.json"]
    for path, digest in delivered["actualCompiledOwnedSources"].items():
        assert record(".codex/local-test/t663-delivery/" + path)["sha256"] == digest
    context = {k: v for k, v in native.items() if k not in ("events", "programs")}
    ui = []
    for path in sorted((ROOT / RAW).glob("cur_l*.json")):
        rel = path.relative_to(ROOT).as_posix()
        strings = list(ui_texts(read(rel)))
        ui.append({**record(rel), "evidenceText": [
            s for s in strings if any(k in s for k in ("GPU M2", "STATE_LOST", "T676-A", "CREATE_DIAGNOSTIC"))]})
    return {
        "schema": "T682_M2_OPEN_CALL_OBJECT_ATTRIBUTION_V1", "productOrTestsRun": False,
        "deviceUsed": False, "buildRun": False, "exportReceipts": receipts,
        "declaration": manifest["declaration"], "newSnapshot": context,
        "events": native["events"], "completedProgramStageDurations": completed, "derivedTiming": timing,
        "sources": sources, "T591": {"raw": record(old_path), "lines": len(rows),
                                    "final": {k: old[k] for k in old_fields},
                                    "apiCursor": "NOT_PRESENT",
                                    "uniqueScopeCount": len({tuple(d[k] for k in ("bootSerial", "epoch", "window", "eglContext", "ownerTid")) for d in attempts})},
        "signatures": packages, "c8434b96UnsignedSha": sign["input"]["sha256"],
        "sourceFiles": [record(p) for p in contract_paths + [CPP + item for item in src_paths]],
        "uiArtifacts": ui, "loadedModule": "UNVERIFIED_MANIFEST_DECLARATION_MISSING",
        "verdict": "T590_OPEN_AT_SAMPLE_FINAL_QUANTUM_LINK_PROGRAM; T591_SOURCE_COMPOSITE_LINK_ONLY",
    }


if __name__ == "__main__":
    result = audit()
    write("docs/analysis-T682-evidence-v1.json", result)
    print(result["verdict"], result["derivedTiming"])
