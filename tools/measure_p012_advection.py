#!/usr/bin/env python3
"""P-012 PERF 动态复核：测量 EAB 颗粒纹理在倾斜段的实际平移量（tile px）。

原理：取沙带内一条水平行（避开界面与边界），与倾斜序列首帧做归一化互相关，
逐帧求最大相关峰的位移量。整数位移即纹理相位平移（tile=256px）。
判定：位移范围须覆盖 ~0→200 tile px 量级（DoD③），且不出现恒定不变（贴纸）
或恒为 0（冻结/失效）两种退化。

用法：python tools/measure_p012_advection.py --frames <glob前缀> --n 16
"""
import os, sys, glob
import numpy as np
import cv2

OUT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
# 沙带 ROI（1280x2832 截图坐标）：y1400..1700 为沙体主区（含颗粒纹理），
# x 取画布内240..1035。y1930..1972 的薄底层在静止段无纹理变化，不用作平流测量。
Y0, Y1 = 1400, 1700
X0, X1 = 260, 1020


def row_profile(path):
    img = cv2.imread(path)
    if img is None:
        return None
    h, w = img.shape[:2]
    sy, sx = h / 2832.0, w / 1280.0
    y0, y1 = int(Y0 * sy), int(Y1 * sy)
    x0, x1 = int(X0 * sx), int(X1 * sx)
    band = img[y0:y1, x0:x1].astype(np.float32)
    gray = cv2.cvtColor(band, cv2.COLOR_BGR2GRAY)
    # 沿行求均值 -> 抑制竖向结构噪声，保留水平纹理
    return gray.mean(axis=0)


def best_shift(ref, cur, max_shift=400):
    ref = (ref - ref.mean()) / (ref.std() + 1e-6)
    cur = (cur - cur.mean()) / (cur.std() + 1e-6)
    n = len(ref)
    scores = []
    for s in range(-max_shift, max_shift + 1):
        if s >= 0:
            a, b = ref[s:], cur[:n - s]
        else:
            a, b = ref[:n + s], cur[-s:]
        m = min(len(a), len(b))
        if m < 200:
            scores.append(-2.0)
            continue
        scores.append(float(np.dot(a[:m], b[:m]) / m))
    scores = np.array(scores)
    k = int(np.argmax(scores))
    return k - max_shift, float(scores[k])


def main():
    prefix = "dyn-s96-tilt"
    n = 16
    if "--frames" in sys.argv:
        prefix = sys.argv[sys.argv.index("--frames") + 1]
    if "--n" in sys.argv:
        n = int(sys.argv[sys.argv.index("--n") + 1])
    files = [os.path.join(OUT, f"{prefix}-{i}.jpeg") for i in range(n)]
    files = [f for f in files if os.path.exists(f)]
    if not files:
        print("no frames", prefix)
        return
    profs = [row_profile(f) for f in files]
    ref = profs[0]
    print(f"{'frame':>6} {'shiftPx':>8} {'peakCorr':>9}")
    shifts = []
    for i, p in enumerate(profs):
        s, c = best_shift(ref, p)
        shifts.append(s)
        print(f"{i:>6} {s:>8} {c:>9.3f}")
    rng = max(shifts) - min(shifts)
    print(f"\nshift range = {rng} px  ({rng/256.0:.2f} tile)")
    print(f"distinct shifts = {len(set(shifts))}")
    verdict = ("OK-有平移且未贴死" if (rng >= 150 and len(set(shifts)) >= 3)
               else "SUSPECT-范围不足或冻结")
    print("verdict:", verdict)


if __name__ == "__main__":
    main()
