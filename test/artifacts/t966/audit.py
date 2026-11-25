"""Recompute T965 findings from existing bytes. This script starts no product or GPU work."""

from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import statistics
import struct

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
Q20 = 1 << 20


def digest(data):
    return hashlib.sha256(data).hexdigest()


def u64(words, index):
    return words[index] | words[index + 1] << 32


def g16(bundle, artifacts, frozen):
    manifest_bytes = (frozen / "g16-manifest.json").read_bytes()
    manifest = json.loads(manifest_bytes)
    output = {"manifest_sha256": digest(manifest_bytes), "runs": {}}
    if (bundle / "manifest.json").exists():
        assert (bundle / "manifest.json").read_bytes() == manifest_bytes
        for name, entry in manifest["files"].items():
            data = (bundle / name).read_bytes()
            assert len(data) == entry["bytes"] and digest(data) == entry["sha256"]
    for arm in ("B13_FREE", "B13_SUPPORT", "THIRD", "THIRD_MIRROR"):
        run = frozen / ("T960_" + arm)
        config_bytes = (run / "initial-config.u32le").read_bytes()
        completion_bytes = (run / "late-completion.u32le").read_bytes()
        c = struct.unpack("<256I", config_bytes)
        r = struct.unpack("<256I", completion_bytes)
        pairs = ((4, 8, 2), (10, 10, 6), (16, 20, 2), (52, 4, 1), (57, 5, 1), (88, 64, 40))
        differences = [[dst+i, src+i, r[dst+i], c[src+i]]
                       for dst, src, size in pairs for i in range(size) if r[dst+i] != c[src+i]]
        assert [row[0] for row in differences] == [126, 127]
        assert r[:4] == (0x36314743, 1, 256, 0xC016CAFE) and r[86:88] == c[62:64]
        assert not any(r[128:])
        if (bundle / run.name).exists():
            assert (bundle / run.name / "completions.u32le").stat().st_size == 0
            assert not (bundle / run.name / "certified-tail-v1.u32le").exists()
        evidence = {"config_sha256": digest(config_bytes), "completion_sha256": digest(completion_bytes),
                    "seed_sha256": digest((run / "seed.disk64").read_bytes()),
                    "run": u64(r, 4), "nonce": u64(r, 6), "fence_generation": u64(r, 34),
                    "branch_inputs": list(c[62:64]), "module_sha_completion_prefix": struct.pack("<6I", *r[120:126]).hex(),
                    "outcome_reason_stage_micro": list(r[40:44]), "bank_words": list(r[48:52]),
                    "segments": r[58], "iterations": r[59], "candidates": r[54],
                    "identity_differences_dst_src_observed_expected": differences}
        stdout_path = artifacts / ("T960_" + arm + ".txt")
        if stdout_path.exists():
            stdout = {row["stage"]: row for row in map(json.loads, stdout_path.read_text().splitlines())}
            hot = stdout["G16_PRIVATE_ROOT"]
            late = stdout["G16_PRIVATE_LATE_TAIL"]
            assert int(hot["run"]) == evidence["run"] and int(hot["nonce"]) == evidence["nonce"]
            assert hot["fence"] == late["originalFence"] and int(late["originalGeneration"]) == evidence["fence_generation"]
            assert not hot["identityOK"] and not late["identityOK"]
            assert hot["privateCommit"] == "0" and hot["acceptedRoot"] is None
            evidence["stdout"] = {"sha256": digest(stdout_path.read_bytes()), "private_root": hot,
                                  "late_tail": late, "fixture": stdout.get("G16_THIRD_IMPACT_FIXTURE")}
        output["runs"][arm] = evidence
    return output


