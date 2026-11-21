"""Recompute T760's BOOT key and delimit the older T730 warm observation."""

import argparse
import json
from pathlib import Path
import re
from struct import pack, unpack_from

from analyze_t660_t662 import ROOT, record, sha, write
from analyze_t764 import COMPILE, packaged_identity


T760_ROWS = "test/reports/t491/t760/t756-compute-persisted.jsonl"
T760_BOOT = "test/reports/t491/t760/t760-diag-full.json"
T730_WARM = "test/reports/t491/t730b/split-cache/856a-warm.jsonl"
RX_LOG = "test/reports/t491/t755/h755.txt"
NATIVE = "entry/src/main/cpp/gpu_resident/"
KEY_FIELDS = ("key", "shaderSHA", "layoutSHA", "environmentSHA", "linkConfigSHA")


def doc(path):
    return json.loads((ROOT / path).read_bytes())


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig", errors="replace")


def jsonl(path):
    return [json.loads(line) for line in read(path).splitlines() if line.strip()]


def cache_key(fields):
    return sha(b"T475_PROGRAM_KEY_V1" + b"".join(pack("<Q", len(raw)) + raw for raw in fields))


def recompute(terminal):
    build = doc(COMPILE)
    project = Path(build["project"])
    source_path = project / build["roles"]["BOOT"]["path"]
    source = (ROOT / source_path).read_bytes()
    assert len(source) == 23861 and sha(source) == terminal["shaderSHA"]
    header = read(source_path.with_name("contract_generated.h"))
    hashes = {part: re.search(r'kHash_' + part + r'SHA\[\]="([a-f0-9]+)"', header)[1]
              for part in ("layout", "math", "gates", "algorithm")}
    binding_path = project / NATIVE / "g13flow/root_binding.h"
    binding = re.search(r'kRootBindingPolicy\[\] = "([^"]+)"', read(binding_path))[1]
    abi = (f'G13_FOUR_SSBO_64_LANES|{hashes["layout"]}|BINDING={binding}|'
           f'MATH={hashes["math"]}|GATES={hashes["gates"]}|ALGORITHM={hashes["algorithm"]}|ROLE=BOOT')
    factory = project / NATIVE / "program_cache/program_factory.h"
    link = re.search(r'kLinkConfig\[\]\s*=\s*"([^"]+)"', read(factory))[1]
    environment = terminal["environmentSHA"]
    fields = (source, abi.encode("ascii"), environment.encode("ascii"), link.encode("ascii"))
    assert binding == "T753-v1" and sha(fields[1]) == terminal["layoutSHA"]
    assert sha(fields[3]) == terminal["linkConfigSHA"] and cache_key(fields) == terminal["key"]
    negative_keys = {}
    for index, tag in enumerate(("source", "layout", "environment", "link")):
        changed = list(fields)
        changed[index] += b"x"
        negative_keys[tag] = cache_key(changed)
    assert len({terminal["key"], *negative_keys.values()}) == 5
    return {"inputs": [record(str(path.as_posix())) for path in (source_path, binding_path, factory)],
            "generatedHashes": hashes, "abi": abi, "link": link, "role": "BOOT", "opcodes": [64, 65, 66],
            "environmentInputEncoding": "ASCII_SHA256_HEX", "exactKey": cache_key(fields),
            "singleInputMutationsOffline": negative_keys}


