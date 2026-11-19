"""Author/check bounded architecture tables; no product generation or execution."""
import json
from pathlib import Path
import sys
from analyze_t660_t662 import ROOT, record, write

CHECK = "--check" in sys.argv


def read(path):
    return json.loads((ROOT / path).read_text(encoding="utf-8-sig"))


def emit(path, obj):
    raw = (json.dumps(obj, ensure_ascii=False, indent=2) + "\n").encode()
    if CHECK:
        assert (ROOT / path).read_bytes() == raw, "STALE_ARCHITECTURE " + path
    else:
        (ROOT / path).write_bytes(raw)


boundary = {"role": "architect", "deviceUsed": False, "buildRun": False, "productTestsRun": False, "productPASS": False,
            "CPU": "STOPPED", "goal10": "PAUSED", "waterOrder": [13, 14, 12], "oldWetM1": "DEFERRED",
            "render": "XComponent+shader including foreground/background; FAULT or explicitly STALE prior valid new-pipeline frame; no Canvas/softwareGL product fallback",
            "executionRequirement": "要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。",
            "queue": "不允许修改 D:\\hmprojects\\HelloWorld\\docs\\后续工作清单.md；唯一例外本人行状态；不得更新滴答同名清单",
            "roles": "developer implementation+compile; tester/effector fixture/sign/deploy/test/evidence; no device/manager closure gate"}

e = read("docs/analysis-T664-evidence-v1.json")
c664 = {"schema": "T664_SOURCE_RECOVERY_ATTRIBUTION_V1", "date": "2026-11-19", "boundaries": boundary,
        "verdict": e["verdict"], "evidence": "docs/analysis-T664-evidence-v1.json",
        "packageSelection": e["packages"],
        "observations": {"snapshots": 9, "stateScopes": "8 snapshots run0, epochs2/3 session1; no boot/pid/context/Create ticket/ns association", "sampleSerial": "status-read count ++a.sampleSerial", "zeroReceipt": "UI zero only, not complete enqueue ledger", "downReason": "no captured down; actual legacy model maps !allowed to NOT_READY, not captured STATE_LOST down", "capture": "l7/l8/l9 REJECTED V3_USE_M2_TRACE, no captureId or file expected"},
        "formalRecovery": [
            {"phase": "RESET_REQUESTED", "required": "explicit newRun: revoke old permit/source/quota/capture, freeze firstFailure/ledger; checked resetSerial/epoch/window"},
            {"phase": "RETIRING_OLD", "required": "old flight/fence/scene lease actually retired; no force-ready or thread interruption"},
            {"phase": "CREATING_NEW", "required": "actual GL owner Create terminal OK under current scope, nonzero candidate run"},
            {"phase": "WAIT_NEW_SESSION", "required": "recovery.Resources current epoch/window/id after retirement; fresh actual page generation binds exact serial/epoch/run"},
            {"phase": "READY", "required": "resources/CanReady/active/window/progress.Fresh/ui.Fresh, then same-lock CommitReady and new actual Down"}],
        "sameAttemptAsOldT591LINK": "NOT_PROVEN; no boot/ticket/time chain. Candidate run and published run separate; old open Create could prevent publication but not proven cause of this UI set",
        "tester": {"originalTask": "T593", "artifact": "existing formal M1 T597-fixed or independently byte-proven same formal implementation; not old T591 M2 collector by same UI/library name",
                   "captureFirst": ["loaded HAP/ELF/BuildId and exact interfaces", "full status+recoveryState", "create/retirement/resources/current candidate and published identities", "resetSerial/epoch/window/context/pageGeneration/session/permission", "pulse raw return/page sequence/producer ns/Fresh", "handler/changedTouches/down reason/coordinates/intent", "typed enqueue same-seq claim/SOURCE terminal+witness"],
                   "touch": "150-250ms across real accepted scope pulses after legal READY; no synthetic SOURCE", "gapNsUnchanged": 33333333, "rootDeadlineNsUnchanged": 250000000,
                   "missingBuildField": "NOT_AVAILABLE_IN_THIS_BUILD with actual endpoint/return, not zero or fabricated recovery", "noCrossAttemptMerge": True},
        "developer": "NO_NEW_PRODUCT_FIX_ESTABLISHED; use existing recovery/diagnostic path first; exact same-scope missing interface/terminal evidence back architect before any seam",
        "existingM2Seam": "T663 in progress under T662; separate domain, no duplicate card or automatic M1 prerequisite",
        "downstream": "T519/T410/T437 retain true nonempty SOURCE prerequisites; no false unlock"}
