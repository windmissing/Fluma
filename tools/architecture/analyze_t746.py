"""Rejoin the captured RX BOOT attempts and rebuild the proposed SOURCE offline."""
import json
from pathlib import Path
import re
import struct
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write
from analyze_t733 import FAMILIES, GROUPS, LIMIT, actual_source, split_inputs
from analyze_t736 import document, source_reflection


CAPTURE = "test/reports/t491/t738/v3"
CPP = "entry/src/main/cpp/gpu_resident"
BOOT_STAGES = (
    "BUILD_ENTER", "BUILD_EXIT", "REFLECT_ENTER", "REFLECT_EXIT",
    "ROOT_INIT_ENTER", "ROOT_INIT_EXIT", "WARM_ENTER", "WARM_EXIT",
    "FINISH_ENTER", "FINISH_EXIT", "MAP_ENTER", "MAP_EXIT", "UNMAP_EXIT",
    "PREPARE_EXIT", "BOOT_AUDIT_OK",
)
SOURCE_BUILD_STAGES = (
    "SHADER_COMPILE_ENTER", "SHADER_COMPILE_EXIT", "PROGRAM_LINK_ENTER", "PROGRAM_LINK_EXIT",
)


def audit_package():
    receipt_path = "test/reports/t257/T738-rx.json"
    compile_path = "test/reports/t734/compile.json"
    receipt = document(receipt_path)
    compile_rx = document(compile_path)["families"]["RX"]
    role = compile_rx["role"]
    boot = record(compile_rx["generatedBootPath"])
    entry = "libs/arm64-v8a/libgrs_g9rx.so"
    assert receipt["verifyApp"] == "PASS"
    assert receipt["input"]["sha256"] == compile_rx["unsignedHap"]["sha256"]
    packages = {}
    libs = []
    for key in ("input", "signed"):
        path = Path(receipt[key]["path"])
        packages[key] = record(path.relative_to(ROOT).as_posix())
        assert packages[key]["sha256"] == receipt[key]["sha256"]
        with ZipFile(path) as archive:
            libs.append(archive.read(entry))
    unsigned, signed = libs
    assert unsigned == signed and sha(signed) == compile_rx["unsignedHap"]["packagedElfSHA256"]
    assert build_id(signed) == compile_rx["linkedElf"]["buildId"]
    assert boot["sha256"] == role["sourceSHA"] and boot["bytes"] == role["sourceBytes"] <= LIMIT
    assert signed.count((ROOT / boot["path"]).read_bytes() + b"\0") == 1
    return {"signReceipt": record(receipt_path), "compileReceipt": record(compile_path),
            "packages": packages, "elfEntry": entry, "elfSHA": sha(signed),
            "mappedBuildId": build_id(signed), "boot": boot, "roleSetSHA": role["roleSetSHA"]}


