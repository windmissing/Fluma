"""Static original-byte checks for the four architecture cards."""
import json
from pathlib import Path
import re
import zipfile

from analyze_t660_t662 import ROOT, artifact, build_id, record, sha, ui_texts, write


def read(path):
    return json.loads((ROOT / path).read_bytes())


def package(task, module):
    result = artifact(task)
    assert result["signedHashMatches"]
    return {"signedSha": result["actualSignedSHA"], "receipt": result["receipt"],
            "modules": [{k: v for k, v in m.items() if k != "embeddedGLSLSources"}
                        for m in result["modules"] if m["entry"].endswith((module, "modules.abc"))]}


def signed_handoff(name):
    path = f"test/reports/t676-same-{name}/handoff.json"
    d = read(path)
    hap = Path(d["signed"]["path"])
    assert sha(hap.read_bytes()) == d["signed"]["sha256"]
    modules = {}
    with zipfile.ZipFile(hap) as z:
        for entry, expected in d["entries"].items():
            raw = z.read(entry)
            assert sha(raw) == expected["sha256"] and len(raw) == expected["bytes"]
            if entry.endswith(".so"):
                assert build_id(raw) == expected["buildId"]
            modules[entry] = expected
    return {"receipt": record(path), "signedSha": d["signed"]["sha256"], "entries": modules}


