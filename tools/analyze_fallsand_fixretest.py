#!/usr/bin/env python3
"""P-011/P-012 fallsand fix-retest analysis: per-transaction WS_EVENT evidence,
shake path reachability, field-state delta, and long-injection freeze stats.
Usage: python analyze_fallsand_fixretest.py <art_dir>
"""
import sys, os, re, json, glob, collections
import numpy as np
from PIL import Image


def openany(p):
    raw = open(p, 'rb').read()
    for enc in ('utf-16', 'utf-8'):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode('utf-8', errors='replace')


def events(path):
    """Return list of (type, payload_dict) parsed from WS_EVENT lines."""
    if not os.path.exists(path):
        return []
    out = []
    for line in openany(path).splitlines():
        m = re.search(r'WS_EVENT (\w+) (\{.*\})\s*$', line)
        if not m:
            continue
        try:
            out.append((m.group(1), json.loads(m.group(2))))
        except Exception:
            pass
    return out


def metric_last(path):
    if not os.path.exists(path):
        return None
    ms = re.findall(r'WS_METRIC (\{.*\})', openany(path))
    if not ms:
        return None
    try:
        return json.loads(ms[-1])
    except Exception:
        return None


def greencount(path):
    if not os.path.exists(path):
        return None
    img = np.asarray(Image.open(path).convert('RGB')).astype(int)
    reg = img[1300:1990, 250:950]
    r, g, b = reg[:, :, 0], reg[:, :, 1], reg[:, :, 2]
    m = (g > r + 12) & (g > b + 8) & (g > 90)
    return int(m.sum())


def fieldstate(m):
    if not m:
        return 'n/a'
    return ('depH=%.9f suspF=%.9f phaseOrd=%.9f phys=%d dormant=%s sandTopDelta=%.9g' % (
        m['depositedHeavyFraction'], m['suspensionFraction'], m['phaseOrderScore'],
        m['physicsTimeMs'], m['dormant'], m['maxSandTopSubstepDeltaObserved']))


def part_a(art):
    print('=' * 78)
    print('PART A — 手势掉沙逐笔取证（WS_EVENT gesture_sand）')
    base = metric_last(os.path.join(art, 'A-metric-baseline.txt'))
    bgreen = greencount(os.path.join(art, 'A-00-baseline.jpeg'))
    print('  基线(沉降后): %s' % fieldstate(base))
    print('  基线水体区绿砂像素 = %s' % bgreen)
    for c in ['swipe-v600', 'fling-v200-s6', 'swipe-v200', 'drag-v200']:
        ep = os.path.join(art, 'A-events-%s.txt' % c)
        evs = events(ep)
        kinds = collections.Counter(k for k, _ in evs)
        gs = [d for k, d in evs if k == 'gesture_sand']
        pu = [d for k, d in evs if k == 'pan_update']
        pe = [d for k, d in evs if k == 'pan_end']
        src = collections.Counter(d.get('coordinateSource') for d in pu)
        acc = collections.Counter(str(d.get('accepted')) for d in pu)
        adm = [d for d in gs if (d.get('admittedH', 0) + d.get('admittedL', 0)) > 0]
        rej = collections.Counter(str(d.get('rejectedReason')) for d in gs)
        m = metric_last(os.path.join(art, 'A-metric-last-%s.txt' % c))
        g = greencount(os.path.join(art, 'A-after-%s.jpeg' % c))
        print('-' * 78)
        print('  [%s]  事件类型计数: %s' % (c, dict(kinds)))
        print('    pan_update: n=%d coordinateSource=%s accepted=%s' % (len(pu), dict(src), dict(acc)))
        print('    pan_end   : %s' % ([dict(d) for d in pe] or '无'))
        print('    gesture_sand: n=%d  准入(>0)=%d  拒绝原因分布=%s' % (len(gs), len(adm), dict(rej)))
        for d in gs[:3]:
            print('      e.g. requested=%.4f/%.4f admitted=%.4f/%.4f reason=%s' % (
                d.get('requested', {}).get('heavy', 0), d.get('requested', {}).get('light', 0),
                d.get('admittedH', 0), d.get('admittedL', 0), d.get('rejectedReason')))
        print('    字段状态: %s' % fieldstate(m))
        print('    绿砂像素: %s (基线 %s, Δ%s)' % (g, bgreen, (g - bgreen) if (g is not None and bgreen is not None) else 'n/a'))


