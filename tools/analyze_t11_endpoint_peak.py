#!/usr/bin/env python3
"""T11 P6 设备取证分析器：endpoint 合规 run 与同账本 peak 双报（判读 + 资格披露）
   协议口径: docs/endpoint-protocol-P011-terminal-nondeposited-v1.md
   资格判据: docs/tester-T40-o2-supplementary-v1.md §4（STREAMED/SUMMARY_ONLY/INCOMPLETE/UNKNOWN）
用法: python analyze_t11_endpoint_peak.py <art_dir>
输入: tools/t11_endpoint_peak_run.ps1 采集的 T11-A/B/C-full.txt 与截图。
"""
import sys, os, re, json, glob
from collections import Counter

ART = sys.argv[1] if len(sys.argv) > 1 else '.'
EPS = 1e-9
THRESH = 0.05


def openany(p):
    raw = open(p, 'rb').read()
    # PS5.1 Out-File -Encoding utf8 写 UTF-8 with BOM；hilog 原生为 UTF-8。
    # 先检 BOM，再按 utf-8-sig/utf-16-le 显式判定；纯猜 utf-16 会把 UTF-8
    # 字节流解成乱码（正则失配 → 误报"无 WS_METRIC"）。
    if raw[:3] == b'\xef\xbb\xbf':
        return raw[3:].decode('utf-8', errors='replace')
    if raw[:2] == b'\xff\xfe':
        return raw.decode('utf-16', errors='replace')
    try:
        return raw.decode('utf-8')
    except UnicodeDecodeError:
        return raw.decode('utf-8', errors='replace')


def jl(s):
    try:
        return json.loads(s)
    except Exception:
        return None


def parse_arm(path):
    t = openany(path)
    out = {'metrics': [], 'snapshots': [], 'gesture': [], 'shake_lift': 0, 'frames': [],
           'obs_store': None, 'init': None}
    for m in re.finditer(r'WS_METRIC (\{.*\})\s*$', t, re.M):
        d = jl(m.group(1))
        if d:
            out['metrics'].append(d)
    for m in re.finditer(r'WS_SNAPSHOT (\{.*\})\s*$', t, re.M):
        d = jl(m.group(1))
        if d:
            out['snapshots'].append(d)
    for m in re.finditer(r'WS_EVENT gesture_sand (\{.*\})\s*$', t, re.M):
        d = jl(m.group(1))
        if d:
            out['gesture'].append(d)
    out['shake_lift'] = len(re.findall(r'WS_EVENT shake_lift', t))
    out['snap_total'] = len(re.findall(r'WS_SNAPSHOT', t))
    for m in re.finditer(r'WS_FRAME (\{.*?\}) seg=', t, re.M):
        d = jl(m.group(1))
        if d:
            out['frames'].append(d)
    m = re.search(r'P011_OBS_STORE (\{.*\})\s*$', t, re.M)
    if m:
        out['obs_store'] = jl(m.group(1))
    return out


def snapshot_recalc(s):
    """从 WS_SNAPSHOT（schema3 全量账本）独立重算 D/U（协议 M1-M8 口径，P=0 时）。"""
    D = sum(s['bh']) + sum(s['bl']) + sum(s['mh']) + sum(s['ml'])
    U = sum(s['suspH']) + sum(s['suspL'])
    return D, U