def demo(log, artifacts):
    data = gzip.decompress(log.read_bytes()) if log.suffix == ".gz" else log.read_bytes()
    rows = list(map(json.loads, data.splitlines()))
    pointer = [r for r in rows if r.get("stage") == "WIN32_POINTER"]
    downs = [r for r in pointer if r["kind"] == "DOWN"]
    ups = [r for r in pointer if r["kind"] == "UP"]
    origin = int(downs[0]["qpc"])
    hz = int(next(r["qpcFrequency"] for r in rows if "qpcFrequency" in r))
    sources, quanta, presents, rejected_sources = {}, {}, {}, {}
    for number, r in enumerate(rows, 1):
        s = r.get("m1Completion")
        if r.get("state") == "M1_SOURCE_REVOKED" and r.get("m1SourceAuthority", {}).get("reason"):
            rejected_sources.setdefault(int(r["m1Nonce"]), r["m1SourceAuthority"]["reason"])
        if s and s[27] == 1 and "m1SourceAuthority" in r:
            a = r["m1SourceAuthority"]
            nonce = u64(s, 4)
            if (r.get("m1FenceVkResult") == 0 and r.get("m1OwnerCode") == 0 and
                    a.get("acceptedRoot") == str(nonce) and s[28] == 1 and s[61] > 0):
                t = (int(r["m1FenceNs"]) - origin*100) / 1e9
                cmd = struct.pack("<32I", *r["m1SourceWords"])
                x, y, requested = struct.unpack_from("<3d", cmd, 16*4)
                sources.setdefault(nonce, {"line": number, "t": t, "hold": int(r["m1Hold"]),
                                          "pointer_seq": int(r["m1PointerSeq"]), "commit": u64(s, 18),
                                          "births": s[66], "admitted_q20": s[61], "rejected_q20": s[62],
                                          "position": [x, y], "requested_units": requested,
                                          "pulse_delta_ns": int(r["m1SourceDeltaNs"]),
                                          "offer_seq": int(r["m1ProducerWords"][8])})
            elif a.get("reason"):
                rejected_sources.setdefault(nonce, a["reason"])
        q = r.get("m1QuantumCompletion")
        if q and q[27] == 2 and "m1QuantumAuthority" in r:
            a = r["m1QuantumAuthority"]
            nonce = u64(q, 4)
            if (r.get("m1QuantumFenceVkResult") == 0 and r.get("m1QuantumOwnerCode") == 0 and
                    a.get("acceptedRoot") == str(nonce) and q[28] == 1):
                quanta.setdefault(nonce, {"line": number, "t": (int(r["m1QuantumFenceNs"]) - origin*100) / 1e9,
                                         "serial": u64(q, 150), "stock": u64(q, 71),
                                         "active": q[88], "free": q[89],
                                         "dispatches": q[110], "source_commit": u64(q, 18),
                                         "submit_to_fence_ms": (int(r["m1QuantumFenceNs"]) / 100 - int(a["submitExitQpc"])) / hz * 1e3})
        view, frame = r.get("m1CompactView"), r.get("windowFrame")
        if view and frame and frame.get("acceptedRoot") not in (None, "0"):
            presents[int(frame["acceptedRoot"])] = {"frame": frame, "view": view}
    assert hz == 10_000_000
    sq = sorted(quanta.values(), key=lambda q: q["serial"])
    assert sq and all(q["serial"] == i for i, q in enumerate(sq, 1))
    source_prefix = {0: 0}
    for s in sorted(sources.values(), key=lambda s: s["commit"]):
        source_prefix[s["commit"]] = source_prefix[s["commit"]-1] + s["admitted_q20"]
    stock_mismatches = [q for q in sq if q["stock"] != source_prefix[q["source_commit"]]]
    assert not stock_mismatches, stock_mismatches[:2]

    def window(begin, end):
        ss = [s for s in sources.values() if begin <= s["t"] < end]
        qs = [q for q in sq if begin <= q["t"] < end]
        visible = [p for p in presents.values() if begin <= (int(p["frame"]["presentExitQpc"]) - origin) / hz < end]
        return {"from_to": [begin, end], "source_roots": len(ss), "births": sum(s["births"] for s in ss),
                "admitted_q20": sum(s["admitted_q20"] for s in ss),
                "admitted_units": sum(s["admitted_q20"] for s in ss) / Q20,
                "requested_units": sum(s["requested_units"] for s in ss),
                "accepted_source_pulse_ns": sum(s["pulse_delta_ns"] for s in ss),
                "rejected_q20_in_accepted_sources": sum(s["rejected_q20"] for s in ss),
                "accepted_quanta": len(qs), "sim_seconds": len(qs) / 120, "accepted_present_rows": len(visible),
                "submit_to_fence_ms_median": statistics.median(q["submit_to_fence_ms"] for q in qs) if qs else None,
                "first_quantum_in_window": qs[0] if qs else None,
                "last_quantum_in_window": qs[-1] if qs else None}

    pixels = {}
    for f in sorted(artifacts.glob("b5-*.png")):
        with Image.open(f) as im:
            rows = Counter()
            points = []
            rgb = iter(im.convert("RGB").tobytes())
            for i, pixel in enumerate(zip(rgb, rgb, rgb, strict=True)):
                if pixel == (184, 145, 89):
                    x, y = i % im.width, i // im.width
                    points.append((x, y))
                    rows[y] += 1
            pixels[f.name] = {"sha256": digest(f.read_bytes()), "extent": list(im.size), "sand_pixels": len(points),
                              "bounds_xyxy": [min(x for x,y in points), min(y for x,y in points),
                                               max(x for x,y in points), max(y for x,y in points)] if points else None,
                              "row_counts": dict(sorted(rows.items()))}
    launcher = ROOT / ".codex/local-test/t960-integrated-demo"
    manifest_bytes = (log.parent / "demo-manifest.json").read_bytes()
    manifest = json.loads(manifest_bytes)
    if (launcher / "manifest.json").exists():
        assert (launcher / "manifest.json").read_bytes() == manifest_bytes
        for entry in manifest["files"]:
            item = (launcher / entry["name"]).read_bytes()
            assert len(item) == entry["bytes"] and digest(item) == entry["sha256"]
    assert all(snap["view"]["client"] == [624, 431] for snap in presents.values())
    assert all(q["dispatches"] == 35 for q in sq)
    return {"log_file": log.relative_to(ROOT).as_posix(), "log_sha256": digest(data), "lines": len(data.splitlines()),
            "demo_manifest_sha256": digest(manifest_bytes),
            "input": [{"hold": int(d["hold"]), "down_qpc": d["qpc"], "up_qpc": u["qpc"],
                       "duration_s": (int(u["qpc"]) - int(d["qpc"])) / hz,
                       "down_xy": [d["x"], d["y"]],
                       "moves": sum(r["kind"] == "MOVE" and r["hold"] == d["hold"] for r in pointer)}
                      for d, u in zip(downs, ups)],
            "windows": [window(0, 4), window(0, (int(ups[0]["qpc"]) - origin) / hz),
                        window((int(downs[1]["qpc"]) - origin) / hz, (int(ups[1]["qpc"]) - origin) / hz),
                        window(0, 60), window(60, 64), window(64, 74)],
            "all_sources": len(sources), "all_quanta": len(quanta), "all_presents": len(presents),
            "completion_phase_words_140_145_nonzero_roots": sum(
                any(r.get("m1QuantumCompletion", [])[140:146]) for r in rows),
            "first_no_active_after_first_up": next(q for q in sq if q["t"] >= (int(ups[0]["qpc"]) - origin) / hz and q["active"] == 0),
            "source_rejections": dict(Counter(rejected_sources.values())),
            "sources_by_nonce": sources, "first_present": next(iter(presents.values())), "pixels": pixels}


if __name__ == "__main__":
    artifacts = ROOT / "test/artifacts/t965-g16-demo-20261125"
    bundle = ROOT / ".codex/local-test/t960-g16-bundle"
    frozen = Path(__file__).resolve().parent
    log = frozen / "demo-r4.jsonl.gz"
    evidence = {"g16": g16(bundle, artifacts, frozen), "demo": demo(log, artifacts)}
    print(json.dumps(evidence, indent=2, ensure_ascii=True))
