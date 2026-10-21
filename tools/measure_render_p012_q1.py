#!/usr/bin/env python3
"""P-012 Q1 display-scale hfH operator (revision v2-draft s2-s3, tester).

Implements the frozen display-scale measurement track SEPARATELY from the
v1 script (which stays untouched, SHA-256 03a80929...). Nothing here runs
against acceptance gates; the revision is DRAFT pending designer/manager
ruling, this tool exists so the operator can be self-tested and reviewed
before any reference band is generated.

Frozen operator (measurement-frozen-appendix-P012-v2-draft.md s2-s3):
- comparison unit = container INNER width; primary tier rescales the inner
  box to 200 analysis px (sensitivity tier = half), aspect preserved
- single-pass area-coverage resampling on float Y (Y=0.299R+0.587G+0.114B);
  no sharpening, no noise, no contrast normalization, no re-encode, no
  chained downscales
- mask-first statistics: a pixel pair (x-1,x) contributes only when BOTH
  pixels' area support falls fully inside the effective sand mask
- SUPPORT_LIMITED semantics: valid-row / valid-pair counts reported; no
  valid pair -> N/A (never synthesized by row replication)
- dual-track output: hfH_native_v1 (v1 logic re-implemented bit-compatibly,
  G0) + hfH_display_v2_proposed (this operator)
- depth strata for thick sand bodies: upper/middle/lower patches of the
  masked sand region (middle = primary, upper/lower = sensitivity)
- G5 diagnostics (same-mask lumStd + normalized autocorrelation), no gates

Usage:
  python tools/measure_render_p012_q1.py --selftest
  python tools/measure_render_p012_q1.py --manifest <q1-manifest.json> \
      --out <result.json> [--tier primary|sensitivity]

Exit codes: 0 ok, 1 selftest failure / usage error.
"""
import argparse
import hashlib
import json
import math
import sys
from pathlib import Path

# Frozen constants (revision draft s2/s3). Do not change without a new
# revision revision.
PRIMARY_INNER_WIDTH_PX = 200
SENSITIVITY_INNER_WIDTH_PX = 100
METHOD_ID_NATIVE = "hfH-native-v1"
METHOD_ID_DISPLAY = "hfH-display-v2-p1"


# ---------------------------------------------------------------------------
# v1 bit-compatible operators (G0 native track)
# ---------------------------------------------------------------------------

def v1_lum(px):
    r, g, b = px[0], px[1], px[2]
    return 0.299 * r + 0.587 * g + 0.114 * b


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
    return math.sqrt(sum((v - mean) ** 2 for v in vals) / n), mean


def native_track(img, sand_roi):
    """G0: bit-compatible v1 sand-box hfH/lumStd on the original ROI."""
    ys, w, h = v1_gray(img, tuple(sand_roi))
    hf = v1_hf_h(ys)
    std, _ = v1_std(ys)
    return {
        "methodId": METHOD_ID_NATIVE,
        "roi": list(sand_roi),
        "hfH": round(hf, 3),
        "lumStd": round(std, 2),
        "validPairs": (w - 1) * h,
    }


# ---------------------------------------------------------------------------
# Display-scale operator (v2 proposed track)
# ---------------------------------------------------------------------------

def float_y(img):
    """Decode to float Y per v1 formula (no integer rounding of inputs)."""
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
    """Single-pass area-coverage resample (box filter), float domain.

    Pixel (i,j) of the source covers the half-open interval
    [i/S, (i+1)/S) in normalized source coordinates; each destination
    pixel averages every source pixel's contribution weighted by the
    overlap area. One pass source->destination; fixed rounding at the
    final readout only (round-half-away-from-zero on export, floats kept
    in memory).
    """
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
    """hfH over left-adjacent pairs where BOTH pixels are fully in-mask.

    Returns (hfH, validPairs, validRows) or (None, 0, 0) when no valid
    pair exists (N/A path - never synthesized).
    """
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


def depth_strata(mask, ys):
    """Upper/middle/lower row strata of the masked region (middle primary).

    Strata split the masked-row range into three equal bands; boundaries
    snap inward so no band is empty when there are >=3 masked rows.
    Returns dict of row ranges (empty list when unsupported).
    """
    rows = [y for y, mrow in enumerate(mask) if any(mrow)]
    if len(rows) < 3:
        return {"upper": [], "middle": [], "lower": []}
    top, bot = rows[0], rows[-1]
    span = bot - top + 1
    third = span / 3.0
    return {
        "upper": [top, top + int(math.ceil(third)) - 1],
        "middle": [top + int(math.ceil(third)), top + int(math.ceil(2 * third)) - 1],
        "lower": [top + int(math.ceil(2 * third)), bot],
    }