def arm_verdict(name, a):
    print('\n===== %s =====' % name)
    ms = a['metrics']
    if not ms:
        print('  无 WS_METRIC：UNPROVEN')
        return
    last = ms[-1]
    E = last['expectedMass']
    print('  样本: metric=%d snapshot=%d gesture_sand=%d shake_lift=%d frame=%d' %
          (len(ms), len(a['snapshots']), len(a['gesture']), a['shake_lift'], len(a['frames'])))
    print('  身份: endpointFieldRevision=%s runId=%s sourceEpoch=%d' %
          (last.get('endpointFieldRevision'), a['obs_store']['runId'] if a['obs_store'] else '?',
           last.get('sourceEpoch', -1)))
    key_nn = sum(1 for d in ms if d.get('metricCommitKey'))
    print('  metricCommitKey 非空: %d/%d（endpointSampler 未装配时恒 null）' % (key_nn, len(ms)))
    print('  particleEvidenceKind=%s（粒子路线未激活 → P=0，分子 M=U）' % last.get('particleEvidenceKind'))
    # ---- endpoint 末 1s 窗（协议 §2） ----
    last4 = ms[-4:]
    ratios = [(d['suspendedMass'] + d['particleMass']) / d['expectedMass'] for d in last4]
    peak = max((d['suspendedMass'] + d['particleMass']) / d['expectedMass'] for d in ms)
    print('  末1s窗 M/R: %s' % ['%.6f' % r for r in ratios])
    print('  全程窗峰 M/R: %.6f' % peak)
    for r in ratios:
        if r > THRESH + EPS:
            verdict = 'FAIL（摇晃态：M/R=%.4f > 5%%，非"摇晃后静置"场景）' % r
            break
    else:
        verdict = 'PASS（末1s窗 M/R ≤ 5%%）'
    print('  endpoint 判定(协议§1.3): %s' % verdict)
    # ---- 守恒 ----
    drift = abs(last['actualMass'] - E)
    print('  守恒: actualMass-E = %.3e（≤1e-9 %s）' % (drift, 'PASS' if drift <= 1e-9 else 'FAIL'))
    # ---- WS_SNAPSHOT 独立重算交叉核对（尾部同时刻对齐） ----
    snap_total = a.get('snap_total', len(a['snapshots']))
    if a['snapshots']:
        # 初始化时会多打一条快照（早于首条 metric），取尾部 min(n) 条同时刻对齐。
        # 设备 hilog 单条约 4096 字节上限：全量账本快照（~50KB）摇晃/注入态被截断，
        # 仅初始化态（数组短）完整 —— 截断行已在解析期过滤，此处披露比例。
        n = min(len(a['snapshots']), len(ms))
        ok = bad = 0
        worst = 0.0
        for i in range(n):
            D, U = snapshot_recalc(a['snapshots'][-n + i])
            m = ms[-n + i]
            e1 = abs(U - m['suspendedMass'])
            e2 = abs(D - m['depositedMass'])
            worst = max(worst, e1, e2)
            if e1 <= 1e-6 and e2 <= 1e-6:
                ok += 1
            else:
                bad += 1
        print('  快照独立重算 vs WS_METRIC（尾部对齐 %d 点）: 一致 %d / 不一致 %d（最大偏差 %.2e）' % (n, ok, bad, worst))
        print('  快照完整率: %d/%d 行（hilog 单条 ~4096B 上限，全量账本快照摇晃态被截断）' %
              (len(a['snapshots']), snap_total))
    # ---- gesture_sand 逐笔（T11-B 外源准入） ----
    gs = a['gesture']
    if gs:
        adm = sum(1 for g in gs if g.get('rejectedReason') in (None, 'None'))
        tot_h = sum(g.get('admittedH', 0) for g in gs)
        tot_l = sum(g.get('admittedL', 0) for g in gs)
        print('  注入: %d 笔，准入 %d（rejectedReason=None），admittedH=%.6f admittedL=%.6f' %
              (len(gs), adm, tot_h, tot_l))
        inj_h = last['sourceInjectedHeavy']; inj_l = last['sourceInjectedLight']
        print('  账本 sourceInjected: H=%.6f L=%.6f（与逐笔累计 %s）' %
              (inj_h, inj_l, '一致' if abs(inj_h - tot_h) <= 1e-6 and abs(inj_l - tot_l) <= 1e-6 else '不一致'))
        print('  T40 口径 externalAdmissionCount==sourceEpoch: epoch=%d vs 准入笔数=%d → %s' %
              (last['sourceEpoch'], adm, '一致（合法相等）' if last['sourceEpoch'] == adm else '不一致'))
        print('  R_WS=E=VH0+VL0+IH+IL: E=%.6f（初值5120 + 注入 %.6f）' % (E, tot_h + tot_l))
    # ---- 帧健康（H01） ----
    if a['frames']:
        mx = max(f.get('frameTimeMs', 0) for f in a['frames'])
        import statistics
        mean = statistics.mean(f.get('frameTimeMs', 0) for f in a['frames'])
        print('  帧: n=%d max=%.1fms mean=%.2fms（>1000ms 计冻结: %d）' %
              (len(a['frames']), mx, mean, sum(1 for f in a['frames'] if f.get('frameTimeMs', 0) > 1000)))


def main():
    print('== T11 设备取证分析: %s' % os.path.abspath(ART))
    arms = sorted(glob.glob(os.path.join(ART, 'T11-*-full.txt')))
    if not arms:
        print('无 T11-*-full.txt'); return
    store_ok = None
    for p in arms:
        a = parse_arm(p)
        arm_verdict(os.path.basename(p).replace('-full.txt', ''), a)
        if a['obs_store']:
            store_ok = a['obs_store'].get('filePath')
    print('\n===== 完整性资格（T40 §4 判据）=====')
    print('  原始事件导出开关（setExternalAdmissionEventSink）: 页面侧无装配 → 无 STREAMED 记录')
    print('  P011_OBS_STORE filePath=%s' % store_ok)
    print('  store worker（P011NaturalObservationWorker.abc）未打入 HAP（hvigor 不为非 workers/ 目录生成独立 abc）')
    print('  ⇒ 资格状态: SUMMARY_ONLY（仅 WS_METRIC/WS_SNAPSHOT 汇总与逐点全量；无准入事务流）')
    print('  peak 双报: peakSampler 需 setEndpointSampling(clock,phase) 显式启用，页面侧无装配')
    print('            + store 不可用 ⇒ BLOCKED（peakNonDepositedFraction 无设备出口）')
    print('\nDONE')


if __name__ == '__main__':
    main()
