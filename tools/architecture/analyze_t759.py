"""Attribute the T755 outcome offline from signed RX binaries, raw logs and T752 code."""
import json
from pathlib import Path
import re
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write


CAPTURE = "test/reports/t491/t755/"
CPP = "entry/src/main/cpp/gpu_resident/g9rx/"
PAGE = "entry/src/main/ets/pages/GpuResidentG9Rx.ets"
NS_PULSE = 33333333
NS_DEADLINE = 250000000


def doc(path):
    return json.loads((ROOT / path).read_bytes())


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def packaged_identity():
    compile_path = "test/reports/t752/compile.json"
    sign_path = "test/reports/t257/T755-rx.json"
    compile_receipt, sign = doc(compile_path), doc(sign_path)
    assert sign["verifyApp"] == "PASS"
    name = "libs/arm64-v8a/libgrs_g9rx.so"
    packaged = compile_receipt["packagedElf"]
    artifacts = {}
    for key in ("input", "signed"):
        path = Path(sign[key]["path"])
        artifacts[key] = record(path.relative_to(ROOT).as_posix())
        assert artifacts[key]["sha256"] == sign[key]["sha256"]
        with ZipFile(path) as archive:
            elf = archive.read(name)
        assert len(elf) == packaged["bytes"] and sha(elf) == packaged["sha256"]
        assert build_id(elf) == packaged["buildId"] == compile_receipt["linkedElf"]["buildId"]
    assert artifacts["input"]["sha256"] == compile_receipt["unsignedHap"]["sha256"]
    roles = {}
    for role in ("boot", "source"):
        expected = compile_receipt["roles"][role]
        fact = record(expected["path"])
        assert fact["sha256"] == expected["sha256"] and fact["bytes"] == expected["bytes"]
        assert elf.count((ROOT / expected["path"]).read_bytes() + b"\0") == 1
        roles[role] = fact
    manifest = compile_receipt["sourceManifest"]
    assert record(manifest["path"])["sha256"] == manifest["sha256"]
    boot_set = doc(manifest["path"])["boot"]["roleSetSHA"]
    assert roles["source"]["bytes"] == 41767 and roles["boot"]["bytes"] == 22318
    return {"compileReceipt": record(compile_path), "signReceipt": record(sign_path), "artifacts": artifacts,
            "mappedBuildId": packaged["buildId"], "elfSHA": packaged["sha256"], "roles": roles,
            "roleSetSHA": compile_receipt["roleSetSHA"], "bootRoleSetSHA": boot_set}


def text_nodes(tree):
    text = tree.get("attributes", {}).get("text")
    if text:
        yield text
    for branch in tree.get("children", []):
        yield from text_nodes(branch)


