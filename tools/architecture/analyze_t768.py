"""Reconcile the T762 DM SOURCE deadline and measure its captured API edges."""

from statistics import median
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, record, sha, write
from analyze_t754 import NATIVE, elf_file, read_json, rows


RECEIPT = "docs/receipt-T758-compile-v1.json"
CAPTURE = "test/reports/t491/t738/v8/dm22-final.jsonl"
WAIT = "test/reports/t491/t738/v8/dm22-wait.txt"
DEADLINE_NS = 250_000_000
FRESH_NS = 33_333_333


def identity(records):
    build = read_json(RECEIPT)
    assert build["schema"] == "T758_DM_SOURCE_COMPILE_V1"
    _, driver = elf_file(build["driver"])
    dso, packaged = elf_file(build["pairedDsoFromHap"])
    cpp = record(build["pairedCpp"]["path"])
    assert (cpp["sha256"], cpp["bytes"]) == (build["pairedCpp"]["sha256"], build["pairedCpp"]["bytes"])
    hap = record(build["unsignedHap"]["path"])
    assert hap["sha256"] == build["unsignedHap"]["sha256"]
    with ZipFile(ROOT / hap["path"]) as archive:
        assert archive.read(NATIVE) == dso
    for role in ("boot", "source"):
        expected = build["roles"][role]
        source = (ROOT / expected["path"]).read_bytes()
        assert (sha(source), len(source)) == (expected["sha256"], expected["bytes"])
        assert dso.count(source + b"\0") == 1
    attempt, = [r for r in records if r.get("stage") == "ATTEMPT_IDENTITY"]
    assert attempt["driverMappedBuildId"] == driver["buildId"]
    assert (attempt["pid"], attempt["processStartTick"], attempt["run"], attempt["context"]) == (
        54691, "9752360", "97523765513750", "97523765513750")
    boot = {r["stage"]: r for r in records if r.get("schema") == "G9BootV1"}
    for r in boot.values():
        assert all(r[k] == attempt[k] for k in ("pid", "run", "owner", "context", "ticket"))
        assert r["tid"] == attempt["pid"] and r["mappedBuildId"] == packaged["buildId"]
        assert r["sourceSHA"] == build["roles"]["boot"]["sha256"]
    assert boot["FINISH_EXIT"]["fenceWaitRaw"] == 37146
    unmap = boot["UNMAP_EXIT"]
    assert tuple(unmap[k] for k in ("raw", "gl", "egl", "error", "seal", "workTailBytes")) == (
        1, 0, 12288, 0, 6559303, 8)
    enter, = [r for r in records if r.get("stage") == "CONFIGURE_ENTER"]
    assert (enter["variant"], enter["roleSetSHA"], enter["sourceSHA"], enter["cacheConfigured"]) == (
        "BOOT_SOURCE", build["roleSetSHA"], build["roles"]["source"]["sha256"], False)
    return {"receipt": record(RECEIPT), "attempt": attempt, "driver": driver, "packagedDso": packaged,
            "hap": hap, "pairedCpp": cpp, "targetWholeFileSha256Captured": False,
            "bootAudit": boot["BOOT_AUDIT_OK"], "bootUnmap": unmap, "sourceConfigureEntry": enter}


def spans(events, phase, edge="RETURNED"):
    selected = [r for r in events if r[0] == phase and r[1] in ("ENTER", edge)]
    assert len(selected) % 2 == 0
    result = []
    for enter, end in zip(selected[::2], selected[1::2]):
        assert (enter[1], end[1]) == ("ENTER", edge)
        duration = int(end[2]) - int(enter[2])
        assert duration >= 0
        result.append({"enter": enter, "return": end, "elapsedNs": duration})
    return result


