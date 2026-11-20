"""Audit the original G11 package, Ark bytecode and saved device text offline."""
import json
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory
import zipfile

from analyze_t660_t662 import ROOT, build_id, record, sha, ui_texts, write


SIGNING = "test/reports/t257/T687-g11.json"
BUILD = "test/reports/t639/compile-receipt-v2.json"
DEVICE = "test/reports/t491/t600b"
RECORD = "com.example.helloworld.entry.ets.workers.G11ColdWorker"


def read_json(path):
    return json.loads((ROOT / path).read_bytes())


def inspect_hap(receipt):
    path = Path(receipt["path"])
    raw = path.read_bytes()
    assert sha(raw) == receipt["sha256"], path
    with zipfile.ZipFile(path) as package:
        assert package.testzip() is None
        abc = package.read("ets/modules.abc")
        native = package.read("libs/arm64-v8a/libgrs_g11.so")
        module = json.loads(package.read("module.json"))
        pages = json.loads(package.read("resources/base/profile/main_pages.json"))
        workers = [name for name in package.namelist() if name.startswith("ets/workers/")]
    entry = next(a for a in module["module"]["abilities"] if a["name"] == module["module"]["mainElement"])
    assert "pages/GpuResidentG11" in pages["src"]
    return {
        "path": str(path), "bytes": len(raw), "sha256": sha(raw), "zipCRC": "OK",
        "mergedAbc": {"bytes": len(abc), "sha256": sha(abc)}, "separateWorkerEntries": workers,
        "libgrs_g11": {"bytes": len(native), "sha256": sha(native), "buildId": build_id(native)},
        "moduleName": module["module"]["name"], "mainAbility": entry["name"],
        "abilitySource": entry["srcEntry"], "g11PageInProfile": True,
        "targetAPI": module["app"]["targetAPIVersion"],
    }, abc


def inspect_worker_bytecode(abc, tool):
    with TemporaryDirectory(prefix="t708-") as tmp:
        binary = Path(tmp, "modules.abc")
        assembly = Path(tmp, "modules.pa")
        binary.write_bytes(abc)
        subprocess.run([tool, binary, assembly], check=True, capture_output=True)
        text = assembly.read_text(encoding="utf-8")
    worker_main = re.search(rf"^\.function any {re.escape(RECORD)}\.func_main_0\([^\n]+\n(.*?)^\}}", text, re.M | re.S)
    assert f".record {RECORD} {{" in text and worker_main
    assert all(marker in worker_main[1] for marker in ("workerPort", "onmessage", "definefunc"))
    assert f'lda.str "entry/ets/workers/G11ColdWorker.ets"' in text
    callback = re.search(rf"^\.function any {re.escape(RECORD)}\.#\*#\([^\n]+\n(.*?)^\}}", text, re.M | re.S)
    assert callback and all(marker in callback[1] for marker in ('"BEGIN"', '"CANCEL"', '"collect"'))
    return {"tool": str(tool), "toolSha256": sha(tool.read_bytes()), "moduleRecord": RECORD,
            "executableMainInstallsOnmessage": True, "messageCallbackHandlesBeginAndCancel": True,
            "hostWorkerURL": "entry/ets/workers/G11ColdWorker.ets",
            "runtimeExecutionProven": False}


