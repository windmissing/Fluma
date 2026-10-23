#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P-012 E3 formal K90 + P64/P256 operators on s133 (tester, offline).

Lands the formal operators registered as gaps in
e3-grid-display-v1-freeze-revision.md §2:

  K90  block-corner density, contour-arc-length normalized
       (algorithm-P012-E3-grid-removal-v1.md §2.2):
       - fixed Y levels 32/64/96/128/160/192/224
       - marching squares with linear edge interpolation, saddle resolved by
         bilinear center value
       - tangent at each contour point from +/-2 analysis px along the arc
       - corner event: turn angle 75-105 deg AND both arms within 15 deg of
         horizontal/vertical
       - events within 4 analysis px of arc length collapse to one (closest
         to 90 deg wins; tie -> lexicographically smaller coordinate)
       - arcs resampled at 0.5 px arc-length steps; closed loops start at the
         lexicographically smallest coordinate; open lines start at the
         smaller endpoint
       - mask endpoints with insufficient arc are excluded
       - K90 = 100 * events / total valid arc length (analysis px)

  P64/P256 periodic-print correlation
       - pixel-domain 2-D detrend: fit and subtract a first-order plane from
         the mask window BEFORE correlation
       - normalized correlation per direction (horizontal/vertical) at the
         logical-pixel period mapped to device display px, +/-10% lag window,
         max within the window
       - full lag spectrum reported (no cherry-picking lag64/lag128)
       - valid-pair counts per lag; <200 pairs per lag -> that lag N/A;
         usable span < 2 lags -> metric N/A
       - residual std after detrend reported

Fog window corrected to the frozen s133 geometry: x in [200,1080),
rows in [950,1100) (the archived analyze_p012_e3_renumber.py wrote the stale
1370-1650 window; its old output analysis-e3-renumber-v1.json is already
marked obsolete by the freeze revision).