def root_timeline(records, facts):
    root, end = records[-2:]
    assert root["stage"] == "SOURCE_ROOT" and end["schema"] == "T758_DM_SOURCE_TRACE_V1"
    attempt = facts["attempt"]
    assert (root["scope"], root["nonce"], root["ticket"]) == (end["scope"], end["nonce"], end["ticket"])
    assert root["scope"] == attempt["run"] and all(root[k] == attempt[k] for k in ("pid", "ticket", "owner", "context"))
    assert root["tid"] == root["pid"] and root["sourceSHA"] == facts["sourceConfigureEntry"]["sourceSHA"]
    assert (root["kind"], root["ordinal"], root["nonce"], root["plan"], root["result"], root["controlFault"]) == (1, 0, "1", "1", 6, 9)
    assert end["exitCode"] == root["result"]
    start, deadline = (int(root[k]) for k in ("rootBeginNs", "deadlineNs"))
    assert deadline - start == DEADLINE_NS
    assert (root["dispatches"], root["barriers"], root["copyBarriers"], end["submitted"], end["advancedCursor"]) == (12, 12, 3, 12, 12)
    assert all(row[:2] == [i, i] for i, row in enumerate(root["passes"]))
    assert all(not root[k] for k in ("mapReturned", "typedAccepted", "committed"))
    assert (end["admittedQ20"], end["outcome"], end["commit"], end["quanta"], end["queueAfter"]) == (None, None, 0, 0, 1)

    events = end["events"]
    assert (len(events), end["eventsDropped"], end["pulsesDropped"]) == (2048, 102821, 0)
    cold = {p: spans(events, p)[0] for p in ("SHADER_COMPILE", "PROGRAM_LINK", "SOURCE_REFLECT")}
    assert [r["elapsedNs"] for r in cold.values()] == [82_820_177_500, 234_473_359_375, 92_656]
    build_end = next(r for r in events if r[:2] == ["SOURCE_BUILD", "EXIT"])
    assert start - int(build_end[2]) == 2_252_344
    assert start - int(facts["bootAudit"]["ns"]) == 317_305_511_562
    copies = spans(events, "COPY")
    assert [r["enter"][3:] for r in copies] == [[1, 2, 591872], [7, 8, 12767232], [7, 9, 12767232]]
    assert [r["elapsedNs"] for r in copies] == [192_968, 85_551_094, 28_906]
    flush = spans(events, "SUBMIT_FLUSH")
    assert [r["elapsedNs"] for r in flush] == [1_563, 77_459_531]
    execute_flush, = spans(events, "EXEC_FLUSH")
    execute_fence, = spans(events, "EXEC_FENCE")
    assert int(execute_fence["return"][2]) - start == 164_158_438
    assert execute_flush["elapsedNs"] == 469
    polls = [r for r in events if r[:2] == ["EXEC_POLL", "ENTER"]]
    gaps = [int(b[2]) - int(a[2]) for a, b in zip(polls, polls[1:])]
    assert (len(polls), min(gaps), median(gaps), max(gaps)) == (233, 6718, 7031, 14844)
    assert int(polls[0][2]) - start == 164_162_031
    last, failed = end["lastGuardSuccess"], end["firstGuardFailure"]
    assert last[:2] == ["PUMP_ENTRY_GUARD", "PASS"]
    assert failed[:2] == ["PRE_EXEC_POLL_GUARD", "DEADLINE"]
    assert last[3:] == [12, 49, 97842142418125] and failed[3:] == [12, None, None]
    before, after = deadline - int(last[2]), int(failed[2]) - deadline
    assert (before, after, before + after) == (469, 1250, 1719)
    pulse_gap = max(p[2] for p in end["pulses"])
    assert len(end["pulses"]) == 49 and all(p[3] == 0 for p in end["pulses"])
    assert pulse_gap == 5_750_469 < FRESH_NS
    execution, retirement = end["executionFence"], end["retirementFence"]
    assert execution["created"] and not execution["signaled"] and execution["lastWaitRaw"] == 37147
    assert execution["polls"] == 13069 and deadline - int(execution["lastPollNs"]) == 2344
    assert retirement["reusedExecution"] and retirement["signaled"] and retirement["lastWaitRaw"] == 37146
    assert retirement["polls"] == 62 and retirement["handle"] == execution["handle"]
    assert int(retirement["lastPollNs"]) - deadline == 81_541_563 and end["retirementState"] == "DRAINED"
    return {"root": root, "preRootApi": cold, "buildQualifiedNs": build_end[2],
            "firstUse": spans(events, "SOURCE_FIRST_USE")[0], "copies": copies, "submitFlushes": flush,
            "executionFenceCreate": execute_fence, "executionFlush": execute_flush, "firstPoll": polls[0],
            "longCopyPlusFlushNs": copies[1]["elapsedNs"] + flush[1]["elapsedNs"],
            "pollPrefix": {"count": len(polls), "minGapNs": min(gaps), "medianGapNs": median(gaps),
                           "maxGapNs": max(gaps), "droppedEvents": end["eventsDropped"]},
            "pulse": {"count": len(end["pulses"]), "maxGapNs": pulse_gap, "ageAtLastGuardNs": int(last[2]) - last[5]},
            "cutoff": {"lastAllowed": last, "firstDenied": failed, "beforeNs": before, "afterNs": after, "widthNs": before + after},
            "execution": execution, "retirement": retirement, "drainBeginNs": end["drainBeginNs"],
            "drainEndNs": end["drainEndNs"], "observedFifoAfter": end["queueAfter"],
            "driverTerminalExitCode": end["exitCode"], "separatelyAttributedShellExit": None}


def main():
    records = rows(CAPTURE)
    assert len(records) == 27
    facts = identity(records)
    write("docs/architecture-T768-evidence-v1.json", {
        "schema": "T768_DM_SOURCE_ROOT_BUDGET_V1", "deviceUsed": False,
        "sourceRecords": [record(CAPTURE), record(WAIT)], "identity": facts,
        "rootTimeline": root_timeline(records, facts),
    })
    print("T768 verified T758 identity, T762 timing and separate fence retirement")


if __name__ == "__main__":
    main()
