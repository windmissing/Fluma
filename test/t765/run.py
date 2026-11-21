"""Run the T765 fixtures against the revised pure guard, original clock and trace reader."""
import copy
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

from check_trace import check, need, temporal

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
NATIVE = ROOT / "entry/src/main/cpp/gpu_resident/g9rx"
BUILD = ROOT / ".codex/local-test/t765-host"
BASE = 1_000_000_000
LIMIT = BASE + 250_000_000
SCOPE = [str(i) for i in range(1, 10)]


def file(path):
    raw = path.read_bytes()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": len(raw),
            "sha256": hashlib.sha256(raw).hexdigest()}


def host_cases():
    compiler = shutil.which("cl.exe")
    need(compiler, "USE_VS_X64_DEVELOPER_SHELL")
    BUILD.mkdir(parents=True, exist_ok=True)
    tool = BUILD / "pulse_fixture.exe"
    command = [compiler, "/nologo", "/EHsc", "/std:c++17", "/I" + str(NATIVE),
               str(HERE / "pulse_fixture.cpp"), str(NATIVE / "progress_clock_v3.cpp"),
               "/Fe:" + str(tool)]
    subprocess.run(command, cwd=BUILD, check=True)
    native = subprocess.run([str(tool)], check=True, text=True, capture_output=True, timeout=10)
    cases = [
        ("old_now_new_pulse", BASE+20_000_000, BASE+20_000_001, BASE, BASE, LIMIT, "PULSE_FROM_FUTURE"),
        ("locked_now_new_pulse", BASE+20_000_002, BASE+20_000_001, BASE, BASE, LIMIT, "NONE"),
        ("fresh_edge", BASE+33_333_333, BASE, BASE, BASE, LIMIT, "NONE"),
        ("fresh_plus_1ns", BASE+33_333_334, BASE, BASE, BASE, LIMIT, "PULSE_EXPIRED"),
        ("deadline_edge", LIMIT, LIMIT-1, BASE, BASE, LIMIT, "NONE"),
        ("fresh_pulse_after_deadline", LIMIT+1, LIMIT, BASE, BASE, LIMIT, "DEADLINE"),
        ("old_owner_new_fifo", BASE, BASE, BASE+1, BASE, LIMIT, "COMMAND_FROM_FUTURE"),
        ("locked_resample_new_fifo", BASE+2, BASE, BASE+1, BASE, LIMIT, "NONE"),
        ("command_age_plus_1ns", LIMIT, LIMIT, BASE-1, BASE, LIMIT, "COMMAND_EXPIRED"),
        ("late_fence_is_deadline", LIMIT+1, LIMIT, BASE, BASE, LIMIT, "DEADLINE"),
    ]
    payload = "".join(f"{name} 1 1 {now} {pulse} {receipt} {begin} {end}\n"
                      for name, now, pulse, receipt, begin, end, _ in cases)
    timed = subprocess.run([str(tool), "guard"], check=True, input=payload, text=True,
                           capture_output=True, timeout=10)
    actual = dict(row.split() for row in timed.stdout.splitlines())
    for name, now, pulse, receipt, begin, end, expected in cases:
        need(temporal(now, pulse, receipt, begin, end) == actual[name] == expected, name)
    clock_cases = dict(row.split() for row in native.stdout.splitlines())
    need(len(clock_cases) == 10 and set(clock_cases.values()) == {"PASS"}, "NATIVE_CASE_COUNT")
    return {"clockAndInterleaving": clock_cases, "deadlineAndFreshness": actual}


def body(data, start):
    at = data.index(start)
    return data[at:data.index("\n}", at) + 2]


