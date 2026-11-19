"""Freeze/check architecture-only T660 tables. Does not generate or build product code."""
import copy
import json
from pathlib import Path
import sys

from analyze_t660_t662 import record, sha, write

ROOT = Path(__file__).resolve().parents[2]
CHECK = "--check" in sys.argv


def emit(path, obj):
    raw = (json.dumps(obj, ensure_ascii=False, indent=2) + "\n").encode()
    if CHECK:
        assert (ROOT / path).read_bytes() == raw, "ARCHITECTURE_TABLE_STALE " + path
    else:
        (ROOT / path).write_bytes(raw)


recipe = json.loads((ROOT / "docs/architecture-T563-visual-recipe-v1.json").read_text())
recipe.update(schema="G11_VISUAL_RECIPE_V2", markerRevision=2, sceneRevision=2)
recipe["persistentGhost"] = {"massWord": 30, "identityWords": [3, 4, 5], "source": "sealed CAPTURE event64[15]; independently produced per role", "noPhysicalAlgorithmChange": True}
rp = "docs/architecture-T660-visual-recipe-v2.json"
emit(rp, recipe)

contract = {
    "schema": "T660_G11_SEED_GHOST_ENGINEERING_V1", "date": "2026-11-19",
    "status": "ENGINEERING_READY_NOT_IMPLEMENTED_NOT_PRODUCT_PASS", "deviceUsed": False,
    "productCodeChanged": False, "productTestsRun": False,
    "authority": {"manifest": "docs/architecture-T660-g11-manifest-v2.json", "recipe": rp,
                  "supersedesOnly": ["T563 actualVisualSeed preimage ambiguity", "T563 marker30 reserved in revision1", "T634 visual seed bootstrap/oldP persistent identity", "T647 actualVisualSeed producer ambiguity"],
                  "originalDeveloperTask": "T639", "consumeActualProofImplementation": "T648 2a670f2"},
    "versions": {"visualConfig": 2, "descriptor": 1, "marker": 2, "glyph": 2, "reader": 2, "bankHeader": 2,
                 "expected224": 1, "completion256": 2, "compositeExtension": 1,
                 "coldSchema": "G11_VIS_COLD_V2", "coldPage": "G11_VIS_SEGMENT_PAGE_V2", "seedSchema": "G11_VIS_SEED_RAW_V1"},
    "configWordValues": {"1": 2, "16": 1, "17": 2, "18": 2, "19": 2},
    "seed": {"encoding": "exact GPU returned LE U32 bytes; concatenate raw segments, no framing/padding/normalization",
             "algorithm": "SHA256", "digestWords": "32 digest bytes grouped in 8 little-endian U32 words",
             "segments": [{"name": "CURRENT_bankHeader128", "workSegment": "control", "relativeByteOffset": 0, "bytes": 512},
                          {"name": "CURRENT_marker32", "workSegment": "markerCURRENT", "relativeByteOffset": 0, "bytesFormula": "128*V"},
                          {"name": "Pstyle512x4", "workSegment": "control", "relativeByteOffset": 1536, "bytes": 8192},
                          {"name": "VisualConfig64V2", "workSegment": "control", "relativeByteOffset": 60544, "bytes": 256}],
             "lengthFormula": "128*V+8960", "initialGeneration": 0, "initialTicks": 0,
             "excluded": ["ACTUAL", "REPLAY", "glyph", "plan", "fourMaps", "counters", "Expected224", "completion256", "fileManifest", "recipeText", "physicalSeedRaw"],
             "externalReceiptBinds": ["physicalSeedSHA", "physicalInitialToken/bank/generation", "pageSession", "inputEpoch", "layoutSHA", "actualPipelineSHA", "BuildId", "witnessCreditGeneration"],
             "expected224Words": [144, 152], "completion256Words": [224, 232],
             "untrustedSubstitutes": ["recipeSHA", "physicalSeedSHA", "zero", "postRootCURRENT", "hashOfSegmentHashes"],
             "lifetime": ["fixed typed parent/config/program", "reserve single witness GPU+native", "GPU cold initialization", "four bounded GPU copies", "actual init fence signaled", "bounded caller-span read and hash", "seal immutable receipt", "retire GPU access and release reader then return witness credit", "private ActiveProgramIdentity", "private ExpectedForActivePipeline", "first root claim"]},
    "ghost": {"markerWord30": {"name": "phaseOldPacketMassQ20", "type": "U32_Q20", "valid": "phaseActive && phaseOldKind==P && owner!=P", "source": "sealed CAPTURE event64[15] equals plan.amount", "otherwise": 0},
              "markerWords3to5": {"P": "current slot/generation/id", "nonP_activeOldP": "ghost old slot/generation/id, not live slot reference", "otherwise": [0, 0, 0]},
              "massSnapshotWord10": "always current owner massSnapshot; F current cohort mass",
              "group": ["run", "context", "oldSlot", "oldGeneration", "oldId", "phaseStartTick"],
              "representative": "independent per-role deterministic minimum marker slot, same-group old mass/anchors equal; marker.flags bit4",
              "sourceZeroTime": "preserve active phase old mass/identity/anchors/start/span", "sameOwnerMix": "preserve old ghost; update only current mass",
              "newOwnerTransition": "existing single-phase rebase retires old ghost; latch only if actual old owner is P", "expire": "clear old mass/oldKind/representative and non-P packet union",
              "defaultBlendTicks": 256, "independentBlendZero": "immediate transition no ghost; distinct configSHA; not default substitute",
              "audit": "all 32 words include mass/identity, independently produced ACTUAL role2 and REPLAY role4; shared glyph cannot authorize replay",
              "glyph6": "active old-P ghost -> marker30; current P -> actual packet mass; otherwise0", "PstyleMassAdded": False},
    "bootstrapMemory": {"witnessHeaderBytes": 4096, "rawBaseByte": 4096, "GPUandNativeCapacityByV": {"2048": 3149824, "8192": 4722688},
                        "newRawCopies": 0, "newResidentGLBuffers": 0, "extraNormalRootReadbackBytes": 0, "extraNormalRootDispatches": 0, "extraNormalRootFences": 0,
                        "newOwnerReceiptPODBytes": 512, "readerCount": 1, "pageMaxBytes": 4096, "pageMaxUTF8": 16384, "ttlNs": 60000000000,
                        "credit": "exclusive with phase witness; no return until actual GL retirement + reader released; no FULL/ROOT slot allocation",
                        "knownGPUColdWitnessPeakUnchangedBytes": 641920128, "twoDMFULLSlotsUnchangedBytes": 388308992,
                        "POD128Words": [
                            {"range": [0, 8], "fields": "magic/rev/state/parent/composite/K/V/flags"},
                            {"range": [8, 56], "fields": "six SHA8: actualVisualSeed, physicalSeed, layout, actualPipeline, visualConfig, visualRecipe"},
                            {"range": [56, 72], "fields": "eight U64: bootstrapAttempt, owner, context, run, surface, pageSession, inputEpoch, physicalBankGeneration"},
                            {"range": [72, 74], "fields": "physicalBankIndex, segmentCount"},
                            {"range": [74, 92], "fields": "nine U64: physicalCommit, initFenceTicket, witnessGeneration, creditTicket, createdNs, deadlineNs, readerId, physicalTokenId, readyNs"},
                            {"range": [92, 96], "fields": "reservedZero"},
                            {"range": [96, 112], "fields": "4x4 U32 segment descriptors: GPU source byteOffset, preimage packed offset, bytes, segmentKind"},
                            {"range": [112, 116], "fields": "totalRawBytes U64, completedRawBytes U64"},
                            {"range": [116, 120], "fields": "error, copyCount, readerActive, glRetired"},
                            {"range": [120, 128], "fields": "reservedZero"}],
                        "excludedFrom512": "raw data, allocator overhead, transient streaming SHA state, separately resident ProgramFacts/ActiveProgramIdentity/proofs; report actual ABI separately"},
    "t648ActualAArch64ABI": {"ExpectedWords": 224, "ExpectedBytes": 896, "ProgramFacts": 408, "ActiveProgramIdentity": 416, "ExpectedContract": 56, "NativeMappedObservation": 80, "CompositeParentR1": 1064, "CompositeParentDM": 1064},
    "controlRanges": [{"name": "FrozenExpected224", "byteStart": 61312, "byteEnd": 62208, "bytes": 896}, {"name": "FrozenExpectedSHA8", "byteStart": 62208, "byteEnd": 62240, "bytes": 32}, {"name": "reservedZero", "byteStart": 62240, "byteEnd": 65536, "bytes": 3296}],
    "generatorHandoff": {"owner": "original T639 developer", "authoritativeManifest": "docs/architecture-T660-g11-manifest-v2.json", "files": ["tools/g11/generate_contract.cjs", "test/t566_generate_composite.cjs", "tools/g11/prepare_compile.cjs", "entry/src/main/cpp/gpu_resident/g11vis/generated_contract.h", "entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl", "entry/src/main/cpp/gpu_resident/g11vis/marker_map.glsl", "entry/src/main/cpp/gpu_resident/g11vis/visual_transport.*", "entry/src/main/cpp/gpu_resident/g11vis/cold.*", "entry/src/main/cpp/gpu_resident/g11vis/kernels.cmake", "reader dictionary and cache ABI"],
                         "dependencies": ["T660 contract", "T660 full manifest", "T660 recipe", "T647 table", "actual source and actual generated bytes"],
                         "newIdentities": ["configSHA", "recipeSHA", "compositeLayoutSHA", "actualShaderSHA", "actualPipelineSHA", "cacheABISalt(T660 contractSHA)", "readerSchema"],
                         "staleBehavior": "FATAL; no old v1 assets/hot continuation", "architectureCardRanProductGenerator": False},
    "boundaries": {"render": "XComponent+shader background and foreground; FAULT or explicitly STALE, no Canvas/softwareGL product fallback", "CPU": "STOPPED", "goal10": "PAUSED", "waterOrder": [13, 14, 12], "managerGateAdded": False, "deviceGateForDeveloperAdded": False}}
