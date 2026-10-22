#!/usr/bin/env python3
"""P-012 Q1 revision runner v2 (tester): G1 completion + G2/G3/G4/G5.

Lands the review-P012-Q1-G1-descriptive-v1.md §5 revision list:

R1  Depth strata + matched-patch: whole-mask values are KEPT and explicitly
    labeled "wholeMask"; reference upper/middle/lower strata are computed per
    the frozen three-way split (design §3.1/3.2, middle = primary) AND a
    device-height-matched patch (same normalized height as the device band,
    middle-positioned) with its support counts. NO post-hoc selection of the
    layer closest to the product: the middle stratum is primary by the frozen
    rule and both are reported.
R2  Reference 100px sensitivity: same source image, same rules, DIRECT to
    100px (never 200->100 chained); reported as its own column with its own
    descriptive range, never mixed with the 200px tier.
R3  Configuration assets: explicit srcInnerBox, mask endpoint convention
    (inclusive x1/y1 both sides, per manifest), tool versions + SHA-256,
    decoder version, derived asset hashes.
R4  Copy relationship resolved: this runner IMPORTS the frozen standard
    operator functions (measure_render_p012_q1.py) as the single source of
    truth and adds an equivalence receipt (recomputed anchors must match the
    committed G1 JSON values).
R5  G2 (scale sensitivity) / G3 (encoding Q75/90 + 4:2:0, same encoder
    version) / G4 (s96 tilt 0..15 display results) / G5 (matched-support
    autocorrelation + normalized spectrum, frequency in cycles per container
    inner width) appended; existing G1 outputs are NOT overwritten.

Method constraints honored: single-pass area-coverage resample; mask-before-
statistics; descriptive min/max never CI; static/dynamic separated; the
s96 dynamic group never merged with s99w static.
"""
import argparse
import hashlib
import importlib.util
import io
import json
import math
import platform
import sys
from pathlib import Path

ART = Path(r"D:/hmprojects/HelloWorld/test/artifacts/p012-measurements")
BASE = Path(r"D:/hmprojects/HelloWorld/test/baselines")
TOOLS = Path(r"D:/hmprojects/HelloWorld/tools")

# Import the frozen standard operator as the SINGLE SOURCE (review R4).
_spec = importlib.util.spec_from_file_location(
    "measure_render_p012_q1", TOOLS / "measure_render_p012_q1.py")
_q1 = importlib.util.module_from_spec(_spec)
sys.modules["measure_render_p012_q1"] = _q1
_spec.loader.exec_module(_q1)

PRIMARY_INNER_WIDTH_PX = _q1.PRIMARY_INNER_WIDTH_PX          # 200
SENSITIVITY_INNER_WIDTH_PX = _q1.SENSITIVITY_INNER_WIDTH_PX  # 100
METHOD_ID_NATIVE = _q1.METHOD_ID_NATIVE
METHOD_ID_DISPLAY = _q1.METHOD_ID_DISPLAY

REF_FRAMES = {
    "video1_frame0.jpg": {"cols": (42, 252), "rows": (169, 314), "state": "static"},
    "video1_frame5.jpg": {"cols": (10, 236), "rows": (150, 304), "state": "static"},
    "video1_frame2.jpg": {"cols": (53, 257), "rows": (210, 314), "state": "dynamic"},
    "video1_frame6.jpg": {"cols": (7, 229), "rows": (150, 305), "state": "dynamic"},
}
# f6 width erratum (review R-text): 229-7+1 = 223 (was misprinted 226 in the
# G1 report prose; the script/JSON always used 223).
DEV_INNER = (178, 888, 1098, 1976)
DEV_SAND_ROWS = (1950, 1971)     # inclusive both ends (22 rows)
DEV_SAND_COLS = (240, 1035)      # inclusive both ends (796 cols) - explicit
                                 # endpoint convention (review R3)