def part_b(art):
    print('=' * 78)
    print('PART B — shake 通路')
    evs = events(os.path.join(art, 'B-events-shake.txt'))
    kinds = collections.Counter(k for k, _ in evs)
    print('  事件类型计数: %s' % dict(kinds))
    lifts = [d for k, d in evs if k == 'shake_lift']
    shakes = [d for k, d in evs if k == 'shake']
    for d in shakes[:3]:
        print('    WS_EVENT shake: %s' % d)
    for d in lifts[:5]:
        print('    WS_EVENT shake_lift: %s' % d)
    if lifts:
        tot = sum(d.get('lifted', 0) for d in lifts)
        cols = sum(d.get('columns', 0) for d in lifts)
        print('    shake_lift 累计: lifted=%.4f columns=%d (事件数 %d)' % (tot, cols, len(lifts)))
    ms = metric_last(os.path.join(art, 'B-metric-last-shake.txt'))
    mc = metric_last(os.path.join(art, 'B-metric-last-control.txt'))
    print('  shake=1.0 末态: %s' % fieldstate(ms))
    print('  shake=0.0 对照末态: %s' % fieldstate(mc))
    gs = greencount(os.path.join(art, 'B-shake-1.0.jpeg'))
    gc = greencount(os.path.join(art, 'B-control-0.0.jpeg'))
    print('  绿砂像素 shake=%.0f 对照=%.0f' % (gs or -1, gc or -1))


def part_c(art):
    print('=' * 78)
    print('PART C — 长时注入稳定性')
    p = os.path.join(art, 'C-longinj-wsframe.txt')
    if not os.path.exists(p):
        print('  (无 WS_FRAME 转储)')
        return
    ft, dor, ss = [], [], []
    for line in openany(p).splitlines():
        m = re.search(r'WS_FRAME (\{.*?\}) seg=', line)
        if not m:
            continue
        try:
            d = json.loads(m.group(1))
        except Exception:
            continue
        ft.append(d.get('frameTimeMs', 0)); dor.append(d.get('dormant')); ss.append(d.get('substeps', 0))
    if not ft:
        print('  (解析到 0 帧)')
        return
    s = sorted(ft); n = len(s)
    pc = lambda x: s[min(n - 1, int(x * n))]
    print('  帧数 n=%d  frameTimeMs min/P50/P95/P99/max = %.0f/%.0f/%.0f/%.0f/%.0f' % (
        n, s[0], pc(.5), pc(.95), pc(.99), s[-1]))
    print('  >1000ms(冻结) 帧数 = %d  | >200ms = %d' % (
        sum(1 for v in s if v > 1000), sum(1 for v in s if v > 200)))
    print('  dormant=True 占比 = %.1f%%   substeps>0 占比 = %.1f%%' % (
        100 * sum(1 for d in dor if d) / n, 100 * sum(1 for x in ss if x > 0) / n))
    ep = os.path.join(art, 'C-longinj-gesture_sand.txt')
    if os.path.exists(ep):
        gs = [d for k, d in events(ep) if k == 'gesture_sand']
        adm = [d for d in gs if (d.get('admittedH', 0) + d.get('admittedL', 0)) > 0]
        rej = collections.Counter(str(d.get('rejectedReason')) for d in gs)
        tot_h = sum(d.get('admittedH', 0) for d in gs)
        tot_l = sum(d.get('admittedL', 0) for d in gs)
        print('  注入事务 n=%d  准入数=%d  累计 admittedH=%.3f admittedL=%.3f  拒绝原因=%s' % (
            len(gs), len(adm), tot_h, tot_l, dict(rej)))


def main():
    art = sys.argv[1] if len(sys.argv) > 1 else '.'
    part_a(art)
    part_b(art)
    part_c(art)


if __name__ == '__main__':
    main()