cp = "docs/architecture-T660-contract-v1.json"
emit(cp, contract)

m = json.loads((ROOT / "docs/architecture-T563-g11-manifest-v1.json").read_text())
m = copy.deepcopy(m)
m.update(schema="T660_G11_VIS_ENGINEERING_V2", date="2026-11-19", status="ENGINEERING_READY_NOT_IMPLEMENTED_NOT_VALIDATED")
m["predecessor"] = record("docs/architecture-T563-g11-manifest-v1.json")
m["amendment"] = record(cp)
m["visualSeedRecipe"] = record(rp)
m["inputs"].append(record(cp))
m["inputs"].append(record("docs/architecture-T647-acceptance-v1.json"))
m["identity"]["actualVisualSeedSHA"] = "T660 exact CURRENT header512 + marker128V + Pstyle8192 + VisualConfig256 GPU bytes, after initialization before first claim"
m["identity"]["revision2Required"] = True
m["configConstants"]["revision"] = 2
m["configWordValues"] = contract["configWordValues"]
m["marker32"][30]["name"] = "phaseOldPacketMassQ20"
m["marker32"][30]["semantics"] = contract["ghost"]["markerWord30"]
for w in [3, 4, 5]:
    m["marker32"][w]["revision2Union"] = contract["ghost"]["markerWords3to5"]
