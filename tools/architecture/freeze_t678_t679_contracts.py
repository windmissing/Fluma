"""Architecture-only contracts and finite bookkeeping checks; no product execution."""
import json
from pathlib import Path
import sys
from analyze_t660_t662 import ROOT, record, write

CHECK = "--check" in sys.argv


def read(path):
    return json.loads((ROOT / path).read_text(encoding="utf-8-sig"))


def emit(path, obj):
    b = (json.dumps(obj, ensure_ascii=False, indent=2) + "\n").encode()
    if CHECK:
        assert (ROOT / path).read_bytes() == b, "ARCHITECTURE_STALE " + path
    else:
        (ROOT / path).write_bytes(b)


boundary = {"deviceUsed": False, "buildRun": False, "productTestsRun": False, "productPASS": False,
            "goalCount": 19, "priorityAfter11": [13, 14, 12, 15, 16, 17, 18, 19], "goal10": "PAUSED", "CPU": "STOPPED", "oldWetM1": "DEFERRED",
            "render": "XComponent+shader foreground/background only; FAULT/explicitly STALE previous valid new-pipeline frame; no Canvas/softwareGL product fallback",
            "roles": "designer algorithm; architect engineering/first attribution; developer implementation+compile; tester offline case/oracle reuse/fixture/host/build/sign/analysis; effector all device deployment/probe/objective/effect, serialized",
            "managerGateAdded": False, "developerDeviceGateAdded": False,
            "queue": "不允许修改 D:\\hmprojects\\HelloWorld\\docs\\后续工作清单.md；唯一例外本人行状态；不更新滴答同名清单",
            "executionRequirement": "要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。"}

e = read("docs/analysis-T678-evidence-v1.json")
c678 = {"schema": "T678_ARTIFACT_AND_RUNTIME_SPLIT_V1", "date": "2026-11-19", "boundaries": boundary,
        "verdict": e["verdict"], "evidence": "docs/analysis-T678-evidence-v1.json", "packages": e["packages"],
        "A2": {"route": "M2 FullM1Transport m2route, not T597 formal M1; same library/page name insufficient", "pid": "14871", "bootAttempt": "3", "epoch": "3", "candidateRun": "1", "publishedRun": "0", "ownerTid": "15134", "bootWindowSurrogate": "1", "actualEGLContext": "NOT_CAPTURED", "lastStage": "ProgramEnter28", "lastNs": "283477263148127", "terminal": False, "createExitObserved": False, "classification": "OPEN_COMPOSITE_PROGRAM_STAGE_AT_OBSERVATION", "apiUnreturned": "UNKNOWN", "permanentHang": "NOT_PROVEN", "root250msApplicableToColdCreate": False},
        "bootFieldSemantics": e["bootSemantics"],
        "M1": {"route": "formal T597-fixed re-signed T593-m1", "run": "2", "epoch": "4", "session": "2", "physicalCounterSequence": e["M1CounterSequence"], "classification": "POST_CREATE_RUNTIME_ACCEPT_CALLBACKS_AND_PRESENTATION_BUSY", "busy": e["M1BusyMeaning"], "GPUCorrectnessOrActivityTimeProven": False, "NeverPublishedREADYSupported": False, "sourceNeverEnqueuedSupported": False, "recoveryEndpointAbsentSupported": False},
        "routing": {"type": "TEST_REPORT_INTERPRETATION_AND_PRECISE_EVIDENCE_GAP", "newProductSeamRequiredNow": False, "designer": "NO_ALGORITHM_DEFECT_ESTABLISHED", "developer": "NO_CONCRETE_NEW_PRODUCT_GAP_ESTABLISHED", "tester": "reuse T676-A/T663 exact M2 cursor package and T676-B formal M1 existing endpoints; no duplicate card", "effector": "original T590/T591/T593 device raw collection separated by actual artifact/attempt; no cross-domain success"},
        "M1ExistingEndpoints": ["status", "recoveryState", "bootDiagnostics", "executionDiagnosticsV1", "timelineSnapshotBegin/Page/End", "coldTraceBeginV1 family"],
        "M2ExistingEndpoints": ["m2CreateDiagnostics", "m2CreateLog"],
        "budget": {"windowMaxSeconds": 60, "diagnosticNativeQueriesIncludingPagesAndLogsMax": 64, "recordMaxUTF8Bytes": 16384, "rawFragmentMaxBytes": 8192, "directoryMaxBytes": 1048576, "indexReserveBytes": 32768, "largeJSON": "one API call, preserve complete raw string in fragments with offsets+full SHA; never re-call per fragment", "existingEndpointCreditAndTTL": "unchanged; all probes count", "normalControlAndInputCalls": "separate log; not hidden as diagnostic budget"},
        "readSemantics": {"status": "ObserveControlLocked can enforce real deadline/revoke, not side-effect-free", "bootDiagnostics": "Store Copy then App mutex for recovery append, not one atomic snapshot or fully independent API", "newSeamCondition": "same-scope exact endpoint start/end/error/module and demonstrated unavailable existing channel; architect specifies only missing bounded field"},
        "missingFields": e["missingSameAttempt"], "separateDomains": ["oldT591 M2 LINK", "T590a2 other M2 boot program stage", "T593 formal M1 runtime", "T496 Windows host/ANGLE"],
        "downstream": "T519/T410/T437 nonempty real SOURCE prerequisites unchanged; current original scopes remain incomplete"}
