"""Read saved evidence and inspect binary files for T684/T685/T686/T688."""
from collections import Counter
from pathlib import Path
import base64
import json
import re
import struct
import subprocess

from analyze_t660_t662 import ROOT, artifact, build_id, record, sha, write


def read(path):
    return json.loads((ROOT / path).read_bytes())


def rows(path):
    return [json.loads(line) for line in (ROOT / path).read_bytes().splitlines()]


def inspect_elf(receipt):
    doc = read(receipt)
    target = Path(doc["executable"]["path"])
    data = target.read_bytes()
    assert data[:6] == b"\x7fELF\x02\x01" and struct.unpack_from("<H", data, 18)[0] == 183
    assert sha(data) == doc["executable"]["sha256"] and build_id(data) == doc["buildId"]
    tool = Path(doc["inspectionTool"]["path"])
    assert sha(tool.read_bytes()) == doc["inspectionTool"]["sha256"]
    dynamic = subprocess.check_output([str(tool), "-d", str(target)], text=True)
    needed = lambda text: re.findall(r"\(NEEDED\).*?\[([^]]+)]", text)
    assert needed(dynamic) == needed(doc["dynamic"])
    return {"receipt": record(receipt), "elf": doc["executable"], "buildId": doc["buildId"],
            "architecture": "AArch64", "needed": needed(dynamic)}


def package(task):
    saved = artifact(task)
    assert saved["signedHashMatches"]
    for module in saved["modules"]:
        module.pop("embeddedGLSLSources", None)
    return saved


def audit_684():
    folder = "test/reports/t496/"
    prefix = folder + "t657v4off/"
    trace = prefix + "gltrace-t657v4off.ndjson"
    log = prefix + "t496_log_t657v4off.ndjson"
    blocker = read(folder + "t657-v4-blocker.json")
    amounts, counts, details, event_counts = Counter(), Counter(), set(), Counter()
    maps, last = 0, None
    for line in (ROOT / trace).open("rb"):
        r = json.loads(line)
        k = r["functionId"]
        amounts[k] += len(line)
        counts[k] += 1
        value = r["detail"].encode()
        if r["event"] == "CPU_MAP_RETURN_BYTES":
            fields = r["detail"].split()
            value = bytes.fromhex(fields[-1].split("=", 1)[1])
            assert len(value) == 1024 and sha(value) == fields[-2].split("=", 1)[1]
            w = struct.unpack("<256I", value)
            assert w[:4] == (1381192263, 2, 256, 3235823838)
            event_counts[(w[27], w[52], w[53], w[54])] += 1
            maps += 1
        details.add(value)
        last = r
    cases = {}
    for run in ["t657v4off", "t657milestoneoff", "t657milestoneon", "t657deliveryd2",
                "t657deliveryc0", "t657v5c1", "t657deliveryc2"]:
        p = folder + run + "/t496_log_" + run + ".ndjson"
        if not (ROOT / p).exists():
            continue
        all_rows = rows(p)
        cases[run] = {"raw": record(p), "actions": sum(str(r.get("actionId", "")).isdigit() for r in all_rows),
                      "acceptedObservations": sum(r.get("phase") == "ACCEPTED_OBSERVED" for r in all_rows)}
    assert sum(amounts.values()) == blocker["traceBytes"] == 33554177
    assert cases["t657v4off"]["actions"] == 46 and cases["t657v4off"]["acceptedObservations"] == 80 and maps == 78
    assert cases["t657milestoneoff"]["actions"] == cases["t657milestoneon"]["actions"] == 52
    assert last["callId"] == 51327 and last["event"] == "ENTER"
    frozen = ".codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/"
    sources = ["test/t496/" + x for x in ["external_source_diff.cpp", "gl_sink.inc", "gl_forward_sink.cpp",
                                         "run_bounded.py", "compare_roots.py", "check_actions.py"]]
    sources += [frozen + x for x in ["m1/full_transport.cpp", "m1/source_transport.cpp", "m1/run.cpp", "app_bridge.cpp"]]
    return {"trace": record(trace), "businessLog": record(log), "blocker": record(folder + "t657-v4-blocker.json"),
            "cases": cases, "traceBytesByFunction": dict(amounts), "traceRowsByFunction": dict(counts),
            "last": last, "maps": maps, "mapEventCounts": [[*k, n] for k, n in sorted(event_counts.items())],
            "prefixEncodingEstimateBytes": sum(counts.values()) * 32 + sum(map(len, details)),
            "source": [record(p) for p in sources]}


def audit_685():
    folder = "test/reports/t491/t541b/"
    boot = read(folder + "spf-boot.json")
    r = boot["recovery"]
    a = boot["attempts"][-1]
    cursor = [x for x in a["rows"] if x["stage"] == "SUBMIT_OBSERVER"][-1]
    reset = r["events"][-1]
    assert cursor["a"] == "28" and reset["kind"] == "RESET_REQUESTED"
    assert (r["resetSerial"], r["epoch"], r["retired"], r["resources"]) == ("2", "4", False, False)
    assert any(e["serial"] == "1" and e["kind"] == "CONTEXT_RETIRED" for e in r["events"])
    crash = (ROOT / (folder + "jscrash-latest.log")).read_text(encoding="utf-8-sig")
    assert all(x in crash for x in ["Cannot read property setActive of undefined", "466:19",
                                    "dlopen failed: load module default/grsm0", "dlopen failed: load module default/grsm1"])
    return {"raw": [record(folder + x) for x in ["spf-boot.json", "jscrash-latest.log", "v2-spf-baseline.json"]],
            "DM": inspect_elf("test/reports/t670/dm-native.json"),
            "SPF": inspect_elf("test/reports/t670/spf-native.json"),
            "packages": {t: package(t) for t in ["T670-dm-final", "T670-spf-final", "T593-m1"]},
            "recovery": r, "attempts": boot["attempts"], "lastProgramEnter": cursor,
            "resetMinusProgramEnterNs": str(int(reset["ns"]) - int(cursor["ns"])),
            "independentDifferential": "NOT_RUN_IN_T541_V2"}