def source_wiring():
    controller = (NATIVE / "controller.cpp").read_text(encoding="utf8")
    transport = (NATIVE / "transport.cpp").read_text(encoding="utf8")
    page = (ROOT / "entry/src/main/ets/pages/GpuResidentG9Rx.ets").read_text(encoding="utf8")
    bridge = (NATIVE / "bridge.cpp").read_text(encoding="utf8")
    sample = body(controller, "SourceGuard Controller::SampleSourceLocked")
    callback = body(controller, "SourceGuard Controller::Check")
    begin = body(controller, "Result Controller::BeginRoot")
    pump = body(controller, "Result Controller::PumpBounded")
    locked = body(transport, "bool Transport::CommitSourceLocked")
    saver = body(transport, "void Transport::RecordSourceGuard")
    pulse = body(controller, "ControlResult Controller::Pulse")
    target = body(controller, "ControlResult Controller::Target")
    checks = {
        "snapshot_fields_then_clock": sample.index("clock_.counters.receiveNs") < sample.index("CheckedClockRead()"),
        "unlocked_callback_takes_protocol_lock": callback.index("lock(self.protocol_") < callback.index("SampleSourceLocked("),
        "begin_after_scene_and_under_lock": begin.index("PollSceneFence()") < begin.index("lock(protocol_") <
            begin.index("SampleSourceLocked(") < begin.index("ExpireLocked(") < begin.index("FreezeLocked("),
        "commit_samples_under_existing_lock": pump.rindex("lock(protocol_", 0, pump.index("COMMIT_PREFLIGHT")) <
            pump.index("auto commitGuard = SampleSourceLocked(") < pump.index("transport_.CommitSourceLocked(") <
            pump.index("clock_ = nextClock;", pump.index("transport_.CommitSourceLocked(")),
        "commit_entry_does_not_relock_or_invoke_egl": all(token not in locked for token in
            ("lock_guard", "unique_lock", "check_(", "Current()", "CheckedClockRead", "glClientWaitSync")),
        "commit_validates_typed_proof_before_swap": locked.index("proof.ProducedBy(this)") <
            locked.index("guard.reason != SourceReject::None") < locked.index("std::swap("),
        "first_reject_is_write_once": "else if (firstSourceReject_.reason == SourceReject::None)" in saver,
        "real_pulse_is_clock_authority": "clock_.PulseLocked(pulse, now.ns)" in pulse and
            "clock_.counters.receiveNs" in sample and
            target.index("#else", target.index('EdgeLocked("TARGET"')) <
            target.index("ownerPulseNs.store") < target.index("#endif", target.index("ownerPulseNs.store")),
        "page_defers_first_pulse_until_original_source_offer": page.index("native.g9RxSetActive(true, this.session)") <
            page.index("const accepted: string = this.offerSource()") < page.index("new Uint32Array(22)") <
            page.index("native.g9RxPulse(pulse)") and "app.WakeSource(Pair(w.words.data(), 18), receipt.ns)" in bridge,
        "pre_map_guard_follows_actual_wait": body(transport, "Result Transport::Pump").index("glClientWaitSync") <
            body(transport, "Result Transport::Pump").index('Allowed("PRE_MAP")') <
            body(transport, "Result Transport::Pump").index("glMapBufferRange"),
    }
    for name, passed in checks.items():
        need(passed, name)
    return dict.fromkeys(checks, "PASS")


def row(stage, ns, a=0, b=0):
    return [stage, str(ns), str(a), str(b), 0, 0]


def pulse(seq, received, ui, gap=0):
    return {"scope": SCOPE, "sequence": str(seq), "uiNs": str(ui), "receiptNs": str(received),
            "publishNs": str(received + 1), "nativeGapNs": str(gap), "uiGapNs": str(gap), "result": 0}


def guard(reason, phase, now, received, seq, pending=0, begin=BASE, deadline=LIMIT):
    return {"phase": phase, "reason": reason, "nowNs": str(now), "pulseNs": str(received),
            "receiptNs": str(BASE-2), "pulseSeq": str(seq), "beginNs": str(begin), "deadlineNs": str(deadline),
            "clockOk": True, "hasPagePulse": True, "pending": pending,
            "expectedScope": SCOPE, "actualScope": SCOPE, "code": 6 if reason == "DEADLINE" else 5}


def log(rows):
    return {"rows": rows, "last": rows[-1] if rows else None, "dropped": 0}