DEV_INNER_W = DEV_INNER[2] - DEV_INNER[0] + 1   # 921
DEV_V1_ROI = (240, 1950, 1035, 1976)  # PIL box semantics: x1/y1 EXCLUSIVE
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
    mean = sum(vals) / len(vals)
    return math.sqrt(sum((v - mean) ** 2 for v in vals) / len(vals))


def native_track(img, roi):
    ys, w, h = v1_gray(img, tuple(roi))
    return {
        "methodId": METHOD_ID_NATIVE,
        "roi": list(roi),
        "roiEndpointConvention": "PIL box: x1/y1 exclusive",
        "hfH": round(v1_hf_h(ys), 3),
        "lumStd": round(v1_std(ys), 2),
        "validPairs": (w - 1) * h,
    }


def gdom_mask_from_crop(crop):
    w, h = crop.size
    px = crop.load()
    return [[(px[x, y][1] > px[x, y][0]) and (px[x, y][1] > px[x, y][2])
             for x in range(w)] for y in range(h)]


def rect_mask(crop, rects):
    w, h = crop.size
    mask = [[False] * w for _ in range(h)]
    for rx0, ry0, rx1, ry1 in rects:
        for y in range(max(0, ry0), min(h, ry1)):
            row = mask[y]
            for x in range(max(0, rx0), min(w, rx1)):
                row[x] = True
    return mask


def display_measure(img, inner_box, mask_kind, mask_rects, tier,
                    depth_patch_rows=None):
    """Single-source display measurement via the frozen standard operator.

    depth_patch_rows: optional (rows_full, rows_strict) override to compute a
    device-height-matched patch on top of the standard three-way strata.
    Returns the standard dict plus R1/R3 extension fields.
    """
    x0, y0, x1, y1 = inner_box
    inner_w = x1 - x0 + 1
    dst_w = (PRIMARY_INNER_WIDTH_PX if tier == "primary"
             else SENSITIVITY_INNER_WIDTH_PX)
    crop = img.crop((x0, y0, x1 + 1, y1 + 1)).convert("RGB")
    ys, w, h = _q1.float_y(crop)
    rs, rw, rh = _q1.area_coverage_resample(ys, w, h, dst_w)
    if mask_kind == "gdom":
        src_mask = gdom_mask_from_crop(crop)
    else:
        src_mask = rect_mask(crop, mask_rects)
    cov, mw, mh = _q1.area_coverage_resample(
        [[1.0 if v else 0.0 for v in row] for row in src_mask], w, h, rw)
    mask = [[v >= 1.0 - 1e-9 for v in row] for row in cov]
    hf, pairs, vrows = _q1.mask_adjacent_pair_hfh(rs, mask)
    strata = _q1.depth_strata(mask, rs)
    strata_out = {}
    for name, rng in strata.items():
        if not rng:
            strata_out[name] = {"rows": [], "hfH": "N/A", "validPairs": 0,
                                "validRows": 0}
            continue
        sub = [[False] * rw for _ in range(rh)]
        for y in range(rng[0], rng[1] + 1):
            sub[y] = mask[y]
        shf, spairs, srows = _q1.mask_adjacent_pair_hfh(rs, sub)
        strata_out[name] = {
            "rows": rng, "hfH": "N/A" if shf is None else round(shf, 4),
            "validPairs": spairs, "validRows": srows,
        }
    # R1: device-height-matched patch (middle-positioned) - same normalized
    # height as the device band, centered vertically in the masked range.
    matched = None
    if depth_patch_rows:
        full, strict = depth_patch_rows
        rows_all = [y for y, mrow in enumerate(mask) if any(mrow)]
        if rows_all and strict >= 1:
            top, bot = rows_all[0], rows_all[-1]
            center = (top + bot) // 2
            half = strict // 2
            p0, p1 = center - half, center - half + strict - 1
            p0 = max(p0, top)
            p1 = min(p1, bot)
            if p1 >= p0:
                sub = [[False] * rw for _ in range(rh)]
                for y in range(p0, p1 + 1):
                    sub[y] = mask[y]
                phf, ppairs, prows = _q1.mask_adjacent_pair_hfh(rs, sub)
                matched = {
                    "rule": "device-band normalized height, middle-positioned "
                            "(frozen selection rule, no post-hoc choice)",
                    "requestedRows": strict,
                    "rows": [p0, p1],
                    "hfH": "N/A" if phf is None else round(phf, 4),
                    "validPairs": ppairs,
                    "validRows": prows,
                    "supportLimited": bool(prows < 8),
                }
    vals = [v for y, row in enumerate(rs) for x, v in enumerate(row)
            if mask[y][x]]
    lum_std = None
    if vals:
        mean = sum(vals) / len(vals)
        lum_std = math.sqrt(sum((v - mean) ** 2 for v in vals) / len(vals))
    result = {
        "methodId": METHOD_ID_DISPLAY,
        "tier": tier,
        "dstWidthPx": rw,
        "dstHeightPx": rh,
        "srcInnerBox": [x0, y0, x1, y1],
        "srcInnerWidthPx": inner_w,
        "srcInnerEndpointConvention": "inclusive x1/y1 both sides (manifest)",
        "hfH": "N/A" if hf is None else round(hf, 4),
        "hfHLabel": "wholeMask",
        "validPairs": pairs,
        "validRows": vrows,
        "supportLimited": bool(vrows < 8),
        "lumStdSameMask": round(lum_std, 2) if lum_std is not None else "N/A",
        "depthStrata": strata_out,
        "depthStrataRule": "three-way split of masked rows, middle=primary "
                           "(frozen); wholeMask kept and labeled separately",
    }
    if matched is not None:
        result["matchedPatch"] = matched
    return result


