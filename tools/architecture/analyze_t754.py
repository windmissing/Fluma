"""Join the two T751 originals with the real T745/T748 ELF bytes and code paths."""
from hashlib import md5
import json
from pathlib import Path
import re
import struct
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write


CPP = "entry/src/main/cpp/gpu_resident/g9dm/"
V7 = "test/reports/t491/t738/v7/"
NATIVE = "libs/arm64-v8a/libgrs_dm_t647.so"
CONTRACT = "docs/architecture-T736-source-contract-v1.json"


def read_json(path):
    return json.loads((ROOT / path).read_bytes())


def rows(path):
    return [json.loads(line) for line in (ROOT / path).read_text(encoding="utf-8-sig").splitlines()]


def elf_file(expected):
    path = expected["path"]
    contents = (ROOT / path).read_bytes()
    facts = {**record(path), "md5": md5(contents, usedforsecurity=False).hexdigest(), "buildId": build_id(contents)}
    assert all(facts[k] == expected[k] for k in ("bytes", "sha256", "buildId"))
    return contents, facts


def audit_artifacts():
    data = {}
    for task in ("t745", "t748"):
        path = f"test/reports/{task}/compile.json"
        receipt = read_json(path)
        assert not receipt["signed"]
        _, driver = elf_file(receipt["driver"])
        shipped_bytes, packaged = elf_file(receipt["pairedDsoFromHap"])
        linked_bytes, linked = elf_file(receipt["linkedDso"])
        hap = receipt["unsignedHap"]["path"]
        assert record(hap)["sha256"] == receipt["unsignedHap"]["sha256"]
        with ZipFile(ROOT / hap) as archive:
            assert archive.read(NATIVE) == shipped_bytes
        assert linked["buildId"] == packaged["buildId"] and linked["sha256"] != packaged["sha256"]
        data[task] = {"receipt": record(path), "driver": driver, "packagedDso": packaged,
                      "linkedDso": linked, "unsignedHap": record(hap)}
        if task == "t748":
            for role in ("boot", "source"):
                original = receipt["roles"][role]
                source = (ROOT / original["path"]).read_bytes()
                assert len(source) == original["bytes"] and sha(source) == original["sha256"]
                assert shipped_bytes.count(source + b"\0") == linked_bytes.count(source + b"\0") == 1
    assert data["t745"]["driver"]["md5"].startswith("738388e7")
    assert data["t748"]["driver"]["md5"].startswith("08044402")
    assert data["t748"]["packagedDso"]["md5"].startswith("b99227cc")
    assert data["t748"]["linkedDso"]["md5"].startswith("780cc7c1")
    report = "docs/test-report-T751-v1.md"
    text = (ROOT / report).read_text(encoding="utf-8")
    assert all(found["md5"][:8] in text for found in (data["t745"]["driver"], data["t748"]["driver"],
                                                     data["t748"]["packagedDso"], data["t748"]["linkedDso"]))
    return {**data, "md5TranscriptionSource": record(report), "targetWholeFileSha256Captured": False,
            "identityConclusion": "SECOND_DRIVER_AND_MAPPED_DSO_ARE_T748_PACKAGED_VARIANT"}


def audit_attempt(raw_path, artifact, configure):
    raw = rows(raw_path)
    ident, = [row for row in raw if row.get("stage") == "ATTEMPT_IDENTITY"]
    assert ident["driverMappedBuildId"] == artifact["driver"]["buildId"]
    selected = [row for row in raw if row.get("schema") == "G9BootV1"]
    assert len({row["stage"] for row in selected}) == len(selected)
    common = ("pid", "run", "owner", "context", "ticket")
    for row in selected:
        assert all(row[k] == ident[k] for k in common) and row["tid"] == ident["pid"]
        assert row["mappedBuildId"] == artifact["packagedDso"]["buildId"]
    stages = {row["stage"]: row for row in selected}
    sequence = ("ROOT_INIT_ENTER", "WARM_ENTER", "FINISH_ENTER", "FINISH_EXIT", "MAP_ENTER",
                "MAP_EXIT", "UNMAP_EXIT", "PREPARE_EXIT", "BOOT_AUDIT_OK")
    timestamps = [int(stages[name]["ns"]) for name in sequence]
    assert timestamps == sorted(set(timestamps))
    assert stages["FINISH_EXIT"]["raw"] == 37146 and stages["MAP_ENTER"]["raw"] == 1166976
    tail = stages["UNMAP_EXIT"]
    assert (tail["raw"], tail["gl"], tail["egl"], tail["error"], tail["seal"]) == (1, 0, 12288, 0, 0x641647)
    timing = {phase: int(stages[phase + "_EXIT"]["ns"]) - int(stages[phase + "_ENTER"]["ns"])
              for phase in ("SHADER_COMPILE", "PROGRAM_LINK")}
    config, = [row for row in raw if row.get("stage") == configure]
    return {"file": record(raw_path), "identity": ident, "actualDsoBuildId": tail["mappedBuildId"],
            "bootTail": [tail["error"], tail["seal"]], "bootAuditNs": stages["BOOT_AUDIT_OK"]["ns"],
            "bootDurationsNs": timing, "terminalConfigure": config}, raw