def audit_686():
    folder = "test/reports/t491/t654b/t672out/"
    report = read("test/reports/t491/t654b/v2-samepage.json")
    before, after = [read(folder + name) for name in ["status-before.json", "r1-status.json"]]
    keys = ("configure.json", "r1-anchor-before.json", "r1-anchor-after.json", "r1-index.json", "r1-failure.json")
    parts, receipts = [], []
    for i in range(4):
        path = folder + f"r1-page{i}.json"
        j = read(path)
        b = base64.b64decode(j["base64"], validate=True)
        assert (j["schema"], j["state"], j["page"], j["offsetBytes"], j["rawBytes"], len(b)) == (
            "G13InitDiagnosticV1", "SNAPSHOT", i, 8192*i, 8192, 8192)
        assert sha(b) == report["pages"][f"page{i}"]["sha256"]
        parts.append(b)
        receipts.append({**record(path), "decodedSha256": sha(b), "sampleSeq": j["sampleSeq"], "offset": j["offsetBytes"]})
    data = b"".join(parts)
    ctl = struct.unpack_from("<11Q", data, 28672)
    stages = []
    for offset in range(4096, 20480, 256):
        s = data[offset:offset+256]
        seq, ns, boot, attempt, ticket, queue, window, context, tid = struct.unpack_from("<9Q", s)
        if not seq:
            continue
        stages.append({"seq": seq, "ns": str(ns), "boot": str(boot), "attempt": str(attempt), "ticket": str(ticket),
                       "queue": str(queue), "window": str(window), "context": str(context), "tid": str(tid),
                       "rawGL": struct.unpack_from("<q", s, 88)[0], "apiReturn": struct.unpack_from("<i", s, 124)[0],
                       "phase": s[128:160].split(b"\0")[0].decode(), "edge": s[160:176].split(b"\0")[0].decode()})
    cap = after["native"]["capability"]
    assert before["native"]["capability"] is None and before["native"]["state"] == "UNCONFIGURED"
    assert after["native"]["firstError"] == 29 and after["initialization"]["firstFailureAttempt"] == "216"
    assert str(ctl[7]) == stages[-1]["boot"] == after["lastCommand"]["boot"]
    assert ctl[8] == ctl[6] == int(stages[-1]["ticket"]) == 216
    assert stages[-1]["phase"] == "CAPABILITY" and stages[-1]["edge"] == "EXIT"
    assert stages[-1]["rawGL"] == cap["rawGL"] == 1280 and cap["floatPrecision"] == 0
    source = ".codex/local-test/t672-build-v2/entry/src/main/cpp/gpu_resident/g13flow/transport.cpp"
    assert b"glGetShaderPrecisionFormat(GL_COMPUTE_SHADER, GL_HIGH_FLOAT" in (ROOT / source).read_bytes()
    return {"package": package("T672"), "source": record(source), "pages": receipts,
            "statusRaw": [record(folder + name) for name in ["status-before.json", "r1-status.json"]],
            "before": before, "r1": after, "control11U64": list(map(str, ctl)), "controlSHA": sha(data[28672:]),
            "stages": stages, "decodeFirst4U64": list(map(str, struct.unpack_from("<4Q", data))),
            "missing": [x for x in keys if not (ROOT / (folder + x)).exists()],
            "syncConfigureReply": "NOT_CAPTURED", "atomicity": "NOT_ESTABLISHED"}


def audit_688():
    folder = "test/reports/t491/t587b/"
    path = folder + "lab-export.json"
    raw = (ROOT / path).read_bytes()
    d = json.loads(raw)
    text = raw.decode("utf-8")
    start = text.index('"records":[') + len('"records":[')
    decoder = json.JSONDecoder()
    objects = []
    while text[start] != "]":
        _, end = decoder.raw_decode(text, start)
        objects.append(text[start:end].encode())
        start = end + (text[end] == ",")
    assert len(objects) == len(d["records"]) == 30 and sha(b"".join(objects)) == d["recordsSHA256"]
    assert d["pendingResources"] == "NONE" and d["cancelRequestedNs"] == d["cancelObservedNs"] == "0"
    assert d["outcome"] == "COMPLETED_FIXTURE" and int(d["terminalNs"]) < int(d["pollSampleNs"])
    return {"raw": record(path), "recordCount": len(objects), "recordsShaMatchesRawConcat": True,
            "head": {k: v for k, v in d.items() if k != "records"},
            "files": [record(folder + f) for f in ["files.txt", "files2.txt", "lab-record-cancel.json",
                                                    "lab-record-5scancel.json", "v3-lab.json"]],
            "packages": {t: package(t) for t in ["T668", "T668-final"]},
            "driver": inspect_elf("test/reports/t668/native.json"),
            "source": [record(x) for x in ["test/t668/driver.cpp", "test/t668/files.py",
                                          "entry/src/main/cpp/gpu_resident/resource_lab/lab.h"]]}


if __name__ == "__main__":
    result = {"schema": "T684_T688_STATIC_ATTRIBUTION_V1", "deviceUsed": False, "buildRun": False,
              "productOrTestsRun": False, "T684": audit_684(), "T685": audit_685(),
              "T686": audit_686(), "T688": audit_688()}
    write("docs/analysis-T684-T688-evidence-v1.json", result)
    print("Verified saved trace/map identity, signed HAPs and three standalone ELF receipts, G13 four pages and lab records.")
