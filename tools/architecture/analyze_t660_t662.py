"""Offline original-byte attribution only. No native load, generator, test or device."""
import collections
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def record(path):
    p = ROOT / path
    b = p.read_bytes()
    return {"path": path, "bytes": len(b), "sha256": sha(b)}


def write(path, obj):
    (ROOT / path).write_bytes((json.dumps(obj, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))


def build_id(b):
    if b[:6] != b"\x7fELF\x02\x01":
        return None
    off = struct.unpack_from("<Q", b, 40)[0]
    stride, count = struct.unpack_from("<HH", b, 58)
    for i in range(count):
        section = struct.unpack_from("<IIQQQQIIQQ", b, off + i * stride)
        if section[1] != 7:
            continue
        at, end = section[4], section[4] + section[5]
        while at + 12 <= end:
            ns, ds, kind = struct.unpack_from("<III", b, at)
            at += 12
            name = b[at:at + ns].rstrip(b"\0")
            at += (ns + 3) // 4 * 4
            desc = b[at:at + ds]
            at += (ds + 3) // 4 * 4
            if name == b"GNU" and kind == 3:
                return desc.hex()
    return None


def artifact(task):
    path = f"test/reports/t257/{task}.json"
    d = json.loads((ROOT / path).read_text(encoding="utf-8-sig"))
    p = Path(d["signed"]["path"])
    out = {"receipt": record(path), "expectedSignedSHA": d["signed"]["sha256"]}
    if not p.exists():
        out["signedArtifact"] = "MISSING_NO_PACKAGE_IDENTITY_PROOF"
        return out
    raw = p.read_bytes()
    out.update(actualSignedSHA=sha(raw), signedHashMatches=sha(raw) == d["signed"]["sha256"], modules=[])
    with zipfile.ZipFile(p) as z:
        for n in z.namelist():
            if n.endswith(".so") or n.endswith(".abc") or n.endswith("module.json"):
                b = z.read(n)
                entry = {"entry": n, "bytes": len(b), "sha256": sha(b)}
                if n.endswith(".so"):
                    entry["buildId"] = build_id(b)
                    if n.endswith("libgrsm1.so"):
                        entry["embeddedGLSLSources"] = []
                        for match in re.finditer(b"#version (?:310|320) es", b):
                            end = b.find(b"\0", match.start())
                            if end > match.start():
                                source = b[match.start():end]
                                entry["embeddedGLSLSources"].append({"fileOffset": match.start(), "bytes": len(source), "sha256": sha(source)})
                out["modules"].append(entry)
    out["loadedDeviceModuleIdentity"] = "NOT_CAPTURED_SAME_PROCESS_MAPS_NOT_PRESENT"
    return out


def t662():
    p = "test/reports/t491/t591c/collector.ndjson"
    raw = (ROOT / p).read_bytes()
    lines = raw.splitlines()
    rows = [json.loads(x) for x in lines]
    tags = collections.Counter(x["tag"] for x in rows)
    diag = [(x, json.loads(x["body"])) for x in rows if x["tag"] in ("ARMED_INITIAL", "DISCOVER_TICK", "LOCKED_TICK")]
    attempts = [(x, d) for x, d in diag if "bootSerial" in d]
    final = attempts[-1][1]
    identity = ["bootSerial", "epoch", "window", "ownerTid", "eglContext", "candidateRun", "surface", "contextGeneration", "geometry", "config", "owner", "layoutSha", "combinedShaderSha", "K", "Kw", "wire", "policy", "nativeBeginNs"]
    logs = [x for x in rows if x["tag"].startswith("OBJ")]
    parsed_logs = [json.loads(x["body"]) for x in logs]
    stable = all(all(d[k] == final[k] for k in identity) for _, d in attempts)
    actual_terminal = [x["seq"] for x, d in attempts if d.get("sealed") is True and int(d.get("nativeEndNs", "0")) > 0]
    out = {"schema": "T662_ORIGINAL_BYTE_ATTRIBUTION_V1", "notUsingDevice": True,
           "original": record(p), "lineCount": len(rows), "tags": dict(tags),
           "sequenceContiguous": [x["seq"] for x in rows] == list(range(len(rows))),
           "diagnosticTicks": [x["tick"] for x, _ in diag],
           "nativeQueryCount": len(diag) + len(logs), "diagnosticQueries": len(diag), "objectLogQueries": len(logs),
           "queryBudget64Satisfied": len(diag) + len(logs) <= 64,
           "reportArithmetic": {"expression": "39*1+8*3+2", "value": 65, "within64": False, "rawActual": 64},
           "maxPersistedUTF8LineBytes": max(map(len, lines)), "directoryBytes": sum((ROOT / p).parent.joinpath(n).stat().st_size for n in ["collector.ndjson", "l1.json", "l2.json"]),
           "directoryBudgetScope": "three original T591c files collector.ndjson/l1.json/l2.json; exclude later other-task captures in shared directory",
           "identity": {k: final[k] for k in identity}, "identityStableAcross39AttemptSamples": stable,
           "actualTerminalRows": actual_terminal, "falseTerminalSeen": tags["TERMINAL_SEEN"],
           "endReason": "BUDGET_EXHAUSTION_INFERRED_FROM64_CALLS_AND_FIXTURE; NO_EXPLICIT_WINDOW_END_RECEIPT",
           "verdict": "OPEN_CALL_AT_OBSERVATION_END_NOT_PERMANENT_HANG",
           "last": {k: final[k] for k in ["nativeEndNs", "sealed", "inCall", "inCallDurationNs", "code", "dropped", "incomplete", "firstFailure", "current", "events", "programs"]},
           "logRounds": [{"tick": tick, "objects": [x["tag"] for x in logs if x["tick"] == tick]} for tick in sorted(set(x["tick"] for x in logs))],
           "allObjectHexEmpty": all(x.get("hex", "") == "" for x in parsed_logs),
           "firstUnreturnedAPI": "UNKNOWN_WITHIN_LINK_STAGE: glLinkProgram OR glGetProgramiv(GL_LINK_STATUS) OR glGetError; observer/recorder gaps also unexcluded",
           "lossClaim": "NO_GAP_IN_PERSISTED_ROWS_AND_NATIVE_DROPPED0_ONLY; collector may silently reject oversized lines, not a general no-loss proof",
           "sourceFiles": [record(x) for x in ["docs/test-report-T591-v3.md", ".codex/local-test/t562-m2-fix/entry/src/main/ets/physics/T591CreateCollector.ets", ".codex/local-test/t562-m2-fix/entry/src/main/cpp/gpu_resident/m2/create_diagnostics.h", ".codex/local-test/t562-m2-fix/entry/src/main/cpp/gpu_resident/m2route/create_shader_identity.h"]],
           "package": artifact("T591-collector")}
    qp = ".codex/local-test/t562-m2-fix/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/quantum_core_kernel.h"
    qb = (ROOT / qp).read_bytes().replace(b"\r\n", b"\n")
    match = re.search(rb'R"([^ (]*)\((.*?)\)\1"', qb, re.S)
    qsource = match.group(2) if match else b""
    embedded = next(m["embeddedGLSLSources"] for m in out["package"]["modules"] if m["entry"].endswith("libgrsm1.so"))
    planned = [(0, "SOURCE_V3", final["programs"][0]["sha256"]), (1, "QUANTUM_INTERMEDIATE", sha(qsource)), (2, "QUANTUM_FINAL_V3", "0ef4f0457a8be135953bff0ce61845d854c6c7774e36ee0d6a79a7a3605a31cc")]
    out["plannedPackageProgramsNotRuntimeSubstitutes"] = [{"object": o, "role": role, "embeddedSources": [s for s in embedded if s["sha256"] == digest], "attemptEnteredBuild": o == 0} for o, role, digest in planned]
    out["plannedShaderABI"] = {"localSizeX": 64, "SSBOBindings": {"0": "Base", "1": "Candidate", "2": "Workspace", "3": "Output"}, "reflectionOrRuntimeBindingReached": False}
    out["sourceFiles"].append(record(qp))
    write("docs/analysis-T662-evidence-v1.json", out)
    return {k: out[k] for k in ["lineCount", "nativeQueryCount", "maxPersistedUTF8LineBytes", "directoryBytes", "identityStableAcross39AttemptSamples", "actualTerminalRows", "logRounds"]}


def ui_texts(value):
    if isinstance(value, dict):
        for k, v in value.items():
            if k == "text" and isinstance(v, str) and v:
                yield v
            else:
                yield from ui_texts(v)
    elif isinstance(value, list):
        for v in value:
            yield from ui_texts(v)


def t661():
    base = ROOT / "test/reports/t491/t654a"
    observations = []
    for p in sorted(base.glob("*.json")):
        if p.name == "v1-partial.json":
            continue
        texts = list(ui_texts(json.loads(p.read_text(encoding="utf-8-sig"))))
        useful = [s for s in texts if any(k in s for k in ["[STATUS", "[FULL", "STALE_", "CONFIG_FAILED", "attempt", "G13", "UNCONFIGURED"])]
        observations.append({**record(p.relative_to(ROOT).as_posix()), "relevantTexts": useful})
    captures = []
    for p in sorted(base.glob("t654-capture*.txt")):
        text = p.read_text(encoding="utf-8-sig")
        statuses = []
        for line in text.splitlines():
            if line.startswith("[STATUS"):
                body = line.split("] ", 1)[1]
                try:
                    json.loads(body)
                    valid = True
                except json.JSONDecodeError:
                    valid = False
                statuses.append({"chars": len(body), "completeJSON": valid, "raw": body})
        captures.append({**record(p.relative_to(ROOT).as_posix()), "statuses": statuses})
    fixture = ".codex/local-test/t638-delivery/entry/src/main/ets/pages/T654Probe.ets"
    out = {"schema": "T661_G13_CAPTURE_ATTRIBUTION_V1", "notUsingDevice": True,
           "captures": captures, "uiOriginals": observations,
           "fixture": record(fixture),
           "fixtureDefects": ["status.slice(0,600) truncates native.capability/initialization", "first attemptId regex reads status API invocation id, not configure queued attempt", "default110/209 are historical process-local ids", "page1/2 body not persisted; page0/3 truncated to2400", "router/new configure changes context rather than same-page stable failure read", "no raw base64 decode/SHA/controlanchor files"],
           "endpoint": {"args": ["decimal attempt string; 0 discovers latest", "integer number page in[0,3]"], "acceptedAttempt": "currentConfigTicket.attempt OR firstFailureAttempt; queuedTicket alone insufficient", "return": "8192 raw bytes base64 plus metadata; independent Store mutex; no App/GL command"},
           "sourceFiles": [record(x) for x in ["docs/test-report-T654-v1.md", ".codex/local-test/t638-delivery/entry/src/main/ets/entryability/G13ResearchAbility.ets", "entry/src/main/ets/pages/GpuResidentG13.ets", "entry/src/main/cpp/gpu_resident/g13flow/init_diagnostics.h", "entry/src/main/cpp/gpu_resident/g13flow/bridge.cpp", "entry/src/main/cpp/gpu_resident/g13flow/transport.cpp"]],
           "signedVersions": [artifact("T654-probe" + (str(i) if i > 1 else "")) for i in range(1, 8)],
           "historicalProbeSources": "Only current .ets source and per-version signed packages/ABC bytes available; no versioned .ets archive proving all seven sources identical",
           "error29": "l2 boot271023133916096/attempt209 and l4 boot271319633950940/attempt110 explicitly CAPABILITY:EXIT then CONFIGURE_ERROR_29; exact failing raw values NOT_CAPTURED; not NOT_REACHED and not CAP_PASS",
           "verdict": "TESTER_FIXTURE_AND_IDENTITY_GAP; SAME_PAGE_COLLECTION_NOT_DISPROVEN; NO_PRODUCT_PERSISTENCE_THREAD_PREREQUISITE"}
    out["currentToBuildTreeSourceComparison"] = []
    for name in ["bridge.cpp", "transport.cpp", "init_diagnostics.h"]:
        rel = "entry/src/main/cpp/gpu_resident/g13flow/" + name
        snap = ".codex/local-test/t638-delivery/" + rel
        out["currentToBuildTreeSourceComparison"].append({"current": record(rel), "buildTree": record(snap), "identical": (ROOT / rel).read_bytes() == (ROOT / snap).read_bytes()})
    write("docs/analysis-T661-evidence-v1.json", out)
    return {"captures": [{"path": c["path"], "statuses": [{k: s[k] for k in ["chars", "completeJSON"]} for s in c["statuses"]]} for c in captures], "uiFiles": len(observations), "signedVersions": [{"sha": x.get("actualSignedSHA"), "matched": x.get("signedHashMatches")} for x in out["signedVersions"]]}


if __name__ == "__main__":
    print(json.dumps({"T662": t662(), "T661": t661()}, ensure_ascii=False, indent=2))