def normalized_spectrum(ys, mask, inner_width_dst):
    """G5 diagnostic: mean-removed row-band power spectrum, frequency in
    cycles per container inner width. Welch-free single FFT per row band;
    diagnostic only, never acts on hfH."""
    import numpy as np
    rows = [y for y, mrow in enumerate(mask) if sum(mrow) >= 32]
    if not rows:
        return {"note": "insufficient rows"}
    sig = np.array([[row[x] if mask[y][x] else float("nan")
                     for x in range(len(row))] for y, row in
                    [(y, ys[y]) for y in rows]])
    # fill masked-out with row mean to keep FFT simple (diagnostic only)
    out = {}
    band = sig[~np.isnan(sig).all(axis=1)]
    band = np.nan_to_num(band, nan=float(np.nanmean(band)))
    band = band - band.mean(axis=1, keepdims=True)
    spec = np.abs(np.fft.rfft(band, axis=1)).mean(axis=0)
    n = band.shape[1]
    freqs = np.fft.rfftfreq(n, d=1.0)  # cycles per pixel
    top = np.argsort(spec)[::-1][:6]
    out["cyclesPerContainerWidth"] = [
        {"cycles": round(float(freqs[i] * inner_width_dst), 2),
         "relPower": round(float(spec[i] / spec[1:].max() if spec[1:].max() > 0 else 0), 4)}
        for i in sorted(top) if i > 0]
    out["unit"] = "cycles per container inner width (200px)"
    return out


