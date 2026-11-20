"""Reconstruct the attempt-250 decision and validate T732 packaging without signing."""
import json
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write


REPORT = "test/reports/t491/t730b/"
PARENT = "test/reports/t732/compile.json"
SIGN = "test/reports/t257/T732-split.json"
NATIVE = "libs/arm64-v8a/libgrs_g13.so"
ASSETS = Path("entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/g13flow/g13_assets")
SCENE = Path("entry/src/main/cpp/gpu_resident/g13flow")


def doc(path):
    return json.loads((ROOT / path).read_bytes())


def exact_diagnostic(node):
    found = []
    raw = node.get("attributes", {}).get("originalText", "")
    if raw.startswith("{"):
        try:
            row = json.loads(raw)
        except json.JSONDecodeError:
            row = {}
        if row.get("schema") == "G13InitDiagnosticV1" and "bootReadback" in row:
            found.append(row)
    for child in node.get("children", []):
        found.extend(exact_diagnostic(child))
    return found


def attempt():
    ui, = exact_diagnostic(doc(REPORT + "v5/l2.json"))
    decode = doc(REPORT + "v5/t732-diag.json")
    raw = ui["bootReadback"]
    assert ui["queuedTicket"] == ui["firstFailureAttempt"] == raw["attempt"] == decode["attempt"] == "250"
    assert raw["boot"] == decode["boot"] and decode["state"] == "QUEUED"
    assert ui["lastProgress"] == "INIT_FP32_RESULT:EXIT" and ui["bootReadyNs"] == "0"
    assert ui["firstConfigFailure"] == "INIT_ERROR_27"
    assert (raw["queue"], raw["run"], raw["context"]) == ("3", "1", "1")
    assert (raw["fence"], raw["firstGl"], raw["mathReadGl"],
            raw["ctrl255"], raw["ctrl7"]) == (0x911A, 0, 0, 0xC0DE1301, 27)
    cpp = (ROOT / SCENE / "transport.cpp").read_text(encoding="utf-8")
    literals = re.search(r"kExpectedFp32\{([^}]+)\}", cpp)[1]
    expected = [int(x[:-1], 0) for x in re.findall(r"(?:0x[0-9a-f]+|\d+)u", literals)]
    a, b = raw["ctrl200To215"][:8], raw["ctrl200To215"][8:]
    different = [i for i, (observed, want) in enumerate(zip(a, expected)) if observed != want]
    assert len(a) == len(b) == len(expected) == 8 and a == b and different == [2, 3]
    assert a[2:4] == [0, 0]
    math = raw["work65_35_41_47_53_59"]
    assert math == [0, 0x3F800000, 0x3F800000, 0, 0x3F800000]
    assert cpp.index('Stage("INIT_FP32_RESULT"') < cpp.index("if (auditFailed)") < cpp.index("if (!qualified)", cpp.index("if (auditFailed)"))
    layout = doc("docs/architecture-T607-g13-layout-v1.json")
    work, = [p["work"][19] for p in layout["profiles"] if p["Nx"] == p["Ny"] == 64]
    assert work["wordStart"] == 145312 and work["words"] >= 572
    return {
        "configureOriginal": record(REPORT + "v5/t732-diag.json"),
        "uiOriginal": record(REPORT + "v5/l2.json"), "ui": ui,
        "expectedOp66": expected, "differingIndicesPerReplay": different,
        "expectedWorkForExactOneSine": [0, 0x3F800000, 0x3F800000, 1, 0x3F800000],
        "status": "OP65_REJECTS_FIRST_AND_OP66_INDEPENDENTLY_FAILS",
        "loadedIdentity": "UNOBSERVED_PID_START_MAPPED_BUILD_ID",
        "controlTailByteRange": [18432 * 4, 18688 * 4],
        "op66ByteRange": [18632 * 4, 18648 * 4],
        "defaultRest64Work65ByteRange": [(145312 + 512) * 4, (145312 + 572) * 4],
    }


def verify_arm(variant, row, old, parent_sha):
    project = Path(row["project"]).resolve()
    unsigned = project / row["unsigned"]
    assert sha(unsigned.read_bytes()) == row["unsignedSHA256"]
    with ZipFile(unsigned) as archive:
        native = archive.read(NATIVE)
    assert (sha(native), build_id(native)) == (row["nativeSHA256"], row["buildId"])
    assets = doc(project / ASSETS / "assets_manifest.json")
    assert assets["pipelineSHA"] == row["pipelineSHA"] and not assets["testHooks"]
    roles = {}
    for item in assets["roles"]:
        roles[item["role"]] = {
            "path": (ASSETS / item["entry"]).as_posix(), "kind": "compute",
            "opcodes": item["opcodes"], "reworked": variant == "SPLIT",
            "sha256": item["sha256"], "bytes": item["bytes"]}
    for item in assets["graphics"]:
        roles[item["entry"]] = {
            "path": (SCENE / item["entry"]).as_posix(), "kind": "graphics", "opcodes": [],
            "reworked": False, "sha256": item["sha256"], "bytes": item["bytes"]}
    normalization = {}
    for name, role in roles.items():
        assert (role["sha256"], role["bytes"]) == (row["roles"][name]["sha256"], row["roles"][name]["bytes"])
        raw = (project / role["path"]).read_bytes()
        normalized = raw.replace(b"\r\n", b"\n")
        assert sha(normalized) == role["sha256"] and len(normalized) == role["bytes"]
        assert normalized + b"\0" in native
        if raw != normalized:
            role.update(sourceNewlines="CPP_TRANSLATION_LF", sourceSHA256=sha(raw))
            normalization[name] = {"rawBytes": len(raw), "crlf": raw.count(b"\r\n"), "rawSHA": sha(raw)}
    bad_kinds = [name for name, r in roles.items() if old["roles"][name]["kind"] != r["kind"]]
    bad_ops = [name for name, r in roles.items() if old["roles"][name]["opcodes"] != r["opcodes"]]
    new = {k: row[k] for k in ("unsigned", "unsignedSHA256", "nativeSHA256", "buildId")}
    new.update(variant=variant, project=str(project), parentReceipt=str(ROOT / PARENT),
               parentSHA256=parent_sha, roles=roles)
    return new, {"graphicsNormalization": normalization, "oldWrongKinds": bad_kinds,
                 "oldWrongOpcodeSets": bad_ops, "embeddedRoles": roles}, native, unsigned, assets