def audit_attempt(lines, lo, hi, pid, tid, package, cold):
    stages = {}
    configure = {}
    stamp = re.compile(rf"^\S+ \S+ {pid} ({pid}|{tid}) ")
    for number in range(lo, hi + 1):
        line = lines[number - 1]
        thread = stamp.match(line)
        if not thread:
            continue
        if "/G9Boot: " in line:
            row = json.loads(line.partition("/G9Boot: ")[2])
            assert (row["schema"], row["family"], row["role"]) == ("G9BootV1", "RX", "BOOT")
            assert int(thread[1]) == row["tid"] == tid and row["pid"] == pid
            assert (row["run"], row["owner"], row["context"]) == ("1", "1", "1")
            assert row["sourceSHA"] == package["boot"]["sha256"]
            assert row["roleSetSHA"] == package["roleSetSHA"]
            assert row["mappedBuildId"] == package["mappedBuildId"]
            assert row["stage"] not in stages
            stages[row["stage"]] = {"line": number, **{k: row[k] for k in ("ns", "raw", "gl", "egl", "error", "seal")}}
        elif "/G9RxConfigure: " in line:
            row = dict(field.split("=", 1) for field in line.partition("/G9RxConfigure: ")[2].split())
            assert row["phase"] not in configure and row["ticket"] == "1"
            assert row["mappedBuildId"] == package["mappedBuildId"]
            assert row["sourceSHA"] == package["boot"]["sha256"]
            configure[row["phase"]] = {"line": number, "tid": int(thread[1]), **row}
    expected = (BOOT_STAGES[:1] + SOURCE_BUILD_STAGES + BOOT_STAGES[1:]) if cold else BOOT_STAGES
    assert tuple(stages) == expected
    timestamps = [int(stage["ns"]) for stage in stages.values()]
    assert all(a < b for a, b in zip(timestamps, timestamps[1:]))
    assert stages["BUILD_ENTER"]["raw"] == package["boot"]["bytes"]
    assert stages["BUILD_EXIT"]["raw"] == 1 and stages["REFLECT_EXIT"]["raw"] == 1
    assert stages["FINISH_ENTER"]["raw"] == 1 and stages["FINISH_EXIT"]["raw"] == 37146
    assert stages["MAP_ENTER"]["raw"] == 1166976 and stages["MAP_EXIT"]["raw"] == 1
    for name in ("UNMAP_EXIT", "PREPARE_EXIT", "BOOT_AUDIT_OK"):
        row = stages[name]
        assert (row["raw"], row["gl"], row["egl"], row["error"], row["seal"]) == (1, 0, 12288, 0, 0x641647)
    begin = configure["ENQUEUED"]
    stop = configure["BOOT_AUDIT_OK"]
    assert (stop["tid"], stop["run"], stop["context"], stop["result"], stop["gl"], stop["egl"]) == (tid, "1", "1", "1638", "0", "12288")
    assert int(begin["ns"]) < int(stages["BUILD_ENTER"]["ns"]) < int(stages["BOOT_AUDIT_OK"]["ns"]) < int(stop["ns"])
    duration = {"configureNs": int(stop["ns"]) - int(begin["ns"]),
                "buildNs": int(stages["BUILD_EXIT"]["ns"]) - int(stages["BUILD_ENTER"]["ns"])}
    if cold:
        for phase in ("SHADER_COMPILE", "PROGRAM_LINK"):
            enter, exit_row = (stages[phase + suffix] for suffix in ("_ENTER", "_EXIT"))
            assert exit_row["raw"] == 1
            duration[phase + "_ns"] = int(exit_row["ns"]) - int(enter["ns"])
    return {"pid": pid, "tid": tid, "processStartTick": None, "run": 1, "owner": 1,
            "context": 1, "ticket": 1, "bootResult": "PASS", "duration": duration,
            "configure": {name: {k: configure[name][k] for k in ("line", "ns", "result")}
                          for name in ("ENQUEUED", "APPLY_ENTER", "BOOT_AUDIT_OK")},
            "stages": stages}


