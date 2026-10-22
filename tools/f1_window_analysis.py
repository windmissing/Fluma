import json, re, sys
from collections import defaultdict

def parse(hilog_path):
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
        ts = re.match(r'\d+-\d+ (\d+):(\d+):(\d+)\.(\d+)', line)
        if ts:
            secs = int(ts.group(1)) * 3600 + int(ts.group(2)) * 60 + int(ts.group(3)) + int(ts.group(4)) / 1000
            d['_t'] = secs
        frames.append(d)
    return frames

def stats(vals):
    vals = sorted(vals)
    n = len(vals)
    if n == 0:
        return 'n=0'
    p50 = vals[n // 2] if n % 2 == 1 else (vals[n // 2 - 1] + vals[n // 2]) / 2
    p95 = vals[min(n - 1, int(round(0.95 * (n - 1))))]
    return f'n={n} p50={p50} p95={p95} mx={vals[-1]}'

frames = parse(sys.argv[1])
# 按 substeps 与 frameTime 分布分桶：静止（substeps=0）vs 动态
static = [f for f in frames if f['substeps'] == 0]
dynamic = [f for f in frames if f['substeps'] > 0]
print('静止帧（substeps=0，dormant 跳过物理）:')
print('  frameTimeMs :', stats([f['frameTimeMs'] for f in static]))
print('动态帧（substeps>=1）:')
print('  frameTimeMs :', stats([f['frameTimeMs'] for f in dynamic]))
print('  physicsTimeMs:', stats([f['physicsTimeMs'] for f in dynamic]))
print('  render+other:', stats([f['frameTimeMs'] - f['physicsTimeMs'] for f in dynamic]))
# 动态帧再分：轻载（substeps=1）与重载（substeps=2）
for s in (1, 2):
    g = [f for f in frames if f['substeps'] == s]
    if g:
        print(f'动态帧 substeps={s}:')
        print('  frameTimeMs :', stats([f['frameTimeMs'] for f in g]))
        print('  physicsTimeMs:', stats([f['physicsTimeMs'] for f in g]))
        print('  render+other:', stats([f['frameTimeMs'] - f['physicsTimeMs'] for f in g]))
# frameTimeMs 节拍分布（vsync 量化）
beat = defaultdict(int)
for f in frames:
    beat[f['frameTimeMs']] += 1
top = sorted(beat.items(), key=lambda kv: -kv[1])[:12]
print('frameTimeMs 直方 top12:', top)
# 动态段的连续 30s 口径模拟：把帧按时间排序，滑窗 30s 计算 fpsP95
dyn = sorted(dynamic, key=lambda f: f['_t'])
if dyn:
    t0, t1 = dyn[0]['_t'], dyn[-1]['_t']
    print(f'动态段时长 {t1 - t0:.1f}s，帧数 {len(dyn)}，平均 fps = {len(dyn) / (t1 - t0):.1f}')
    # 30s 滑窗 fpsP95（每个窗口 = 该窗内帧间隔的 P95 → fps）
    win = 30.0
    worst = None
    best = None
    fpsp95_list = []
    i = 0
    while i < len(dyn):
        j = i
        while j < len(dyn) and dyn[j]['_t'] - dyn[i]['_t'] <= win:
            j += 1
        seg = dyn[i:j]
        if len(seg) > 30:
            fts = sorted(f['frameTimeMs'] for f in seg)
            p95 = fts[min(len(fts) - 1, int(round(0.95 * (len(fts) - 1))))]
            fpsp95 = 1000.0 / p95
            fpsp95_list.append(fpsp95)
        i = j
    if fpsp95_list:
        fpsp95_list.sort()
        print(f'30s 滑窗 fpsP95：min={fpsp95_list[0]:.1f} 中位={fpsp95_list[len(fpsp95_list)//2]:.1f} max={fpsp95_list[-1]:.1f}')
