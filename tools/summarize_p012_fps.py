#!/usr/bin/env python3
"""P-012 PERF：把perf-ladder 的 FPS 裁剪图去重聚类，输出代表图清单+ 帧时统计。

FPS 数字目前靠多模态读数（同一数字的裁剪图 md5 完全一致，故可按 md5 聚类，
每簇只需读一次）。读到的值写进同目录 fps_map.json 后，本脚本即可给出
每档位 FPS 分布与 P50/P95 帧时（frame_ms = 1000/fps）。

用法：
  python tools/summarize_p012_fps.py --tag s99c# 列出唯一裁剪图（待读数）
  python tools/summarize_p012_fps.py --tag s99c          # 有映射则输出统计
"""
import os, sys, json, hashlib, glob

OUT_DIR = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
MAP_PATH = os.path.join(OUT_DIR, "fps_map.json")


def main():
    tag = None
    if "--tag" in sys.argv:
        tag = sys.argv[sys.argv.index("--tag") + 1]
    pattern = f"perf-ladder-{tag}-*-fpscrop.png" if tag else "perf-ladder-*-fpscrop.png"
    files = sorted(glob.glob(os.path.join(OUT_DIR, pattern)))
    clusters = {}
    for f in files:
        h = hashlib.md5(open(f, "rb").read()).hexdigest()
        clusters.setdefault(h, []).append(os.path.basename(f))
    mapping = {}
    if os.path.exists(MAP_PATH):
        mapping = json.load(open(MAP_PATH, encoding="utf-8"))
    unknown = [h for h in clusters if h not in mapping]
    print(f"tag={tag} crops={len(files)} unique={len(clusters)} unmapped={len(unknown)}")
    if unknown:
        print("--- 待读数代表图（每行一张，读出 FPS 后写入 fps_map.json）---")
        for h in unknown:
            print(f"{h}  {clusters[h][0]}   x{len(clusters[h])}")
        return
    # 统计：逐文件按 md5 映射读数，再按档位分组（md5 可能在多档位间复用，
    # 不能按聚类展开，否则同一读数会被重复计入多个档位）。
    per = {}
    for f in files:
        h = hashlib.md5(open(f, "rb").read()).hexdigest()
        tier = os.path.basename(f).split("-")[3]
        per.setdefault(tier, []).append(mapping[h])
    def pct(vals, p):
        v = sorted(vals)
        if not v:
            return float("nan")
        k = (len(v) - 1) * p / 100.0
        lo, hi = int(k), min(int(k) + 1, len(v) - 1)
        return v[lo] + (v[hi] - v[lo]) * (k - lo)
    print(f"{'tier':6} {'n':>3} {'fps min/med/max':>18} {'frame_ms P50':>12} {'P95':>8} {'P95 fps':>8}")
    for tier in sorted(per):
        fps = per[tier]
        ms = [1000.0 / f for f in fps]
        p50ms, p95ms = pct(ms, 50), pct(ms, 95)
        print(f"{tier:6} {len(fps):>3} {min(fps):>5}/{pct(fps,50):>5.1f}/{max(fps):<5} "
              f"{p50ms:>12.2f} {p95ms:>8.2f} {1000.0/p95ms:>8.1f}")
    print("\n窗口原始值：")
    for tier in sorted(per):
        print(f"  {tier}: {per[tier]}")


if __name__ == "__main__":
    main()
