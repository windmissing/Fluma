"""Link T766's two UI terminal records to the evidence actually captured."""

import json
from pathlib import Path
import re
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write
from analyze_t759 import text_nodes


COMPILE = "test/reports/t763/compile.json"
SIGN = "test/reports/t257/T766-rx.json"
CAPTURE = "test/reports/t491/t766/"
CPP = "entry/src/main/cpp/gpu_resident/g9rx/"
PAGE = "entry/src/main/ets/pages/GpuResidentG9Rx.ets"


def doc(path):
    return json.loads((ROOT / path).read_bytes())


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig", errors="replace")


def packaged_identity():
    compile_receipt, sign = doc(COMPILE), doc(SIGN)
    assert sign["verifyApp"] == "PASS" and sign["input"]["sha256"] == compile_receipt["unsignedHap"]["sha256"]
    facts, files = {}, []
    for key in ("input", "signed"):
        path = Path(sign[key]["path"])
        facts[key] = record(path.relative_to(ROOT).as_posix())
        assert facts[key]["sha256"] == sign[key]["sha256"]
        with ZipFile(path) as archive:
            files.append(archive.read("libs/arm64-v8a/libgrs_g9rx.so"))
    elf = files[1]
    assert elf == files[0]
    packaged = compile_receipt["packagedElf"]
    assert (build_id(elf), len(elf), sha(elf)) == (packaged["buildId"], packaged["bytes"], packaged["sha256"])
    manifest = compile_receipt["roleManifest"]
    assert record(manifest["path"])["sha256"] == manifest["sha256"]
    roles = {}
    for role, size in (("boot", 22318), ("source", 41767)):
        expected = compile_receipt["roles"][role]
        source = (ROOT / expected["path"]).read_bytes()
        assert (sha(source), len(source)) == (expected["sha256"], expected["bytes"])
        assert len(source) == size and elf.count(source + b"\0") == 1
        roles[role] = {"bytes": size, "sha256": sha(source)}
    return {"receipts": [record(COMPILE), record(SIGN)], "hap": facts, "roles": roles,
            "mappedBuildId": packaged["buildId"], "packagedElfSHA": packaged["sha256"],
            "roleSetSHA": compile_receipt["roleSetSHA"], "bootRoleSetSHA": doc(manifest["path"])["boot"]["roleSetSHA"]}


def capture(identity):
    chain_path, boot_path = (CAPTURE + name for name in ("v6/chain.txt", "v6/boot.txt"))
    chain = read(chain_path).splitlines()
    configure, source = [], []
    for line in chain:
        prefix = re.match(r"^(\d\d-\d\d \d\d:\d\d:\d\d\.\d+)\s+(\d+)\s+(\d+)\s+.*?/(G9RxConfigure|G9RxSource): (.+)$", line)
        assert prefix and int(prefix[2]) == 927
        fields = dict(re.findall(r"(\w+)=([^ ]*)", prefix[5]))
        fields.update({"wall": prefix[1], "tid": int(prefix[3])})
        if prefix[4] == "G9RxConfigure":
            configure.append(fields)
        else:
            fields["phase"] = prefix[5].split()[0]
            source.append(fields)
    assert len(configure) == 11 and len(source) == 2 and all(r["ticket"] == "2" for r in configure)
    assert all(r["mappedBuildId"] == identity["mappedBuildId"] and
               r["sourceSHA"] == identity["roles"]["source"]["sha256"] for r in configure)
    assert configure[0]["tid"] == 927
    assert all(r["tid"] == 1129 for r in configure[1:] + source)
    assert all((r["run"], r["context"]) == ("2", "1") for r in configure[2:])
    events = {r["phase"]: r for r in configure}
    assert events["APPLIED"]["result"] == events["CONFIGURE_RESEARCH_EXIT"]["result"] == "0"
    assert (source[0]["phase"], source[0]["bytes"], source[0]["sha"], source[0]["roles"]) == (
        "SOURCE_BUILD_ENTER", "41767", identity["roles"]["source"]["sha256"], identity["roleSetSHA"])
    assert tuple(source[-1][k] for k in ("phase", "program", "activeUniforms", "qualified")) == (
        "SOURCE_REFLECT_EXIT", "4", "40", "1")
    timing = {stage: int(events[stage + "_EXIT"]["ns"]) - int(events[stage + "_ENTER"]["ns"])
              for stage in ("SHADER_COMPILE", "PROGRAM_LINK")}
    timing["enqueueToApplied"] = int(events["APPLIED"]["ns"]) - int(events["ENQUEUED"]["ns"])
    assert tuple(timing.values()) == (82_302_926_875, 98_107_827_969, 180_601_356_406)

    raw_boot = [json.loads(line.partition("/G9Boot: ")[2]) for line in read(boot_path).splitlines()]
    assert raw_boot == doc(CAPTURE + "v6/g9boot-t766.jsonl") and len(raw_boot) == 12
    assert all((r["pid"], r["tid"], r["run"], r["owner"], r["context"]) == (927, 1129, "2", "1", "1")
               and r["mappedBuildId"] == identity["mappedBuildId"]
               and r["sourceSHA"] == identity["roles"]["boot"]["sha256"]
               and r["roleSetSHA"] == identity["bootRoleSetSHA"] for r in raw_boot)
    boot = {r["stage"]: r for r in raw_boot}
    assert boot["FINISH_EXIT"]["raw"] == 37146 and boot["MAP_ENTER"]["raw"] == 1166976
    assert tuple(boot["UNMAP_EXIT"][k] for k in ("raw", "gl", "egl", "error", "seal")) == (1, 0, 12288, 0, 6559303)
    assert int(boot["BOOT_AUDIT_OK"]["ns"]) < int(events["SHADER_COMPILE_ENTER"]["ns"])
    layouts = {name: list(text_nodes(doc(CAPTURE + name))) for name in ("l2.json", "l4.json", "l5.json")}
    assert "RX #1 PROGRAM_LINK_ENTER" in layouts["l2.json"]
    assert "PUMP_FAILED code=5" in layouts["l4.json"] and "PUMP_FAILED code=6" in layouts["l5.json"]
    for name in ("h766.txt", "h766b.txt", "v6/pump.txt"):
        assert not any(word in read(CAPTURE + name) for word in ("G9RxConfigure:", "G9RxSource:", "PUMP_FAILED", "RX_SOURCE_TRACE_V1"))
    assert not list((ROOT / CAPTURE).rglob("g9rx-source-*.json"))
    return {"captureFiles": [record(p) for p in (chain_path, boot_path, *(CAPTURE + k for k in layouts))],
            "firstAttempt": {"layoutTicket1Config": "PROGRAM_LINK_ENTER", "layoutTerminal": 5, "nativeScope": None},
            "secondAttempt": {"pid": 927, "ownerTid": 1129, "processStartTick": None,
                              "ticket": "2", "runOwnerContext": ["2", "1", "1"], "layoutTerminal": 6,
                              "sourceConfigure": configure, "sourceProgram": source[-1], "durationsNs": timing,
                              "boot": [boot[k] for k in ("FINISH_EXIT", "MAP_ENTER", "MAP_EXIT", "UNMAP_EXIT", "BOOT_AUDIT_OK")]},
            "perAttemptExportArchived": False}