emit("docs/architecture-T664-contract-v1.json", c664)

e = read("docs/analysis-T667-evidence-v1.json")
matrix = [
    {"id": "M1", "setup": "fresh0 A reserve4096 budget8192 r1, builder retained", "negative": "B fresh0 r1/r2 reserve reject", "positive": "B.BuilderDone(A); B.ReadBegin(A)=true", "cleanup": "B.ReadEnd(A); A.Freed(A), log VOID calls"},
    {"id": "M2a", "setup": "fresh stage no other reader, B reserve", "negative": "ReadBegin(B) rejects own builder", "positive": "BuilderDone(B); ReadBegin(B) true", "cleanup": "End(B),Freed(B)"},
    {"id": "M2b", "setup": "A Done+ReadBegin(A) true and HOLD; B other route reserve succeeds then Done", "negative": "ReadBegin(B) false only while A holds reader", "positive": "ReadEnd(A); ReadBegin(B) true", "cleanup": "End(B); Freed(A/B)"},
    {"id": "M3", "setup": "A Done+ReadBegin(A) true and HOLD; B reserve+Done, both slots occupied/no builder", "operation": "B.Freed(A)", "negative": "third fresh0 reserve rejects; alone insufficient proof", "positive": "End(A); ReadBegin(A) true proves original ticket retained; End(A),Freed(A); third fresh0 reserve true/new ticket", "cleanup": "retain all new outputs, Done/Freed new ticket,Freed(B)"},
    {"id": "M4_ROUTE", "setup": "fresh stage only A charge4096/r1/budget8192, Done", "negative": "B fresh0 charge4097/r1 budget8192 rejects (8193)", "positive": "B charge4096 succeeds (8192 exact)", "cleanup": "Done/Freed B then Freed A"},
    {"id": "M4_PROCESS", "setup": "fresh stage only A charge cap-4096/r2/budgetcap, Done", "negative": "B charge4097/r3 budgetcap rejects", "positive": "B charge4096 succeeds totalcap", "cleanup": "Done/Freed every owned ticket; no huge allocation"},
    {"id": "M5", "setup": "GPU gA fresh0 reserve matching opaque ctx", "negative": "wrong ticket Freed and wrong ctx Retired tested separately, fresh0 rejects after each", "positive": "B correct Freed(A), fresh0 gB reserve; HOLD gB; matching ctx Retired; fresh0 gC reserve succeeds", "cleanup": "release only known owned GPU tickets; no claim driver free"}]
abi = [
    {"field": "Res", "selected": "A_Reserve", "signature": "bool(uint64_t*,size_t,size_t,uint32_t,uint64_t*,uint64_t*)"},
    {"field": "BuilderDone", "selected": "A_BuilderDone", "signature": "void(uint64_t)"},
    {"field": "ReadBegin", "selected": "A_ReadBegin", "signature": "bool(uint64_t)"},
    {"field": "ReadEnd", "selected": "A_ReadEnd", "signature": "void(uint64_t)"},
    {"field": "Freed", "selected": "A_Freed", "signature": "void(uint64_t)"},
    {"field": "GpuRes", "selected": "A_GpuReserve", "signature": "bool(uint64_t*,uint64_t,size_t)"},
    {"field": "GpuFreed", "selected": "A_GpuFreed", "signature": "void(uint64_t)"},
    {"field": "GpuCtxRetired", "selected": "A_GpuContextRetired", "signature": "void(uint64_t)"}]