def audit_cache(logs, lines, package):
    receipt_path = "test/reports/t491/t738/rx-cache/97ef.jsonl"
    binary_path = "test/reports/t491/t738/rx-cache/97ef.bin"
    rows = [json.loads(line) for line in (ROOT / receipt_path).read_text(encoding="utf-8").splitlines()]
    exported, warmed = [row for row in rows if row["stage"] in ("EXPORT_WRITE_SUBMITTED", "WARMUP_OK")]
    for row in (exported, warmed):
        assert row["persistence"] == "PERSISTED" and row["shaderSHA"] == package["boot"]["sha256"]
    assert {k: exported[k] for k in ("key", "binarySHA", "binaryBytes")} == {
        k: warmed[k] for k in ("key", "binarySHA", "binaryBytes")}
    assert sha((ROOT / binary_path).read_bytes()[-warmed["binaryBytes"]:]) == warmed["binarySHA"]
    factory_path = CPP + "/program_cache/program_factory.h"
    observer_path = CPP + "/g9_boot.h"
    source_route_path = CPP + "/m2/create_diagnostics.h"
    factory = (ROOT / factory_path).read_text(encoding="utf-8")
    observer = (ROOT / observer_path).read_text(encoding="utf-8")
    source_route = (ROOT / source_route_path).read_text(encoding="utf-8")
    load_at = factory.index("glProgramBinary(program,")
    assert load_at < factory.index("return true;", factory.index('p.route = "BINARY_LOAD";', load_at)) < factory.index("create468::Build", load_at)
    assert all(f"observer.Emit(SubmitStage::{stage}Enter)" in source_route for stage in ("Shader", "Program"))
    assert "trace.ProgramObserver()" in observer and "2 * sizeof(uint32_t), GL_MAP_READ_BIT" in observer
    assert "const auto error = tail[0], seal = tail[1]" in observer
    assert all("BINARY_LOAD" not in log for log in logs)
    assert "45505 45618" in lines[5741] and "Shader compile fails" in lines[5741]
    return {"earlierPersisted": {"receipt": record(receipt_path), "wrappedBinary": record(binary_path),
                                **{k: warmed[k] for k in ("beginNs", "packageSource", "key", "binaryBytes", "binarySHA")}},
            "warmBranch": "BINARY_LOAD_INFERRED_FROM_PROGRAM_FACTORY_AND_OBSERVED_STAGES",
            "warmDirectReceipt": None, "warmVendorWarningLines": [5742, 5743],
            "implementation": [record(path) for path in (factory_path, observer_path, source_route_path)]}


def audit_source(compiler):
    name, lib, assets, source_sha = FAMILIES["RX"]
    parent, identity = actual_source(name, lib, source_sha)
    assert (ROOT / assets / "pipeline.comp").read_bytes().replace(b"\r\n", b"\n") == parent
    ops, build = split_inputs(parent.decode())
    shader, _ = build(GROUPS["SOURCE"])
    prior = document("docs/architecture-T736-source-contract-v1.json")
    rx = prior["families"]["RX"]
    role = rx["sourceRole"]
    assert len(shader) == role["bytes"] <= LIMIT and sha(shader) == role["sha256"]
    header = (ROOT / assets / "schema_generated.h").read_text(encoding="utf-8")
    manifest = re.search(r"kSourcePasses\s*=\s*\{\{(.*?)\}\};", header, re.S)[1]
    table = [(op, ops["G9_OP_" + op], int(u_role), int(j), int(micro))
             for op, u_role, j, micro in re.findall(r"\{Op::(\w+),\s*(\d+),\s*(\d+),\s*(-?\d+)\}", manifest)]
    assert [item[1:] for item in table] == [(i, 0, 0, -1) for i in range(12)]
    assert sha(json.dumps([row[1:] for row in table], separators=(",", ":")).encode()) == role["passSHA"]
    with TemporaryDirectory(prefix="t746-reflect-") as directory:
        candidate = Path(directory) / "rx-source.comp"
        candidate.write_bytes(shader)
        reflection = source_reflection(compiler, candidate)
    assert reflection == prior["reflection"]
    page_path = "entry/src/main/ets/pages/GpuResidentG9Rx.ets"
    page = (ROOT / page_path).read_text(encoding="utf-8")
    assert "native.g9RxSource(" in page
    command = re.search(r'"sourceId":"2","mass":"0\.2","x":"64","y":"200","vx":"0","vy":"0"', page)[0]
    args = json.loads("{" + command + "}")
    bridge_path = CPP + "/g9rx/bridge.cpp"
    bridge = (ROOT / bridge_path).read_text(encoding="utf-8")
    ingress = bridge.split('if (api == "g9RxSource")', 1)[1].split('if (api ==', 1)[0]
    for snippet in ('"mass", "x", "y", "vx", "vy"', "std::strtod", "RealPut(body.data(), 64 + 2 * i, number)",
                    "body[74] = source;", "body[75] = 1;", "body[76] = 63701;", "core.OfferSource(body, intent, receipt)"):
        assert snippet in ingress
    values = tuple(float(args[k]) for k in ("mass", "x", "y", "vx", "vy"))
    words = [*struct.unpack("<10I", struct.pack("<5d", *values)), int(args["sourceId"]), 1, 63701]
    assert words == rx["inputBody64To76"]
    num, den = values[0].as_integer_ratio()
    admitted = (num << 20) // den
    seed_path = "docs/architecture-T641-seed-config-v1.json"
    config = document(seed_path)["profiles"][0]
    expected = prior["firstRootExpected"]
    assert sha(struct.pack("<64I", *config["config64"])) == rx["parameterSHA"] == config["parameterSHA"]
    assert rx["seedTemplateSHA"] == config["templateSHA"]
    assert admitted == expected["admittedQ"] == 209715
    assert 2 * (40 << 20) + admitted <= expected["y2Q21"] == int(values[2]) << 21 <= (512 << 20) - admitted
    assert expected["xQ20"] == 64 << 20 and admitted <= 244666
    assert b"g9DecodeBinary64(g9InputPair(64u),20u,false,mass)" in shader
    assert expected["baseMassQ"] + admitted == expected["finalMassQ"] == 5368918835
    return {"parent": identity, "role": role, "compilerSHA": sha(compiler.read_bytes()),
            "reflection": reflection, "originalNapiPage": record(page_path), "input": args,
            "originalNativeIngress": record(bridge_path),
            "seedAndConfig": record(seed_path), "seedTemplateSHA": rx["seedTemplateSHA"],
            "parameterSHA": rx["parameterSHA"], "inputMagic": rx["inputMagic"],
            "inputRevision": rx["inputRevision"], "profileCode": rx["profileCode"], "bodyWords64To76": words,
            "passes": prior["passTable"], "expected": expected, "runtimeResult": "NOT_RUN"}


