"""Read preserved device and host bytes without executing the product."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import zipfile

from analyze_t660_t662 import ROOT, artifact, build_id, record, sha, ui_texts, write


def read(path):
    return json.loads((ROOT / path).read_bytes())


def files_in(path):
    return [record(p.relative_to(ROOT).as_posix()) for p in sorted((ROOT / path).glob("*")) if p.is_file()]


def dm_evidence():
    base = "test/reports/t491/t569b"
    execution_path = f"{base}/dm20-execution.jsonl"
    header, result = [json.loads(line) for line in (ROOT / execution_path).read_bytes().splitlines()]
    assert header["evidenceOrigin"] == "ACTUAL_GL_EXECUTION" and result == {"stage": "CREATE", "result": 8}
    receipt = read("test/reports/t670/dm-native.json")
    exe = Path(receipt["executable"]["path"])
    on_disk = exe.read_bytes()
    generated_base = exe.parent.relative_to(ROOT).as_posix()
    source_path = f"{generated_base}/dm_native/dm_assets/pipeline.comp"
    normalized_source = (ROOT / source_path).read_bytes().replace(b"\r\n", b"\n")
    lines = normalized_source.decode().splitlines()
    assert "uint slot=best" in lines[1727] and "for(uint slot" in lines[1726]
    packages = []
    for name in ("T569-dmfix", "T670-dm-final"):
        package = artifact(name)
        assert package["signedHashMatches"]
        library = next(module for module in package["modules"] if module["entry"].endswith("libgrs_dm_t647.so"))
        with zipfile.ZipFile(Path(read(f"test/reports/t257/{name}.json")["signed"]["path"])) as hap:
            elf = hap.read(library["entry"])
        packages.append({"name": name, "signedSha256": package["actualSignedSHA"],
                         "library": library, "containsExactNormalizedSource": normalized_source in elf})
    fixtures = {}
    for folder in ("dm20-input", "dm20b-input", "dm20c-input"):
        manifest = read(f"{base}/{folder}/fixture.json")
        files = files_in(f"{base}/{folder}")
        actual = {Path(item["path"]).name: {key: item[key] for key in ("bytes", "sha256")} for item in files}
        mismatches = [name for name, expected in manifest["files"].items() if actual.get(name) != expected]
        fixtures[folder] = {"files": files, "localManifestMismatches": mismatches}
    return {
        "executionOriginal": record(execution_path), "execution": [header, result],
        "reportedDeviceSignature": "0:1728 S0022 Symbol 'slot' redeclared",
        "signatureReceipt": record(f"{base}/v2-dm20.json"),
        "loadedDeviceIdentity": "NOT_CAPTURED",
        "standaloneLocal": {"expectedSha256": receipt["executable"]["sha256"], "actualSha256": sha(on_disk),
                            "expectedBuildId": receipt["buildId"], "actualBuildId": build_id(on_disk)},
        "source": {**record(source_path), "normalizedSha256": sha(normalized_source),
                   "scopeExcerpt": {str(n): lines[n-1] for n in range(1723, 1729)}},
        "packages": packages, "localInputsNotProvenLoaded": fixtures,
        "otherArms": {"DM22": "NOT_RUN", "DM29": "NOT_RUN", "DM31": "NOT_RUN"},
    }


def v5_evidence():
    base = "test/reports/t491/t594b"
    first_path, last_path = (f"{base}/v5-m3b-{suffix}.json" for suffix in ("first", "last"))
    first, last = read(first_path), read(last_path)
    request, reply, selected = (json.loads(last[field]) for field in ("request", "reply", "selected"))
    call = reply["inputCall"]
    assert request["length"] == 32 and request["byteLength"] == 128 and request["words"][15] == 0
    assert selected["ok"] is True and reply["requestTicket"] == "1" and first["currentCall"] == call
    assert first["state"] == "NO_FIRST_FAILURE" and selected["normalizedElementCount"] == 32
    caps_reads = []
    for suffix in "BCD":
        path = f"{base}/c{suffix}.json"
        raw = next(s for s in ui_texts(read(path)) if '"api":"m5ColdCapabilities"' in s)
        caps = json.loads(raw)
        assert all(caps[k] == call[k] for k in ("bootStamp", "diagGeneration"))
        assert caps["code"] == "M5_NOT_READY"
        caps_reads.append({"original": record(path), "reply": caps})
    assert [entry["reply"]["attemptId"] for entry in caps_reads] == ["3", "4", "5"]
    package = artifact("T669-v5")
    assert package["signedHashMatches"]
    library = next(m for m in package["modules"] if m["entry"].endswith("libgrsm1.so"))
    return {
        "firstOriginal": record(first_path), "lastOriginal": record(last_path),
        "repeatedLast": record(f"{base}/v5-m3b-last2.json"),
        "configureRequest": request, "configureWordsLE_SHA256": sha(struct.pack("<32I", *request["words"])),
        "configureReply": reply, "configureFirstFailureScope": first,
        "coldCapabilities": caps_reads, "bootFile": record(f"{base}/v5-boot.json"),
        "signedSha256": package["actualSignedSHA"],
        "packageLibrary": {k: v for k, v in library.items() if k != "embeddedGLSLSources"},
        "loadedDeviceIdentity": "NOT_CAPTURED", "initArchive": "NOT_IN_FORMAL_HANDOFF",
        "verdict": "REQUEST_ACCEPTED_CONTROLLER_ABSENT_AT_CAPS_QUERIES_FIRST_BLOCKER_UNKNOWN",
    }


def host_evidence():
    folders = {arm: f"test/reports/t496/t693-{arm}" for arm in
               ("second-off", "second-on", "replay-off", "replay-on")}
    main = read("test/reports/t693/main-pair.json")
    replay = read("test/reports/t693/replay-pair.json")
    action_result = subprocess.check_output([sys.executable, "test/t693/compare_actions.py",
                                             folders["second-off"], folders["second-on"]], cwd=ROOT)
    replay_result = subprocess.check_output([sys.executable, "test/t693/compare_replay.py",
                                             folders["replay-off"], folders["replay-on"],
                                             folders["second-off"], folders["second-on"]], cwd=ROOT)
    actions, correlation = json.loads(action_result), json.loads(replay_result)
    assert actions["actions"] == "MATCH" and correlation["correlatedWithMain"] == [92, 92]
    assert correlation["rootCount"] == 92 and correlation["replayByteMatch"] is True
    assert replay["originalMapBindingGeneration"] == "NOT_CAPTURED" and replay["fullT684Proof"] == "INCOMPLETE"
    return {"folders": {arm: files_in(path) for arm, path in folders.items()}, "actionComparison": actions,
            "replayComparison": correlation, "mainPair": record("test/reports/t693/main-pair.json"),
            "replayPair": record("test/reports/t693/replay-pair.json"),
            "mainArms": {arm: {k: m[k] for k in ("rows", "rootCount", "mapCount", "rawBytes")}
                         for arm, m in main["arms"].items()},
            "fullT684Proof": "INCOMPLETE", "cycle32": "NOT_MEASURED"}


def link_evidence():
    base = "test/reports/t491/t591c/out"
    manifest_path = f"{base}/t468-create-1-1795136945713-manifest.json"
    manifest = read(manifest_path)
    native_path = f'{base}/{manifest["native"]["name"]}'
    for entry in [manifest["native"], *manifest["rawLogs"]]:
        actual = record(f'{base}/{entry["name"]}')
        assert (actual["bytes"], actual["sha256"]) == (entry["bytes"], entry["sha256"])
    native = read(native_path)
    current, cursor = native["current"], native["apiCursor"]
    assert native["inCall"] and not native["sealed"] and native["nativeEndNs"] == "0"
    assert native["firstFailure"] is None and current["stage"] == "LINK"
    assert (cursor["api"], cursor["object"], cursor["edge"], cursor["returned"], cursor["rawGl"]) == (6, 0, 1, -1, -1)
    assert cursor["endNs"] == "0"
    for k in ("bootSerial", "epoch", "window", "ownerTid"):
        assert cursor[k] == native[k]
    assert cursor["context"] == native["eglContext"] and current["handle"] == cursor["handle"]
    observed_at = int(current["enterNs"]) + int(native["inCallDurationNs"])
    open_lower_bound = observed_at - int(cursor["beginNs"])
    assert open_lower_bound == 32233489688
    compile_event = next(e for e in native["events"] if e["stage"] == "COMPILE")
    source = native["programs"][0]
    assert source["compileLog"]["queried"] and not source["linkLog"]["queried"]
    old = read("docs/architecture-T682-contract-v1.json")
    package = artifact("T590-cursor2")
    assert package["signedHashMatches"]
    module = next(m for m in package["modules"] if m["entry"].endswith("libgrsm1.so"))
    assert any(item["sha256"] == source["sha256"] and item["bytes"] == source["sourceBytes"]
               for item in module["embeddedGLSLSources"])
    handoff = artifact("T676-original-a")
    assert handoff["signedHashMatches"]
    originals = read("test/reports/t676-original-a/original-identity.json")
    assert {m["entry"]: (m["sha256"], m["buildId"]) for m in handoff["modules"] if m["entry"].endswith(".so")} == {
        name: (m["sha256"], m["BuildId"]) for name, m in originals["native"].items()}
    return {
        "v4Manifest": record(manifest_path), "v4Native": record(native_path),
        "v4RawLogs": [record(f'{base}/{entry["name"]}') for entry in manifest["rawLogs"]],
        "declaration": manifest["declaration"], "packageSignedSha256": package["actualSignedSHA"],
        "packageLibrary": {k: v for k, v in module.items() if k != "embeddedGLSLSources"},
        "followUpSignedA": {"receipt": handoff["receipt"], "signedSha256": handoff["actualSignedSHA"],
                            "nativeIdentity": record("test/reports/t676-original-a/original-identity.json"),
                            "device": "NOT_RUN_IN_THIS_AUDIT"},
        "loadedDeviceIdentity": "NOT_CAPTURED", "preArmAndContinuousTail": "NOT_CAPTURED",
        "nativeScope": {k: native[k] for k in ("bootSerial", "epoch", "window", "eglContext", "ownerTid",
                                               "candidateRun", "layoutSha", "combinedShaderSha", "nativeBeginNs")},
        "apiCursor": cursor, "lastCompletedStage": native["events"][-1], "programs": native["programs"],
        "compileStageNs": int(compile_event["exitNs"]) - int(compile_event["enterNs"]),
        "observedAtDerivedNativeNs": observed_at, "openLowerBoundNativeNs": open_lower_bound,
        "attemptAgeNativeNs": observed_at - int(native["nativeBeginNs"]),
        "oldComparators": old["verdict"], "oldEvidence": record("docs/analysis-T682-evidence-v1.json"),
        "singleApiOpenObservationsAcrossRoles": 2, "sameRoleRepeatedSingleApiProven": False,
        "permanentDeadlockProven": False, "productPASS": False,
    }


if __name__ == "__main__":
    out = {"schema": "T702_T705_ORIGINAL_BYTE_ATTRIBUTION_V1", "deviceUsed": False,
           "T702": {"dm": dm_evidence(), "v5": v5_evidence()}, "T703": host_evidence(), "T705": link_evidence()}
    write("docs/analysis-T702-T705-evidence-v1.json", out)
    print("T702 DM/V5 originals, T703 both pairs, T705 original cursor/manifest verified")
