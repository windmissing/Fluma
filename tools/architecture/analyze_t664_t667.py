"""Read saved files/ELF bytes only; never run product, shader, generator or device."""
import collections
import difflib
import json
from pathlib import Path
import re
import subprocess
import zipfile

from analyze_t660_t662 import ROOT, artifact, build_id, record, sha, ui_texts, write


def text(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")


def hap_module(task, module):
    receipt = json.loads(text("test/reports/t257/" + task + ".json"))
    p = Path(receipt["signed"]["path"])
    with zipfile.ZipFile(p) as z:
        b = z.read("libs/arm64-v8a/" + module)
    return {"task": task, "packageSHA": sha(p.read_bytes()), "packageReceiptMatched": sha(p.read_bytes()) == receipt["signed"]["sha256"],
            "module": module, "bytes": len(b), "sha256": sha(b), "buildId": build_id(b)}, b


def t664():
    rows = []
    for p in sorted((ROOT / "test/reports/t491/t591c").glob("t593_l*.json")):
        strings = list(ui_texts(json.loads(p.read_text(encoding="utf-8-sig"))))
        relevant = [s for s in strings if any(k in s for k in ["STATE_LOST", "GRS_M1_PROGRESS", "REJECTED", "NOT_READY", "收据", "未采集", "FAULT", "READY"])]
        scopes = []
        for s in strings:
            m = re.search(r"GRS_M1_PROGRESS_CLOCK_V(\d+) / run (\d+):(\d+) / epoch (\d+) / session (\d+) / sample (\d+)", s)
            if m:
                scopes.append(dict(zip(["clockRevision", "runHi", "runLo", "epoch", "session", "sampleSerial"], m.groups())))
        rows.append({**record(p.relative_to(ROOT).as_posix()), "scopes": scopes, "relevantTexts": relevant})
    packages = []
    for task in ["T591-collector", "T597-fixed"]:
        meta, b = hap_module(task, "libgrsm1.so")
        meta["markerStringsPresent"] = {s: s.encode() in b for s in ["beginRecoverySession", "recoveryState", "V3_USE_M2_TRACE", "WAIT_NEW_SESSION", "T468_M2_V3_DIAGNOSTIC", "PAGE_PULSE_ACCEPTED", "T575"]}
        packages.append(meta)
    legacy = ".codex/local-test/t562-m2-fix/entry/src/main/"
    formal = ".codex/local-test/t597-fixed/entry/src/main/"
    sources = [legacy + x for x in ["cpp/gpu_resident/app_bridge.cpp", "cpp/gpu_resident/bridge_state.h", "cpp/gpu_resident/evidence_capture_napi.inc", "ets/pages/GpuResidentM1.ets", "ets/physics/GpuM1DemoController.ets"]]
    sources += [formal + x for x in ["cpp/gpu_resident/app_bridge.cpp", "cpp/gpu_resident/recovery_v3.h", "ets/pages/GpuResidentM1.ets", "ets/physics/GpuRecoveryPulse.ets", "ets/physics/GpuM1DemoController.ets"]]
    out = {"schema": "T664_ORIGINAL_SCOPE_AND_SOURCE_ATTRIBUTION_V1", "notUsingDevice": True,
           "uiOriginals": rows, "uiRows": len(rows), "packages": packages,
           "sourceFiles": [record(x) for x in sources],
           "earliestObservedGate": "UI run0 and STATE_LOST: no current published permission/resource evidence; exact Create terminal for this process NOT_CAPTURED",
           "sampleSerialProducer": "app_bridge status increments ++a.sampleSerial on every status read, not accepted page pulse, owner thread progress, or physical commit",
           "down": "legacy model.down returns NOT_READY when !allowed; no handler/down original is present in these UI files; STATE_LOST UI label is not a captured model.down reason",
           "capture": "l7/l8/l9 explicitly REJECTED V3_USE_M2_TRACE; legacy beginEvidenceCapture unconditionally rejects; no output is expected for that wrong endpoint",
           "sameAttemptAsT591": "NOT_PROVEN: shared package claim/path only; no UI boot/pid/ticket/ns/context fields linking earlier collector serial1 to these9 snapshots",
           "sourceReceiptZero": "UI receipt #0/commit0 only; no nonempty typed enqueue/claim/accept proof, not complete queue ledger",
           "verdict": "WRONG_ARTIFACT_AND_CAPTURE_ROUTE_WITH_UNPUBLISHED_RUN; NOT_PROVEN_MISSING_FORMAL_M1_RECOVERY",
           "route": "original T593 tester must select existing formal M1 recovery/input artifact by actual ELF/page/ABI and collect same-scope creation/recovery/down/offer/claim; no speculative product clock fix"}
    write("docs/analysis-T664-evidence-v1.json", out)
    return {"snapshots": len(rows), "packages": packages, "uiScopes": [r["scopes"][:1] for r in rows], "verdict": out["verdict"]}


def t667():
    log = "test/reports/t636/t656-run2.ndjson"
    rows = [json.loads(s) for s in text(log).splitlines() if s.strip()]
    source = "test/t636/driver_v2.cpp"
    state = [
        {"afterSeq": 8, "observed": "ReadBegin(ticket1)=true", "heldReader": "ticket1", "inference": False},
        {"afterSeq": 10, "observed": "ReadEnd(ticket1),Freed(ticket1) calls in source and logged", "heldReader": "none", "inference": "void result not returned; existing implementation ends/frees1"},
        {"afterSeq": 13, "observed": "ReadBegin(ticket2) after BuilderDone=true", "heldReader": "ticket2", "inference": "readerA-active detail string false; ticket1 already released"},
        {"afterSeq": 14, "observed": "second ReadBegin(ticket2)=false", "heldReader": "ticket2", "inference": "A.ReadEnd(ticket1) cannot end ticket2; false skips conditional cleanup"},
        {"afterSeq": 15, "observed": "Reserve ticket3=true", "heldReader": "ticket2", "builder": "ticket3"},
        {"afterSeq": 16, "observed": "Reserve fresh probe after Freed(ticket3)=true; output not logged", "heldReader": "ticket2", "builder": "new probe ticket (serial4 inferred, not observed)", "inference": "ReadBegin(ticket3) return ignored: builder3 and existing reader2 reject it; Freed3 therefore not tested with active reader3; other slot genuinely free"},
        {"afterSeq": 17, "observed": "ReadEnd/ReadBegin(ticket3)=false", "heldReader": "ticket2", "inference": "ticket3 was freed, not an active reader ignored"},
        {"afterSeq": 18, "observed": "Reserve fresh0=false", "heldReader": "ticket2", "builder": "leaked new probe", "inference": "earlier builder/two-slot gates; not credit-cap evidence"},
        {"afterSeq": 19, "observed": "M4 initial Reserve=false", "inference": "M4 cumulative route/process matrix NOT_RUN"},
        {"afterSeq": 24, "observed": "fresh0 GPU Reserve after B.ContextRetired(matching ctx)=true", "inference": "narrow opaque-context component lease evidence; not eglDestroyContext or driver free"}]
    broker = ".codex/local-test/t636-harness/libgrs_cold_slots.so"
    release = ".codex/local-test/t608-release/entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libgrs_cold_slots.so"
    out = {"schema": "T667_SAVED_CALL_ORDER_ATTRIBUTION_V1", "notUsingDevice": True, "noHarnessRun": True,
           "log": record(log), "rows": rows, "rowCount": len(rows), "contiguousSeq": [r["seq"] for r in rows] == list(range(len(rows))),
           "keysInEveryRawRow": sorted(set.intersection(*(set(r) for r in rows))),
           "bootPidAttemptPresent": all(all(k in r for k in ["boot", "pid", "attempt"]) for r in rows),
           "fileOpenMode": "std::ios::app, not exclusive create-new", "stateLedger": state,
           "broker": record(broker), "releaseBroker": record(release), "sameBrokerBytes": (ROOT / broker).read_bytes() == (ROOT / release).read_bytes(),
           "brokerBuildId": build_id((ROOT / broker).read_bytes()),
           "sources": [record(x) for x in [source, "test/t636/adapter_a.cpp", "test/t636/adapter_b.cpp", "entry/src/main/cpp/gpu_resident/cold_slot_broker.cpp", "entry/src/main/cpp/gpu_resident/cold_slot_broker.h"]],
           "loader": {"all8NullChecked": True, "selectedPrefix": "A_ on both distinct DSOs", "resolvedNameActuallyLogged": ["Reserve"], "notAll8ModuleAddressesLogged": True, "unusedBAliasBuilderDoneBool": "B_BuilderDone returns bool but loader selects A_BuilderDone with void signature, not an observed ABI fault"},
           "hostVsGPUReserve": "host permits known same-route non-reader ticket rebuild when no builder; GPU requires *ticket==0. New isolated probes use fresh0 in both, but do not generalize GPU gate to all host calls",
           "M4ExtraExpectationBug": "same route 4096+4096==8192 should succeed after BuilderDone with free slot, not expect0",
           "verdict": "FIXTURE_TICKET_READER_AND_CLEANUP_ERROR; D2_ACTIVE_READER_FREED_NOT_EXERCISED; NO_BROKER_SEMANTIC_CHANGE",
           "M5": "MATCHING_CONTEXT_RETIREMENT_COMPONENT_POSITIVE_CONTROL_OBSERVED; no claim all8 guards/driverfree"}
    inspector = Path("C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/llvm/bin/llvm-objdump.exe")
    cmd = [str(inspector), "--disassemble-symbols=GrsColdFreed", "--no-show-raw-insn", broker]
    disassembly = subprocess.check_output(cmd, cwd=ROOT).decode("utf-8").replace("\r\n", "\n")
    out["brokerFreedStaticDisassembly"] = {"toolSHA256": sha(inspector.read_bytes()), "command": cmd, "outputSHA256": sha(disassembly.encode()), "output": disassembly,
                                          "readerGuard": "0x6600 ldrb [slot+21], 0x6604 tbz to clear only if reader bit0=0; true branch skips slot clear and unlocks. Static inspection, no native library load/execution"}
    write("docs/analysis-T667-evidence-v1.json", out)
    return {"rowCount": len(rows), "keys": out["keysInEveryRawRow"], "broker": out["broker"], "verdict": out["verdict"]}


def shader_candidates(b):
    out = []
    for m in re.finditer(b"#version (?:310|320) es", b):
        end = b.find(b"\0", m.start())
        if end > m.start():
            raw = b[m.start():end]
            out.append(({"fileOffset": m.start(), "bytes": len(raw), "sha256": sha(raw), "lines": len(raw.splitlines())}, raw))
    return out


def t665():
    meta, rx = hap_module("T645-rx", "libgrs_g9rx.so")
    old = json.loads(text("test/reports/t644/development.json"))
    oldElf = (ROOT / old["elf"]["path"]).read_bytes()
    digest = old["pipelineIdentity"]["shaderSHA"]
    new_shader = next((m, b) for m, b in shader_candidates(rx) if m["sha256"] == digest)
    old_shader = next((m, b) for m, b in shader_candidates(oldElf) if m["sha256"] == digest)
    bytecode = new_shader[1]
    path = ".codex/local-test/t645-tree/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/g9rx/rx_assets/pipeline.comp"
    generated = (ROOT / path).read_bytes()
    lines = bytecode.decode().splitlines()
    variants = []
    for prefix in [".codex/local-test/t644-assets", ".codex/local-test/t644-harness-assets", ".codex/local-test/t644-delivery-v1/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/g9rx/rx_assets"]:
        p = prefix + "/pipeline.comp"
        if (ROOT / p).exists():
            raw = (ROOT / p).read_bytes()
            variants.append({**record(p), "LFNormalizedSHA": sha(raw.replace(b"\r\n", b"\n")), "sameSourceAfterLF": raw.replace(b"\r\n", b"\n") == bytecode})
    driver_files = []
    messages = []
    for p in sorted((ROOT / "test/reports/t645a").glob("rx-hilog*.txt")):
        matches = [s for s in p.read_text(encoding="utf-8-sig").splitlines() if "S0022" in s or "redeclared" in s]
        driver_files.append({**record(p.relative_to(ROOT).as_posix()), "matchingMessages": matches})
        messages += matches
    ui = []
    for p in sorted((ROOT / "test/reports/t645a").glob("l*.json")):
        strings = list(ui_texts(json.loads(p.read_text(encoding="utf-8-sig"))))
        ui.append({**record(p.relative_to(ROOT).as_posix()), "relevantTexts": [s for s in strings if any(k in s for k in ["CONFIGURE_FAILED", "memoryBuildIdMatched", "LOADED_FILE_OBSERVED", "firstError", "SHADER_FAILED"])]})
    start = next(i for i, l in enumerate(lines) if l.startswith("bool g9Capture(){"))
    end = next(i for i in range(start + 1, len(lines)) if lines[i] == "}")
    function = "\n".join(lines[start:end + 1]) + "\n"
    source_rows = []
    for relative in ["tools/g9rx/generate_assets.py", "entry/src/main/cpp/gpu_resident/g9rx/frozen/pipeline.comp", "entry/src/main/cpp/gpu_resident/g9rx/frozen/source_receipt.json", "entry/src/main/cpp/gpu_resident/g9rx/kernel_assets.cpp", "entry/src/main/cpp/gpu_resident/g9rx/transport.cpp", "entry/src/main/cpp/gpu_resident/g9rx/CMakeLists.txt", "entry/src/main/cpp/gpu_resident/program_cache/program_factory.h", "entry/src/main/cpp/gpu_resident/m2/create_diagnostics.h"]:
        snap = ".codex/local-test/t645-tree/" + relative
        oldsnap = ".codex/local-test/t644-delivery-v1/" + relative
        current = (ROOT / relative).read_bytes()
        entry = {"current": record(relative)}
        for name, candidate in [("testerBuildTree", snap), ("developerBuildTree", oldsnap)]:
            if (ROOT / candidate).exists():
                entry[name] = record(candidate)
                entry[name + "MatchesCurrent"] = (ROOT / candidate).read_bytes() == current
        source_rows.append(entry)
    out = {"schema": "T665_RX_SHADER_SOURCE_AND_ARTIFACT_ATTRIBUTION_V1", "notUsingDevice": True, "noGLSLCompilerRun": True,
           "testedPackage": meta, "developerELF": {**record(old["elf"]["path"].replace("\\", "/")), "buildId": build_id(oldElf), "receiptMatches": sha(oldElf) == old["elf"]["sha256"]},
           "testedEmbeddedShader": new_shader[0], "developerEmbeddedShader": old_shader[0], "embeddedShaderBytesEqual": new_shader[1] == old_shader[1],
           "generated": {**record(path), "LFNormalizedSHA": sha(generated.replace(b"\r\n", b"\n")), "matchesEmbeddedAfterLF": generated.replace(b"\r\n", b"\n") == bytecode},
           "sourceVariants": variants, "driverLog": record("test/reports/t645a/rx-hilog3.txt"), "matchingDriverMessages": messages,
           "driverFiles": driver_files, "distinctMatchingDriverMessages": sorted(set(messages)), "uiOriginals": ui,
           "submittedLine1728": lines[1727], "nearbyLines": [{"line": i + 1, "text": lines[i]} for i in range(1722, 1733)],
           "captureFunction": {"firstLine": start + 1, "lastLine": end + 1, "sha256LF": sha(function.encode()), "source": function},
           "lineDirectives": [l for l in lines if l.lstrip().startswith("#line")], "defines": [l for l in lines if l.lstrip().startswith("#define")], "version": lines[0],
           "sourceChain": "kernel_assets kRxPipeline -> transport programcache::Build -> create468::GlApi::Source: glShaderSource(shader,1,&src,nullptr), no injected define/line in inspected tested snapshot",
           "sourceFiles": source_rows,
           "staticCompilerEvidence": "developer report says static GLSL pass; saved development/build/inputs receipts have no exact glslang command/version/macros/log proving target compiler equivalence. Do not fabricate version or compiler-conformance root cause",
           "deviceActualGlShaderSourceBytes": "NOT_DIRECTLY_CAPTURED; package-embedded shader/source-chain evidence is strong but not a same-attempt dump/hash receipt",
           "verdict": "RX_SOURCE_LOCAL_SYMBOL_REUSE_TARGET_COMPILER_REJECTION; SAME_SHADER_IN_DIFFERENT_NATIVE_ELFS; NOT_PROVEN_STALE_SHADER_OR_PHYSICS_DEFECT",
           "next": "developer freezes RX-only deterministic capture-loop symbol/scope hygiene, exact one-source identity and fatal generation checks; tester original T645 captures exact program receipt/driver source identity and resumes remaining scope after repaired artifact"}
    harness = ROOT / ".codex/local-test/t644-harness-assets/pipeline.comp"
    if harness.exists():
        out["savedHarnessVsProductionDiff"] = "".join(difflib.unified_diff(lines, harness.read_text(encoding="utf-8-sig").splitlines(), fromfile="production", tofile="savedHarness", lineterm="\n", n=1))
        out["savedHarnessIsProductSourceProof"] = False
    write("docs/analysis-T665-evidence-v1.json", out)
    return {"testedELF": meta, "shader": new_shader[0], "sameShaderOldNew": out["embeddedShaderBytesEqual"], "line1728": lines[1727], "driverMessages": messages, "variants": variants}


if __name__ == "__main__":
    print(json.dumps({"T664": t664(), "T667": t667(), "T665": t665()}, ensure_ascii=True, indent=2))