def audit(compiler):
    package = audit_package()
    paths = [f"{CAPTURE}/{name}" for name in ("h-cold.txt", "h-cold2.txt", "h-warm.txt")]
    logs = [(ROOT / path).read_text(encoding="utf-8-sig", errors="replace") for path in paths]
    lines = logs[-1].splitlines()
    assert not any(re.search(r"processStart|startTick|/proc/\d+/stat", log, re.I) for log in logs)
    assert "onProcessDied" in lines[1667] and "pid: 43774" in lines[1667]
    cold = audit_attempt(lines, 837, 869, 43774, 44158, package, True)
    warm = audit_attempt(lines, 5716, 5759, 45505, 45618, package, False)
    assert [a["duration"]["configureNs"] for a in (cold, warm)] == [637260938, 186281094]
    return {"schema": "T746_RX_BOOT_SOURCE_PRECONDITION_V1", "deviceUsed": False,
            "qualification": "BOOT_TAIL_PASS_TWICE",
            "sourceDispatchPrecondition": "PENDING_SAME_ATTEMPT_PROCESS_START_TICK_PER_T736",
            "package": package, "capturedLogs": [record(path) for path in paths],
            "cold": cold, "warm": warm, "coldProcessDeathLine": 1668,
            "tail": {"range": 7, "byteOffset": 1166976, "byteLength": 8,
                     "words": [0, 0x641647], "wordsEncodedLE": struct.pack("<2I", 0, 0x641647).hex()},
            "cache": audit_cache(logs, lines, package), "source": audit_source(compiler)}


if __name__ == "__main__":
    write("docs/architecture-T746-evidence-v1.json", audit(Path(sys.argv[1]).resolve()))
    print("T746 checked signed RX BOOT, two PID-scoped tails, and the static 12-pass 209715-Q20 SOURCE")
