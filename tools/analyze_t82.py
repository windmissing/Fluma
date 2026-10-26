#!/usr/bin/env python3
# T82 触摸修复后复测分析器
#   依据: docs/task-T82-touch-fix-retest-runbook-v1.md
#   L1-L6 从 demo_diag（T71 新 kind：input.RECEIVE/MAP/START/STOP + 旧 tick/dry.request/dry.receipt）
#   L7 截图像素统计（容器区可见沙）+ present 三态（code/currentFrameSucceeded/STALE/FAULT）
#   速率核算: dry.request requested ≈8 unit/s ±20%
import json, os, re, sys, glob, hashlib

ART = 'D:/hmprojects/HelloWorld/test/artifacts/t82-touch-fix-retest-20261026'
ARMS = ['R0', 'R1', 'R2', 'R3', 'R4', 'R5']
BOX = (51, 624, 1229, 2650)  # 容器 layout bounds（T74 实测；onAreaChange 后同区）

def openany(path):
    with open(path, 'rb') as f:
        raw = f.read()
    if raw.startswith(b'\xff\xfe') or raw.startswith(b'\xfe\xff'):
        return raw.decode('utf-16')
    if raw.startswith(b'\xef\xbb\xbf'):
        return raw.decode('utf-8-sig')
    return raw.decode('utf-8', errors='replace')

def jfix(payload):
    try:
        return json.loads(payload)
    except Exception:
        stack, in_str, esc = [], False, False
        for ch in payload:
            if in_str:
                if esc: esc = False
                elif ch == '\\': esc = True
                elif ch == '"': in_str = False
                continue
            if ch == '"': in_str = True
            elif ch in '{[': stack.append('}' if ch == '{' else ']')
            elif ch in '}]' and stack: stack.pop()
        try:
            return json.loads(payload + ('"' if in_str else '') + ''.join(reversed(stack)))
        except Exception:
            return None

def parse_diag(path):
    rows, dropped, broken = [], 0, 0
    summaries = []
    if not os.path.exists(path):
        return rows, dropped, broken, summaries
    for line in openany(path).splitlines():
        if 'demo_diag' in line:
            m = re.search(r'demo_diag\s+(\{.*\})\s*$', line.strip())
            if m:
                chunk = jfix(m.group(1))
                if chunk:
                    dropped = chunk.get('dropped', dropped)
                    for row in chunk.get('rows', []):
                        ev = row.get('event', {})
                        ev['_seq'] = row.get('sequence')
                        rows.append(ev)
                else:
                    broken += 1
        elif 'empty_sand_demo' in line:
            m = re.search(r'empty_sand_demo\s+(\{.*\})\s*$', line.strip())
            if m:
                s = jfix(m.group(1))
                if s:
                    summaries.append(s)
                else:
                    broken += 1
    return rows, dropped, broken, summaries

def kinds(rows):
    out = {}
    for r in rows:
        k = r.get('kind', '?')
        out[k] = out.get(k, 0) + 1
    return out

def pixel_audit(name):
    try:
        from PIL import Image
    except ImportError:
        return None
    p = os.path.join(ART, name)
    if not os.path.exists(p):
        return None
    img = Image.open(p).convert('RGB')
    box = img.crop(BOX).resize((100, 100))
    px = list(box.getdata())
    nonbg = sum(1 for r, g, b in px if abs(r - 231) > 18 or abs(g - 228) > 18 or abs(b - 220) > 18)
    dark = sum(1 for r, g, b in px if r < 120 and g < 120 and b < 120)
    return {'file': name, 'nonbg_px': nonbg, 'dark_px': dark, 'md5': hashlib.md5(img.tobytes()).hexdigest()[:10]}