def autocorr_normalized(ys, mask, max_lag=None):
    """G5 diagnostic: per-row mean-removed horizontal autocorrelation at
    small lags, normalized by lag-0 variance. Diagnostic only; never acts
    on the primary hfH."""
    if max_lag is None:
        max_lag = 16
    out = {}
    for lag in range(1, max_lag + 1):
        pairs = []
        for y, row in enumerate(ys):
            mrow = mask[y]
            for x in range(lag, len(row)):
                if mrow[x] and mrow[x - lag]:
                    pairs.append((row[x], row[x - lag]))
        if len(pairs) < 100:
            out[lag] = None
            continue
        n = len(pairs)
        mx = sum(p[0] for p in pairs) / n
        my = sum(p[1] for p in pairs) / n
        vx = sum((p[0] - mx) ** 2 for p in pairs) / n
        vy = sum((p[1] - my) ** 2 for p in pairs) / n
        if vx <= 1e-12 or vy <= 1e-12:
            out[lag] = None
            continue
        cov = sum((p[0] - mx) * (p[1] - my) for p in pairs) / n
        out[lag] = round(cov / math.sqrt(vx * vy), 4)
    return out


def display_track(img, geometry, mask_spec, tier):
    """v2 proposed operator on one image.

    geometry: {innerBox:[x0,y0,x1,y1]} in source pixels (manifest).
    mask_spec: revision s3.4 mask (gdom source map or rects).
    tier: 'primary' (200) or 'sensitivity' (half of primary).
    """
    inner = geometry["innerBox"]
    x0, y0, x1, y1 = inner
    inner_w = x1 - x0 + 1
    inner_h = y1 - y0 + 1
    dst_w = PRIMARY_INNER_WIDTH_PX if tier == "primary" else PRIMARY_INNER_WIDTH_PX // 2
    # scale by inner width; keep aspect
    crop = img.crop((x0, y0, x1 + 1, y1 + 1)).convert("RGB")
    ys, w, h = float_y(crop)
    rs, rw, rh = area_coverage_resample(ys, w, h, dst_w)
    # mask in resampled coordinates: compute gdom/rect coverage on the
    # cropped source then resample with the same operator.
    if mask_spec["kind"] == "gdom":
        rgb = crop.load()
        src_mask = [[(rgb[x, y][1] > rgb[x, y][0]) and (rgb[x, y][1] > rgb[x, y][2])
                     for x in range(w)] for y in range(h)]
    else:
        # rect coords are inner-box-relative
        src_mask = [[False] * w for _ in range(h)]
        for rx0, ry0, rx1, ry1 in mask_spec["rects"]:
            for y in range(max(0, ry0), min(h, ry1)):
                row = src_mask[y]
                for x in range(max(0, rx0), min(w, rx1)):
                    row[x] = True
    cov, mw, mh = area_coverage_resample(
        [[1.0 if v else 0.0 for v in row] for row in src_mask], w, h, rw)
    mask = [[v >= 1.0 - 1e-9 for v in row] for row in cov]
    hf, pairs, vrows = mask_adjacent_pair_hfh(rs, mask)
    strata = depth_strata(mask, rs)
    strata_out = {}
    for name, rng in strata.items():
        if not rng:
            strata_out[name] = {"rows": [], "hfH": "N/A", "validPairs": 0}
            continue
        sub_mask = [[False] * rw for _ in range(rh)]
        for y in range(rng[0], rng[1] + 1):
            sub_mask[y] = mask[y]
        shf, spairs, srows = mask_adjacent_pair_hfh(rs, sub_mask)
        strata_out[name] = {
            "rows": rng, "hfH": "N/A" if shf is None else round(shf, 4),
            "validPairs": spairs,
        }
    vals = [v for y, row in enumerate(rs) for x, v in enumerate(row) if mask[y][x]]
    if vals:
        mean = sum(vals) / len(vals)
        lum_std = math.sqrt(sum((v - mean) ** 2 for v in vals) / len(vals))
    else:
        lum_std = None
    result = {
        "methodId": METHOD_ID_DISPLAY,
        "tier": tier,
        "dstWidthPx": rw,
        "dstHeightPx": rh,
        "srcInnerBox": [x0, y0, x1, y1],
        "srcInnerWidthPx": inner_w,
        "hfH": "N/A" if hf is None else round(hf, 4),
        "validPairs": pairs,
        "validRows": vrows,
        "supportLimited": bool(vrows < 8),
        "lumStdSameMask": round(lum_std, 2) if lum_std is not None else "N/A",
        "depthStrata": strata_out,
    }
    if hf is not None:
        result["autocorrLag1to16"] = autocorr_normalized(rs, mask)
    return result, rs, mask


# ---------------------------------------------------------------------------
# Operator self-tests (revision s4: run BEFORE any real numbers)
# ---------------------------------------------------------------------------

def _const_image(w, h, v):
    return [[float(v)] * w for _ in range(h)]


