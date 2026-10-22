import json, re, sys, glob

def parse_run(hilog_path):
    frames = []
    for line in open(hilog_path, encoding='utf-8', errors='replace'):
        if 'WS_FRAME' not in line:
            continue
        m = re.search(r'WS_FRAME (\{.*\})', line)
        if not m:
            continue
        try:
            d = json.loads(m.group(1))
        except Exception:
            continue
        frames.append(d)
    return frames

def stats(vals):
    vals = sorted(vals)
    n = len(vals)
    if n == 0:
        return {}
    p50 = vals[n // 2] if n % 2 == 1 else (vals[n // 2 - 1] + vals[n // 2]) / 2
    p95 = vals[min(n - 1, int(round(0.95 * (n - 1))))]
    return dict(n=n, mean=sum(vals) / n, p50=p50, p95=p95, mx=vals[-1])

for path in sys.argv[1:]:
    frames = parse_run(path)
    if not frames:
        print(path, ': no WS_FRAME')
        continue
    ft = [f['frameTimeMs'] for f in frames]
    pt = [f['physicsTimeMs'] for f in frames]
    # 渲染+合成+其它 = frame - physics
    rdt = [max(0, f['frameTimeMs'] - f['physicsTimeMs']) for f in frames]
    sub = [f['substeps'] for f in frames]
    print('===', path.split('artifacts')[-1], f'frames={len(frames)}')
    print(' frameTimeMs  :', stats(ft))
    print(' physicsTimeMs:', stats(pt))
    print(' render+other:', stats(rdt))
    print(' substeps dist:', {s: sub.count(s) for s in set(sub)})
    # fpsP95 = 1000/p95(frameTimeMs)
    s = stats(ft)
    if s:
        print(' fpsP95(1000/p95ft) = %.1f' % (1000.0 / s['p95']))
