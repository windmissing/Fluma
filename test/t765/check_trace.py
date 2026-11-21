"""Independent T759 timing and no-commit checks for the bounded RX source export."""
import argparse
import json
from pathlib import Path

PULSE_LIMIT = 33_333_333
ROOT_LIMIT = 250_000_000
COMPLETED = (0x911A, 0x911C)


def need(condition, message):
    if not condition:
        raise ValueError(message)


def temporal(now, pulse, receipt, begin, deadline, *, clock=True, has_pulse=True):
    if not clock:
        return "CLOCK"
    if now < begin:
        return "BEFORE_ROOT"
    if now > deadline:
        return "DEADLINE"
    if not has_pulse:
        return "MISSING_PAGE_PULSE"
    if not 0 <= now - pulse <= PULSE_LIMIT:
        return "PULSE_FROM_FUTURE" if now < pulse else "PULSE_EXPIRED"
    if not 0 <= now - receipt <= ROOT_LIMIT:
        return "COMMAND_FROM_FUTURE" if now < receipt else "COMMAND_EXPIRED"
    return "NONE"


def check_guard(sample, pulses, incomplete=False):
    now, pulse, receipt, begin, deadline = map(
        lambda k: int(sample[k]), ("nowNs", "pulseNs", "receiptNs", "beginNs", "deadlineNs"))
    reason = sample["reason"]
    origin_scope = sample["expectedScope"]
    live_scope = sample["actualScope"]
    need(len(origin_scope) == len(live_scope) == 9, "BAD_GUARD_SCOPE")
    if begin:
        need(deadline - begin == ROOT_LIMIT, "ROOT_DEADLINE_CHANGED")
    if reason == "SCOPE":
        need(origin_scope != live_scope, "FALSE_SCOPE_REJECT")
    if reason == "GENERATION":
        need(sample["clockGeneration"] != sample["actualClockGeneration"] or
             sample["lifecycleGeneration"] != sample["actualLifecycleGeneration"], "FALSE_GENERATION_REJECT")
    time_reason = temporal(now, pulse, receipt, begin, deadline,
                           clock=bool(sample["clockOk"]), has_pulse=bool(sample["hasPagePulse"]))
    if reason in {"NONE", "CLOCK", "BEFORE_ROOT", "DEADLINE", "MISSING_PAGE_PULSE",
                  "PULSE_FROM_FUTURE", "PULSE_EXPIRED", "COMMAND_FROM_FUTURE", "COMMAND_EXPIRED"}:
        need(reason == time_reason or (sample["phase"] == "PRE_CLAIM" and
             reason == "COMMAND_FROM_FUTURE" and now < receipt) or
             (sample["phase"] == "PRE_CLAIM" and reason == "COMMAND_EXPIRED" and
              now - receipt > ROOT_LIMIT), "FALSE_TIME_VERDICT")
    if sample["hasPagePulse"] and sample["clockOk"]:
        available = [p for p in pulses if p["scope"] == live_scope and int(p["publishNs"]) <= now]
        match = [p for p in available if int(p["sequence"]) == int(sample["pulseSeq"])]
        if not incomplete or match:
            need(bool(match) and int(match[-1]["receiptNs"]) == pulse, "PULSE_NOT_FROM_PAGE_PULSE")
        if available and (not incomplete or match):
            need(int(available[-1]["sequence"]) == int(sample["pulseSeq"]), "PULSE_NOT_LATEST")
    if reason == "DEADLINE":
        need(sample["code"] == 6, "DEADLINE_RAW_CODE")
    if reason == "PULSE_EXPIRED":
        need(sample["code"] == 5, "STALE_RAW_CODE")


