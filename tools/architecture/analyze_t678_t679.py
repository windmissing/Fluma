"""Offline evidence, SDK declarations and symbolic byte-count inspection only."""
import json
import math
from pathlib import Path
import re
import zipfile
from analyze_t660_t662 import ROOT, build_id, record, sha, ui_texts, write


def read(path):
    return json.loads((ROOT / path).read_text(encoding="utf-8-sig"))


def package(task):
    p = "test/reports/t257/" + task + ".json"
    r = read(p)
    file = Path(r["signed"]["path"])
    result = {"receipt": record(p), "packageSHA": sha(file.read_bytes()), "receiptMatched": sha(file.read_bytes()) == r["signed"]["sha256"], "inputSHA": r["input"]["sha256"]}
    with zipfile.ZipFile(file) as z:
        for n in ["libs/arm64-v8a/libgrsm1.so", "ets/modules.abc"]:
            b = z.read(n)
            d = {"entry": n, "bytes": len(b), "sha256": sha(b)}
            if n.endswith(".so"):
                d["buildId"] = build_id(b)
                d["registeredNameStringsPresent"] = {s: s.encode() in b for s in ["recoveryState", "beginRecoverySession", "bootDiagnostics", "m2CreateDiagnostics", "executionDiagnosticsV1", "timelineSnapshotBegin", "coldTraceBeginV1"]}
            result[n.rsplit("/", 1)[1]] = d
    assert result["receiptMatched"]
    return result


def t678():
    bp = "test/reports/t491/t590c/t590-boot.json"
    boot = read(bp)
    summaries = []
    for a in boot["attempts"]:
        summaries.append({**{k: v for k, v in a.items() if k != "rows"}, "rows": a["rows"], "interpretation": "lifecycle revoked attempt" if a["bootSerial"] in ["1", "2"] else "OPEN_PROGRAM_STAGE_AT_OBSERVATION; no create EXIT/ready/root/draw observed"})
    ui = []
    for p in sorted((ROOT / "test/reports/t491/t593c").glob("l*.json")):
        strings = list(ui_texts(json.loads(p.read_text(encoding="utf-8-sig"))))
        relevant = [s for s in strings if "GRS_M1_PROGRESS_CLOCK" in s]
        scopes = []
        for s in relevant:
            m = re.search(r"run (\d+):(\d+) / epoch (\d+) / session (\d+) / sample (\d+)", s)
            if m:
                d = dict(zip(["runHi", "runLo", "epoch", "session", "sampleSerial"], m.groups()))
                c = re.search(r"物理 (\d+):(\d+)", s)
                if c:
                    d["physicalCommitHi"], d["physicalCommitLo"] = c.groups()
                d["busy"] = "DRAW_PENDING" if "DRAW_PENDING" in s else "GPU_WORK_PENDING" if "GPU_WORK_PENDING" in s else "OTHER"
                scopes.append(d)
        ui.append({**record(p.relative_to(ROOT).as_posix()), "texts": relevant, "scopes": scopes})
    a2ui = []
    for p in sorted((ROOT / "test/reports/t491/t590c").glob("l*.json")):
        strings = list(ui_texts(json.loads(p.read_text(encoding="utf-8-sig"))))
        relevant = [s for s in strings if any(k in s for k in ["GRS_M1_PROGRESS", "CREATING_NEW", "CONFIGURE_FAILED", "READY"])]
        a2ui.append({**record(p.relative_to(ROOT).as_posix()), "texts": relevant})
    a2 = ".codex/local-test/t541-t624-a2/entry/src/main/"
    m1 = ".codex/local-test/t597-fixed/entry/src/main/"
    sources = [a2 + p for p in ["cpp/gpu_resident/app_bridge.cpp", "cpp/gpu_resident/boot_probe.h", "cpp/gpu_resident/submit_observer.h", "cpp/gpu_resident/m2/create_diagnostics.h", "cpp/gpu_resident/m2route/full_transport.cpp", "cpp/CMakeLists.txt"]]
    sources += [m1 + p for p in ["cpp/gpu_resident/app_bridge.cpp", "cpp/gpu_resident/boot_probe.h", "cpp/gpu_resident/recovery_v3.h", "ets/physics/GpuM1DemoController.ets", "ets/pages/GpuResidentM1.ets", "ets/physics/GpuRecoveryPulse.ets"]]
    out = {"schema": "T678_M1_BUSY_AND_A2_BOOT_ATTRIBUTION_V1", "deviceUsed": False, "buildRun": False, "productTestsRun": False,
           "bootOriginal": record(bp), "bootPID": boot["pid"], "attempts": summaries, "transitions": boot["transitions"], "recovery": boot["recovery"],
           "bootSemantics": {"terminal": "explicit terminal Mark, revoke transition, failed Draw, or first quantum accepted AND first Draw done; not glCreate completion flag", "complete": "terminal && dropped==0, not product success", "firstFailure": "failure-marked lifecycle event including SURFACE_CREATED/reset, not necessarily raw driver error", "windowId": "bootSurface surrogate counter, not native pointer", "ownerTid": "observed owner thread, no captured EGLContext pointer in JSON", "snapshotNowNs": "NOT_CAPTURED; filename wall clock is not monotonic call duration"},
           "A2LastPoint": {"major": "TRANSPORT_CREATE_ENTER then SUBMIT_OBSERVER26/27/28", "stage28": "ProgramEnter", "code": "0 is Enter/default, not successful link return", "missing": "ProgramExit29, TransportCreateExit, READY, root/draw", "exactAPI": "not separable within existing coarse link/status/error sequence", "permanentHang": "NOT_PROVEN"},
           "M1UI": ui, "packages": {"T590-a2": package("T590-a2"), "T593-m1": package("T593-m1")},
           "A2UIOriginals": a2ui,
           "M1CounterSequence": [int(s["physicalCommitHi"]) * 4294967296 + int(s["physicalCommitLo"]) for row in ui for s in row["scopes"] if s.get("runLo") == "2" and s.get("epoch") == "4" and "physicalCommitLo" in s],
           "M1BusyMeaning": {"GPU_WORK_PENDING": "FreezePermitLocked status marks old frame stale for a new runtime root; not creation marker", "DRAW_PENDING": "AcceptLocked after guarded accepted callback updates accepted/source/physics counters then awaits presentation", "statusReady": "AllowedLocked permission separate from current/stale display state", "downAllowed": "does not require s.state==CURRENT; UI STALE label alone is not NOT_READY evidence", "sourceReceipt0": "not a complete enqueue/claim ledger"},
           "M1EndpointAvailability": "recoveryState/beginRecoverySession/bootDiagnostics exist in actual T597 module names and source; report NOT_AVAILABLE not established by missing UI button",
           "sources": [record(p) for p in sources],
           "hilog": record("test/reports/t491/t593c/hilog-t593c.txt"),
           "missingSameAttempt": ["pid-start/process boot key/context pointer/source/layout in current M1 raw", "full READY/current permission vs render/stale status", "pulse receipt/Fresh/gravity", "Down handler/reason/typed enqueue and same seq claim/root", "runtime first accepted/draw root tickets and original GL/EGL/clock times"],
           "verdict": "SPLIT_A2_M2_OPEN_CREATE_FROM_FORMAL_M1_POST_CREATE_BUSY; SHARED_CREATE_ROOT_CAUSE_NOT_ESTABLISHED",
           "noNewProductSeamEstablished": True, "reuse": "T676-A/T663 for new M2 exact API; T676-B existing formal M1 endpoints/collector; no duplicate implementation or device gate"}
    write("docs/analysis-T678-evidence-v1.json", out)
    return {"A2module": out["packages"]["T590-a2"]["libgrsm1.so"], "M1module": out["packages"]["T593-m1"]["libgrsm1.so"], "counterSequence": out["M1CounterSequence"]}


