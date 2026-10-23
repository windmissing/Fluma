#!/usr/bin/env python3
"""P-012 帧时三段拆分分析（P-013 立项前置）。

输入：tools/seg3_capture.sh 产的 seg3-<tag>-<scene>-<abl>-hilog.txt（6 块）
输出：三场景×OFF/ON 的 phys/render/present P50/P95 + 90Hz 可达性判读。

判读逻辑（architecture-P012-E2-route3 §5.3 口径）：
- shader 化理论可省段 = renderMs（Canvas draw 提交）+ RS 合成（present 段中
  排除 vsync 空转的部分——本埋点 presentMs 含 vsync 等待，用 present 段
  P50 与「节拍锁定值」的差估计真实合成开销）
- 90Hz 单拍可达性：phys + shader 化后的渲染残段（预估 ≤2ms 保守值）
  < 11.1ms 且 present 空转充足
- 热态过滤：physicsTimeMs（物理引擎内计时）>20ms 的帧剔除（F1 §2.1）
"""
import json, re, sys, glob, os
from statistics import median

def pctl(vals, p):
    s = sorted(vals)
    if not s:
        return float('nan')
    i = min(len(s) - 1, int(p * len(s)))
    return s[i]

def load(path):
    frames = []
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.search(r'WS_FRAME (\{.*?\}) seg=(\{.*?\})', line)
        if not m:
            continue
        try:
            d = json.loads(m.group(1))
            seg = json.loads(m.group(2))
        except json.JSONDecodeError:
            continue
        d.update(seg)
        frames.append(d)
    # 热态过滤：物理引擎内计时正常 ≤20ms（正常频率轻载 ~12ms；热态 45ms+）
    return [f for f in frames if f.get('physicsTimeMs', 99) <= 20 and f.get('substeps', 0) > 0]

def stats(frames, key):
    vals = [f[key] for f in frames if key in f]
    return (pctl(vals, 0.5), pctl(vals, 0.95), len(vals)) if vals else (float('nan'), float('nan'), 0)

def main():
    tag = sys.argv[1] if len(sys.argv) > 1 else 's134'
    base = os.path.join(os.path.dirname(__file__), '..', 'test', 'artifacts', 'p012-measurements')
    scenes = ['settle', 'tilt', 'heavy']
    print(f'{"场景":8} {"臂":9} {"n":>4} | {"phys P50/P95":>14} | {"render P50/P95":>14} | {"present P50/P95":>15} | {"frame P50":>9}')
    for scene in scenes:
        for abl in ['off', 'toplight']:
            path = os.path.join(base, f'seg3-{tag}-{scene}-{abl}-hilog.txt')
            if not os.path.exists(path):
                print(f'{scene:8} {abl:9}    - | 缺文件')
                continue
            fr = load(path)
            if not fr:
                print(f'{scene:8} {abl:9}    0 | 无有效帧（全热态或未采集）')
                continue
            pp = stats(fr, 'physMs'); rr = stats(fr, 'renderMs')
            ss = stats(fr, 'presentMs'); ff = stats(fr, 'frameTimeMs')
            print(f'{scene:8} {abl:9} {pp[2]:>4} | {pp[0]:>6.1f}/{pp[1]:<6.1f} | {rr[0]:>6.1f}/{rr[1]:<6.1f} | {ss[0]:>7.1f}/{ss[1]:<7.1f} | {ff[0]:>9.1f}')
    # 90Hz 可达性判读（OFF 臂 tilt 场景=代表动态轻载）
    print()
    path = os.path.join(base, f'seg3-{tag}-tilt-off-hilog.txt')
    if os.path.exists(path):
        fr = load(path)
        if fr:
            physP95 = stats(fr, 'physMs')[1]
            renderP95 = stats(fr, 'renderMs')[1]
            presentP50 = stats(fr, 'presentMs')[0]
            frameP50 = stats(fr, 'frameTimeMs')[0]
            shaderRender = 2.0   # shader 化后渲染残段保守预估（ms）
            reachable = physP95 + shaderRender < 11.1
            print('90Hz 单拍可达性判读（tilt/OFF 代表窗）:')
            print(f'  物理 P95 = {physP95:.1f}ms')
            print(f'  Canvas 渲染 P95 = {renderP95:.1f}ms（shader 化理论可省段，保守残段按 {shaderRender}ms）')
            print(f'  present P50 = {presentP50:.1f}ms（含 vsync 空转；frame P50 = {frameP50:.1f}）')
            print(f'  → 物理P95 + shader残段 = {physP95 + shaderRender:.1f}ms {"<" if reachable else "≥"} 11.1ms 单拍预算')
            print(f'  → 实测级结论：{"90Hz 单拍可达（shader 化后渲染不构成瓶颈，物理是约束）" if reachable else "90Hz 单拍不可达（物理段超预算——须先治物理，换栈收益有限）"}')
    else:
        print('（tilt/OFF 数据未采集——真机出数后重跑本脚本）')

if __name__ == '__main__':
    main()