m["controlSegments"] = [x for x in m["controlSegments"] if x["name"] != "reservedZero"] + copy.deepcopy(contract["controlRanges"])
m["controlABI"]["bankHeader128"] = m["controlABI"]["bankHeader128"].replace("1 rev", "1 rev2")
m["controlABI"]["markerRevision"] = 2
m["controlABI"]["roleNormalization"] += "; T660 includes word30 and ghost identity union; independent per-role representative min, no shared glyph authorization"
m["seedRawContract"] = contract["seed"]
m["bootstrapMemory"] = contract["bootstrapMemory"]
m["cold"]["revision2Schemas"] = [contract["versions"]["coldSchema"], contract["versions"]["coldPage"], contract["versions"]["seedSchema"]]
m["cold"]["capability"] = contract["versions"]["coldSchema"]
m["cold"]["page"] = contract["versions"]["coldPage"]
emit("docs/architecture-T660-g11-manifest-v2.json", m)

checks = []
for r in m["resources"]:
    segs = {s["name"]: s for s in r["workSegments"]}
    chain = r["workSegments"]
    assert all(s["byteEnd"] - s["byteStart"] == s["bytes"] for s in chain)
    assert all(a["byteEnd"] == b["byteStart"] for a, b in zip(chain, chain[1:]))
    assert chain[-1]["byteEnd"] == r["totalWorkBytes"]
    packed = 0
    desc = []
    for s in contract["seed"]["segments"]:
        size = s.get("bytes", 128 * r["V"])
        start = segs[s["workSegment"]]["byteStart"] + s["relativeByteOffset"]
        assert start + size <= segs[s["workSegment"]]["byteEnd"]
        desc.append({"name": s["name"], "sourceWorkByteOffset": start, "bytes": size, "preimageByteOffset": packed, "witnessByteOffset": 4096 + packed})
        packed += size
    assert packed == 128 * r["V"] + 8960
    capacity = contract["bootstrapMemory"]["GPUandNativeCapacityByV"][str(r["V"])]
    assert 4096 + packed <= capacity
    checks.append({"parent": r["parent"], "K": r["K"], "V": r["V"], "totalWorkBytes": r["totalWorkBytes"], "preimageBytes": packed, "witnessCapacityEach": capacity, "segments": desc})