def read_capture(identity):
    cold = jsonl(T760_ROWS)
    assert [r["stage"] for r in cold] == [
        "COMPILE_ENTER", "LINK_ENTER", "LINK_OK_REFLECTION_PENDING", "REFLECT_OK",
        "EXPORT_LENGTH_ENTER", "EXPORT_ENTER", "EXPORT_WRITE_SUBMITTED",
    ]
    persisted = cold[-1]
    assert all(r[k] == persisted[k] for r in cold for k in KEY_FIELDS)
    assert all(r["role"] == "G13_COMPUTE" and r["route"] == "SOURCE_COMPILE" for r in cold)
    assert (persisted["binaryBytes"], persisted["binaryFormat"], persisted["persistence"]) == (301961, 38769, "PERSISTED")
    assert persisted["binaryFormat"] in persisted["supportedFormats"]
    boot = doc(T760_BOOT)["bootQualification"]
    scope, program = boot["scope"], boot["program"]
    assert (scope["pid"], scope["processStartTicks"], scope["ownerTid"], scope["attempt"]) == (46924, "8613992", 47218, "191")
    assert scope["mappedBuildId"] == identity["buildId"]
    assert (persisted["programHandle"], persisted["contextHandle"]) == (scope["program"], int(scope["eglContext"]))
    assert (persisted["key"], persisted["shaderSHA"], persisted["environmentSHA"]) == (
        program["cacheKey"], program["sourceSHA"], program["environmentSHA"])
    assert program["route"] == "SOURCE_COMPILE" and not program["binarySHA"]
    assert boot["effectiveRejectStage"] == "OP65" and boot["result"] == 27
    durations = {label: persisted[label + "EndNs"] - persisted[label + "BeginNs"]
                 for label in ("compile", "link", "reflect")}
    assert tuple(durations.values()) == (65_237_058_593, 110_555_203_906, 234_375)

    historical = jsonl(T730_WARM)
    old_cold, warm_enter, warm_check, warm_link, warm_reflect, warm_prepared = historical[-6:]
    assert [r["stage"] for r in historical[-5:]] == [
        "BINARY_LOAD_ENTER", "LOAD_CHECKED", "LOAD_LINK_OK_REFLECTION_PENDING", "REFLECT_OK", "REFLECT_PREPARE_OK",
    ]
    assert (old_cold["persistence"], old_cold["programHandle"], warm_prepared["programHandle"]) == ("PERSISTED", 2, 6)
    assert all(warm_prepared[k] == old_cold[k] for k in (*KEY_FIELDS, "binarySHA", "contextHandle", "contextGeneration"))
    assert warm_link["route"] == warm_reflect["route"] == warm_prepared["route"] == "BINARY_LOAD"
    assert warm_prepared["key"] != persisted["key"] and warm_prepared["binarySHA"] != persisted["binarySHA"]
    assert warm_prepared["binaryBytes"] == persisted["binaryBytes"]
    load_ns = warm_check["loadEndNs"] - warm_enter["loadBeginNs"]
    warm_ns = warm_prepared["endNs"] - warm_prepared["beginNs"]
    assert (load_ns, warm_ns) == (5_645_000, 10_601_094)
    assert warm_prepared["compileBeginNs"] == warm_prepared["linkBeginNs"] == 0
    rx = [line for line in read(RX_LOG).splitlines() if "/G9RxConfigure:" in line and "ticket=1 " in line]
    assert any("CACHE_PRELOAD_READY" in line for line in rx) and any("APPLIED" in line for line in rx)
    assert not any("BINARY_LOAD" in line for line in rx)
    return {"sources": [record(p) for p in (T760_ROWS, T760_BOOT, T730_WARM, RX_LOG)],
            "t760": {"coldProgram": persisted, "scope": scope, "durationsNs": durations,
                     "effectiveRejectStage": boot["effectiveRejectStage"], "numericResult": boot["result"],
                     "archivedBinaryFile": False, "sameKeyWarmNewProcess": "NOT_OBSERVED"},
            "t730": {"coldProgram": old_cold, "warmProgram": warm_prepared, "loadNs": load_ns,
                     "buildToPreparedReceiptNs": warm_ns, "newProcessProven": False},
            "t755": {"observedPreload": True, "sourceCompiledAndLinked": True, "binaryLoadObserved": False}}


def audit_historical_file(path, expected):
    wire = path.read_bytes()
    assert len(wire) >= 144 and wire[:8] == b"T475B001", "CACHE_HEADER"
    format_id, size = unpack_from("<II", wire, 8)
    assert (format_id, size, len(wire)) == (expected["binaryFormat"], expected["binaryBytes"], 144 + size), "CACHE_LENGTH_OR_FORMAT"
    assert wire[16:80].decode("ascii") == path.stem == expected["key"], "CACHE_KEY_OR_FILENAME"
    assert wire[80:144].decode("ascii") == sha(wire[144:]) == expected["binarySHA"], "CACHE_PAYLOAD_SHA"
    print(f"Verified historical T760 disk artifact ({len(wire)} bytes): {sha(wire)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--t760-bin", type=Path, help="check an untouched historical <key>.bin read-only")
    args = parser.parse_args()
    identity = packaged_identity()
    captures = read_capture(identity)
    terminal = captures["t760"]["coldProgram"]
    if args.t760_bin:
        audit_historical_file(args.t760_bin, terminal)
    else:
        write("docs/architecture-T783-evidence-v1.json", {
            "schema": "T783_G13_BOOT_CACHE_EVIDENCE_V1", "deviceUsed": False,
            "artifact": identity, "capture": captures, "keyReconstruction": recompute(terminal),
        })
    print("T783 pinned the T760 BOOT scope and separated older binary load from cross-process proof")


if __name__ == "__main__":
    main()