emit("docs/architecture-T678-contract-v1.json", c678)

constraints = [
    {"id": "E01", "area": "SSBO/block/binding/shared/checked arithmetic", "class": "DEVICE_QUERY_PLUS_NEW_LAYOUT_UNKNOWN", "required": "per-pass actual arrays/buffer/range/offset/stage-active count; observed limits scoped to driver/context, padded bytes and overflow checked"},
    {"id": "E02", "area": "shader/source/cache identity and program scale", "class": "EXISTING_ENGINEERING_PATTERN_NOT_SIZE_LIMIT", "required": "canonical source/ELF actual bytes, target compile/link/program identity, fatal dependency checks; no inferred KB threshold"},
    {"id": "E03", "area": "dispatch/global synchronization/full work", "class": "DESIGN_REQUIRED", "required": "all clear/transfer/sort/contact/solver/audit/replay/scene passes and barriers; no cross-workgroup barrier assumption or hidden per-step CPU readback"},
    {"id": "E04", "area": "determinism/atomic/order/wide arithmetic", "class": "DESIGN_AND_TARGET_EVIDENCE_REQUIRED", "required": "fixed order or proven finite exact accumulation, range/rounding/collision and full state replay, no assumed float atomics"},
    {"id": "E05", "area": "contact/search/history/capacity", "class": "CANDIDATE_UNKNOWN", "required": "Nd/Ccap/Zcap/Bmax, stable pair+generation key/history; overflow rejects candidate, no truncation"},
    {"id": "E06", "area": "state banks/trial/reject/replay/transaction", "class": "DESIGN_REQUIRED", "required": "complete physical/source/tensor/contact/geometry/water state, all failed+successful micro work, private proof/atomic commit pattern without reusing old ABI"},
    {"id": "E07", "area": "origin/source ownership", "class": "DESIGN_REQUIRED", "required": "per-point/mixed-grid source representation and bounds; one owner per actual mass; no silent loss/duplicate mass"},
    {"id": "E08", "area": "topology/dimensionality/geometry/migration", "class": "NOT_IN_EXISTING_HEIGHT_STATE", "required": "true free-volume/support/position/stress/holes, explicit seed or reconstruction assumption, actual old-new overlap"},
    {"id": "E09", "area": "water geometry/load/interface", "class": "HIGHER_PRIORITY_REAL_PREREQUISITES", "required": "13 then14 then12 trusted interfaces, GCL/volume/face and total traction/impulse/work; no rectangle-spectrum transfer to 3D/cutcell"},
    {"id": "E10", "area": "scene/shader/accepted token", "class": "NEW_REPRESENTATION_DESIGN_REQUIRED", "required": "full volume/surface visibility and layer/3D consumers, GPU packing/barriers/fences/lease/fragment cost; no stretch as physics"},
    {"id": "E11", "area": "cold/credit/real retirement", "class": "REUSE_PATTERN_NEW_BUDGET_REQUIRED", "required": "all new source/history/geometry/water archives, fixed page/builder/reader/TTL bounds, physical freeing and GL retirement before credit return"},
    {"id": "E12", "area": "peak memory/old-new overlap", "class": "UNMEASURED", "required": "max over actual lifetime, GPU/native/FBO/driver separated; single block limit is not total memory; no net-delta-only budget"},
    {"id": "E13", "area": "rotation input/time/lifetime", "class": "DESIGN_AND_INPUT_QUALIFICATION_REQUIRED", "required": "unwrapped theta/Omega/alpha/accepted interval/source and overflow, pause/reverse/3D axis; callback receive time not real hand speed"},
    {"id": "E14", "area": "performance/30fps/real-time/thermal", "class": "COST_NOT_MEASURED", "required": "new accepted frames distinct from STALE repeats, acceptedSimTime/wallTime, root p95/p99/worst/all-trial GPU and submission/thermal costs; no120fps render requirement"},
    {"id": "E15", "area": "reference/cases/provenance/role", "class": "REFERENCE_REQUIRED_OR_NOT_RUN", "required": "same observable/reference uncertainty and dimension/domain, C01-C24 preserved, tester offline/effector device; planning not physical PASS"}]
