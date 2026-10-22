#!/usr/bin/env python3
"""P-012 Q1 G1 primary-scale static measurement run (frozen v2.0 caliber, tester).

First G1 data run under measurement-frozen-appendix-P012-v2 (FROZEN, manager
approved 10:53) + q1-input-manifest v1.0. Order discipline: REFERENCE FIRST
(bands from the reference set ONLY - f0/f5 static, f2/f6 dynamic), then the
s99w pairs are compared against those bands. EAB/OFF data never participates
in band formation.

Outputs (descriptive, NOT acceptance verdicts):
- reference single-frame values + descriptive min/max bands (static and
  dynamic bands kept separate); only 2 primary frames per state -> single
  frame values + min/max, never presented as confidence intervals
- s99w three pairs x {off, eab} (w1 anchor frames) on both tracks:
  hfH_native_v1 (v1 ROI, bit-compatible) and hfH_display_v2_proposed
  (primary 200px + sensitivity 100px columns)
- SUPPORT_LIMITED annotation with valid-row/valid-pair counts for the thin
  device sand band
- all numbers are descriptive; the manager three-way ruling happens later

Exit codes: 0 ok, 1 usage/selftest error.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

PRIMARY_INNER_WIDTH_PX = 200
SENSITIVITY_INNER_WIDTH_PX = 100
METHOD_ID_NATIVE = "hfH-native-v1"
METHOD_ID_DISPLAY = "hfH-display-v2-p1"
ART = Path(r"D:/hmprojects/HelloWorld/test/artifacts/p012-measurements")
BASE = Path(r"D:/hmprojects/HelloWorld/test/baselines")

# Frozen reference geometry (manifest s3.2/s4.2 final masks).
REF_FRAMES = {
    "video1_frame0.jpg": {"cols": (42, 252), "rows": (169, 314), "state": "static"},
    "video1_frame5.jpg": {"cols": (10, 236), "rows": (150, 304), "state": "static"},
    "video1_frame2.jpg": {"cols": (53, 257), "rows": (210, 314), "state": "dynamic"},
    "video1_frame6.jpg": {"cols": (7, 229), "rows": (150, 305), "state": "dynamic"},
}
# Frozen device geometry (manifest s3.1 + s4.1 final 22-row mask).
DEV_INNER = (178, 888, 1098, 1976)   # inner box x0,y0,x1,y1 (inclusive)
DEV_SAND_ROWS = (1950, 1971)         # v2 22-row mask (y<=1971)
DEV_INNER_W = DEV_INNER[2] - DEV_INNER[0] + 1   # 921

# v1 native ROI (dual-track preserved as-is).
DEV_V1_ROI = (240, 1950, 1035, 1976)
REF_V1_ROI = (30, 190, 230, 300)


def file_sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def v1_lum(px):
    return 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2]


def v1_gray(img, box):
    crop = img.crop(box).convert("RGB")
    w, h = crop.size
    px = crop.load()
    return [[v1_lum(px[x, y]) for x in range(w)] for y in range(h)], w, h


def v1_hf_h(ys):
    n = 0
    s = 0.0
    for row in ys:
        for x in range(1, len(row)):
            s += abs(row[x] - row[x - 1])
            n += 1
    return s / n


def v1_std(ys):
    vals = [v for row in ys for v in row]
    n = len(vals)
    mean = sum(vals) / n
    return math.sqrt(sum((v - mean) ** 2 for v in vals) / n)


def float_y(img):
    rgb = img.convert("RGB")
    w, h = rgb.size
    px = rgb.load()
    ys = [[0.0] * w for _ in range(h)]
    for y in range(h):
        row = ys[y]
        for x in range(w):
            r, g, b = px[x, y]
            row[x] = 0.299 * r + 0.587 * g + 0.114 * b
    return ys, w, h


def area_coverage_resample(ys, src_w, src_h, dst_w, dst_h=None):
    if dst_h is None:
        dst_h = max(1, int(round(dst_w * src_h / src_w)))
    sx = src_w / dst_w
    sy = src_h / dst_h
    out = [[0.0] * dst_w for _ in range(dst_h)]
    for dy in range(dst_h):
        y0, y1 = dy * sy, (dy + 1) * sy
        iy0, iy1 = int(math.floor(y0)), min(int(math.ceil(y1)), src_h)
        for dx in range(dst_w):
            x0, x1 = dx * sx, (dx + 1) * sx
            ix0, ix1 = int(math.floor(x0)), min(int(math.ceil(x1)), src_w)
            acc = 0.0
            area = 0.0
            for iy in range(iy0, iy1):
                wy = min(y1, iy + 1) - max(y0, iy)
                if wy <= 0:
                    continue
                row = ys[iy]
                for ix in range(ix0, ix1):
                    wx = min(x1, ix + 1) - max(x0, ix)
                    if wx <= 0:
                        continue
                    wgt = wx * wy
                    acc += row[ix] * wgt
                    area += wgt
            out[dy][dx] = acc / area if area > 0 else 0.0
    return out, dst_w, dst_h


def mask_adjacent_pair_hfh(ys, mask):
    n = 0
    s = 0.0
    rows_with_pairs = 0
    for y, row in enumerate(ys):
        mrow = mask[y]
        row_pairs = 0
        for x in range(1, len(row)):
            if mrow[x] and mrow[x - 1]:
                s += abs(row[x] - row[x - 1])
                n += 1
                row_pairs += 1
        if row_pairs:
            rows_with_pairs += 1
    if n == 0:
        return None, 0, 0
    return s / n, n, rows_with_pairs


def gdom_mask(crop, col_range, row_range):
    """Reference sand mask (manifest final): pure sand cols x rows intersection.
    Source-mask in crop coordinates; gdom criterion G>R and G>B."""
    w, h = crop.size
    px = crop.load()
    mask = [[False] * w for _ in range(h)]
    cx0, cx1 = col_range
    ry0, ry1 = row_range
    for y in range(max(0, ry0), min(h, ry1 + 1)):
        for x in range(max(0, cx0), min(w, cx1 + 1)):
            r, g, b = px[x, y]
            if g > r and g > b:
                mask[y][x] = True
    return mask


def display_track(img, inner_box, mask_kind, mask_rects, tier):
    """v2 display operator on one image (frozen s2/s3)."""
    x0, y0, x1, y1 = inner_box
    inner_w = x1 - x0 + 1
    dst_w = PRIMARY_INNER_WIDTH_PX if tier == "primary" else SENSITIVITY_INNER_WIDTH_PX
    crop = img.crop((x0, y0, x1 + 1, y1 + 1)).convert("RGB")
    ys, w, h = float_y(crop)
    rs, rw, rh = area_coverage_resample(ys, w, h, dst_w)
    if mask_kind == "gdom":
        px = crop.load()
        src_mask = [[(px[x, y][1] > px[x, y][0]) and (px[x, y][1] > px[x, y][2])
                     for x in range(w)] for y in range(h)]
    else:
        src_mask = [[False] * w for _ in range(h)]
        for rx0, ry0, rx1, ry1 in mask_rects:
            for y in range(max(0, ry0), min(h, ry1)):
                row = src_mask[y]
                for x in range(max(0, rx0), min(w, rx1)):
                    row[x] = True
    cov, mw, mh = area_coverage_resample(
        [[1.0 if v else 0.0 for v in row] for row in src_mask], w, h, rw)
    mask = [[v >= 1.0 - 1e-9 for v in row] for row in cov]
    hf, pairs, vrows = mask_adjacent_pair_hfh(rs, mask)
    vals = [v for y, row in enumerate(rs) for x, v in enumerate(row) if mask[y][x]]
    lum_std = None
    if vals:
        mean = sum(vals) / len(vals)
        lum_std = math.sqrt(sum((v - mean) ** 2 for v in vals) / len(vals))
    return {
        "methodId": METHOD_ID_DISPLAY,
        "tier": tier,
        "dstWidthPx": rw,
        "dstHeightPx": rh,
        "srcInnerWidthPx": inner_w,
        "hfH": "N/A" if hf is None else round(hf, 4),
        "validPairs": pairs,
        "validRows": vrows,
        "supportLimited": bool(vrows < 8),
        "lumStdSameMask": round(lum_std, 2) if lum_std is not None else "N/A",
    }


def native_track(img, roi):
    ys, w, h = v1_gray(img, tuple(roi))
    return {
        "methodId": METHOD_ID_NATIVE,
        "roi": list(roi),
        "hfH": round(v1_hf_h(ys), 3),
        "lumStd": round(v1_std(ys), 2),
        "validPairs": (w - 1) * h,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", default=str(ART / "q1-g1-primary-run.json"))
    args = ap.parse_args()

    from PIL import Image
    out = {
        "run": "Q1-G1-primary-static-v1",
        "caliber": "measurement-frozen-appendix-P012-v2 (FROZEN v2.0) + manifest v1.0",
        "order": "reference first (bands from reference set only), then comparison",
        "nature": "DESCRIPTIVE ONLY - not an acceptance verdict; single frame "
                  "values + min/max are NOT confidence intervals",
        "reference": {}, "comparison": {}, "bands": {},
    }

    # ---- Step 1: reference set (band formation, reference frames ONLY) ----
    ref_results = {}
    for fname, geo in REF_FRAMES.items():
        path = BASE / fname
        img = Image.open(path)
        col_w = geo["cols"][1] - geo["cols"][0] + 1
        # inner box for the resample = pure-sand column range x full sand row
        # range (perspective discipline: scale ratio from pure-sand width;
        # no straightening, no anisotropic warp).
        inner = (geo["cols"][0], geo["rows"][0], geo["cols"][1], geo["rows"][1])
        res = {
            "sha256": file_sha256(path),
            "state": geo["state"],
            "pureSandColWidth": col_w,
            "native": native_track(img, REF_V1_ROI),
            "displayPrimary": display_track(
                img, inner, "gdom", None, "primary"),
            "displaySensitivity": {
                "note": "perspective sensitivity at width ends; primary "
                        "ratio uses this frame's pure-sand width",
                "srcWidthMin": col_w, "srcWidthMax": col_w,
            },
        }
        ref_results[fname] = res
    out["reference"] = ref_results

    # Bands: ONLY reference frames. static = f0/f5; dynamic = f2/f6.
    for state, frames in (("static", ["video1_frame0.jpg", "video1_frame5.jpg"]),
                          ("dynamic", ["video1_frame2.jpg", "video1_frame6.jpg"])):
        vals_nat = [ref_results[f]["native"]["hfH"] for f in frames]
        vals_disp = [ref_results[f]["displayPrimary"]["hfH"] for f in frames]
        out["bands"][state] = {
            "frames": frames,
            "hfH_native_v1": {"perFrame": vals_nat, "min": min(vals_nat),
                              "max": max(vals_nat),
                              "note": "descriptive min/max of 2 frames, NOT a CI"},
            "hfH_display_v2_proposed": {
                "perFrame": vals_disp, "min": min(vals_disp), "max": max(vals_disp),
                "note": "descriptive min/max of 2 frames, NOT a CI"},
        }

    # ---- Step 2: s99w three pairs (comparison; never fed into the bands) ----
    # w1 = anchor measurement frame (v1 archive values exist); w0/w2 measured on
    # the display track for frame-spread context (static 25s frozen state).
    for pair in (1, 2, 3):
        for arm in ("off", "eab"):
            for widx in (0, 1, 2):
                path = ART / f"eab-s99w-pair{pair}-{arm}-w{widx}.jpeg"
                img = Image.open(path)
                mask_rects = [(240 - DEV_INNER[0], DEV_SAND_ROWS[0] - DEV_INNER[1],
                               1035 - DEV_INNER[0] + 1,
                               DEV_SAND_ROWS[1] - DEV_INNER[1] + 1)]
                res = {
                    "sha256": file_sha256(path),
                    "anchorFrame": widx == 1,
                    "native": native_track(img, DEV_V1_ROI),
                    "displayPrimary": display_track(
                        img, DEV_INNER, "rect", mask_rects, "primary"),
                    "displaySensitivity": display_track(
                        img, DEV_INNER, "rect", mask_rects, "sensitivity"),
                }
                out["comparison"][f"s99w-pair{pair}-{arm}-w{widx}"] = res

    # ---- Step 3: descriptive comparison positions (NOT an acceptance verdict) ----
    def position(value, band):
        lo, hi = band["min"], band["max"]
        if value < lo:
            return "below"
        if value > hi:
            return "above"
        return "within"

    stat = out["bands"]["static"]
    dyn = out["bands"]["dynamic"]
    # anchor frames only (w1) summarized vs bands; per-frame values identical
    # within each (pair, arm) group (deterministic render, frozen static state).
    comparison_summary = {}
    for pair in (1, 2, 3):
        for arm in ("off", "eab"):
            res = out["comparison"][f"s99w-pair{pair}-{arm}-w1"]
            nat = res["native"]["hfH"]
            disp = res["displayPrimary"]["hfH"]
            sens = res["displaySensitivity"]["hfH"]
            comparison_summary[f"s99w-pair{pair}-{arm}"] = {
                "native": {
                    "value": nat,
                    "staticBand": [stat["hfH_native_v1"]["min"], stat["hfH_native_v1"]["max"]],
                    "staticPosition": position(nat, stat["hfH_native_v1"]),
                    "dynamicBand": [dyn["hfH_native_v1"]["min"], dyn["hfH_native_v1"]["max"]],
                    "dynamicPosition": position(nat, dyn["hfH_native_v1"]),
                },
                "display200": {
                    "value": disp,
                    "staticBand": [stat["hfH_display_v2_proposed"]["min"],
                                   stat["hfH_display_v2_proposed"]["max"]],
                    "staticPosition": position(disp, stat["hfH_display_v2_proposed"]),
                    "dynamicBand": [dyn["hfH_display_v2_proposed"]["min"],
                                    dyn["hfH_display_v2_proposed"]["max"]],
                    "dynamicPosition": position(disp, dyn["hfH_display_v2_proposed"]),
                },
                "display100": {
                    "value": sens,
                    "note": "no reference band at sensitivity tier (revision s2.4: "
                            "downscale sensitivity, never mixed with primary)",
                },
                "support": {
                    "validRows200": res["displayPrimary"]["validRows"],
                    "validPairs200": res["displayPrimary"]["validPairs"],
                    "SUPPORT_LIMITED": res["displayPrimary"]["supportLimited"],
                    "note": "22 source rows -> 3 fully-supported 200px rows "
                            "(strict full-area-support rule; ~4.8 expected from "
                            "raw row ratio, 2 more rows are partial-support)",
                },
            }
    out["comparisonSummary"] = comparison_summary

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out).write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n",
                              encoding="utf-8")
    # console summary
    print("== reference bands (reference set ONLY) ==")
    for state, b in out["bands"].items():
        print(f"  {state}: native {b['hfH_native_v1']['perFrame']} "
              f"[{b['hfH_native_v1']['min']}, {b['hfH_native_v1']['max']}]  "
              f"display {b['hfH_display_v2_proposed']['perFrame']} "
              f"[{b['hfH_display_v2_proposed']['min']}, {b['hfH_display_v2_proposed']['max']}]")
    print("== s99w comparison (descriptive) ==")
    for key, res in out["comparison"].items():
        d = res["displayPrimary"]
        s = res["displaySensitivity"]
        anchor = "*" if res.get("anchorFrame") else " "
        print(f"  {key}{anchor}: native={res['native']['hfH']} "
              f"display200={d['hfH']} (pairs={d['validPairs']}, rows={d['validRows']}, "
              f"SL={d['supportLimited']}) display100={s['hfH']} "
              f"(pairs={s['validPairs']}, rows={s['validRows']}, SL={s['supportLimited']})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