def timeline(identity):
    log_path = CAPTURE + "h755.txt"
    lines = (ROOT / log_path).read_text(encoding="utf-8-sig", errors="replace").splitlines()
    configure, boot, source = [], [], []
    for number, line in enumerate(lines, 1):
        prefix = re.match(r"^(\d\d-\d\d \d\d:\d\d:\d\d\.\d+)\s+(\d+)\s+(\d+)\s+", line)
        for tag, records in (("G9RxConfigure", configure), ("G9Boot", boot), ("G9RxSource", source)):
            marker = f"/{tag}: "
            if marker not in line or not prefix or prefix[2] != "31536":
                continue
            raw = line.partition(marker)[2]
            if tag == "G9Boot":
                fields = json.loads(raw)
            else:
                fields = dict(re.findall(r"(\w+)=([^ ]*)", raw))
                if tag == "G9RxSource":
                    fields["stage"] = raw.split()[0]
            records.append({"line": number, "wall": prefix[1], "logPid": int(prefix[2]), "logTid": int(prefix[3]), **fields})
    assert len({row["ticket"] for row in configure}) == 1 and configure[0]["ticket"] == "1"
    assert configure[0]["phase"] == "ENQUEUED" and configure[0]["logTid"] == 31536
    assert all(row["logTid"] == 31696 for row in configure[1:] + boot + source)
    assert all(row["mappedBuildId"] == identity["mappedBuildId"] for row in configure + boot)
    assert all(row["sourceSHA"] == identity["roles"]["source"]["sha256"] for row in configure)
    assert all((row["pid"], row["tid"], row["run"], row["owner"], row["context"], row["role"]) ==
               (31536, 31696, "1", "1", "1", "BOOT") and row["roleSetSHA"] == identity["bootRoleSetSHA"]
               and row["sourceSHA"] == identity["roles"]["boot"]["sha256"] for row in boot)
    by_stage = {row["stage"]: row for row in boot}
    assert by_stage["FINISH_EXIT"]["raw"] == 37146 and by_stage["MAP_ENTER"]["raw"] == 1166976
    assert tuple(by_stage["UNMAP_EXIT"][k] for k in ("raw", "gl", "egl", "error", "seal")) == (1, 0, 12288, 0, 0x641647)
    source_enter, reflected = source
    assert source_enter["line"] > by_stage["BOOT_AUDIT_OK"]["line"] and source_enter["stage"] == "SOURCE_BUILD_ENTER"
    assert (source_enter["bytes"], source_enter["sha"], source_enter["roles"]) == (
        "41767", identity["roles"]["source"]["sha256"], identity["roleSetSHA"])
    assert tuple(reflected[k] for k in ("stage", "program", "activeUniforms", "qualified")) == ("SOURCE_REFLECT_EXIT", "4", "40", "1")
    marks = {row["phase"]: row for row in configure if row["line"] > source_enter["line"]}
    assert all((row["run"], row["context"]) == ("1", "1") for row in marks.values())
    assert tuple(marks) == ("SHADER_COMPILE_ENTER", "SHADER_COMPILE_EXIT", "PROGRAM_LINK_ENTER", "PROGRAM_LINK_EXIT",
                            "CONFIGURE_RESEARCH_EXIT", "APPLIED")
    assert all(marks[name]["result"] == "0" for name in ("APPLIED", "CONFIGURE_RESEARCH_EXIT"))
    applied = int(marks["APPLIED"]["ns"])
    durations = {stage.lower() + "Ns": int(marks[stage + "_EXIT"]["ns"]) - int(marks[stage + "_ENTER"]["ns"])
                 for stage in ("SHADER_COMPILE", "PROGRAM_LINK")}
    assert list(durations.values()) == [85323972187, 235364243437]
    durations["configureToAppliedNs"] = applied - int(configure[0]["ns"])
    durations["bootToAppliedNs"] = applied - int(by_stage["BOOT_AUDIT_OK"]["ns"])
    assert durations["configureToAppliedNs"] == 321466825000
    original_ui = CAPTURE + "l4.json"
    assert "PUMP_FAILED code=5" in list(text_nodes(doc(original_ui)))
    assert not any(word in "\n".join(lines) for word in ("PUMP_FAILED", "FIRST_VALID_SWAP", "processStartTick="))
    return {"raw": record(log_path), "layoutTerminal": record(original_ui), "ticket": "1", "pid": 31536, "ownerTid": 31696,
            "bootAbridged": [by_stage[name] for name in ("FINISH_EXIT", "MAP_ENTER", "MAP_EXIT", "UNMAP_EXIT", "BOOT_AUDIT_OK")],
            "enqueued": configure[0], "sourceBuild": source_enter, "sourceReflect": reflected,
            "sourceConfigure": list(marks.values()), "durations": durations,
            "uiTerminal": {"phase": "PUMP_FAILED", "result": 5, "nativeNs": None, "pumpIndex": None,
                           "firstFailedGuard": None, "dispatchCount": None, "fenceReached": None}}