def jpeg_reencode(img, quality):
    """G3: re-encode to JPEG with the given quality + 4:2:0, decode back.
    Same encoder version (PIL), recorded. Never feeds band formation."""
    from PIL import Image
    buf = io.BytesIO()
    img.convert("RGB").save(buf, format="JPEG", quality=quality,
                            subsampling=0)  # 0 = 4:4:4? PIL: 2 = 4:2:0
    buf2 = io.BytesIO()
    img.convert("RGB").save(buf2, format="JPEG", quality=quality,
                            subsampling=2)  # 4:2:0
    return Image.open(buf2).convert("RGB")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", default=str(ART / "q1-revision-run-v2.json"))
    ap.add_argument("--groups", default="G1,G2,G3,G4,G5",
                    help="comma list of groups to run")
    args = ap.parse_args()
    groups = {g.strip().upper() for g in args.groups.split(",")}

    from PIL import Image
    import PIL
    out = {
        "run": "Q1-revision-v2 (G1 completion + G2..G5)",
        "review": "review-P012-Q1-G1-descriptive-v1.md §5",
        "singleSource": {
            "module": "tools/measure_render_p012_q1.py",
            "sha256": file_sha256(TOOLS / "measure_render_p012_q1.py"),
            "note": "runner imports the frozen operator functions; "
                    "copy-drift risk resolved (review R4)",
        },
        "config": {
            "python": sys.version.split()[0],
            "platform": platform.platform(),
            "pillow": PIL.__version__,
            "decoder": f"PIL {PIL.__version__} JPEG decoder",
            "primaryWidthPx": PRIMARY_INNER_WIDTH_PX,
            "sensitivityWidthPx": SENSITIVITY_INNER_WIDTH_PX,
            "deviceInnerBox": list(DEV_INNER),
            "deviceSandRowsInclusive": list(DEV_SAND_ROWS),
            "deviceSandColsInclusive": list(DEV_SAND_COLS),
            "endpointConvention": "inner box and mask rects: inclusive x1/y1 "
                                  "both sides; native v1 PIL ROI: exclusive",
            "maskRule": "reference: gdom per-pixel (G>R and G>B); device: "
                        "frozen rect rows 1950..1971 inclusive",
            "derivedAssets": "display PNG previews not regenerated this run; "
                             "float operator values remain the measurement "
                             "input (revision s3.3)",
        },
        "reference": {}, "comparison": {}, "bands": {}, "groups": {},
    }

    # ---------- G1 completion (R1/R2/R3) ----------
    if "G1" in groups:
        ref = {}
        for fname, geo in REF_FRAMES.items():
            path = BASE / fname
            img = Image.open(path)
            inner = (geo["cols"][0], geo["rows"][0], geo["cols"][1], geo["rows"][1])
            col_w = geo["cols"][1] - geo["cols"][0] + 1
            res = {
                "sha256": file_sha256(path),
                "state": geo["state"],
                "pureSandColWidth": col_w,
                "native": native_track(img, REF_V1_ROI),
                "displayPrimary": display_measure(img, inner, "gdom", None,
                                                  "primary"),
                # R2: direct-to-100px sensitivity (same image, same rules)
                "displaySensitivity": display_measure(img, inner, "gdom",
                                                      None, "sensitivity"),
            }
            ref[fname] = res
        out["reference"] = ref
        for state, frames in (("static", ["video1_frame0.jpg", "video1_frame5.jpg"]),
                              ("dynamic", ["video1_frame2.jpg", "video1_frame6.jpg"])):
            def col(key):
                return [ref[f][key]["hfH"] for f in frames]
            out["bands"][state] = {
                "frames": frames,
                "hfH_native_v1": {"perFrame": col("native"),
                                  "min": min(col("native")), "max": max(col("native"))},
                "hfH_display_v2_proposed_primary200": {
                    "perFrame": col("displayPrimary"),
                    "min": min(col("displayPrimary")),
                    "max": max(col("displayPrimary"))},
                "hfH_display_v2_proposed_sensitivity100": {
                    "perFrame": col("displaySensitivity"),
                    "min": min(col("displaySensitivity")),
                    "max": max(col("displaySensitivity"))},
                "middleStratumPrimary": {
                    "perFrame": [ref[f]["displayPrimary"]["depthStrata"]["middle"]["hfH"]
                                 for f in frames],
                    "note": "frozen middle stratum = primary depth position"},
                "note": "descriptive min/max of 2 frames, NOT a CI; tiers and "
                        "strata reported separately, never mixed",
            }
        # device side: matched patch + strata
        comp = {}
        for pair in (1, 2, 3):
            for arm in ("off", "eab"):
                path = ART / f"eab-s99w-pair{pair}-{arm}-w1.jpeg"
                img = Image.open(path)
                rects = [(DEV_SAND_COLS[0] - DEV_INNER[0],
                          DEV_SAND_ROWS[0] - DEV_INNER[1],
                          DEV_SAND_COLS[1] - DEV_INNER[0] + 1,
                          DEV_SAND_ROWS[1] - DEV_INNER[1] + 1)]
                res = {
                    "sha256": file_sha256(path),
                    "native": native_track(img, DEV_V1_ROI),
                    "displayPrimary": display_measure(
                        img, DEV_INNER, "rect", rects, "primary",
                        depth_patch_rows=(22, 3)),
                    "displaySensitivity": display_measure(
                        img, DEV_INNER, "rect", rects, "sensitivity",
                        depth_patch_rows=(22, 1)),
                }
                comp[f"s99w-pair{pair}-{arm}"] = res
        out["comparison"] = comp

    # ---------- G2: scale sensitivity (reference direct-to-100px) ----------
    if "G2" in groups:
        g2 = {}
        for fname, geo in REF_FRAMES.items():
            r = out["reference"].get(fname)
            if r is None:
                continue
            g2[fname] = {
                "state": geo["state"],
                "primary200": r["displayPrimary"]["hfH"],
                "sensitivity100": r["displaySensitivity"]["hfH"],
                "ratio100over200": (
                    round(r["displaySensitivity"]["hfH"] /
                          r["displayPrimary"]["hfH"], 4)
                    if isinstance(r["displayPrimary"]["hfH"], float)
                    and r["displayPrimary"]["hfH"] else "N/A"),
            }
        out["groups"]["G2"] = {
            "note": "same source image, same rules, DIRECT to 100px; own "
                    "descriptive range; never mixed with 200px tier",
            "reference": g2,
        }

    # ---------- G3: encoding sensitivity ----------
    if "G3" in groups:
        g3 = {}
        targets = []
        for fname, geo in REF_FRAMES.items():
            inner = (geo["cols"][0], geo["rows"][0], geo["cols"][1], geo["rows"][1])
            targets.append((fname, BASE / fname, inner, "gdom"))
        for pair in (1, 2, 3):
            for arm in ("off", "eab"):
                rects = [(DEV_SAND_COLS[0] - DEV_INNER[0],
                          DEV_SAND_ROWS[0] - DEV_INNER[1],
                          DEV_SAND_COLS[1] - DEV_INNER[0] + 1,
                          DEV_SAND_ROWS[1] - DEV_INNER[1] + 1)]
                targets.append((f"s99w-pair{pair}-{arm}",
                                ART / f"eab-s99w-pair{pair}-{arm}-w1.jpeg",
                                DEV_INNER, ("rect", rects)))
        for name, path, inner, mk in targets:
            img = Image.open(path)
            row = {}
            for q in (75, 90):
                reimg = jpeg_reencode(img, q)
                if isinstance(mk, tuple):
                    m = display_measure(reimg, inner, mk[0], mk[1], "primary")
                else:
                    m = display_measure(reimg, inner, mk, None, "primary")
                row[f"q{q}"] = {"sha256": file_sha256_from_bytes(
                    _jpeg_bytes(img, q)), "hfH": m["hfH"],
                    "validPairs": m["validPairs"]}
            g3[name] = row
        out["groups"]["G3"] = {
            "note": "JPEG re-encode Q75/90, 4:2:0, same encoder (PIL "
                    f"{PIL.__version__}); sensitivity only; never feeds band "
                    "formation; does not undo original compression history",
            "results": g3,
        }

    # ---------- G4: dynamic (s96 tilt 0..15) ----------
    if "G4" in groups:
        g4 = {}
        for i in range(16):
            path = ART / f"dyn-s96-tilt-{i}.jpeg"
            if not path.exists():
                continue
            img = Image.open(path)
            rects = [(DEV_SAND_COLS[0] - DEV_INNER[0],
                      DEV_SAND_ROWS[0] - DEV_INNER[1],
                      DEV_SAND_COLS[1] - DEV_INNER[0] + 1,
                      DEV_SAND_ROWS[1] - DEV_INNER[1] + 1)]
            g4[f"tilt-{i}"] = {
                "sha256": file_sha256(path),
                "displayPrimary": display_measure(img, DEV_INNER, "rect",
                                                  rects, "primary",
                                                  depth_patch_rows=(22, 3)),
            }
        out["groups"]["G4"] = {
            "note": "s96 dynamic group kept SEPARATE from s99w static "
                    "(different build); full ordered sequence, no merging",
            "build": "entry-default-s96-signed.hap (non-PERF-fix build)",
            "results": g4,
        }

    # ---------- G5: matched-support diagnostics ----------
    if "G5" in groups:
        g5 = {}
        for fname, geo in REF_FRAMES.items():
            inner = (geo["cols"][0], geo["rows"][0], geo["cols"][1], geo["rows"][1])
            crop = Image.open(BASE / fname).crop(
                (inner[0], inner[1], inner[2] + 1, inner[3] + 1)).convert("RGB")
            ys, w, h = _q1.float_y(crop)
            rs, rw, rh = _q1.area_coverage_resample(ys, w, h, 200)
            mask = [[v for v in row] for row in
                    _q1.area_coverage_resample(
                        [[1.0 if v else 0.0 for v in row]
                         for row in gdom_mask_from_crop(crop)], w, h, rw)[0]]
            mask = [[v >= 1.0 - 1e-9 for v in row] for row in mask]
            g5[fname] = {
                "autocorrLag1to16": _q1.autocorr_normalized(rs, mask),
                "spectrum": normalized_spectrum(rs, mask, 200),
            }
        for pair in (1, 2, 3):
            for arm in ("off", "eab"):
                img = Image.open(ART / f"eab-s99w-pair{pair}-{arm}-w1.jpeg")
                crop = img.crop((DEV_INNER[0], DEV_INNER[1],
                                 DEV_INNER[2] + 1, DEV_INNER[3] + 1)).convert("RGB")
                ys, w, h = _q1.float_y(crop)
                rs, rw, rh = _q1.area_coverage_resample(ys, w, h, 200)
                rects = [(DEV_SAND_COLS[0] - DEV_INNER[0],
                          DEV_SAND_ROWS[0] - DEV_INNER[1],
                          DEV_SAND_COLS[1] - DEV_INNER[0] + 1,
                          DEV_SAND_ROWS[1] - DEV_INNER[1] + 1)]
                cov, _, _ = _q1.area_coverage_resample(
                    [[1.0 if v else 0.0 for v in row]
                     for row in rect_mask(crop, rects)], w, h, rw)
                mask = [[v >= 1.0 - 1e-9 for v in row] for row in cov]
                g5[f"s99w-pair{pair}-{arm}"] = {
                    "autocorrLag1to16": _q1.autocorr_normalized(rs, mask),
                    "spectrum": normalized_spectrum(rs, mask, 200),
                    "note": "thin band: autocorr/spectrum from 3 fully-"
                            "supported rows only; treat as diagnostic",
                }
        out["groups"]["G5"] = {
            "note": "diagnostic only, no gates; frequency unit = cycles per "
                    "container inner width (200px); lag1-16 autocorr does not "
                    "claim to represent the full spectrum",
            "results": g5,
        }

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out).write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n",
                              encoding="utf-8")
    print(json.dumps({"out": str(args.out),
                      "groups": sorted(groups),
                      "referenceFrames": len(out.get("reference", {})),
                      "comparison": len(out.get("comparison", {}))},
                     ensure_ascii=False))
    return 0


def _jpeg_bytes(img, quality):
    buf = io.BytesIO()
    img.convert("RGB").save(buf, format="JPEG", quality=quality, subsampling=2)
    return buf.getvalue()


def file_sha256_from_bytes(data):
    return hashlib.sha256(data).hexdigest()


if __name__ == "__main__":
    raise SystemExit(main())