def audit():
    packs = {n: package(n, so) for n, so in (
        ("T594-rx", "libgrs_g9rx.so"), ("T687-rx", "libgrs_g9rx.so"), ("T669-v5", "libgrsm1.so"),
        ("T590-cursor2", "libgrsm1.so"), ("T597-fixed", "libgrsm1.so"), ("T593-probe", "libgrsm1.so"))}
    so = lambda p: next(m for m in packs[p]["modules"] if m["entry"].endswith(".so"))
    assert so("T594-rx") == so("T687-rx")
    assert so("T597-fixed") == so("T593-probe")
    source = (".codex/local-test/t674-delivery-v2/entry/.cxx/default/default/debug/arm64-v8a/"
              "gpu_resident/g9rx/rx_assets/pipeline.comp")
    raw = (ROOT / source).read_bytes()
    lines = raw.decode().splitlines()
    assert len(lines) == 2144 and len(raw) == 145458
    assert "g9AddMass(C[at+3u],q,next)" in lines[1062]
    assert "g9AddMass(W[at+3u],mass,next)" in lines[1521]
    assert "g9ProvisionalAdd" in lines[1547] and "W[at+3u]" in lines[1547]
    sig = read("test/reports/t257/T594-rx.json")
    with zipfile.ZipFile(sig["signed"]["path"]) as z:
        assert raw + b"\0" in z.read(so("T594-rx")["entry"])
    log = "test/reports/t491/t594b/h594.txt"
    error_lines = [i for i, x in enumerate((ROOT / log).read_bytes().splitlines(), 1)
                   if b"0:1063: S0001: Function call discards 'coherent'" in x]
    assert error_lines
    rx_layout = list(ui_texts(read("test/reports/t491/t594b/l2.json")))
    loaded_text = next(x for x in rx_layout if x.startswith("CONFIGURE_FAILED_8 {"))
    loaded = json.loads(loaded_text.split(" ", 1)[1])
    assert loaded["memoryBuildIdMatched"] is True
    assert loaded["BuildId"] == so("T594-rx")["buildId"] and loaded["sha256"] == so("T594-rx")["sha256"]

    same = {x: signed_handoff(x) for x in ("a", "b")}
    key = "libs/arm64-v8a/libgrsm1.so"
    assert same["a"]["entries"][key]["buildId"] != so("T590-cursor2")["buildId"]
    assert same["b"]["entries"][key]["buildId"] != so("T597-fixed")["buildId"]
    t593 = "test/reports/t491/t593c/"
    boot4 = read(t593 + "t676b-boot2.json")
    boot5 = read(t593 + "t676b-boot3.json")
    assert boot4["pid"] != boot5["pid"]
    assert boot5["recovery"]["phase"] == "READY" and boot5["attempts"][-1]["readyEpoch"] == "4"
    current = [x for x in ui_texts(read(t593 + "p6.json")) if "sample 5981" in x]
    assert any("CURRENT" in x and "NONE" in x for x in current)
    assert any("0:649" in x for x in current)
    layouts = [record(p.relative_to(ROOT).as_posix()) for p in (ROOT / t593).glob("*.json")]

    t677 = "test/reports/t677/"
    replay = read(t677 + "receipt-replay.json")
    for n, cmp in replay["comparisons"].items():
        assert cmp["receiptSHA"] == cmp["historicalSHA"] == record("test/reports/t666/" + n)["sha256"]
    ninja = read(t677 + "independent-current-ninja.json")
    assert ninja["exit"] == 0 and "broker_compile/anchor.cpp" in ninja["stdout"]
    assert "libgrs_foundation.a" in ninja["stdout"] and "libgrsT666Consumer.so" not in ninja["stdout"]
    receipt = read(t677 + "grs_cold_slots.link-receipt.json")
    assert receipt["manifestSHA256"] == record(t677 + "grs_cold_slots.link.json")["sha256"]
    assert receipt["rspSHA256"] == record(t677 + "grs_cold_slots.full.rsp")["sha256"]
    assert receipt["diagnosticSHA256"] == record(t677 + "grs_cold_slots.driver-diagnostic.log")["sha256"]
    assert receipt["returnCode"] == 0 and len(receipt["argv"]) == 2
    original = (ROOT / "tools/build/build_receipt.py").read_text(encoding="utf-8")
    assert all(x in original for x in ("t666-delivery.log", "t666-final-incremental.log", "final ninja no work to do"))
    dynamic = (ROOT / (t677 + "dynamic.txt")).read_text(encoding="utf-8")
    ids = re.findall(r"Build ID: ([0-9a-f]+)", dynamic)
    assert len(ids) == 2 and ids[0].startswith("38d4c72d")

    resources = []
    for k, kw in [(192, 64), (192, 128), (512, 64), (512, 128)]:
        pad = 256 if k == 192 else 512
        words = 16*k + 16*kw + 2048 + 256
        angles = 32 + pad + 2*kw + 127*16 + 512 + k + kw
        work = angles + 61444 + 4096*16
        capacity = 4*(48*k + 16*kw + 254) + k
        resources.append({"K": k, "Kw": kw, "QCandidateB": words*4, "baseWorkspaceB": work*4,
                          "fullM1WorkspaceB": (work+words+256)*4, "T663M2WorkspaceB": (work+words+256+8192)*4,
                          "singleQArenaB": capacity*256})
    paths = ["docs/algorithm-T689-gpu-create-program-reuse-v1.md", "docs/design-T689-static-evidence-v1.json",
             "entry/src/main/cpp/gpu_resident/m1/quantum_candidate.cpp",
             "entry/src/main/cpp/gpu_resident/m1/full_transport.cpp",
             "entry/src/main/cpp/gpu_resident/m1/source_transport.cpp",
             "entry/src/main/cpp/gpu_resident/program_cache/program_factory.h",
             "tools/m2/prepare_cursor_build.cjs", "test/t468/generate.cjs", "test/t475/generate.cjs"]
    return {"schema": "T691_T696_ORIGINAL_EVIDENCE_V1", "deviceUsed": False, "buildRun": False,
            "productRun": False, "T691": {"packages": {k: packs[k] for k in packs if k in ("T594-rx", "T687-rx", "T669-v5")},
                "source": record(source), "sourceLines": {str(i): lines[i-1] for i in [978,979,980,1019,1063,1522,1548]},
                "driverLog": record(log), "driverErrorLines": error_lines, "deviceLoadedModule": loaded,
                "layout": [record(f"test/reports/t491/t594b/l{i}.json") for i in (1,2)]},
            "T694": {"baselines": {k: packs[k] for k in ("T590-cursor2", "T597-fixed", "T593-probe")},
                     "supplemental": same, "v4Pid": boot4["pid"], "v5Pid": boot5["pid"],
                     "v5Recovery": {k: boot5["recovery"][k] for k in ["phase","epoch","session","resources"]},
                     "sameAttemptV4V5": False, "scopeEvidence": layouts, "currentUI": current,
                     "collector": [record(p) for p in ["test/t676/prepare.py","test/t676/T676Collector.ets","test/t676/audit_capture.py"]]},
            "T695": {"replay": record(t677 + "receipt-replay.json"), "comparison": replay["comparisons"],
                     "actualNinja": record(t677 + "independent-current-ninja.json"),
                     "independentLog": record(t677 + "independent.log"), "linkReceipt": record(t677 + "grs_cold_slots.link-receipt.json"),
                     "baselineElfSHA": receipt["ELFSHA256"], "buildIds": ids,
                     "script": record("tools/build/build_receipt.py")},
            "T696": {"inputs": [record(p) for p in paths], "resourceArithmetic": resources,
                     "measuredDeviceBenefits": False}}


if __name__ == "__main__":
    write("docs/analysis-T691-T696-evidence-v1.json", audit())
    print("T691/T694/T695/T696 original bytes and static contracts verified.")
