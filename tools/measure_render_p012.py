#!/usr/bin/env python3
"""P-012 frozen render-appearance measurement appendix (effector, v1).
Implements docs/measurement-frozen-appendix-P012-v1.md exactly. Do not modify metrics
without a new appendix revision. Usage:
  python tools/measure_render_p012.py --image <path> --profile <profile> [--json out.json]
Profiles: target-static (video1_frame*), target-flow (video1_frame*), off-ws0, off-ws15,
          off-hybrid0, off-hybrid3 (p012-cur-*.jpeg / p012-hybrid-tilt*.jpeg)
"""
import argparse, json, math
from PIL import Image

# ---------- frozen ROI coordinates (absolute pixels) ----------
ROIS = {
    # target video frames: 260x330
    "target-water":      {"box": (30, 85, 230, 150)},   # water interior (mint), excludes hand/background top & interface
    "target-interface":  {"band": (30, 150, 230, 185)},  # normal-direction band across interface
    "target-sand":       {"box": (30, 190, 230, 300)},   # sand interior (light green), excludes bottom border
    "target-flowrow":    {"y": 270, "y0": 270, "y1": 300, "x0": 30, "x1": 230},
    # current OFF-baseline screenshots: 1280x2832, canvas x 173..1104, y 883..1982
    "off-water":         {"box": (240, 1400, 1035, 1555)},  # hybrid3 blue water / ws15 water 1360..1750
    "off-water-ws":      {"box": (240, 1400, 1035, 1720)},  # ws15 water (larger band)
    "off-interface":     {"band": (240, 1500, 1035, 1700)}, # hybrid interface band
    "off-interface-ws":  {"band": (240, 1700, 1035, 1900)}, # ws15: water->yellow->green transition
    "off-sand":          {"box": (240, 1620, 1035, 1930)},  # hybrid3 sand interior
    "off-sand-ws":       {"box": (240, 1860, 1035, 1965)},  # ws15 green sand
    "off-sand-ws0":      {"box": (240, 1950, 1035, 1976)},  # ws0 green sand (thin band above bottom border)
    "off-flowrow":       {"y": 1600, "y0": 1595, "y1": 1900, "x0": 240, "x1": 1035},
}

def lum(px):
    r, g, b = px[0], px[1], px[2]
    return 0.299*r + 0.587*g + 0.114*b

def gray(img, box):
    crop = img.crop(box).convert("RGB")
    w, h = crop.size
    px = crop.load()
    ys = [[lum(px[x, y]) for x in range(w)] for y in range(h)]
    return ys, w, h

def m_std(vals):
    n = len(vals)
    mean = sum(vals)/n
    return math.sqrt(sum((v-mean)**2 for v in vals)/n), mean

def hf_h(ys):
    n = 0; s = 0.0
    for row in ys:
        for x in range(1, len(row)):
            s += abs(row[x]-row[x-1]); n += 1
    return s/n

def hf_v(ys):
    n = 0; s = 0.0
    for y in range(1, len(ys)):
        for x in range(len(ys[0])):
            s += abs(ys[y][x]-ys[y-1][x]); n += 1
    return s/n

def max_grad(ys):
    g = 0.0
    for y in range(1, len(ys)):
        for x in range(len(ys[0])):
            g = max(g, abs(ys[y][x]-ys[y-1][x]))
    return g

def transition_width_10_90(ys):
    """Rows averaged to a 1-D profile (top->bottom); width between 10% and 90% of total rise."""
    prof = [sum(row)/len(row) for row in ys]
    lo, hi = min(prof), max(prof)
    if hi - lo < 1e-9:
        return None, prof
    t10 = lo + 0.1*(hi-lo); t90 = lo + 0.9*(hi-lo)
    i10 = next((i for i, v in enumerate(prof) if v >= t10), None)
    i90 = next((i for i in range(len(prof)-1, -1, -1) if v >= t90), None) if False else next(
        (i for i, v in enumerate(prof) if v >= t90), None)
    # width = distance between first crossing of t90 (from top) and first crossing of t10
    i90_first = next((i for i, v in enumerate(prof) if v >= t90), None)
    if i10 is None or i90_first is None:
        return None, prof
    return abs(i90_first - i10), prof