def device_evidence():
    log = f"{DEVICE}/h600.txt"
    markers = ("workers num 1", "Cannot attach current thread", "stageModule: 1",
               "G11ColdWorker.abc", "null safeData file", "host post nullptr to worker", "host receive terminate")
    lines = (ROOT / log).read_text(encoding="utf-8-sig").splitlines()
    hits = [{"line": n, "text": line} for n, line in enumerate(lines, 1)
            if re.search(r"\b48183\s+(48183|49307)\s", line) and any(m in line for m in markers)]
    assert len([row for row in hits if "G11ColdWorker.abc" in row["text"]]) == 3
    layouts = []
    for n in range(1, 5):
        path = f"{DEVICE}/l{n}.json"
        labels = list(ui_texts(read_json(path)))
        status = next(label for label in labels if label == "STOPPED\n等待有效新管线帧")
        assert "G11 · 接受通量组合研究" in labels
        state, presentation = status.splitlines()
        layouts.append({**record(path), "G11Page": True, "state": state, "presentedMappingDisplay": presentation})
    return {"report": record("docs/test-report-T600-v2.md"), "summary": record(f"{DEVICE}/v2-g11.json"),
            "hilog": record(log), "workerAttemptLog": hits, "layouts": layouts,
            "nativeConfigureReplyOrSameRunLoadedMap": "NOT_IN_HANDOFF",
            "workerOnmessageOrTerminalReceipt": "NOT_IN_HANDOFF"}


def main(tool):
    signing, build = read_json(SIGNING), read_json(BUILD)
    unsigned, unsigned_abc = inspect_hap(signing["input"])
    signed, signed_abc = inspect_hap(signing["signed"])
    assert unsigned["sha256"] == build["hap"]["sha256"]
    assert unsigned_abc == signed_abc and signed["mergedAbc"] == build["abc"]
    assert unsigned["libgrs_g11"] == signed["libgrs_g11"]
    assert signed["libgrs_g11"]["buildId"] == build["packagedNative"]["BuildId"]
    assert not signed["separateWorkerEntries"] and not unsigned["separateWorkerEntries"]
    project = Path(build["buildDirectory"])
    source_paths = [
        "entry/src/main/ets/entryability/G11ResearchAbility.ets",
        "entry/src/main/ets/pages/GpuResidentG11.ets",
        "entry/src/main/ets/physics/GpuG11Port.ets",
        "entry/src/main/ets/physics/GpuG11Cold.ets",
        "entry/src/main/ets/physics/G11ColdArchiveCore.ets",
        "entry/src/main/ets/workers/G11ColdWorker.ets",
        "entry/src/main/cpp/gpu_resident/g11vis/bridge.cpp",
        "entry/src/main/cpp/gpu_resident/g11vis/controller.cpp",
    ]
    for name in source_paths:
        assert sha((project / name).read_bytes()) == build["compiledSources"][name]["rawCompiledSHA256"]
    profile_path = (project / "entry/build-profile.json5").relative_to(ROOT).as_posix()
    loader_path = (project / "entry/build/default/intermediates/loader/default/loader.json").relative_to(ROOT).as_posix()
    profile, loader = read_json(profile_path), read_json(loader_path)
    assert "./src/main/ets/workers/G11ColdWorker.ets" in profile["buildOption"]["sourceOption"]["workers"]
    assert loader["compileMode"] == "esmodule" and str(project / source_paths[5]) in loader["workers"]
    out = {"schema": "T708_G11_ORIGINAL_BYTE_ATTRIBUTION_V1", "deviceUsed": False,
           "unsigned": unsigned, "signed": signed, "signingReceipt": record(SIGNING), "compileReceipt": record(BUILD),
           "buildProfile": record(profile_path), "effectiveLoader": {**record(loader_path), "compileMode": loader["compileMode"],
                                                                      "workers": loader["workers"]},
           "verifiedOriginalSource": {name: build["compiledSources"][name] for name in source_paths},
           "workerBytecode": inspect_worker_bytecode(signed_abc, tool), "deviceOriginals": device_evidence(),
           "verdict": {"separatePathLookupFailed": True, "workerMissingFromPackage": False,
                       "terminalWorkerLoadFailureProven": False, "workerOnConfigurePath": False,
                       "firstConfigureStopCause": "UNRESOLVED_NO_SAME_SCOPE_RAW_REPLY", "productPASS": False}}
    write("docs/analysis-T708-evidence-v1.json", out)
    print("T708 signed and unsigned bytes, executable worker module, build closure and device originals verified")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