All outputs are descriptive inputs for designer/manager rulings; no PASS/FAIL
is declared here. The frozen anchors J95/J90/std-ratio are NOT touched.
"""
import hashlib
import json
import math
import os
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(r"D:/hmprojects/HelloWorld/test/artifacts/p012-e3-renumber-20261022")
X0, X1 = 200, 1080          # frozen s133 fog window columns [200,1080)
FOG_Y0, FOG_Y1 = 950, 1100  # frozen s133 fog window rows [950,1100)
METHOD_ID = "e3-grid-device-native-s133-v1"
MASK_SET = "masks-e3q1-frozen-v1"
MASK_SET_SHA = "a114b23ea0bf4e2841fa094f59a6edcb3a719de6058464d8b9d907545d875c14"
Y_LEVELS = (32, 64, 96, 128, 160, 192, 224)
RESAMPLE_STEP = 0.5         # analysis px along arc
TANGENT_OFFSET = 2.0        # analysis px each side for tangent estimation
TURN_MIN, TURN_MAX = 75.0, 105.0
AXIS_TOL = 15.0             # deg from horizontal/vertical
CLUSTER_RADIUS = 4.0        # analysis px along arc
P_WINDOW_FRAC = 0.10        # +/-10% around the center period
MIN_PAIRS_PER_LAG = 200
DETREND = "first-order plane (least squares, per 2-D window)"

BUILD = "entry-default-s133-signed.hap"
BUILD_SHA = "b0c67920c9e2f0b5b54f86f66f5f3de5e4900ca76c7783bd3e2e4d9df124d6f3"
SOURCE_COMMIT = "9d11849"


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load_window(path):
    a = np.asarray(Image.open(path).convert("RGB"), dtype=np.float64)
    Y = 0.299 * a[:, :, 0] + 0.587 * a[:, :, 1] + 0.114 * a[:, :, 2]
    return Y[FOG_Y0:FOG_Y1, X0:X1]


# ---------------------------------------------------------------------------
# K90: marching squares + arc-length normalized corner density
# ---------------------------------------------------------------------------

def _lerp(p1, p2, v1, v2, level):
    t = (level - v1) / (v2 - v1) if v2 != v1 else 0.5
    return (p1[0] + t * (p2[0] - p1[0]), p1[1] + t * (p2[1] - p1[1]))


def marching_segments(Y, level):
    """One-pixel-cell marching squares -> list of segments ((x1,y1),(x2,y2))
    in continuous coordinates (cell corner = integer pixel position)."""
    h, w = Y.shape
    segs = []
    for y in range(h - 1):
        row0, row1 = Y[y], Y[y + 1]
        for x in range(w - 1):
            v = (row0[x], row0[x + 1], row1[x + 1], row1[x])  # TL TR BR BL
            code = 0
            for i, val in enumerate(v):
                if val >= level:
                    code |= 1 << i
            if code in (0, 15):
                continue
            top = _lerp((x, y), (x + 1, y), v[0], v[1], level)
            right = _lerp((x + 1, y), (x + 1, y + 1), v[1], v[2], level)
            bottom = _lerp((x, y + 1), (x + 1, y + 1), v[3], v[2], level)
            left = _lerp((x, y), (x, y + 1), v[0], v[3], level)
            # Saddle: resolve with bilinear center value.
            if code in (5, 10):
                center = (v[0] + v[1] + v[2] + v[3]) / 4.0
                if code == 5:  # TL+BR vs TR+BL
                    if (center - level) * (v[0] - level) >= 0:
                        segs += [(top, left), (bottom, right)]
                    else:
                        segs += [(top, right), (bottom, left)]
                else:  # TR+BL
                    if (center - level) * (v[1] - level) >= 0:
                        segs += [(top, right), (bottom, left)]
                    else:
                        segs += [(top, left), (bottom, right)]
                continue
            edges = {0: (top, right), 1: (right, bottom), 2: (bottom, left),
                     3: (left, top)}
            connect = {
                1: (3, 2), 2: (2, 1), 3: (3, 1), 4: (0, 2), 6: (0, 1),
                7: (3, 0), 8: (0, 3), 9: (1, 0), 11: (1, 2), 12: (2, 0),
                13: (0, 2), 14: (2, 3),
            }[code]
            segs.append((edges[connect[0]][0], edges[connect[1]][1])
                        if False else (edges[connect[0]][0],
                                       edges[connect[1]][1]))
    return segs


def chain_segments(segs):
    """Chain unordered segments into polylines (lists of points)."""
    def key(p):
        return (round(p[0], 6), round(p[1], 6))
    adj = {}
    for a, b in segs:
        adj.setdefault(key(a), []).append(key(b))
        adj.setdefault(key(b), []).append(key(a))
    used = set()
    lines = []
    for a, b in segs:
        ka, kb = key(a), key(b)
        if (ka, kb) in used or (kb, ka) in used:
            continue
        # walk both directions from this segment
        line = [ka, kb]
        used.add((ka, kb))
        for end, rev in ((kb, False), (ka, True)):
            while True:
                cur = line[-1] if not rev else line[0]
                nxts = [n for n in adj.get(cur, [])
                        if (cur, n) not in used and (n, cur) not in used]
                if not nxts:
                    break
                nxt = nxts[0]
                used.add((cur, nxt))
                if not rev:
                    line.append(nxt)
                else:
                    line.insert(0, nxt)
        lines.append(line)
    return lines


def resample_line(line, step=RESAMPLE_STEP):
    """Resample a polyline at fixed arc-length steps; returns (points, arclen)
    or (None, arclen) when too short for the tangent offset."""
    pts = np.array(line, dtype=np.float64)
    if len(pts) < 2:
        return None, 0.0
    d = np.sqrt(((pts[1:] - pts[:-1]) ** 2).sum(axis=1))
    arclen = float(d.sum())
    if arclen < 2 * TANGENT_OFFSET + step:
        return None, arclen
    s = np.concatenate([[0.0], np.cumsum(d)])
    n = max(2, int(arclen / step))
    targets = np.linspace(0.0, arclen, n)
    rx = np.interp(targets, s, pts[:, 0])
    ry = np.interp(targets, s, pts[:, 1])
    return np.stack([rx, ry], axis=1), arclen


def tangents(pts):
    """Tangent angle (deg, atan2) at each resampled point using +/-TANGENT_OFFSET
    arc-length neighbors."""
    n = len(pts)
    tans = np.full(n, np.nan)
    for i in range(n):
        # search window by arc distance
        j0 = i
        while j0 > 0 and pts[i, 0] is not None and \
                np.hypot(*(pts[i] - pts[max(0, j0 - 1)])) < TANGENT_OFFSET and i - j0 < 50:
            j0 -= 1
            if i - j0 > 40:
                break
        j1 = i
        while j1 < n - 1 and \
                np.hypot(*(pts[min(n - 1, j1 + 1)] - pts[i])) < TANGENT_OFFSET and j1 - i < 50:
            j1 += 1
            if j1 - i > 40:
                break
        if j1 - j0 < 1:
            continue
        dx = pts[j1, 0] - pts[j0, 0]
        dy = pts[j1, 1] - pts[j0, 1]
        if dx == 0 and dy == 0:
            continue
        tans[i] = math.degrees(math.atan2(dy, dx)) % 180.0
    return tans


def corner_events(pts, tans):
    """Count 90-degree corner events per the frozen definition."""
    n = len(pts)
    events = []
    for i in range(1, n - 1):
        a0, a1 = tans[i - 1], tans[i + 1]
        if math.isnan(a0) or math.isnan(a1):
            continue
        diff = abs(a1 - a0)
        turn = min(diff, 180.0 - diff)
        if not (TURN_MIN <= turn <= TURN_MAX):
            continue
        # both arms within AXIS_TOL of horizontal (0/180) or vertical (90)
        def axis_dist(ang):
            return min(ang, abs(ang - 90.0), abs(ang - 180.0))
        if axis_dist(a0) > AXIS_TOL or axis_dist(a1) > AXIS_TOL:
            continue
        score = abs(turn - 90.0)
        events.append((score, pts[i][0], pts[i][1], i))
    # cluster within CLUSTER_RADIUS arc steps (index*step approximates arc here
    # because resample step is uniform)
    events.sort(key=lambda e: e[3])
    clustered = []
    for e in events:
        if clustered and e[3] - clustered[-1][3] <= CLUSTER_RADIUS / RESAMPLE_STEP:
            if e[0] < clustered[-1][0]:
                clustered[-1] = e
        else:
            clustered.append(e)
    return clustered


def k90_for_window(Y):
    """Full K90 over the fog window. Returns dict with events, arcLen, K90."""
    total_arc = 0.0
    total_events = 0
    per_level = {}
    for level in Y_LEVELS:
        segs = marching_segments(Y, float(level))
        lines = chain_segments(segs)
        lv_arc = 0.0
        lv_events = 0
        for line in lines:
            pts, arclen = resample_line(line)
            if pts is None:
                continue  # arc too short for tangent estimation: excluded
            tans = tangents(pts)
            evs = corner_events(pts, tans)
            lv_arc += arclen
            lv_events += len(evs)
        per_level[str(level)] = {"arcLen": round(lv_arc, 1),
                                 "events": lv_events,
                                 "K90": round(100.0 * lv_events / lv_arc, 4)
                                 if lv_arc > 0 else "N/A"}
        total_arc += lv_arc
        total_events += lv_events
    k90 = round(100.0 * total_events / total_arc, 4) if total_arc > 0 else "N/A"
    return {"K90": k90, "totalArcLenPx": round(total_arc, 1),
            "totalEvents": total_events, "perLevel": per_level,
            "levels": list(Y_LEVELS), "resampleStepPx": RESAMPLE_STEP,
            "tangentOffsetPx": TANGENT_OFFSET,
            "turnRangeDeg": [TURN_MIN, TURN_MAX],
            "axisTolDeg": AXIS_TOL, "clusterRadiusPx": CLUSTER_RADIUS}


# ---------------------------------------------------------------------------
# P64/P256: pixel-domain 2-D detrend + normalized correlation, full spectrum
# ---------------------------------------------------------------------------

def detrend_plane(win):
    """Subtract the least-squares first-order plane. Returns (residual, R2)."""
    h, w = win.shape
    yy, xx = np.mgrid[0:h, 0:w]
    A = np.stack([xx.ravel(), yy.ravel(), np.ones(h * w)], axis=1)
    b = win.ravel()
    coef, *_ = np.linalg.lstsq(A, b, rcond=None)
    fit = (A @ coef).reshape(h, w)
    resid = win - fit
    ss_tot = float(((win - win.mean()) ** 2).sum())
    ss_res = float((resid ** 2).sum())
    r2 = 1.0 - ss_res / ss_tot if ss_tot > 0 else 0.0
    return resid, round(r2, 4)


def corr_at_lag(resid, dx, dy):
    """Normalized correlation between shifted copies; returns (r, n)."""
    h, w = resid.shape
    if dy >= 0:
        a = resid[:h - dy, :w - dx].ravel() if dx > 0 else resid[:h - dy, :].ravel()
        b = resid[dy:, dx:].ravel() if dx > 0 else resid[dy:, :].ravel()
    else:
        a = resid[:h + dy, :w - dx].ravel() if dx > 0 else resid[:h + dy, :].ravel()
        b = resid[-dy:, dx:].ravel() if dx > 0 else resid[-dy:, :].ravel()
    if len(a) != len(b) or len(a) == 0:
        return None, 0
    va, vb = a.std(), b.std()
    if va <= 1e-9 or vb <= 1e-9:
        return None, len(a)
    return float(((a - a.mean()) * (b - b.mean())).mean() / (va * vb)), len(a)


def p_metric(resid, axis, center_lag_px, label):
    """P-metric along `axis` ('h' or 'v') at center lag +/-10% window.
    Full spectrum reported. Returns dict."""
    h, w = resid.shape
    max_lag = (w if axis == "h" else h) - 1
    lo = max(1, int(math.floor(center_lag_px * (1 - P_WINDOW_FRAC))))
    hi = min(max_lag, int(math.ceil(center_lag_px * (1 + P_WINDOW_FRAC))))
    if lo > hi:
        return {
            "label": label, "centerLagPx": center_lag_px,
            "windowLags": None,
            "peakInWindow": None,
            "N_A": f"center period {center_lag_px:.1f}px exceeds max lag "
                   f"{max_lag}px in this window (windowLags inverted)",
            "fullSpectrum": {}, "usableSpanLags": 0,
            "note": "period longer than the measurable span - N/A per the "
                    "usable-span rule",
        }
    lags = {}
    peak = None
    for lag in range(1, max_lag + 1):
        if axis == "h":
            r, n = corr_at_lag(resid, lag, 0)
        else:
            r, n = corr_at_lag(resid, 0, lag)
        if r is None or n < MIN_PAIRS_PER_LAG:
            lags[str(lag)] = {"r": None, "n": n}
            continue
        lags[str(lag)] = {"r": round(r, 4), "n": n}
        if lo <= lag <= hi:
            if peak is None or r > peak["r"]:
                peak = {"lag": lag, "r": round(r, 4), "n": n}
    in_window = [lags[str(l)]["r"] for l in range(lo, hi + 1)
                 if str(l) in lags and lags[str(l)]["r"] is not None]
    return {
        "label": label,
        "centerLagPx": center_lag_px,
        "windowLags": [lo, hi],
        "peakInWindow": peak,
        "windowValues": in_window,
        "fullSpectrum": lags,
        "usableSpanLags": sum(1 for v in lags.values() if v["r"] is not None),
        "note": "pixel-domain 2-D first-order-plane detrended normalized "
                "correlation; full spectrum reported, window peak is the "
                "P-value candidate",
    }


def p64_p256_for_window(Y):
    resid, r2 = detrend_plane(Y)
    # Logical px -> device display px ratio: cols=128 across the inner width
    # 921 px (manifest) -> colW = 921/128 = 7.195...; fog/mottle period is 64
    # logical px => 64 * colW device px. Pure sand tile 256 logical px.
    col_w = 921.0 / 128.0
    p64_center = 64.0 * col_w
    p256_center = 256.0 * col_w
    out = {
        "detrend": DETREND, "detrendR2": r2,
        "residualStd": round(float(resid.std()), 3),
        "logicalToDisplayPx": round(col_w, 5),
        "P64": {
            "horizontal": p_metric(resid, "h", p64_center, "P64 horizontal"),
            "vertical": p_metric(resid, "v", p64_center, "P64 vertical"),
        },
        "P256": {
            "horizontal": p_metric(resid, "h", p256_center, "P256 horizontal"),
            "vertical": p_metric(resid, "v", p256_center, "P256 vertical"),
        },
        "minPairsPerLag": MIN_PAIRS_PER_LAG,
        "windowFrac": P_WINDOW_FRAC,
    }
    return out


# ---------------------------------------------------------------------------

def analyze_frame(path):
    Y = load_window(path)
    return {
        "sha256": sha256(path),
        "K90": k90_for_window(Y),
        "P64P256": p64_p256_for_window(Y),
    }


def main():
    frames = {}
    for d in sorted(os.listdir(ROOT)):
        dd = ROOT / d
        if not dd.is_dir():
            continue
        for f in sorted(os.listdir(dd)):
            if "static-1" in f and f.endswith(".jpeg"):
                frames[f.replace(".jpeg", "")] = analyze_frame(dd / f)
    pairs = {}
    for p in (1, 2):
        off = frames[f"pair{p}-off-static-1"]
        on = frames[f"pair{p}-on-static-1"]
        def ratio(k):
            vo, vn = off["K90"]["K90"], on["K90"]["K90"]
            if isinstance(vo, float) and isinstance(vn, float) and vo > 0:
                return round(vn / vo, 4)
            return "N/A"
        def peak(key_path):
            def get(node):
                for k in key_path.split("."):
                    node = node[k]
                return node
            po = get(off)
            pn = get(on)
            return {"OFF": po, "ON": pn}
        pairs[f"pair{p}"] = {
            "K90_ON_over_OFF": ratio("K90"),
            "P64_horizontal_peak": peak("P64P256.P64.horizontal.peakInWindow"),
            "P64_vertical_peak": peak("P64P256.P64.vertical.peakInWindow"),
        }
    out = {
        "run": "E3 formal K90 + P64/P256 operators on s133",
        "methodId": METHOD_ID,
        "maskSet": MASK_SET, "maskSetSha256": MASK_SET_SHA,
        "fogWindow": {"x": [X0, X1], "rows": [FOG_Y0, FOG_Y1],
                      "endpointConvention": "numpy slice right/bottom exclusive",
                      "note": "frozen s133 geometry; stale 1370-1650 window "
                              "outputs marked obsolete by the freeze revision"},
        "build": BUILD, "buildSha256": BUILD_SHA,
        "sourceCommit": SOURCE_COMMIT,
        "nature": "descriptive formal-operator results; PASS/FAIL rulings "
                  "belong to designer/manager; frozen anchors J95/J90/std "
                  "untouched",
        "frames": frames, "pairs": pairs,
    }
    outp = ROOT / "analysis-e3-formal-k90-p64-v1.json"
    outp.write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n",
                    encoding="utf-8")
    print("== K90 (contour-arc-length normalized) ==")
    for k, f in frames.items():
        print(f"  {k}: K90={f['K90']['K90']} arc={f['K90']['totalArcLenPx']} "
              f"events={f['K90']['totalEvents']}")
    for p, agg in pairs.items():
        print(f"  {p}: K90_ON/OFF={agg['K90_ON_over_OFF']}")
    print("== P64 window peaks ==")
    for k, f in frames.items():
        for d in ("horizontal", "vertical"):
            pk = f["P64P256"]["P64"][d]["peakInWindow"]
            print(f"  {k} P64.{d}: peak={pk}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