def scenario(committed=False):
    control = [row("APPLIED", BASE-12), row("EXTENT_QUALIFIED", BASE-9),
               row("SET_ACTIVE", BASE-8), row("QUEUE", BASE-4), row("FIRST_PULSE", BASE-1),
               row("CLAIM", BASE), row("FREEZE", BASE+1)]
    if committed:
        control.append(row("COMMIT", BASE+9))
    src = {"variant": "BOOT_SOURCE", "roleSetSHA": "synthetic-role", "committed": committed,
           "typedAccepted": committed, "planRetired": committed, "pendingAtClaim": 0, "activeRemaining": 0,
           "queueBefore": 1, "queueAfter": 0 if committed else 1,
           "beginNs": str(BASE), "fenceNs": str(BASE+8), "commitNs": str(BASE+9), "deadlineNs": str(LIMIT),
           "workReadBytes": 1024, "fenceWaitRaw": 37146, "dispatches": 12, "dispatchBarriers": 12,
           "copyBarriers": 3, "gl": 0, "egl": 12288, "outcome": 1, "admittedQ20": 209715}
    return {"status": {"pid": 1, "processStartTick": "1", "configure": {"ticket": "1", "mappedBuildId": "synthetic"},
                       "source": src},
            "trace": {"schema": "RX_SOURCE_TRACE_V1", "firstReject": None,
                      "recentPass": guard("NONE", "COMMIT_LOCKED", BASE+9, BASE-2, 1) if committed else None,
                      "control": log(control), "pulses": log([pulse(1, BASE-2, BASE-2)]),
                      "transport": log([row("EXEC_FENCE_WAIT_EXIT", BASE+7, 74, 37146)] if committed else [])}}


def synthetic():
    positive = scenario(True)
    stopped = scenario()
    stopped["trace"]["firstReject"] = guard("OWNER_INACTIVE", "PUMP_ENTRY", BASE+50, BASE-2, 1)
    scope = copy.deepcopy(stopped)
    scope["trace"]["firstReject"].update(reason="SCOPE", expectedScope=SCOPE[:-1]+["99"])
    typed = copy.deepcopy(stopped)
    typed["trace"]["firstReject"].update(reason="TYPED_PROOF", phase="TYPED_PROOF", code=3)
    pending = scenario()
    pending["trace"]["control"] = log(pending["trace"]["control"]["rows"][:5])
    pending["trace"]["pulses"] = log([pulse(1, BASE-2, BASE-2), pulse(2, BASE+8_999_998, BASE+8_999_998, 9_000_000)])
    pending["trace"]["firstReject"] = guard("QUOTA_ALREADY_PENDING", "PRE_CLAIM",
        BASE+9_000_000, BASE+8_999_998, 2, pending=1, begin=0, deadline=2**64-1)
    late = scenario()
    late["trace"]["pulses"] = log([pulse(1, BASE-2, BASE-2)] + [
        pulse(i+2, BASE-2+(i+1)*30_000_000, BASE-2+(i+1)*30_000_000, 30_000_000)
        for i in range(8)])
    late["trace"]["firstReject"] = guard("DEADLINE", "PRE_MAP", LIMIT+1, BASE+239_999_998, 9)
    late["trace"]["transport"] = log([row("EXEC_FENCE_WAIT_EXIT", LIMIT+1, 74, 37146),
                                      row("RETIRE_FENCE_WAIT_ENTER", LIMIT+3, 74, 0),
                                      row("RETIRE_FENCE_WAIT_EXIT", LIMIT+3, 74, 37146),
                                      row("RETIRE_COMPLETE", LIMIT+4)])
    target = scenario()
    target["trace"]["control"]["rows"].append(row("TARGET", BASE+33_333_331))
    target["trace"]["firstReject"] = guard("PULSE_EXPIRED", "PUMP_ENTRY",
                                          BASE+33_333_332, BASE-2, 1)
    accepted = {
        "positive": (positive, "SOURCE_COMMIT_RECEIPT"),
        "scope": (scope, "REJECT_OBSERVED"),
        "stop": (stopped, "REJECT_OBSERVED"),
        "typed_reject": (typed, "REJECT_OBSERVED"),
        "pending": (pending, "REJECT_OBSERVED"),
        "late_fence_retired": (late, "REJECT_RETIRED"),
        "target_cannot_extend": (target, "REJECT_OBSERVED"),
        "no_first_reject_originals": (scenario(), "INCOMPLETE"),
    }
    results = {}
    for name, (fixture, expected) in accepted.items():
        results[name] = check(fixture)
        need(results[name]["observation"] == expected, name)
    late_commit = copy.deepcopy(late)
    late_commit["status"]["source"]["committed"] = True
    late_map = copy.deepcopy(late)
    late_map["trace"]["transport"]["rows"].append(row("MAP_ENTER", LIMIT+2, 6, 1171072))
    fake_target = copy.deepcopy(target)
    fake_target["trace"]["firstReject"]["pulseNs"] = str(BASE+33_333_331)
    fake_target["trace"]["firstReject"].update(reason="NONE", code=0)
    early = copy.deepcopy(positive)
    early["trace"]["control"]["rows"][0][1] = str(BASE+3)
    retire_without_wait = copy.deepcopy(late)
    retire_without_wait["trace"]["transport"]["rows"] = [
        item for item in retire_without_wait["trace"]["transport"]["rows"] if item[0] != "RETIRE_FENCE_WAIT_EXIT"]
    for name, fixture, error in [
        ("late_commit_rejected", late_commit, "COMMIT_AFTER_FIRST_REJECT"),
        ("late_map_rejected", late_map, "MAP_AFTER_LATE_FENCE"),
        ("forged_target_receipt_rejected", fake_target, "PULSE_NOT_FROM_PAGE_PULSE"),
        ("claim_before_applied_rejected", early, "PRE_APPLIED_OR_MISORDERED_ROOT"),
        ("retire_without_wait_rejected", retire_without_wait, "RETIRE_WITHOUT_SIGNAL_OR_MAPPED_FLIGHT"),
    ]:
        try:
            check(fixture)
        except ValueError as exc:
            need(str(exc) == error, f"{name}: {exc}")
            results[name] = {"rejected": str(exc)}
        else:
            raise ValueError(name)
    return results