def audit_root(data, raw):
    receipt = read_json("test/reports/t748/compile.json")
    manifest = read_json(receipt["sourceManifest"]["path"])
    assert record(receipt["sourceManifest"]["path"])["sha256"] == receipt["sourceManifest"]["sha256"]
    source, = [row for row in raw if row.get("stage") == "SOURCE_ROOT"]
    begin, = [row for row in raw if row.get("stage") == "CONFIGURE_ENTER"]
    admission, = [row for row in raw if row.get("stage") == "SOURCE_ADMISSION_EXIT"]
    assert begin["variant"] == "BOOT_SOURCE" and begin["roleSetSHA"] == manifest["roleSetSHA"] == receipt["roleSetSHA"]
    assert begin["cacheConfigured"] is False and data["terminalConfigure"]["result"] == 0
    assert data["terminalConfigure"]["bootAudited"] is True and source["actualProgram"] == data["terminalConfigure"]["sourceProgram"]
    assert source["sourceSHA"] == begin["sourceSHA"] == manifest["source"]["sha256"]
    assert all(row["sourceSHA"] == manifest["boot"]["sourceSHA"] and row["roleSetSHA"] == manifest["boot"]["roleSetSHA"]
               for row in raw if row.get("schema") == "G9BootV1")
    assert all(source[k] == data["identity"][k] for k in ("pid", "owner", "context", "ticket"))
    assert source["scope"] == data["identity"]["run"]
    assert all(admission[k] == 0 for k in ("pulse", "offered", "began"))
    assert all(source[k] is False for k in ("fenceCreated", "mapReturned", "typedAccepted", "preflight",
                                           "proposedPlanRetired", "committed"))
    assert (source["result"], source["controlFault"], source["kind"], source["ordinal"]) == (6, 9, 1, 0)
    assert (source["dispatches"], source["barriers"], source["copyBarriers"], source["fencePolls"]) == (12, 12, 3, 0)
    assert source["fenceWaitRaw"] is None and source["unmapRaw"] is None
    assert source["commitNs"] == source["fenceNs"] == "0"
    assert source["workObject"] == source["workReadBytes"] == 0
    assert source["baseCommit"] == source["commit"] == source["baseQuanta"] == source["quanta"] == "0"
    assert source["uploaded"] and source["queueBefore"] == 1 and source["pendingAtClaim"] == 0
    assert source["parameterSHA"] == receipt["fixture"]["config.u32"]["sha256"]
    layout_source = (ROOT / (CPP + "dm_contract_generated.h")).read_text(encoding="utf-8")
    assert f'kPhysicalLayoutSHA="{source["layoutSHA"]}"' in layout_source
    words = struct.unpack("<64I", (ROOT / receipt["fixture"]["config.u32"]["path"]).read_bytes())
    seed = bytearray((ROOT / receipt["fixture"]["seed.u32"]["path"]).read_bytes())
    assert sha(seed) == receipt["fixture"]["seed.u32"]["sha256"]
    assert struct.pack("<8I", *words[32:40]).hex() == sha(seed)
    for offset, key in ((4, "scope"), (6, "owner"), (8, "context"), (14, "inputEpoch")):
        struct.pack_into("<Q", seed, offset * 4, int(source[key]))
    seed[42 * 4:50 * 4] = bytes.fromhex(source["parameterSHA"])
    assert sha(seed) == source["seedSHA"]
    candidate, base = source["candidateObject"], source["baseObject"]
    assert (base, candidate) == (1, 3) and (source["baseBank"], source["candidateBank"], source["arena"]) == (0, 2, 0)
    table = read_json(CONTRACT)
    assert manifest["passTable"] == table["passTable"]
    for obs, expected in zip(source["passes"], table["passTable"], strict=True):
        ordinal = expected["ordinal"]
        assert obs == [ordinal, ordinal, 0, 0, 0xFFFFFFFF, *[i + 1 for i in expected["bufferIndices"]]]
    wall_limits = int(source["deadlineNs"]) - int(source["rootBeginNs"])
    configure_gap = int(source["rootBeginNs"]) - int(data["bootAuditNs"])
    assert wall_limits == 250000000 and configure_gap == 316035252656
    return {"sourceManifest": record(receipt["sourceManifest"]["path"]), "sourceAdmission": admission,
            "rootRecord": source, "bootAuditToRootBeginNs": configure_gap, "rootBudgetNs": wall_limits,
            "classification": "TRANSPORT_DEADLINE_BEFORE_EXECUTION_FENCE_AND_AFTER_LAST_DISPATCH_AND_BARRIER_SUBMITTED",
            "rootExpiryExactCheckpoint": "NOT_OBSERVED", "sourceCompileLinkWallTime": "NOT_OBSERVED_WITHIN_PRE_ROOT_GAP",
            "rootWorkProof": "NOT_REACHED", "retirementOutcome": "NOT_OBSERVED",
            "seedReconstructedFromImmutableTemplateMatchesReceipt": True,
            "expectedQ20FromContract": table["firstRootExpected"]["admittedQ"]}