def row_osc(ys):
    """row-mean local extrema count with smoothing window 5, prominence 2.0, min interval 3 rows."""
    prof = [sum(row)/len(row) for row in ys]
    W, PROM, MINIV = 5, 2.0, 3
    sm = []
    for i in range(len(prof)):
        lo, hi = max(0, i-W//2), min(len(prof), i+W//2+1)
        sm.append(sum(prof[lo:hi])/(hi-lo))
    ext = []
    for i in range(1, len(sm)-1):
        if (sm[i]-sm[i-1])*(sm[i+1]-sm[i]) < 0 and abs(sm[i]-min(sm[i-1], sm[i+1])) >= PROM/2:
            ext.append(i)
    filtered = []
    for i in ext:
        if not filtered or i - filtered[-1] >= MINIV:
            filtered.append(i)
    rng = max(prof) - min(prof)
    return len(filtered), rng

def flow_steps(img, y, x0, x1, thr=6, y0=None, y1=None):
    """Max gray-step count over a row window [y0,y1) (frozen: threshold >6)."""
    best = 0; best_y = None
    rows = range(y, y1) if (y0 is None and y1 is not None) else ((range(y0, y1) if y0 is not None else range(y, y+1)))
    for yy in rows:
        vals = [lum(img.getpixel((x, yy))) for x in range(x0, x1)]
        n = sum(1 for i in range(1, len(vals)) if abs(vals[i]-vals[i-1]) > thr)
        if n > best:
            best, best_y = n, yy
    return {"steps": best, "row": best_y}

def rgb_mean(img, box):
    crop = img.crop(box)
    px = list(crop.getdata())
    n = len(px)
    return (sum(p[0] for p in px)/n, sum(p[1] for p in px)/n, sum(p[2] for p in px)/n)

def measure_box(img, roi):
    ys, w, h = gray(img, roi["box"])
    std, mean = m_std([v for row in ys for v in row])
    osc_n, osc_rng = row_osc(ys)
    rgb = rgb_mean(img, roi["box"])
    return {
        "roi": roi["box"], "lumMean": round(mean, 2), "lumStd": round(std, 2),
        "hfH": round(hf_h(ys), 3), "hfV": round(hf_v(ys), 3),
        "rowOscCount": osc_n, "rowOscRange": round(osc_rng, 1),
        "rgbMean": [round(c, 1) for c in rgb],
    }

def measure_band(img, roi):
    ys, w, h = gray(img, roi["band"])
    width, prof = transition_width_10_90(ys)
    return {
        "roi": roi["band"], "maxGrad": round(max_grad(ys), 2),
        "transitionWidthRows": width if width is None else round(width, 1),
        "lumTop": round(prof[0], 1), "lumBottom": round(prof[-1], 1),
    }

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", required=True)
    ap.add_argument("--profile", required=True,
                    choices=["target-static", "target-flow", "off-ws0", "off-ws15", "off-hybrid0", "off-hybrid3"])
    ap.add_argument("--json", default=None)
    args = ap.parse_args()
    img = Image.open(args.image).convert("RGB")
    out = {"image": args.image, "profile": args.profile}
    if args.profile in ("target-static", "target-flow"):
        out["water"] = measure_box(img, ROIS["target-water"])
        out["interface"] = measure_band(img, ROIS["target-interface"])
        out["sand"] = measure_box(img, ROIS["target-sand"])
        out["flowSteps"] = flow_steps(img, ROIS["target-flowrow"]["y"], ROIS["target-flowrow"]["x0"],
                                      ROIS["target-flowrow"]["x1"], y0=ROIS["target-flowrow"]["y0"],
                                      y1=ROIS["target-flowrow"]["y1"])
    elif args.profile == "off-hybrid3":
        out["water"] = measure_box(img, ROIS["off-water"])
        out["interface"] = measure_band(img, ROIS["off-interface"])
        out["sand"] = measure_box(img, ROIS["off-sand"])
        out["flowSteps"] = flow_steps(img, ROIS["off-flowrow"]["y"], ROIS["off-flowrow"]["x0"],
                                      ROIS["off-flowrow"]["x1"], y0=ROIS["off-flowrow"]["y0"],
                                      y1=ROIS["off-flowrow"]["y1"])
    elif args.profile == "off-hybrid0":
        out["water"] = measure_box(img, ROIS["off-water"])
        out["interface"] = measure_band(img, ROIS["off-interface"])
        out["sand"] = measure_box(img, ROIS["off-sand"])
    elif args.profile == "off-ws15":
        out["water"] = measure_box(img, ROIS["off-water-ws"])
        out["interface"] = measure_band(img, ROIS["off-interface-ws"])
        out["sand"] = measure_box(img, ROIS["off-sand-ws"])
    elif args.profile == "off-ws0":
        out["water"] = measure_box(img, ROIS["off-water-ws"])
        out["sand"] = measure_box(img, ROIS["off-sand-ws0"])
    text = json.dumps(out, indent=2, ensure_ascii=False)
    print(text)
    if args.json:
        with open(args.json, "w", encoding="utf-8") as f:
            f.write(text + "\n")

if __name__ == "__main__":
    main()