def packaging(readback):
    parent_sha = sha((ROOT / PARENT).read_bytes())
    parent, sign = doc(PARENT), doc(SIGN)
    originals = {key: ROOT / f"test/reports/t730b/t732-{key.lower()}.json" for key in ("MONOLITH", "SPLIT")}
    signed = Path(sign["signed"]["path"])
    assert sha(signed.read_bytes()) == sign["signed"]["sha256"] and sign["verifyApp"] == "PASS"
    audit = {}
    with TemporaryDirectory(prefix="t737-unsigned-") as temp:
        temp = Path(temp)
        tool = [sys.executable, ROOT / "test/t729/sign_pair.py"]
        result = subprocess.run([*tool, *originals.values(), temp / "bad.json", "--verify-only"],
                                cwd=ROOT, capture_output=True, text=True)
        assert result.returncode != 0 and "SHA_MISMATCH" in result.stderr and "scene.vert" in result.stderr
        corrected = []
        for key, source in originals.items():
            row, detail, native, unsigned, assets = verify_arm(key, parent["builds"][key], doc(source), parent_sha)
            f = temp / source.name
            f.write_text(json.dumps(row), encoding="utf-8")
            corrected.append(f)
            audit[key] = detail | {k: row[k] for k in ("unsignedSHA256", "nativeSHA256", "buildId")}
            if key == "SPLIT":
                boot = next(role for role in assets["roles"] if role["role"] == "BOOT")
                assert boot["sha256"] == readback["ui"]["bootReadback"]["sourceSHA"]
                program = (Path(row["project"]) / ASSETS / boot["entry"]).read_text(encoding="utf-8")
                assert program.count("fail(27u)") == 1 and "c[18439]=c[514]" in program
                with ZipFile(signed) as dest, ZipFile(unsigned) as source_zip:
                    assert dest.read(NATIVE) == native
                    assert len(source_zip.namelist()) == sign["payloadEntriesUnchanged"] == 14
                    assert all(dest.read(name) == source_zip.read(name) for name in source_zip.namelist())
        fixed = subprocess.run([*tool, *corrected, temp / "verified.json", "--verify-only"],
                               cwd=ROOT, capture_output=True, text=True, check=True)
        assert fixed.returncode == 0 and json.loads((temp / "verified.json").read_bytes())["signed"] is False
    return {"compileReceipt": record(PARENT), "signedReceipt": record(SIGN),
            "signedSHA": sign["signed"]["sha256"], "splitPayloadEntriesEqual": 14,
            "before": "SHA_MISMATCH_scene.vert", "after": "BOTH_ARMS_STRICT_VERIFY_ONLY_PASS",
            "arms": audit}


def cache():
    path = REPORT + "split-cache/856a-warm.jsonl"
    rows = [json.loads(line) for line in (ROOT / path).read_bytes().splitlines()]
    cold, = [r for r in rows if r["stage"] == "EXPORT_WRITE_SUBMITTED"]
    warm, = [r for r in rows if r["stage"] == "REFLECT_PREPARE_OK"]
    assert warm["route"] == "BINARY_LOAD" and cold["key"] == warm["key"]
    assert cold["binarySHA"] == warm["binarySHA"] and cold["binaryBytes"] == warm["binaryBytes"] == 301961
    def elapsed(row, start, end):
        return int(row[end]) - int(row[start])
    return {"original": record(path), "key": warm["key"], "bootSHA": warm["shaderSHA"],
            "binarySHA": warm["binarySHA"], "binaryBytes": warm["binaryBytes"],
            "coldCompileNs": elapsed(cold, "compileBeginNs", "compileEndNs"),
            "coldLinkNs": elapsed(cold, "linkBeginNs", "linkEndNs"),
            "coldBuildNs": elapsed(cold, "beginNs", "buildEndNs"),
            "warmLoadNs": elapsed(warm, "loadBeginNs", "loadEndNs"),
            "warmBuildNs": elapsed(warm, "beginNs", "buildEndNs"),
            "scope": "HISTORICAL_BOOT_ONLY_NO_ATTEMPT250_OR_FRESH_PID_PAIR"}


if __name__ == "__main__":
    info = attempt()
    result = {"schema": "T737_BOOT_FP32_AND_SIGNING_V1", "deviceUsed": False,
              "attempt": info, "packaging": packaging(info), "historicalCache": cache(),
              "bootReady": "NOT_REACHED", "computeReady": "NOT_REACHED", "firstSwap": "NOT_REACHED"}
    write("docs/architecture-T737-evidence-v1.json", result)
    print("T737 original probe words and both packaging arms verified")