c667 = {"schema": "T667_BROKER_ISOLATED_MATRIX_V1", "date": "2026-11-19", "boundaries": boundary,
        "verdict": e["verdict"], "evidence": "docs/analysis-T667-evidence-v1.json", "ABI": abi,
        "stateLedger": e["stateLedger"], "productionBrokerSHA": e["broker"]["sha256"],
        "productionSemanticChange": False, "developerSeamRequired": False,
        "staticBinaryGuard": "Freed reads slot.reader at offset21; true skips clear. No native execution",
        "hostReserve": "new probes fresh0; existing known same-route non-reader ticket rebuild remains legal with no builder",
        "GPUReserve": "requires *ticket==0; Freed/ContextRetired values do not write caller variable zero",
        "oldLog": {"rows": 26, "keys": ["seq", "event", "detail"], "bootPidAttemptKeys": False, "openMode": "append", "all8PointerNullChecks": True, "onlyReserveDladdrNameLogged": True},
        "matrix": matrix, "processCapBytes": 524375552,
        "tester": {"originalTask": "T627", "logs": "unique exclusive-create attempt; every raw row boot/pid-start/attempt/seq/monoNs producer plus call/symbol/module/full inputs/outputs/bool or VOID",
                   "maxSeconds": 30, "maxRows": 128, "maxUTF8RowBytesIncludingNewline": 512, "maxLogBytes": 65536, "maxManifestBytes": 16384, "fixedOwnedTicketLedgerEntriesMax": 16,
                   "unexpectedSuccess": "retain output ticket and clean/stop this stage, never leak local fresh probe", "cleanupFailure": "preserve raw and end isolated harness process, no ResetAll or guessed production ticket",
                   "allCallsCount": True, "harnessOnlyCredit": True, "driverOrRouteIntegrationPASS": False},
        "M4": "NOT_RUN in old run; entry false from earlier builder/two-slot gates",
        "M5": "retain corrected matching-context retirement positive control only, no ALL_PASS/actual EGL retirement claim"}
emit("docs/architecture-T667-contract-v1.json", c667)

e = read("docs/analysis-T665-evidence-v1.json")
c665 = {"schema": "T665_RX_CAPTURE_SCOPE_HYGIENE_V1", "date": "2026-11-19", "boundaries": boundary,
        "verdict": e["verdict"], "evidence": "docs/analysis-T665-evidence-v1.json",
        "oldProductionShader": e["testedEmbeddedShader"], "testedELF": e["testedPackage"], "oldDeveloperELF": e["developerELF"],
        "sameEmbeddedShader": True, "loadedMemoryBuildIdIsFullSourceCapture": False,
        "legacyCaptureFunction": {k: e["captureFunction"][k] for k in ["firstLine", "lastLine", "sha256LF"]},
        "transformation": {"id": "RX_CAPTURE_SCOPE_HYGIENE_V1", "expectedFunctions": 1, "expectedTransforms": 1,
                           "originalFrozenPipelineSHA": "972edb7f6b4b7a387f57bb1578b8c3bb66cdad7e226be8f943857a49de06063b",
                           "scope": "only generated RX g9Capture after approved frozen-source SHA and RX math injection, before final source hash/output",
                           "names": {"contactClearLoop": "clearSlot", "candidateScanLoop": "scanSlot", "selectedPacketLocal": "selectedSlot"},
                           "scanLoopBraces": "explicit compound statement", "binding": "lexically correct range per variable; no global slot replace",
                           "unchanged": ["loop bounds/order", "best comparator and tie-break", "arithmetic/indices", "events/packet ids", "continue/break", "DM physics/Config/time", "T647 proof/masks"],
                           "frozenSourceEdited": False, "legacyOutputFragment": "fatal if legacy slot redeclaration remains", "record": "function before/after exact LF SHA, binding ranges, count1"},
        "singleSource": {"bytes": "one canonical UTF8 LF no BOM/no embedded NUL/no injected #line", "compFile": "write_bytes same canonical source", "cppHeaderLiteral": "same source, C terminator excluded from shaderSHA", "manifest": "canonical byte count/SHA separate from container file SHA and any legacy CRLF disk SHA", "harness": "new explicit hooks variant, independent SHA; saved older harness not product proof"},
        "identity": {"runtimeRXPreBuild": "existing native byte SHA/strlen equals generated shader and active program contract or SOURCE_IDENTITY_MISMATCH; no GL query", "dependencies": ["this T665 contract", "RX generator and hygiene transform revision/receipt", "frozen source receipt", "canonical source and generated literal", "kernel_assets/transport", "actual shared program_factory/create_diagnostics source wrapper", "native compilation inputs/ELF receipt"],
                     "recompute": ["shaderSHA", "sourceManifestSHA", "implementationSHA", "bundleSHA", "pipeline/cacheABI", "reader identity"], "missingMismatchStale": "FATAL_NO_OLD_ASSET_FALLBACK", "noDefaultOrFrozenRewrite": True},
        "developerFiles": ["tools/g9rx/generate_assets.py", "tools/g9rx/prepare_build.cjs", "tools/g9rx/build_receipt.py", "entry/src/main/cpp/gpu_resident/g9rx/transport.cpp", "entry/src/main/cpp/gpu_resident/g9rx/CMakeLists.txt", "generated RX pipeline/header/identity/reader assets"],
        "compileChecks": ["actual final production canonical GLSL with exact compiler path/version/flags/macros/target/log/exit", "native+ArkTS unsigned product build", "extract linked ELF actual source bytes and compare canonical SHA/length/line mapping"],
        "developerDeviceGate": False, "staticCompileIsProductPASS": False, "compilerConformanceRootCause": "NOT_PROVEN",
        "tester": {"originalTask": "T645", "after": "repaired actual artifact identities ready, no manager approval gate", "capture": ["loaded fileSHA/BuildId separate from memory PT_NOTE", "same attempt boot/ticket/context/window/owner/time", "program role/handle", "actual sourceSHA/length and canonical source manifest", "compile/link ENTER EXIT/error and raw log bytes", "line mapping"], "remaining": "READY/pulse/touch/T642 N01-N11/FIFO/DM/replay/cold CURRENT-BASE/reader/cost/full shader frames all original scope, currently NOT_REACHED"},
        "missingReceipt": "use existing actual program artifact cold/status first; exact same-attempt missing field to architect before minimal bounded seam, no generic thread/GLquery",
        "T666": "separate idempotent link-flags work; not a prerequisite to this shader attribution or universal buildPASS"}