def check(doc, compile_receipt=None):
    status = doc["status"]
    source, configure, trace = status["source"], status["configure"], doc["trace"]
    need(trace["schema"] == "RX_SOURCE_TRACE_V1" and source["variant"] == "BOOT_SOURCE", "WRONG_VARIANT")
    need(int(status["pid"]) > 0 and int(status["processStartTick"]) > 0 and int(configure["ticket"]) > 0,
         "MISSING_PROCESS_IDENTITY")
    need(bool(configure["mappedBuildId"]) and bool(source["roleSetSHA"]), "MISSING_LOAD_IDENTITY")
    if compile_receipt:
        need(configure["mappedBuildId"] == compile_receipt["packagedElf"]["buildId"] and
             source["sourceSHA"] == compile_receipt["roles"]["source"]["sha256"] and
             source["roleSetSHA"] == compile_receipt["roleSetSHA"], "WRONG_ELF_OR_ROLE")
    control, renders, pulse_log = trace["control"], trace["transport"], trace["pulses"]
    logs = (control, renders, pulse_log)
    dropped = any(log["dropped"] for log in logs)
    for log, limit in zip(logs, (160, 384, 96)):
        need(len(log["rows"]) <= limit and (not log["dropped"] or len(log["rows"]) == limit), "TRACE_CAP")

    accepted = []
    for pulse in pulse_log["rows"]:
        previous = accepted[-1] if accepted else None
        if pulse["result"] != 0:
            need(int(pulse["publishNs"]) == 0, "REJECTED_PULSE_PUBLISHED")
            continue
        receive, ui, publication = (int(pulse[k]) for k in ("receiptNs", "uiNs", "publishNs"))
        need(0 < receive <= publication, "PULSE_PUBLICATION_ORDER")
        if previous:
            native_gap = receive - int(previous["receiptNs"])
            ui_gap = ui - int(previous["uiNs"])
            need(pulse["scope"] == previous["scope"] and
                 int(pulse["sequence"]) > int(previous["sequence"]) and
                 native_gap == int(pulse["nativeGapNs"]) and ui_gap == int(pulse["uiGapNs"]) and
                 0 <= native_gap <= PULSE_LIMIT and 0 < ui_gap <= PULSE_LIMIT, "ACCEPTED_PULSE_GAP_OR_SCOPE")
        accepted.append(pulse)

    for sample in (trace["firstReject"], trace["recentPass"]):
        if sample:
            check_guard(sample, accepted, dropped)
    first = trace["firstReject"]
    need(not first or first["reason"] != "NONE", "FIRST_REJECT_IS_PASS")
    need(not trace["recentPass"] or trace["recentPass"]["reason"] == "NONE", "RECENT_PASS_IS_REJECT")
    order = {phase: [int(row[1]) for row in control["rows"] if row[0] == phase]
             for phase in ("APPLIED", "EXTENT_QUALIFIED", "SET_ACTIVE", "QUEUE", "FIRST_PULSE", "CLAIM", "FREEZE", "COMMIT")}
    if order["CLAIM"]:
        names = tuple(order)[:-1]
        need(all(order[name] for name in names), "MISSING_POST_COMPILE_ROOT_EDGE")
        times = [order[name][0] for name in names]
        need(times == sorted(times) and times[0] > 0, "PRE_APPLIED_OR_MISORDERED_ROOT")
        need(len(order["CLAIM"]) == 1, "SECOND_SOURCE_CLAIM")
    edges = renders["rows"] + ([renders["last"]] if renders["last"] else [])
    retired = any(row[0] == "RETIRE_COMPLETE" for row in edges)
    if retired:
        # A mapped flight can retire without another wait. Otherwise require
        # the actual retire wait result, never the EXEC fence result alone.
        witnessed = any(row[0] == "READBACK_COMPLETE" or
                        (row[0] == "RETIRE_FENCE_WAIT_EXIT" and int(row[3]) in COMPLETED)
                        for row in edges)
        need(witnessed or renders["dropped"], "RETIRE_WITHOUT_SIGNAL_OR_MAPPED_FLIGHT")
        retired = witnessed
    committed = source["committed"] or bool(order["COMMIT"])
    if first:
        need(not committed, "COMMIT_AFTER_FIRST_REJECT")
        if first["reason"] == "QUOTA_ALREADY_PENDING":
            need(first["pending"] > 0 and not order["CLAIM"], "CLAIM_AFTER_PENDING")
        if first["phase"] == "PRE_MAP" and first["reason"] == "DEADLINE":
            need(not any(row[0] == "MAP_ENTER" for row in renders["rows"]), "MAP_AFTER_LATE_FENCE")
        return {"observation": "REJECT_RETIRED" if retired else "REJECT_OBSERVED",
                "reason": first["reason"], "phase": first["phase"], "truncated": dropped}
    if committed:
        need(not control["dropped"] and not renders["dropped"] and accepted and
             trace["recentPass"] and any(int(p["sequence"]) == int(trace["recentPass"]["pulseSeq"])
                                        for p in accepted) and
             source["committed"] and len(order["COMMIT"]) == 1 and
             source["typedAccepted"] and source["planRetired"] and source["pendingAtClaim"] == 0 and
             source["activeRemaining"] == 0 and source["queueBefore"] == 1 and source["queueAfter"] == 0,
             "INCOMPLETE_OR_UNTYPED_COMMIT")
        begin, fence, commit, deadline = (int(source[k]) for k in ("beginNs", "fenceNs", "commitNs", "deadlineNs"))
        need(deadline - begin == ROOT_LIMIT and 0 < begin <= fence <= commit <= deadline and
             commit == order["COMMIT"][0] and begin == order["CLAIM"][0],
             "LATE_OR_MISMATCHED_COMMIT")
        need(source["fenceWaitRaw"] in COMPLETED and source["workReadBytes"] == 1024 and
             source["dispatches"] == source["dispatchBarriers"] == 12 and source["copyBarriers"] == 3 and
             source["gl"] == 0 and source["egl"] == 12288 and source["outcome"] == 1 and source["admittedQ20"] == 209715,
             "INCOMPLETE_SOURCE_OR_NUMERIC_REJECT")
        need(any(row[0] == "EXEC_FENCE_WAIT_EXIT" and int(row[3]) == source["fenceWaitRaw"]
                 and int(row[1]) <= fence for row in renders["rows"]), "EXEC_FENCE_NOT_OBSERVED")
        return {"observation": "SOURCE_COMMIT_RECEIPT", "truncated": dropped}
    return {"observation": "INCOMPLETE", "truncated": dropped}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("--compile-receipt", type=Path)
    args = parser.parse_args()
    bound = json.loads(args.compile_receipt.read_bytes()) if args.compile_receipt else None
    print(json.dumps(check(json.loads(args.trace.read_bytes()), bound), indent=2))