def rate_check(reqs):
    """dry.request 速率核算：相邻请求 requested/Δt ≈ 8 unit/s。"""
    if len(reqs) < 2:
        return None
    import math
    rates = []
    for a, b in zip(reqs, reqs[1:]):
        dt = (b.get('time', 0) - a.get('time', 0)) / 1000
        if dt > 0.05:
            rates.append((b.get('requested', 0) - 0) / dt if False else (a.get('requested', 0) / dt))
    # 逐请求：requested/dt（相邻 request 间隔≈帧间隔）
    vals = []
    for a, b in zip(reqs, reqs[1:]):
        dt = (b.get('time', 0) - a.get('time', 0)) / 1000
        if dt > 0.005 and a.get('requested'):
            vals.append(a['requested'] / dt)
    if not vals:
        return None
    vals.sort()
    return {'n': len(vals), 'median': vals[len(vals)//2], 'min': vals[0], 'max': vals[-1]}

def analyze_arm(arm):
    res = {'arm': arm}
    rows, dropped, broken, summaries = parse_diag(os.path.join(ART, f'{arm}-full.txt'))
    res['diag'] = {'rows': len(rows), 'dropped': dropped, 'broken': broken, 'kinds': kinds(rows),
                   'summaries': len(summaries)}
    # L1
    recv = [r for r in rows if r.get('kind') == 'input.RECEIVE']
    downs = [r for r in recv if r.get('source') == 'DOWN']
    ups = [r for r in recv if r.get('source') == 'UP']
    ready = all(r.get('geometryReady') for r in recv) if recv else None
    wv = sorted({r.get('widthVp') for r in recv if r.get('widthVp')})
    hv = sorted({r.get('heightVp') for r in recv if r.get('heightVp')})
    res['L1'] = {'receive': len(recv), 'downs': len(downs), 'ups': len(ups), 'geometry_ready': ready,
                 'widthVp': wv, 'heightVp': hv, 'first': recv[0] if recv else None}
    # L2
    maps = [r for r in rows if r.get('kind') == 'input.MAP']
    starts = [r for r in rows if r.get('kind') == 'input.START']
    stops = [r for r in rows if r.get('kind') == 'input.STOP']
    model_starts = [r for r in rows if r.get('kind') == 'model.start']
    res['L2'] = {'maps': len(maps), 'map_sample': maps[:3],
                 'start_reasons': [s.get('reason') for s in starts],
                 'accepted_starts': sum(1 for s in starts if s.get('reason') == 'ACCEPTED'),
                 'stop_reasons': [s.get('reason') for s in stops],
                 'model_start_reasons': [m.get('reason') for m in model_starts]}
    # L3
    ticks = [r for r in rows if r.get('kind') == 'tick']
    stale = sum(1 for t in ticks if t.get('source') == 'stale')
    res['L3'] = {'ticks': len(ticks), 'stale': stale}
    # L4
    reqs = [r for r in rows if r.get('kind') == 'dry.request']
    res['L4'] = {'requests': len(reqs), 'requested_total': sum(r.get('requested') or 0 for r in reqs),
                 'rate_check': rate_check(reqs), 'first': reqs[0] if reqs else None,
                 'x_range': [min((r.get('x') for r in reqs), default=None), max((r.get('x') for r in reqs), default=None)],
                 'y_range': [min((r.get('y') for r in reqs), default=None), max((r.get('y') for r in reqs), default=None)]}
    # L5
    recs = [r for r in rows if r.get('kind') == 'dry.receipt']
    reasons = {}
    for r in recs:
        reasons[r.get('reason', '?')] = reasons.get(r.get('reason', '?'), 0) + 1
    res['L5'] = {'receipts': len(recs), 'reasons': reasons,
                 'admitted_total': sum(r.get('admitted') or 0 for r in recs)}
    # L6
    pub = {}
    if summaries:
        last = summaries[-1]
        dry = last.get('dry') or {}
        met = last.get('metrics') or {}
        pub = {'last_dry': dry, 'metrics_subset': {k: met.get(k) for k in
               ('particleMass', 'depositedMass', 'expectedMass', 'sourceEpoch', 'particleEvidenceKind',
                'depositedFraction', 'suspFraction', 'airborneFraction') if k in met},
               'tilt': last.get('effectiveTiltDeg'), 'input': last.get('input')}
    res['L6'] = pub
    # L7 present
    presents = [r for r in rows if r.get('kind') == 'present']
    codes = {}
    stale_frames = 0
    for p in presents:
        codes[p.get('code')] = codes.get(p.get('code'), 0) + 1
        if p.get('reason') and 'STALE' in str(p.get('reason')):
            stale_frames += 1
    res['L7_present'] = {'presents': len(presents), 'codes': codes, 'stale_frames': stale_frames,
                         'failure_reasons': sorted({str(p.get('reason')) for p in presents if p.get('reason')})[:6]}
    # L7 pixels
    shots = {}
    for f in sorted(glob.glob(os.path.join(ART, f'{arm}-*.jpeg'))):
        a = pixel_audit(os.path.basename(f))
        if a:
            shots[os.path.basename(f)] = a
    res['L7_pixels'] = shots
    # verdict
    v = {}
    v['L1'] = 'PASS' if (recv and downs and ready) else 'FAIL'
    v['L2'] = ('PASS' if (maps and any(s.get('reason') == 'ACCEPTED' for s in starts)) else 'FAIL') if v['L1'] == 'PASS' else 'N/A'
    v['L3'] = ('PASS' if ticks else 'FAIL') if v['L2'] == 'PASS' else 'N/A'
    v['L4'] = ('PASS' if reqs else 'FAIL') if v['L2'] == 'PASS' else 'N/A'
    v['L5'] = ('PASS' if any(r.get('reason') == 'ACCEPTED' and (r.get('admitted') or 0) > 0 for r in recs) else 'FAIL') if reqs else ('N/A' if v['L4'] != 'PASS' else 'FAIL')
    pm = (pub.get('metrics_subset') or {}).get('particleMass') or 0
    v['L6'] = ('PASS' if pm > 0 else 'FAIL') if v['L5'] == 'PASS' else 'N/A'
    good_pix = any((a['nonbg_px'] > 50 or a['dark_px'] > 20) for a in shots.values()) if shots else False
    v['L7'] = ('PASS' if (good_pix and not stale_frames and codes.get(0, 0) > 0) else 'FAIL') if v['L6'] == 'PASS' else 'N/A'
    res['verdict'] = v
    res['first_fail'] = next((k for k in ('L1', 'L2', 'L3', 'L4', 'L5', 'L6', 'L7') if v.get(k) == 'FAIL'), None)
    return res

def main():
    out = {'artifact_dir': ART,
           'build': {'commit': '114a7ee', 'tree_fingerprint': '403b08514017b326',
                     'hap': 'entry-default-s144-signed.hap',
                     'hap_sha256': 'af049753c8dda7e169bcf0613f384b7affa3aa3cf1fb1fac7cd65ad64db31be8'},
           'arms': {}}
    for arm in ARMS:
        if not os.path.exists(os.path.join(ART, f'{arm}-full.txt')):
            continue
        r = analyze_arm(arm)
        out['arms'][arm] = r
        print(f"\n===== {arm} =====")
        d = r['diag']
        print(f"  diag: rows={d['rows']} dropped={d['dropped']} broken={d['broken']} kinds={d['kinds']}")
        print(f"  L1: {r['verdict']['L1']} recv={r['L1']['receive']} downs={r['L1']['downs']} ups={r['L1']['ups']} "
              f"ready={r['L1']['geometry_ready']} W/H={r['L1']['widthVp']}x{r['L1']['heightVp']}")
        print(f"  L2: {r['verdict']['L2']} maps={r['L2']['maps']} accepted={r['L2']['accepted_starts']} "
              f"start_reasons={r['L2']['start_reasons']} stops={r['L2']['stop_reasons'][:8]} model={r['L2']['model_start_reasons'][:4]}")
        print(f"  L3: {r['verdict']['L3']} ticks={r['L3']['ticks']} stale={r['L3']['stale']}")
        rc = r['L4']['rate_check']
        print(f"  L4: {r['verdict']['L4']} reqs={r['L4']['requests']} total={r['L4']['requested_total']:.2f} "
              f"rate~{rc['median']:.2f}u/s n={rc['n']} min={rc['min']:.2f} max={rc['max']:.2f}" if rc else
              f"  L4: {r['verdict']['L4']} reqs={r['L4']['requests']} total={r['L4']['requested_total']:.2f} rate=N/A")
        print(f"      x_range={r['L4']['x_range']} y_range={r['L4']['y_range']}")
        print(f"  L5: {r['verdict']['L5']} receipts={r['L5']['receipts']} reasons={r['L5']['reasons']} admitted={r['L5']['admitted_total']:.3f}")
        print(f"  L6: {r['verdict']['L6']} metrics={r['L6'].get('metrics_subset')} dry={r['L6'].get('last_dry')}")
        print(f"  L7 present: {r['verdict']['L7']} codes={r['L7_present']['codes']} stale={r['L7_present']['stale_frames']} fails={r['L7_present']['failure_reasons']}")
        for f, a in r['L7_pixels'].items():
            print(f"      {f}: nonbg={a['nonbg_px']} dark={a['dark_px']} md5={a['md5']}")
        print(f"  first_fail = {r['first_fail']}")
    with open(os.path.join(ART, 't82-analysis.json'), 'w', encoding='utf-8') as fh:
        json.dump(out, fh, ensure_ascii=False, indent=1, default=str)
    print(f"\nwrote {os.path.join(ART, 't82-analysis.json')}")

if __name__ == '__main__':
    main()