mapping = {
    "C01": ["E06", "E07", "E11"], "C02": ["E08", "E13", "E15"], "C03": ["E01", "E03", "E12", "E14"], "C04": ["E08", "E09"],
    "C05": ["E04", "E08", "E15"], "C06": ["E06", "E07", "E08", "E14", "E15"], "C07": ["E03", "E04", "E15"], "C08": ["E07", "E08", "E12"],
    "C09": ["E04", "E05", "E08"], "C10": ["E05", "E08", "E13"], "C11": ["E05", "E06", "E08"], "C12": ["E01", "E05", "E07", "E12"],
    "C13": ["E04", "E13"], "C14": ["E05", "E06", "E13", "E15"], "C15": ["E06", "E07", "E11", "E12", "E13", "E14"], "C16": ["E08", "E09", "E13"],
    "C17": ["E06", "E08", "E15"], "C18": ["E04", "E06", "E08"], "C19": ["E04", "E06", "E07", "E09"], "C20": ["E01", "E12", "E14", "E15"],
    "C21": ["E08", "E09", "E15"], "C22": ["E07", "E08", "E10"], "C23": ["E05", "E08", "E09", "E15"], "C24": ["E06", "E07", "E08", "E09", "E10", "E12", "E14"]}
e = read("docs/analysis-T679-evidence-v1.json")
c679 = {"schema": "T679_G15_G19_GPU_RESEARCH_CONSTRAINTS_V1", "date": "2026-11-19", "boundaries": boundary,
        "status": "RESEARCH_CONSTRAINT_INVENTORY_COMPLETE_NOT_IMPLEMENTATION_READY", "algorithmChoiceFrozen": False, "MPMChosen": False, "twoLayerFChosen": False,
        "goal1to14NewGate": False, "currentG9G11G13ContractChanged": False, "noNetworkResearchPerformed": True,
        "evidence": "docs/analysis-T679-evidence-v1.json", "document": "docs/architecture-G15-G19-research-constraints-v1.md",
        "evidenceClasses": ["D-HIST historical device pbuffer", "D-M2 same context actual limit query", "S-SDK API declaration only", "C-PROFILE existing contract bytes not allocation or new-route PASS", "A-RESEARCH explicit assumptions/symbolic byte arithmetic", "U-UNKNOWN"],
        "constraints": constraints,
        "caseMapping": [{**c, "engineeringConstraints": mapping[c["id"]], "originalCriterionUnchanged": True} for c in e["cases"]],
        "candidateRepresentations": [
            {"id": "A", "representation": "fixed-column S/F/P limited tilt expansion", "goals": [15], "status": "CANDIDATE; not90/180 support", "constraints": ["E01", "E04", "E06", "E07", "E08", "E14"]},
            {"id": "B", "representation": "directional/local multi-chart plus free volume", "goals": [15, 16, 17], "status": "CANDIDATE", "constraints": ["E06", "E07", "E08", "E10", "E12", "E13"]},
            {"id": "C", "representation": "full-volume material-point/grid continuum MPM class", "goals": [15, 16, 17, 18, 19], "status": "CANDIDATE_NOT_FROZEN", "constraints": ["E01", "E02", "E03", "E04", "E05", "E06", "E07", "E08", "E09", "E10", "E11", "E12", "E14"]},
            {"id": "D", "representation": "full-volume Eulerian stress/volume fraction", "goals": [15, 16, 17, 18, 19], "status": "CANDIDATE", "constraints": ["E01", "E03", "E04", "E06", "E07", "E08", "E09", "E12"]},
            {"id": "E", "representation": "full DEM", "goals": [15, 16, 17, 18, 19], "status": "CANDIDATE", "constraints": ["E01", "E03", "E04", "E05", "E06", "E07", "E08", "E12", "E14"]},
            {"id": "F", "representation": "continuum/local DEM hybrid", "goals": [15, 16, 17, 18, 19], "status": "CANDIDATE", "constraints": ["E05", "E06", "E07", "E08", "E09", "E11", "E12", "E14"]},
            {"id": "LAYER", "representation": "2/4-layer or direct full-volume profile", "goals": [18], "status": "COMPARISON_REQUIRED_NOT_PRECONDITION_FOR9_12_15_17", "constraints": ["E04", "E06", "E07", "E10", "E12", "E15"]},
            {"id": "3D", "representation": "finite-thickness full3D sand-water,6walls", "goals": [19], "status": "REQUIRES_TRUE_Z_MOTION_NOT_RENDER_STRETCH", "constraints": [c["id"] for c in constraints]}],
        "observedM2NumericLimitsScope": {"blockMaxBytes": 1073741824, "globalBindings": 32, "computeBlocks": 16, "fragmentBlocks": 16, "offsetAlignmentBytes": 256, "sharedBytes": 32768, "groupCountEachAxis": 65535, "groupSizeEachAxis": 1024, "groupTotalInvocations": 1024, "newRouteAllocationPASS": False},
        "examples": e["examples"], "sharedTileExamples": e["sharedTileExamples"], "sourceMapExample": e["additionalSourceMapExample"],
        "existingBudgets": e["legacyContracts"], "existingKnownCombinedGPUAndHost641920128IsGPUCapacity": False,
        "brokerCaps": {"hostComponentCreditBytes": 524375552, "GPUFULLComponentCreditBytes": 366564432, "notSystemRSSOrGPUAllocationLimits": True},
        "formalG15MinimumInputs": ["angle/rate/material/wet/dimensional/domain/units and initial-boundary scope", "same-observable A/C and other candidate comparison with reference/error", "complete operators/stress/contact/history/work/source/trial/replay and migration", "N/Np/Ng/q/NS/Nd/Ccap/Zcap/bin/dt/iteration/trial/event bound and numerical range/rounding", "existing actual physical evidence and real wet interfaces only where needed", "GPU/native/FBO/driver/lifetime/old-new overlap and new-accepted30fps/real-time/thermal targets"],
        "unmeasured": e["unknown"], "existingTools": ["canonical source generation/fatal dependency and ELF/BuildId provenance", "typed proof/full tail audit/atomic commit patterns, not old ABI for new state", "bounded cold broker/actual retirement/streaming raw pages", "GPU shader scene pack/lease/barriers on exact route", "tester static/offline cases and effector exact-device acquisition"]}