assert m["configConstants"]["markerWords"] == 32 and m["configConstants"]["glyphWords"] == 8
assert all(a["byteEnd"] == b["byteStart"] for a, b in zip(m["controlSegments"], m["controlSegments"][1:]))
assert m["controlSegments"][0]["byteStart"] == 0 and m["controlSegments"][-1]["byteEnd"] == 65536
pod = contract["bootstrapMemory"]["POD128Words"]
assert pod[0]["range"][0] == 0 and pod[-1]["range"][1] == 128
assert all(a["range"][1] == b["range"][0] for a, b in zip(pod, pod[1:]))
evidence = {"schema": "T660_STATIC_CONTRACT_CHECK_V1", "notUsingDevice": True, "noProductTestRun": True, "noBusinessCodeChanged": True,
            "result": "STATIC_CONTRACT_CHECK_PASS_NOT_IMPLEMENTATION_PASS", "resourceRows": checks,
            "controlPartitionBytes": 65536, "T647FrozenExpectedAndHashBytes": 928, "markerBytes": 128, "glyphBytes": 32,
            "newHostReceiptBookkeepingBytes": 512, "newGPUOrColdRawAllocationBytes": 0,
            "authority": [record(x) for x in [cp, rp, "docs/architecture-T660-g11-manifest-v2.json"]],
            "consumedCurrentSources": [record(x) for x in ["docs/developer-T639-v1.md", "docs/developer-T648-v1.md", "entry/src/main/cpp/gpu_resident/acceptance_t647/contract.cpp", "entry/src/main/cpp/gpu_resident/g11vis/composite_gate.cpp", "entry/src/main/cpp/gpu_resident/g11vis/marker_map.glsl", "entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl", "tools/g11/generate_contract.cjs", "test/t566_generate_composite.cjs", "tools/g11/prepare_compile.cjs"]],
            "implementationState": "Existing product generator/source still revision1 and rejects default P capture; original T639 must implement revision2, not passed by this architecture check"}
if not CHECK:
    write("docs/analysis-T660-evidence-v1.json", evidence)
print(json.dumps({"result": "CHECK_PASS" if CHECK else "FROZEN_STATIC_CONTRACT", "resourceRows": len(checks), "preimageBytesByV": {str(x["V"]): x["preimageBytes"] for x in checks}, "deviceUsed": False}))
