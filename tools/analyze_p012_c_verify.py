# -*- coding: utf-8 -*-
# P-012 C-verify OFF/ON paired pixel analysis
# Hypothesis: C stratification adds horizontal stripe texture inside the light-sand
# (yellow) band. OFF (p012ablate=strat) should show lower row-luminance oscillation.
import os, sys, json
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

# Container box measured from 516x1080 frames: x ~ [66,450], y ~ [336,750]
BOX_X0, BOX_X1 = 80, 436
BOX_Y0, BOX_Y1 = 345, 742

def lum(img):
    a = np.asarray(img.convert("RGB"), dtype=np.float64)
    return 0.299*a[:,:,0] + 0.587*a[:,:,1] + 0.114*a[:,:,2]

def classify_rows(L, box):
    """Return row-class map: 2=water(blue), 1=sand(yellow/green), 0=bg"""
    rows = {}
    for y in range(box[1], box[3]):
        row = L[y, box[0]:box[2]]
        rows[y] = row.mean()
    return rows

def analyze_frame(path):
    img = Image.open(path)
    L = lum(img)
    W, H = img.size
    sx, sy = W/516.0, H/1080.0
    x0,x1 = int(BOX_X0*sx), int(BOX_X1*sx)
    y0,y1 = int(BOX_Y0*sy), int(BOX_Y1*sy)
    res = {"file": os.path.basename(path), "size": [W,H]}
    # --- locate blue water band bottom and green heavy band top by hue ---
    a = np.asarray(img.convert("RGB"), dtype=np.float64)
    R,G,B = a[:,:,0],a[:,:,1],a[:,:,2]
    blue = (B > R+20) & (B > 60)
    green = (G > R+20) & (G > B+10)
    yellow = (R > B+30) & (G > B+20) & (R>80)
    rowb = blue[:, x0:x1].mean(axis=1)
    rowg = green[:, x0:x1].mean(axis=1)
    rowy = yellow[:, x0:x1].mean(axis=1)
    def first_below(arr, th, start, end, step):
        for y in range(start, end, step):
            if arr[y] < th: return y
        return None
    # blue band: first rows from y0 where blue dominant
    blue_rows = [y for y in range(y0,y1) if rowb[y] > 0.5]
    yellow_rows = [y for y in range(y0,y1) if rowy[y] > 0.5]
    green_rows = [y for y in range(y0,y1) if rowg[y] > 0.5]
    if not (blue_rows and yellow_rows and green_rows):
        res["error"] = "band classification failed"
        return res
    yB0, yB1 = min(blue_rows), max(blue_rows)
    yY0, yY1 = min(yellow_rows), max(yellow_rows)
    yG0, yG1 = min(green_rows), max(green_rows)
    res["bands"] = {"blue":[yB0,yB1], "yellow":[yY0,yY1], "green":[yG0,yG1]}
    # --- stratification metric: row-luminance oscillation inside yellow band ---
    # use middle 80% of yellow band to avoid edges; detrend with running mean
    ya, yb = yY0 + (yY1-yY0)//5, yY1 - (yY1-yY0)//5
    prof = L[ya:yb, x0:x1].mean(axis=1)
    # detrend: subtract 9-row moving average
    k = 9
    pad = np.pad(prof, k//2, mode="edge")
    mov = np.convolve(pad, np.ones(k)/k, mode="valid")
    resid = prof - mov
    res["yellow_band"] = {"y0":ya, "y1":yb,
        "rowOscStd": float(resid.std()),
        "rowOscP2P": float(resid.max()-resid.min())}
    # same for green band
    ga, gb = yG0 + (yG1-yG0)//5, yG1 - (yG1-yG0)//5
    profg = L[ga:gb, x0:x1].mean(axis=1)
    padg = np.pad(profg, k//2, mode="edge")
    movg = np.convolve(padg, np.ones(k)/k, mode="valid")
    residg = profg - movg
    res["green_band"] = {"y0":ga, "y1":gb,
        "rowOscStd": float(residg.std()),
        "rowOscP2P": float(residg.max()-residg.min())}
    # --- column-luminance oscillation (vertical grain) inside yellow band ---
    profc = L[ya:yb, x0:x1].mean(axis=0)
    padc = np.pad(profc, k//2, mode="edge")
    movc = np.convolve(padc, np.ones(k)/k, mode="valid")
    resc = profc - movc
    res["yellow_colOscStd"] = float(resc.std())
    # --- 2D high-frequency energy in yellow band (FFT-free: Laplacian var) ---
    crop = L[ya:yb, x0:x1]
    lap = crop[1:-1,1:-1]*4 - crop[:-2,1:-1] - crop[2:,1:-1] - crop[1:-1,:-2] - crop[1:-1,2:]
    res["yellowLapVar"] = float(lap.var())
    res["lumMean"] = float(L[y0:y1, x0:x1].mean())
    return res

def main():
    out = {"pairs": []}
    for p in (1,2,3):
        for m in ("off","on"):
            d = os.path.join(ROOT, f"pair{p}-{m}")
            for f in sorted(os.listdir(d)):
                if "static" in f or "tilt" in f:
                    r = analyze_frame(os.path.join(d,f))
                    out["pairs"].append({"pair":p,"mode":m, **r})
    with open(os.path.join(ROOT, OUT_NAME), "w", encoding="utf-8") as fp:
        json.dump(out, fp, indent=1, ensure_ascii=False)
    # summary table
    print(f"{'pair':4} {'mode':4} {'frame':16} {'yellowOscStd':>12} {'yellowP2P':>10} {'greenOscStd':>11} {'lapVar':>9} {'colOsc':>8}")
    for r in out["pairs"]:
        if "error" in r:
            print(r["pair"], r["mode"], r["file"], "ERROR", r["error"]); continue
        print(f"{r['pair']:<4} {r['mode']:4} {r['file'][:16]:16} "
              f"{r['yellow_band']['rowOscStd']:12.3f} {r['yellow_band']['rowOscP2P']:10.2f} "
              f"{r['green_band']['rowOscStd']:11.3f} {r['yellowLapVar']:9.2f} {r['yellow_colOscStd']:8.3f}")

if __name__ == "__main__":
    main()