def historical():
    log_path = ROOT / "test/reports/t491/t755/h755.txt"
    ui_path = ROOT / "test/reports/t491/t755/l4.json"
    log_lines = log_path.read_text(encoding="utf8", errors="replace").splitlines()
    lines = [line for line in log_lines if re.search(r"\b31536\s+31696\b", line)]
    build_start = next(i for i, line in enumerate(lines) if "G9RxSource: SOURCE_BUILD_ENTER " in line)
    stage = {}
    for line in lines[build_start:]:
        found = re.search(r"G9RxConfigure: ticket=1 ns=(\d+) phase=(\w+)\b", line)
        if found:
            stage[found[2]] = int(found[1])
    edges = [stage[k] for k in ("SHADER_COMPILE_ENTER", "SHADER_COMPILE_EXIT",
            "PROGRAM_LINK_ENTER", "PROGRAM_LINK_EXIT", "CONFIGURE_RESEARCH_EXIT", "APPLIED")]
    need(edges == sorted(edges) and "PUMP_FAILED code=5" in ui_path.read_text(encoding="utf8"),
         "T755_RAW_BOUNDARIES")
    return {"hilog": file(log_path), "ui": file(ui_path), "appliedNs": str(edges[-1]),
            "terminal": "PUMP_FAILED code=5", "firstRejectedPredicate": "UNKNOWN",
            "rootCursor": "NOT_OBSERVED"}


def main():
    receipt_path = ROOT / "test/reports/t763/compile.json"
    receipt = json.loads(receipt_path.read_bytes())
    for key in ("packagedElf", "unsignedHap", "roleManifest"):
        item = receipt[key]
        observed = file(ROOT / item["path"])
        need(all(item[field] == value for field, value in observed.items()), "T763_BYTE_IDENTITY_" + key)
    result = {"schema": "T765_RX_SOURCE_OFFLINE_V1", "deviceUsed": False,
              "signed": False, "deviceRuntime": "NOT_RUN", "t763": file(receipt_path),
              "t763ElfBuildId": receipt["packagedElf"]["buildId"], "history": historical(),
              "nativeFixtures": host_cases(), "productionWiringStatic": source_wiring(),
              "syntheticTraceCases": synthetic()}
    out = ROOT / "test/reports/t765/offline.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2)+"\n", encoding="utf8", newline="\n")
    print("T765: native clock and timing, static wiring, synthetic trace and historical originals verified")


if __name__ == "__main__":
    main()