def audit_code():
    keys = (CPP + "research_config.h", CPP + "control_types.h", CPP + "transport.cpp",
            CPP + "controller.cpp", CPP + "source_trace.h", CPP + "progress_clock_v3.cpp",
            "test/t670/dm_source_run.h")
    sources = [(ROOT / path).read_text(encoding="utf-8") for path in keys]
    result, fault, transport, controller, trace, clock, runner = sources
    for content, kind, code in ((result, "Result", 6), (fault, "ControlResult", 9)):
        values = re.search(rf"enum class {kind}[^{{]*{{([^}}]*)}}", content)[1].split(",")
        assert values[code].strip() == "DEADLINE"
    assert "return now.ns > permit_.deadline ? Result::DEADLINE : Result::STALE;" in transport
    dispatch = transport.split("bool Transport::Dispatch(", 1)[1].split("Result Transport::Pump()", 1)[0]
    pump = transport.split("Result Transport::Pump()", 1)[1].split("bool Transport::CommitAccepted", 1)[0]
    assert dispatch.index("glDispatchCompute") < dispatch.index("trace->dispatches") < dispatch.rindex("return Allowed();")
    assert pump.index("if (!Dispatch") < pump.index("++next_") < pump.index("glFlush();") < pump.index("fence_ = glFenceSync")
    assert "if (!Allowed())\n      return Denied();\n    fence_ = glFenceSync" in pump
    assert "if(result!=Result::OK){std::lock_guard<std::recursive_mutex> lock(mutex_);RevokeLocked(result==Result::DEADLINE?ControlResult::DEADLINE" in controller
    revoke = controller.split("ControlResult Controller::RevokeLocked", 1)[1].split("Result Controller::ConfigureResearch", 1)[0]
    assert not any(word in revoke for word in ("head_", "size_", "queue_", "commit_"))
    assert "open=ready=baseline=false" in clock and "pending=activeRemaining=0" in clock
    assert "queueAfter = 0" in trace and controller.count("trace->queueAfter = size_") == 1
    assert "std::atomic<uint32_t> result_{0}" in runner
    assert runner.index("SetSourceTrace(prior_)") < runner.index("controller_.Stop()") < runner.index("controller_.Drain()")
    retire = transport.split("bool Transport::RetireRejected", 1)[1].split("accept647::ProgramFacts", 1)[0]
    assert "glFenceSync" in retire and "glClientWaitSync" in retire and "GL_CONDITION_SATISFIED" in retire
    return {"sourceFiles": [record(path) for path in keys], "deadlineEnums": {"transport": 6, "control": 9},
            "preFenceCandidateWindows": ["final_dispatch_post_guard", "after_submit_loop_before_flush", "after_flush_before_fence"],
            "queueAfterAndCompletionOnFailure": "DEFAULT_FIELDS_NOT_SAMPLED_AT_REJECTION"}


def audit():
    identity = audit_artifacts()
    old, old_raw = audit_attempt(V7 + "dm21.jsonl", identity["t745"], "CREATE_EXIT")
    new, new_raw = audit_attempt(V7 + "dm21-v2-final.jsonl", identity["t748"], "CONFIGURE_EXIT")
    assert (len(old_raw), len(new_raw), old["identity"]["pid"], new["identity"]["pid"]) == (25, 26, 4219, 6695)
    assert old["terminalConfigure"]["result"] == 0x666 and old["terminalConfigure"]["rootsAuthorized"] is False
    assert not any(row.get("stage") == "SOURCE_ROOT" for row in old_raw)
    return {"schema": "T754_DM_SOURCE_FIRST_ATTRIBUTION_V1", "deviceUsed": False,
            "artifacts": identity, "initialBootOnly": old, "formalBootSource": new,
            "source": audit_root(new, new_raw), "code": audit_code()}


if __name__ == "__main__":
    write("docs/architecture-T754-evidence-v1.json", audit())
    print("T754 joined packaged and linked ELF, two distinct attempts, twelve submissions and pre-fence deadline")