def run_selftests():
    failures = []
    checks = 0

    def check(name, ok, detail=""):
        nonlocal checks
        checks += 1
        if not ok:
            failures.append(f"{name}: {detail}")

    # T1: constant image -> hfH == 0 exactly (both resampled & pair path).
    ys = _const_image(64, 32, 128.0)
    rs, rw, rh = area_coverage_resample(ys, 64, 32, 32)
    flat = all(abs(v - 128.0) < 1e-9 for row in rs for v in row)
    mask = [[True] * rw for _ in range(rh)]
    hf, pairs, rows = mask_adjacent_pair_hfh(rs, mask)
    check("T1 constant image hfH==0 exact",
          flat and hf == 0.0 and pairs == (rw - 1) * rh,
          f"flat={flat} hf={hf} pairs={pairs}")

    # T2: linear contrast change Y'=a*Y+b multiplies hfH by |a|
    # (scale-dependence declaration, revision s3 / design s2.1-3).
    import random
    rng = random.Random(20261021)
    base = [[rng.uniform(40, 200) for _ in range(64)] for _ in range(32)]
    mask = [[True] * 64 for _ in range(32)]
    hf0, _, _ = mask_adjacent_pair_hfh(base, mask)
    a, b = 1.7, -13.0
    scaled = [[a * v + b for v in row] for row in base]
    hf1, _, _ = mask_adjacent_pair_hfh(scaled, mask)
    check("T2 linear contrast |a| scaling exact",
          abs(hf1 - a * hf0) < 1e-9,
          f"hf1={hf1} a*hf0={a * hf0}")

    # T2b: area-coverage resample preserves the |a| law through scaling.
    rs0, _, _ = area_coverage_resample(base, 64, 32, 32)
    rs1, _, _ = area_coverage_resample(scaled, 64, 32, 32)
    m32 = [[True] * 32 for _ in range(32)]
    r0, _, _ = mask_adjacent_pair_hfh(rs0, m32)
    r1, _, _ = mask_adjacent_pair_hfh(rs1, m32)
    check("T2b resample keeps |a| law",
          abs(r1 - a * r0) < 1e-7,
          f"r1={r1} a*r0={a * r0}")

    # T3: fully-excluded mask -> N/A path (no valid pair).
    empty = [[False] * 64 for _ in range(32)]
    hf, pairs, rows = mask_adjacent_pair_hfh(base, empty)
    check("T3 empty mask -> N/A",
          hf is None and pairs == 0 and rows == 0)

    # T3b: single-column mask (no left-adjacent pairs) -> N/A.
    single = [[x == 5 for x in range(64)] for _ in range(32)]
    hf, pairs, _ = mask_adjacent_pair_hfh(base, single)
    check("T3b single-column mask -> N/A", hf is None and pairs == 0)

    # T4: resample mass preservation on a constant (area weights sum to 1).
    rs, rw, rh = area_coverage_resample(_const_image(50, 40, 77.0), 50, 40, 25)
    check("T4 area weights normalized",
          all(abs(v - 77.0) < 1e-9 for row in rs for v in row))

    # T5: SUPPORT_LIMITED trigger - 3 valid rows only.
    thin = [[True] * 64 for _ in range(3)] + [[False] * 64 for _ in range(5)]
    hf, pairs, vrows = mask_adjacent_pair_hfh(base[:8], thin)
    check("T5 thin support flagged",
          hf is not None and vrows == 3 and vrows < 8,
          f"vrows={vrows}")

    return checks, failures


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def file_sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--selftest", action="store_true",
                    help="run operator self-tests and exit")
    ap.add_argument("--manifest", help="q1-manifest JSON (media entries)")
    ap.add_argument("--image", help="single image measurement mode")
    ap.add_argument("--geometry", help="JSON file with innerBox for --image")
    ap.add_argument("--mask", help="JSON file with mask spec for --image")
    ap.add_argument("--tier", choices=["primary", "sensitivity"],
                    default="primary")
    ap.add_argument("--out", help="result JSON path")
    args = ap.parse_args(argv)

    if args.selftest:
        checks, failures = run_selftests()
        print(f"selftest: {checks - len(failures)}/{checks} passed")
        for f in failures:
            print("  FAIL:", f)
        return 1 if failures else 0

    if args.image:
        from PIL import Image
        img = Image.open(args.image)
        result = {"image": args.image, "sha256": file_sha256(args.image),
                  "tier": args.tier}
        if args.geometry:
            geometry = json.loads(Path(args.geometry).read_text(encoding="utf-8"))
        else:
            geometry = {"innerBox": [178, 888, 1098, 1976]}  # device default
        if args.mask:
            mask_spec = json.loads(Path(args.mask).read_text(encoding="utf-8"))
        else:
            mask_spec = {"kind": "gdom"}  # device default (pure sand band)
        disp, _, _ = display_track(img, geometry, mask_spec, args.tier)
        result["displayTrack"] = disp
        text = json.dumps(result, indent=1, ensure_ascii=False)
        print(text)
        if args.out:
            Path(args.out).write_text(text + "\n", encoding="utf-8")
        return 0

    ap.print_help()
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