emit("docs/architecture-T679-constraints-v1.json", c679)

assert len(c679["caseMapping"]) == 24 and set(mapping) == {c["id"] for c in e["cases"]}
assert all(set(m["engineeringConstraints"]) <= {c["id"] for c in constraints} for m in c679["caseMapping"])
assert len(c679["constraints"]) == 15
assert c679["examples"][-1]["stateOneBankBytes"] * 3 > 1073741824
assert c679["examples"][-1]["sortOneBufferBytes"] * 2 > 1073741824
assert c679["examples"][-1]["sortOneBufferBytes"] < 1073741824
assert c678["M1"]["physicalCounterSequence"] == [88, 318, 536, 1002, 1513]
assert c678["A2"]["root250msApplicableToColdCreate"] is False
assert all(p["receiptMatched"] for p in c678["packages"].values())
assert c678["packages"]["T593-m1"]["libgrsm1.so"]["sha256"] == "527c863f36bf543f1d4c45d212ef60ec62c2401f5fe693e5a453589c7f4280bf"
assert c678["packages"]["T593-m1"]["libgrsm1.so"]["registeredNameStringsPresent"]["recoveryState"]
for p in ["docs/architecture-T678-v1.md", "docs/architecture-G15-G19-research-constraints-v1.md"]:
    assert (ROOT / p).exists()
result = {"schema": "T678_T679_STATIC_CHECK_V1", "result": "STATIC_ORIGINAL_BYTES_AND_RESEARCH_CONSTRAINT_CHECK_PASS", "deviceUsed": False, "buildRun": False, "productTestsRun": False, "productPASS": False,
          "scope": ["two actual signed packages/ELF identities", "a2 boot3 last stage and lifecycle terminal semantics", "M1 same-scope counter sequence and existing endpoint names", "24 cases mapped to15 engineering constraints without changing criteria", "four explicit byte examples, per-block/shared limits under same scoped observation", "19-goal/role/no-new-upstream-gate boundaries"],
          "contracts": [record("docs/architecture-T678-contract-v1.json"), record("docs/architecture-T679-constraints-v1.json")]}
if not CHECK:
    write("docs/analysis-T678-T679-check-v1.json", result)
print(json.dumps({"result": "CHECK_PASS" if CHECK else "FROZEN_AND_CHECKED", "caseMappings": 24, "constraints": 15, "deviceUsed": False}))
