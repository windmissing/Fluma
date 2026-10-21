#!/usr/bin/env python3
"""P-012 PERF：从 perf-ladder 快照中裁出左上角 FPS 文本块，供人工/多模态读数。

用法：python tools/extract_p012_fps.py --tag s99
输出：<快照名>-fpscrop.png（放大 2x，便于读数）
"""
import os, sys, glob
import cv2

OUT_DIR = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
# 画布内 FPS 文本块（Text position x=20,y=40，实际渲染 y≈250..340）
BASE_CROP = (30, 240, 360, 350)


def crop_fps(img):
    h, w = img.shape[:2]
    x0, y0, x1, y1 = BASE_CROP
    if h < 1200 or w < 700:
        sx = w / 720.0
        sy = h / 1280.0
        x0, y0, x1, y1 = int(x0 * sx), int(y0 * sy), int(x1 * sx), int(y1 * sy)
    return img[max(0, y0):min(h, y1), max(0, x0):min(w, x1)]


def main():
    d = OUT_DIR
    tag = None
    if "--dir" in sys.argv:
        d = sys.argv[sys.argv.index("--dir") + 1]
    if "--tag" in sys.argv:
        tag = sys.argv[sys.argv.index("--tag") + 1]
    files = sorted(glob.glob(os.path.join(d, "perf-ladder-*.jpeg")))
    if tag:
        files = [f for f in files if f"-{tag}-" in os.path.basename(f)]
    for f in files:
        img = cv2.imread(f)
        if img is None:
            print("READ FAIL", f)
            continue
        c = crop_fps(img)
        c = cv2.resize(c, None, fx=2, fy=2, interpolation=cv2.INTER_CUBIC)
        out = os.path.splitext(f)[0] + "-fpscrop.png"
        cv2.imwrite(out, c)
        print(os.path.basename(f), "->", os.path.basename(out))


if __name__ == "__main__":
    main()