def check_code():
    files = (
        PAGE, CPP + "bridge.cpp", CPP + "controller.cpp", CPP + "transport.cpp", CPP + "progress_clock_v3.cpp",
        CPP + "rx_protocol.h", CPP + "research_config.h")
    page, bridge, controller, transport, clock, protocol, enums = map(read, files)
    code_names = re.search(r"enum class Result[^\{]*\{([^}]+)\}", enums)[1].split(",")
    assert [code_names[i].strip() for i in (5, 6)] == ["STALE", "DEADLINE"]
    tick = page.split("tick(frame: RxFrame", 1)[1].split("private cancelInput", 1)[0]
    assert tick.index("attempt.phase !== 'APPLIED'") < tick.index("native.g9RxSetActive") < tick.index("this.offerSource()") < tick.index("native.g9RxPulse")
    assert "source.phase + ' code=' + String(source.fault)" in tick
    assert "if (result == ControlResult::OK)\n      app.WakeSource();" in bridge
    assert bridge.index('ConfigureStage("APPLIED")') < bridge.index("controller_.BeginRoot(CheckedClockRead()")
    begin = controller.split("Result Controller::BeginRoot", 1)[1].split("Result Controller::PumpBounded", 1)[0]
    assert begin.index("ExpireLocked(now.ns)") < begin.index("SourceClaimGate") < begin.index("FreezeLocked") < begin.index("transport_.Begin(")
    pump_call = controller.split("Result Controller::PumpBounded", 1)[1].split("if (blocked)", 1)[0]
    assert pump_call.index("transport_.Pump()") < pump_call.index('PublishSourceLocked("PUMP_FAILED"') < pump_call.index("transport_.AcceptedProof()")
    assert "sourceReport_.fault = before.fault ? before.fault : fault" in controller
    guard = controller.split("bool Controller::Check(", 1)[1].split("Result Controller::ConfigureResearch", 1)[0]
    assert all(x in guard for x in ("sync.ownerPulseNs.load()", "now < pulse", "now - pulse > ProgressClockV3::GapNs",
                                   "sync.lifecycleInvalidation.load()", "p == self.active_", "CurrentAge(now, Pair(frozen.data(), 43))"))
    allowed = transport.split("bool Transport::Allowed() const", 1)[1].split("Result Transport::Denied", 1)[0]
    assert allowed.index("ReadNow(now)") < allowed.index("check_(checkUser_, permit_, now.ns)")
    assert "return now.ns > permit_.deadline ? Result::DEADLINE : Result::STALE;" in transport
    assert "else if (ready)\n    nextBaseline = true;" in clock and "now >= counters.receiveNs" in clock
    assert "now >= receipt && now - receipt <= 250000000" in protocol
    expiry = controller.split("void Controller::ExpireLocked", 1)[1].split("Result Controller::OfferSource", 1)[0]
    assert "if (protocol_.CurrentAge(now, Pair(input.data(), 43)))\n      break;\n    DropHeadLocked" in expiry
    target = controller.split("ControlResult Controller::Target", 1)[1].split("void Controller::DropHeadLocked", 1)[0]
    assert "ownerPulseNs.store(now.ns)" in target
    assert "!check_ || !check_(checkUser_, p, commitTime.ns)" in transport
    commit = controller.split("Result Controller::PumpBounded", 1)[1].split("Result Controller::Stop()", 1)[0]
    assert commit.index("if (!accepted)") < commit.index("if (!RecheckLocked") < commit.index("transport_.CommitAccepted(*accepted)")
    return {"files": [record(p) for p in files], "enum": {"STALE": 5, "DEADLINE": 6},
            "inferredFromUiAndSource": ["APPLIED", "SOURCE_AT_FIFO_HEAD", "BASELINE_AND_FRESH_AT_CLAIM", "ROOT_FROZEN",
                                       "TRANSPORT_BEGIN_SUCCEEDED", "PUMP_STALE_BEFORE_TYPED_PROCESSING"],
            "liveTransportStaleWindows": ["PUMP_ENTER", "DISPATCH_OR_COPY_GUARD", "FLUSH_EDGE", "FENCE_OR_PRE_MAP_GUARD"],
            "unknown": ["ACTUAL_FIRST_FALSE_PREDICATE", "FRAME_CALLBACK_NS", "PULSE_COUNT_AND_NS", "PUMP_ORDINAL",
                        "DISPATCH_CURSOR", "FENCE_OR_MAP_PROGRESS", "RETIREMENT_ORIGINALS"]}


def clock_cases():
    begin = 1_000_000_000
    deadline = begin + NS_DEADLINE

    def allowed(now, pulse):
        return begin <= now <= deadline and pulse <= now <= pulse + NS_PULSE

    # Illustrative ordering controls, with all non-clock guards true.
    cases = [
        ("old_now_new_pulse", begin + 20000000, begin + 20000001, False),
        ("same_interleaving_resampled", begin + 20000002, begin + 20000001, True),
        ("last_eligible_pulse_age", begin + NS_PULSE, begin, True),
        ("first_late_pulse_age", begin + NS_PULSE + 1, begin, False),
        ("root_exact_cutoff", deadline, deadline - 1, True),
        ("fresh_pulse_cannot_extend_root", deadline + 1, deadline, False),
    ]
    for _, now, pulse, expected in cases:
        assert allowed(now, pulse) is expected
    return {"origin": "OFFLINE_DETERMINISTIC_EXAMPLES", "beginNs": begin, "deadlineNs": deadline,
            "pulseWindowNs": NS_PULSE, "rootWindowNs": NS_DEADLINE,
            "cases": [{"case": name, "ownerNowNs": now, "pulseReceiveNs": pulse, "allowed": expected}
                      for name, now, pulse, expected in cases]}


if __name__ == "__main__":
    artifact = packaged_identity()
    write("docs/architecture-T759-evidence-v1.json", {
        "schema": "T759_RX_SOURCE_PULSE_ATTRIBUTION_V1", "deviceUsed": False, "artifact": artifact,
        "timeline": timeline(artifact), "sourcePath": check_code(), "clockExamples": clock_cases()})
    print("T759 checked signed BOOT/SOURCE, exact pre-claim durations, PUMP_FAILED path and clock counterexamples")
