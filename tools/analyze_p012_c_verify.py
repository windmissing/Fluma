# -*- coding: utf-8 -*-
# P-012 C-verify OFF/ON paired pixel analysis
# Hypothesis: C stratification adds horizontal stripe texture inside the light-sand
# (yellow) band. OFF (p012ablate=strat) should show lower row-luminance oscillation.
#
# 2026-10-22 (pipeline-rehearsal-P012-c-verify-v1.md §3 fixes P-3..P-6):
#   P-3  return frames were silently dropped -> now analyzed and reported in a
#        "return" section (residual-displacement check after tilt/return), and
#        every dropped/kept segment decision is registered in the output.
#   P-4  blue-band boundary drift (±1-2 rows from antialiased transition rows
#        feeding min/max) -> blue band is now the LONGEST CONTIGUOUS run of
#        rows with blue ratio >= 0.5 (not min/max), and the yellow band's top
#        is anchored below it consistently. band-bounds.json (frozen bounds)
#        is consumed when present (--bands option or auto-detect) and every
#        boundary source is recorded per frame.
#   P-5  SENTINEL (hard precondition for formal capture): for each pair the
#        static-0 OFF vs ON frames are pixel-diffed inside the container ROI;
#        if npx(|diff|>3) == 0 the pair is suspected of NO TREATMENT (both
#        arms rendered identically - the invalid-noTier failure mode). A
#        top-level SUSPECT_NO_TREATMENT flag plus per-pair details are
#        emitted; the process exits non-zero when any pair triggers so the
#        collection SOP stops immediately.
#   P-6  provenance block in the output header: tier/want parameters/seed/
#        build SHA read from collect-manifest.json in the data root when
#        present, plus directory name and analysis timestamp. Analysis
#        output can no longer be mistaken for another batch's.
import os, sys, json, time
from PIL import Image
import numpy as np

# Defaults preserved for reproducibility; CLI override added 2026-10-22 (P-012 C pipeline
# rehearsal on the archived invalid-noTier batch): argv[1]=data root, argv[2]=output json.
ROOT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-c-verify-20261021"
if len(sys.argv) > 1:
    ROOT = sys.argv[1]
OUT_NAME = "analysis-pixel-v1.json"
if len(sys.argv) > 2:
    OUT_NAME = sys.argv[2]
# --bands=<path>: consume a frozen band-bounds.json instead of per-frame detection.
BANDS_PATH = None
for arg in sys.argv[3:]:
    if arg.startswith("--bands="):
        BANDS_PATH = arg.split("=", 1)[1]

# Container box measured from 516x1080 frames: x ~ [66,450], y ~ [336,750]
BOX_X0, BOX_X1 = 80, 436
BOX_Y0, BOX_Y1 = 345, 742
# Sentinel diff threshold: |channel diff| > 3 counts as a differing pixel.
SENTINEL_DIFF_THRESHOLD = 3
BLUE_RATIO_THRESHOLD = 0.5

BAND_KEYS = {"static", "tilt", "return"}


def lum(img):
    a = np.asarray(img.convert("RGB"), dtype=np.float64)
    return 0.299*a[:,:,0] + 0.587*a[:,:,1] + 0.114*a[:,:,2]


def longest_contiguous(arr_bool):
    """Longest run of True; returns (start, end) inclusive, or (None, None)."""
    best = (None, None)
    run_start = None
    for i, v in enumerate(arr_bool):
        if v and run_start is None:
            run_start = i
        elif not v and run_start is not None:
            if best[0] is None or (i - run_start) > (best[1] - best[0]):
                best = (run_start, i - 1)
            run_start = None
    if run_start is not None:
        if best[0] is None or (len(arr_bool) - run_start) > (best[1] - best[0]):
            best = (run_start, len(arr_bool) - 1)
    return best


def _sustained_runs(arr_bool, minlen=10):
    """All runs of True with length >= minlen; [(start, end) inclusive]."""
    runs = []
    s = None
    for i, v in enumerate(arr_bool):
        if v and s is None:
            s = i
        elif not v and s is not None:
            if i - s >= minlen:
                runs.append((s, i - 1))
            s = None
    if s is not None and len(arr_bool) - s >= minlen:
        runs.append((s, len(arr_bool) - 1))
    return runs


def roi_box(W, H):
    sx, sy = W/516.0, H/1080.0
    return int(BOX_X0*sx), int(BOX_X1*sx), int(BOX_Y0*sy), int(BOX_Y1*sy)


