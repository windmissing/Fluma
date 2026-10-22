# -*- coding: utf-8 -*-
"""F1 消融分析：off vs dyn 配对块的静态/动态层占比
render+other 差 = 静态层（底色/边框/雾/颗粒/层理/高光）成本
按段（静置/慢倾/回正）分桶统计。"""
import json
import re
import sys
from collections import defaultdict


def parse(path):
    frames = []
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.search(r'WS_FRAME (\{.*\})', line)
        if not m:
            continue
        try:
            d = json.loads(m.group(1))
        except json.JSONDecodeError:
            continue
        ts = re.match(r'\d+-\d+ (\d+):(\d+):(\d+)\.(\d+)', line)
        if ts:
            d['_t'] = int(ts.group(1)) * 3600 + int(ts.group(2)) * 60 + int(ts.group(3)) + int(ts.group(4)) / 1000.0
        else:
            d['_t'] = 0
        frames.append(d)
    return frames


def seg_split(frames):
    """按 substeps 分段：静置(substeps=0) vs 动态(substeps>0)，再按 physicsTime 细分轻/重"""
    segs = {'static': [], 'dyn_light': [], 'dyn_heavy': []}
    for f in frames:
        if f['substeps'] == 0:
            segs['static'].append(f)
        elif f['physicsTimeMs'] <= 40:
            segs['dyn_light'].append(f)
        else:
            segs['dyn_heavy'].append(f)
    return segs


def pstats(vals):
    if not vals:
        return 'n=0'
    s = sorted(vals)
    return 'n=%d p50=%.1f p95=%.1f' % (len(s), s[len(s) // 2], s[int(len(s) * 0.95)] if len(s) > 1 else s[-1])


def main():
    off = parse(sys.argv[1])
    dyn = parse(sys.argv[2])
    print('off frames:', len(off), ' dyn frames:', len(dyn))
    soff = seg_split(off)
    sdyn = seg_split(dyn)
    print()
    print('%-10s | %-28s | %-28s | render+other 差(off-dyn)' % ('段', 'off', 'dyn'))
    for seg in ['static', 'dyn_light', 'dyn_heavy']:
        ro = [f['frameTimeMs'] - f['physicsTimeMs'] for f in soff[seg]]
        rd = [f['frameTimeMs'] - f['physicsTimeMs'] for f in sdyn[seg]]
        po = [f['physicsTimeMs'] for f in soff[seg]]
        # 配对差 p50：取两段各自 p50 之差（帧不逐一对齐）
        def med(v):
            s = sorted(v)
            return s[len(s) // 2] if s else float('nan')
        print('%-10s | %s | %s | Δp50=%+.1fms' % (seg, pstats(ro), pstats(rd), med(ro) - med(rd)))
        print('%-10s | physics %s' % ('', pstats(po)))
    # 帧时全貌
    print()
    for name, frames in [('off', off), ('dyn', dyn)]:
        ft = sorted(f['frameTimeMs'] for f in frames)
        print('%s frameTime: %s' % (name, pstats(ft)))
    # 静态层成本占比
    ro_s = [f['frameTimeMs'] - f['physicsTimeMs'] for f in soff['static']]
    rd_s = [f['frameTimeMs'] - f['physicsTimeMs'] for f in sdyn['static']]
    if ro_s and rd_s:
        m1 = sorted(ro_s)[len(ro_s) // 2]
        m2 = sorted(rd_s)[len(rd_s) // 2]
        print()
        print('静态层成本（静置段 render+other 差）: %.1f ms' % (m1 - m2))
        print('占 off 全量 render 预算 %.1f ms 的 %.0f%%' % (m1, 100 * (m1 - m2) / m1 if m1 > 0 else 0))
    ro_l = [f['frameTimeMs'] - f['physicsTimeMs'] for f in soff['dyn_light']]
    rd_l = [f['frameTimeMs'] - f['physicsTimeMs'] for f in sdyn['dyn_light']]
    if ro_l and rd_l:
        m1 = sorted(ro_l)[len(ro_l) // 2]
        m2 = sorted(rd_l)[len(rd_l) // 2]
        print('静态层成本（动态轻载段差）  : %.1f ms（占 %.0f%%）' % (m1 - m2, 100 * (m1 - m2) / m1 if m1 > 0 else 0))


if __name__ == '__main__':
    main()