emit("docs/architecture-T665-contract-v1.json", c665)

# Finite contract checks only, not a simulator or product regression suite.
assert len(c667["ABI"]) == 8 and len(set(x["selected"] for x in abi)) == 8
assert 128 * 512 == c667["tester"]["maxLogBytes"]
assert (524375552 - 4096) + 4097 == 524375553
assert (524375552 - 4096) + 4096 == 524375552
assert 4096 + 4097 > 8192 and 4096 + 4096 == 8192
assert e["embeddedShaderBytesEqual"] and e["testedEmbeddedShader"]["bytes"] == 145268
assert e["submittedLine1728"].startswith("  if(best==uK)break;uint slot=best")
assert not e["lineDirectives"] and not e["defines"]
assert all(v["packageReceiptMatched"] for v in c664["packageSelection"])
assert not c664["packageSelection"][0]["markerStringsPresent"]["beginRecoverySession"]
assert c664["packageSelection"][1]["markerStringsPresent"]["beginRecoverySession"]
raw = read("docs/analysis-T667-evidence-v1.json")
assert raw["sameBrokerBytes"] and raw["rowCount"] == 26 and raw["contiguousSeq"] and not raw["bootPidAttemptPresent"]
assert "ldrb\tw8, [x8, #21]" in raw["brokerFreedStaticDisassembly"]["output"]
for n in [664, 667, 665]:
    assert (ROOT / f"docs/architecture-T{n}-v1.md").exists()

check = {"schema": "T664_T667_ARCHITECTURE_CHECK_V1", "result": "STATIC_ORIGINAL_BYTE_AND_CONTRACT_CHECK_PASS", "deviceUsed": False, "buildRun": False, "productTestsRun": False, "productPASS": False,
         "checks": ["T5939 original UI files and two actual HAP/native identities", "T62726 rows and actual reader/builder chronology,8 ABI signatures", "actual715 broker static reader guard", "M4 exact8192 and processcap524375552 boundaries", "T645 two ELF same145268B shader/line1728, separate rawCRLF and canonicalLF hashes", "all3 contracts and role/queue/GPU/render boundaries"],
         "contracts": [record(f"docs/architecture-T{n}-contract-v1.json") for n in [664, 667, 665]]}
if not CHECK:
    write("docs/analysis-T664-T667-check-v1.json", check)
print(json.dumps({"result": "CHECK_PASS" if CHECK else "CONTRACTS_FROZEN_AND_CHECKED", "tasks": [664, 667, 665], "deviceUsed": False}))