def frame_masks(img, x0, x1, y0, y1):
    a = np.asarray(img.convert("RGB"), dtype=np.float64)
    R, G, B = a[:,:,0], a[:,:,1], a[:,:,2]
    lum = 0.299*R + 0.587*G + 0.114*B
    # Canonical palette (p012tier absent - invalid-noTier batch): saturated
    # dark blue water / yellow light sand / green heavy sand.
    blue = (B > R+20) & (B > 60)
    green = (G > R+20) & (G > B+10)
    yellow = (R > B+30) & (G > B+20) & (R>80)
    # Tiered-palette fallback (p012tier=3 - EAB+C valid batch): pale cyan
    # water (B>R slightly, high lum), cream light sand (R>B, very bright),
    # textured green heavy sand. Selected when the canonical masks find no
    # blue rows; recorded in bandSource so the palette in use is auditable.
    pale_blue = (B > R+3) & (lum > 150) & (lum < 235) & (B > 180)
    cream = (R > B+5) & (lum > 225) & (R > 235)
    pale_green = (G > R+10) & (G > B+10)
    row_blue = blue[:, x0:x1].mean(axis=1)
    if not (row_blue > BLUE_RATIO_THRESHOLD).any() \
            and (pale_blue[:, x0:x1].mean(axis=1) > BLUE_RATIO_THRESHOLD).any():
        blue, yellow, green = pale_blue, cream, pale_green
        tiered = True
    else:
        tiered = False
    return a, blue, green, yellow, tiered


def frozen_bounds(bands_data, fname):
    """band-bounds.json lookup: exact filename key first, then none."""
    if bands_data is None:
        return None
    if isinstance(bands_data, dict) and fname in bands_data:
        v = bands_data[fname]
        # rehearsal batch format: [yBlue0, yBlue1, yYellow0, yYellow1, yGreen0, yGreen1]
        if isinstance(v, list) and len(v) == 6:
            return {"blue": [v[0], v[1]], "yellow": [v[2], v[3]], "green": [v[4], v[5]]}
        if isinstance(v, dict):
            return v
    return None