def t679():
    cases = read("docs/design-T673-research-cases-v1.json")["cases"]
    design = read("docs/design-T673-roadmap-evidence-v1.json")
    rawrows = (ROOT / "test/reports/t491/t591c/collector.ndjson").read_text(encoding="utf-8-sig").splitlines()
    samples = [json.loads(json.loads(r)["body"]) for r in rawrows if json.loads(r)["tag"] in ["DISCOVER_TICK", "LOCKED_TICK"]]
    device = samples[-1]
    sdk = Path("C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/sysroot/usr/include/GLES3/gl31.h")
    header = sdk.read_text(encoding="utf-8-sig")
    symbols = ["GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS", "GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS", "GL_MAX_SHADER_STORAGE_BLOCK_SIZE", "GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT", "glDispatchCompute", "glMemoryBarrier"]
    assert all(s in header for s in symbols)
    examples = []
    for row in design["payloadExamples"]:
        grid = row["grid"];cells = math.prod(grid);nodes = math.prod(n + 1 for n in grid)
        points = cells * (4 if len(grid) == 2 else 8)
        perbank = row["pointStride"] * points + row["nodeStride"] * nodes
        sort_one = 16 * row["support"] * points
        assert row["stateBytes"] == 3 * perbank and row["sortBytes"] == 2 * sort_one
        faces = sum(math.prod((v + 1) if k == j else v for k, v in enumerate(grid)) for j in range(len(grid)))
        examples.append({**row, "classification": "RESEARCH_ASSUMPTION_BYTE_ARITHMETIC_ONLY", "stateOneBankBytes": perbank, "pointOneBankBytes": row["pointStride"] * points, "nodeOneBankBytes": row["nodeStride"] * nodes, "sortOneBufferBytes": sort_one, "packed3StateBanksVsObserved1GiBBlock": "EXCEEDS_OBSERVED_SINGLE_BLOCK_NUMERIC_LIMIT" if 3 * perbank > 1073741824 else "NUMERIC_ONLY_NOT_ALLOCATION_PASS", "facesMAC": faces, "newGpuAllocationsActuallyRun": 0})
    g11 = read("docs/architecture-T660-g11-manifest-v2.json")["resources"][-1]
    dm = read("docs/architecture-T554-g9-dm-manifest-v2.json")["resources"][-1]
    contract_data = {"G11_DM_K512_V8192": {k: g11[k] for k in ["residentPayloadBytes", "fullGPUAdditionalBytes", "coldFULLSlotBytes", "twoFULLSlotsBytes", "optionalSinglePhaseWitnessBytes", "knownPayloadWithWitnessGPUAndNative", "largestGLObjectBytes"]},
                     "DM_K512": {k: dm[k] for k in ["residentPayloadBytes", "fullGPUAdditionalBytes", "twoFullSlotsBytes", "maxGLBufferObjectBytes"]}}
    out = {"schema": "T679_GPU_RESEARCH_CONSTRAINT_EVIDENCE_V1", "deviceUsed": False, "buildRun": False, "productTestsRun": False, "algorithmChoiceFrozen": False,
           "designInputs": [record(p) for p in ["docs/algorithm-G15-G19-roadmap-v1.md", "docs/design-T673-research-cases-v1.json", "docs/design-T673-roadmap-evidence-v1.json"]],
           "cases": [{"id": c["id"], "goals": c["goals"], "kind": c["kind"], "criterion": c["criterion"], "reference": c["reference"], "productStatus": c["productStatus"]} for c in cases],
           "historicalT131": {"original": record("test/reports/t131/t131.json"), "identity": read("test/reports/t131/t131.json")["t131"]["identity"], "limits": read("test/reports/t131/t131.json")["t131"]["limits"], "class": "HISTORICAL_DEVICE_PBUFFER_SCOPE"},
           "historicalT232": {"original": record("test/reports/t232/t232.json"), "raw": read("test/reports/t232/t232.json")["t232"], "class": "HISTORICAL_DEVICE_PBUFFER_SCOPE_NOT_WINDOW_QUALIFICATION"},
           "M2ObservedLimits": {"original": record("test/reports/t491/t591c/collector.ndjson"), "bootSerial": device["bootSerial"], "epoch": device["epoch"], "window": device["window"], "context": device["eglContext"], "ownerTid": device["ownerTid"], "driver": device["driver"], "limits": device["limits"], "probe": device["probe"], "class": "DEVICE_QUERY_VALUES_ONLY_SAME_M2_CONTEXT_NOT_NEW_KERNEL_PASS"},
           "SDK": {"path": str(sdk), "bytes": sdk.stat().st_size, "sha256": sha(sdk.read_bytes()), "declaredSymbolsChecked": symbols, "class": "SDK_API_DECLARATIONS_NOT_DEVICE_LIMITS_OR_PRODUCT_PASS"},
           "examples": examples, "sharedTileExamples": {"assumptions": "8 interior cells per dimension + 1-cell halo on both sides, full node payload in shared, illustrative only", "2DNodesBytes": 10 * 10 * 64, "3DNodesBytes": 10 * 10 * 10 * 96, "observedSharedBytes": 32768, "class": "ASSUMPTION_NUMERIC_BOUND_ONLY"},
           "additionalSourceMapExample": {"assumptions": "3 banks, per-cell 64 full origin entries each16B only IF chosen new algorithm needs this layout; not additional mandatory cost or origin capacity freeze", "2D128sqBytes": 3 * 128 * 128 * 64 * 16, "3D64x64x8Bytes": 3 * 64 * 64 * 8 * 64 * 16},
           "legacyContracts": contract_data, "legacyCountsAreAllocatedDevicePeak": False,
           "sourceReceipts": [record(p) for p in ["docs/architecture-T231-gpu-resident-platform-v1.md", "docs/architecture-T660-g11-manifest-v2.json", "docs/architecture-T554-g9-dm-manifest-v2.json", "docs/architecture-T647-acceptance-v1.json", "entry/src/main/cpp/gpu_resident/g13flow/assets_manifest.json", "docs/analysis-T661-evidence-v1.json", "docs/analysis-T665-evidence-v1.json", "docs/developer-T663-v1.md", "entry/src/main/cpp/gpu_resident/cold_slot_broker.cpp"]],
           "unknown": ["target new route window-context allocation peak/RSS", "new program compile/link/dispatch/float behavior", "physics discretization/domain/reference and source/contact caps", "new route iterations/trials/root wall-time/thermal30fps", "full scene/cold/old-new overlap budgets", "G13/14/12 actual trusted prerequisites"],
           "priorityAfter11": [13, 14, 12, 15, 16, 17, 18, 19], "noNewGateForGoals1to14": True}
    assert len(cases) == 24 and set(g for c in cases for g in c["goals"]) == {15, 16, 17, 18, 19}
    assert examples[-1]["partialTotalBytes"] == 3098990880
    assert out["sharedTileExamples"]["3DNodesBytes"] > 32768
    write("docs/analysis-T679-evidence-v1.json", out)
    return {"cases": len(cases), "examples": examples, "algorithmChoiceFrozen": False}


if __name__ == "__main__":
    print(json.dumps({"T678": t678(), "T679": t679()}, ensure_ascii=True, indent=2))