def code_boundary():
    transport, controller, bridge, guard, timeline = (
        read(CPP + name) for name in ("transport.cpp", "controller.cpp", "bridge.cpp", "source_guard.h", "source_timeline.h"))
    page = read(PAGE)
    pump = transport.split("Result Transport::Pump()", 1)[1].split("bool Transport::CommitAccepted", 1)[0]
    assert pump.index("if (!busy_)") < pump.index('Allowed("PUMP_ENTRY")') < pump.index("glClientWaitSync") < pump.index('Allowed("PRE_MAP")')
    begin = transport.split("Result Transport::Begin(", 1)[1].split("void Transport::Bind(", 1)[0]
    assert begin.index('Allowed("BEGIN_GUARD")') < begin.index("glBufferSubData") < begin.index("busy_ = true")
    publish = controller.split("Result Controller::PumpBounded", 1)[1].split("auto accepted = transport_.AcceptedProof();", 1)[0]
    assert publish.index("transport_.Pump()") < publish.index('PublishSourceLocked("PUMP_FAILED"')
    assert "return sourceDenied_ == Result::OK ? Result::STALE : sourceDenied_;" in transport
    assert "case SourceReject::Deadline: return Result::DEADLINE;" in timeline
    assert guard.index("t.now > t.deadline") < guard.index("t.now - t.pulse") < guard.index("t.now - t.receipt")
    assert "wake_.wait_for(lock, std::chrono::milliseconds(2)" in bridge and "sourceWake_ ||" in bridge
    native_tick = page.split("tick(frame: RxFrame", 1)[1].split("private cancelInput", 1)[0]
    assert native_tick.index("attempt.phase !== 'APPLIED'") < native_tick.index("native.g9RxSetActive") < native_tick.index("this.offerSource()") < native_tick.index("native.g9RxPulse")
    archive = page.split("private async archive()", 1)[1].split("private frame", 1)[0]
    assert archive.index("native.g9RxSourceDiagnostics()") < archive.index("await fs.fsync") < archive.index("this.cold.collect(")
    return {"inferredForBothIfPaired": ["FIRST_ACCEPTED_PAGE_PULSE", "CLAIM", "FREEZE", "BEGIN_OK", "PUMP_FAILED"],
            "deadlineGuard": "nowNs > deadlineNs", "observedSecondTerminalCode": 6,
            "candidateGuardSites": ["PUMP_ENTRY", "DISPATCH_ENTRY", "PRE_COPY", "POST_COPY", "PRE_DISPATCH",
                                    "POST_DISPATCH", "PRE_FLUSH", "PRE_FENCE_CREATE", "PRE_FENCE_WAIT", "PRE_MAP"],
            "missingDirectEvidence": ["GUARD_SITE_ORIGINAL", "ROOT_CLOCK_VALUES", "PULSE_AND_RECEIPT_AGES",
                                      "CURSOR_AND_SUBMITTED", "EXECUTION_AND_RETIREMENT_FENCE", "FROZEN_INPUT_WORDS"]}


def main():
    identity = packaged_identity()
    write("docs/architecture-T772-evidence-v1.json", {
        "schema": "T772_RX_SOURCE_DEADLINE_ATTRIBUTION_V1", "deviceUsed": False,
        "artifact": identity, "t766Capture": capture(identity), "t763Boundary": code_boundary(),
    })
    print("T772 verified signed T763 bytes, two T766 terminals, ticket 2 BOOT and deadline semantics")


if __name__ == "__main__":
    main()