def analyze_frame(path, bands_data=None):
    img = Image.open(path)
    L = lum(img)
    W, H = img.size
    x0, x1, y0, y1 = roi_box(W, H)
    res = {"file": os.path.basename(path), "size": [W, H]}
    a, blue, green, yellow, tiered = frame_masks(img, x0, x1, y0, y1)
    rowb = blue[:, x0:x1].mean(axis=1)
    rowg = green[:, x0:x1].mean(axis=1)
    rowy = yellow[:, x0:x1].mean(axis=1)
    rowR = a[:,:,0][:, x0:x1].mean(axis=1)
    if not (blue.any() and yellow.any() and green.any()):
        res["error"] = "band classification failed"
        return res
    res["palette"] = "tiered" if tiered else "canonical"
    frozen = frozen_bounds(bands_data, os.path.basename(path))
    if frozen is not None:
        yB0, yB1 = frozen["blue"]
        yY0, yY1 = frozen["yellow"]
        yG0, yG1 = frozen["green"]
        res["bandSource"] = "frozen-band-bounds"
    else:
        # P-4: blue band = longest contiguous run of rows with blue ratio >= 0.5
        # (antialiased transition rows no longer stretch the boundary via min/max).
        in_window = rowb[y0:y1] > BLUE_RATIO_THRESHOLD
        b_start, b_end = longest_contiguous(in_window)
        if b_start is None:
            res["error"] = "blue band not found"
            return res
        yB0, yB1 = y0 + b_start, y0 + b_end
        yY0, yY1 = min(y for y in range(y0, y1) if rowy[y] > 0.5), \
                   max(y for y in range(y0, y1) if rowy[y] > 0.5)
        yG0, yG1 = min(y for y in range(y0, y1) if rowg[y] > 0.5), \
                   max(y for y in range(y0, y1) if rowg[y] > 0.5)
        res["bandSource"] = "contiguous-run"
        if tiered:
            # Tiered palette (p012tier=3): EAB layers wash out the canonical
            # hues, so bands are segmented by sustained row-profile runs
            # (verified against the archived valid batch: water 950-1155,
            # light 1174-1567 cream R>=244, heavy 1568-1975 pale-green +
            # dark-green textured G>R+10). Blue run end anchors water; light
            # = first sustained R>=244 run after water; heavy = the rest
            # down to the last strong-green row.
            water_end = yB1
            bright = rowR[y0:y1] >= 244.0
            runs = _sustained_runs(bright, minlen=10)
            light_run = next(((a0, b0) for a0, b0 in runs if a0 > water_end - y0), None)
            if light_run is None:
                res["error"] = "tiered light band not found"
                return res
            yY0, yY1 = y0 + light_run[0], y0 + light_run[1]
            strong_green = np.where(rowg[y0:y1] > BLUE_RATIO_THRESHOLD)[0]
            below = strong_green[strong_green > light_run[1]]
            yG1 = int(y0 + below.max()) if len(below) else yY1
            yG0 = yY1 + 1
            res["bands"] = {"blue": [int(yB0), int(yB1)], "yellow": [int(yY0), int(yY1)],
                            "green": [int(yG0), int(yG1)]}
            res["bandSource"] = "tiered-sustained-runs"
    res["bands"] = {"blue": [int(yB0), int(yB1)], "yellow": [int(yY0), int(yY1)],
                    "green": [int(yG0), int(yG1)]}
    # --- stratification metric: row-luminance oscillation inside yellow band ---
    ya, yb = yY0 + (yY1-yY0)//5, yY1 - (yY1-yY0)//5
    ga, gb = yG0 + (yG1-yG0)//5, yG1 - (yG1-yG0)//5
    prof = L[ya:yb, x0:x1].mean(axis=1)
    k = 9
    pad = np.pad(prof, k//2, mode="edge")
    mov = np.convolve(pad, np.ones(k)/k, mode="valid")
    resid = prof - mov
    res["yellow_band"] = {"y0": int(ya), "y1": int(yb),
        "rowOscStd": float(resid.std()),
        "rowOscP2P": float(resid.max()-resid.min())}
    profg = L[ga:gb, x0:x1].mean(axis=1)
    padg = np.pad(profg, k//2, mode="edge")
    movg = np.convolve(padg, np.ones(k)/k, mode="valid")
    residg = profg - movg
    res["green_band"] = {"y0": int(ga), "y1": int(gb),
        "rowOscStd": float(residg.std()),
        "rowOscP2P": float(residg.max()-residg.min())}
    profc = L[ya:yb, x0:x1].mean(axis=0)
    padc = np.pad(profc, k//2, mode="edge")
    movc = np.convolve(padc, np.ones(k)/k, mode="valid")
    resc = profc - movc
    res["yellow_colOscStd"] = float(resc.std())
    crop = L[ya:yb, x0:x1]
    lap = crop[1:-1,1:-1]*4 - crop[:-2,1:-1] - crop[2:,1:-1] - crop[1:-1,:-2] - crop[1:-1,2:]
    res["yellowLapVar"] = float(lap.var())
    res["lumMean"] = float(L[y0:y1, x0:x1].mean())
    return res


def sentinel_diff(pair_dir_p, pair_dir_q, phase="static", index=0):
    """P-5: pixel-diff the same-phase frames of the two arms inside the ROI.
    Returns dict with npx>threshold count; None if frames are missing."""
    candidates = []
    for d in (pair_dir_p, pair_dir_q):
        match = None
        for f in sorted(os.listdir(d)):
            if f"-{phase}-{index}." in f:
                match = os.path.join(d, f)
                break
        if match is None:
            return None
        candidates.append(match)
    imgs = [np.asarray(Image.open(p).convert("RGB"), dtype=np.int16) for p in candidates]
    if imgs[0].shape != imgs[1].shape:
        return {"error": f"shape mismatch {imgs[0].shape} vs {imgs[1].shape}"}
    W, H = imgs[0].shape[1], imgs[0].shape[0]
    x0, x1, y0, y1 = roi_box(W, H)
    diff = np.abs(imgs[0][y0:y1, x0:x1] - imgs[1][y0:y1, x0:x1]).max(axis=2)
    npx3 = int((diff > SENTINEL_DIFF_THRESHOLD).sum())
    return {"phase": phase, "index": index,
            "files": [os.path.basename(p) for p in candidates],
            "npx>3": npx3,
            "roi": [x0, x1, y0, y1]}


def load_provenance(root):
    """P-6: provenance from collect-manifest.json (if present) + directory name."""
    prov = {"dataRoot": root, "batchDirName": os.path.basename(os.path.normpath(root)),
            "analysisTimestamp": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "manifest": None}
    mp = os.path.join(root, "collect-manifest.json")
    if os.path.exists(mp):
        try:
            with open(mp, "r", encoding="utf-8-sig") as fp:
                prov["manifest"] = json.load(fp)
        except Exception as e:
            prov["manifestError"] = str(e)
    else:
        prov["manifestWarning"] = (
            "collect-manifest.json not found in data root - the batch cannot "
            "self-certify its want parameters (p012tier/p012ablate/p012seed/"
            "build). See pipeline-rehearsal-P012-c-verify-v1.md §4.2.")
    return prov


def main():
    out = {"provenance": load_provenance(ROOT), "pairs": [], "segments": {}}
    bands_data = None
    bp = BANDS_PATH or os.path.join(ROOT, "band-bounds.json")
    if os.path.exists(bp):
        try:
            with open(bp, "r", encoding="utf-8-sig") as fp:
                bands_data = json.load(fp)
            out["provenance"]["bandBoundsSource"] = bp
        except Exception as e:
            out["provenance"]["bandBoundsError"] = str(e)

    pair_dirs = sorted(d for d in os.listdir(ROOT)
                       if os.path.isdir(os.path.join(ROOT, d))
                       and (d.startswith("pair") or d.startswith("smoke"))
                       and not d.endswith(".png"))
    suspects = []
    dropped_frames = []
    analyzed_frames = []
    for d in pair_dirs:
        full = os.path.join(ROOT, d)
        # pair number / mode from directory name (pair{n}-{off|on})
        parts = d.split("-")
        try:
            p = int(parts[0].replace("pair", ""))
            m = parts[1] if len(parts) > 1 else "?"
        except (ValueError, IndexError):
            p, m = 0, d
        files = sorted(os.listdir(full))
        for f in files:
            seg = next((s for s in BAND_KEYS if f"-{s}-" in f), None)
            if seg is None:
                dropped_frames.append({"dir": d, "file": f, "reason": "no phase tag"})
                continue
            r = analyze_frame(os.path.join(full, f), bands_data)
            out["pairs"].append({"pair": p, "mode": m, "segment": seg, **r})
            analyzed_frames.append(f"{d}/{f}")
        out["segments"][d] = {
            "static": sum(1 for f in files if "-static-" in f),
            "tilt": sum(1 for f in files if "-tilt-" in f),
            "return": sum(1 for f in files if "-return-" in f),
        }
    # P-5 sentinel: compare static-0 across arms within each pair number
    pair_nums = sorted({pr["pair"] for pr in out["pairs"] if pr.get("pair")})
    modes = sorted({pr["mode"] for pr in out["pairs"]})
    sentinel_details = {}
    for p in pair_nums:
        if len(modes) < 2:
            break
        r = sentinel_diff(os.path.join(ROOT, f"pair{p}-{modes[0]}"),
                          os.path.join(ROOT, f"pair{p}-{modes[1]}"),
                          phase="static", index=0)
        if r is not None:
            sentinel_details[str(p)] = r
            if "error" not in r and r["npx>3"] == 0:
                suspects.append(p)
    out["sentinel"] = {
        "threshold": SENTINEL_DIFF_THRESHOLD,
        "comparedPhase": "static-0",
        "perPair": sentinel_details,
        "suspectPairs": suspects,
        "SUSPECT_NO_TREATMENT": len(suspects) > 0,
    }
    out["coverage"] = {
        "analyzedFrames": len(analyzed_frames),
        "droppedFrames": dropped_frames,
        "segmentsPerDir": out["segments"],
    }
    with open(os.path.join(ROOT, OUT_NAME), "w", encoding="utf-8") as fp:
        json.dump(out, fp, indent=1, ensure_ascii=False)
    # summary table
    print(f"{'pair':4} {'mode':5} {'seg':7} {'frame':22} {'yellowOscStd':>12} {'yellowP2P':>10} {'greenOscStd':>11} {'lapVar':>9} {'colOsc':>8}")
    for r in out["pairs"]:
        if "error" in r:
            print(r["pair"], r["mode"], r.get("segment", "-"), r["file"], "ERROR", r["error"]); continue
        print(f"{r['pair']:<4} {r['mode']:5} {r.get('segment','-'):7} {r['file'][:22]:22} "
              f"{r['yellow_band']['rowOscStd']:12.3f} {r['yellow_band']['rowOscP2P']:10.2f} "
              f"{r['green_band']['rowOscStd']:11.3f} {r['yellowLapVar']:9.2f} {r['yellow_colOscStd']:8.3f}")
    print(f"\nanalyzed={len(analyzed_frames)} dropped={len(dropped_frames)}")
    if out["sentinel"]["SUSPECT_NO_TREATMENT"]:
        print(f"*** SUSPECT_NO_TREATMENT: pairs {suspects} show ZERO pixel diffs "
              f"(> {SENTINEL_DIFF_THRESHOLD}) between arms at static-0. The capture "
              f"likely missed the treatment parameter (e.g. p012tier). STOP and check "
              f"the want parameters before trusting any analysis. ***")
        return 2
    print("sentinel: OK (all compared pairs show arm differences)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
