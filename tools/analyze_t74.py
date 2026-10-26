#!/usr/bin/env python3
# T74 触摸零源设备补采分析器
#   依据: docs/task-T74-touch-source-device-capture-runbook-v1.md
#   输入: test/artifacts/t74-touch-source-capture-20261026/R*-full.txt
#   输出: t74-analysis.json + 控制台 L1-L7 逐级 verdict
# 事件链: L1 RECEIVE(touch) -> L2 MAP/START(model.start/move) -> L3 CLOCK(tick source/stale)
#         -> L4 REQUEST(dry.request) -> L5 RECEIPT(dry.receipt) -> L6 PUBLICATION(metrics/dry非零)
#         -> L7 RENDER(present code/swap + 截图)
import json, os, re, sys, glob

ART = 'D:/hmprojects/HelloWorld/test/artifacts/t74-touch-source-capture-20261026'
ARMS = ['R0', 'R1', 'R2', 'R3', 'R4']

def openany(path):
    with open(path, 'rb') as f:
        raw = f.read()
    if raw.startswith(b'\xff\xfe') or raw.startswith(b'\xfe\xff'):
        return raw.decode('utf-16')
    if raw.startswith(b'\xef\xbb\xbf'):
        return raw.decode('utf-8-sig')
    return raw.decode('utf-8', errors='replace')

def parse_diag_chunks(path):
    """解析 demo_diag chunk 行（T72 环形缓冲 flush，单行 JSON）。"""
    rows = []
    dropped_total = 0
    chunks = 0
    broken = 0
    demo_summaries = []
    present_lines = []  # present 也以单事件记录在 chunk 内，但兜底扫描散行
    if not os.path.exists(path):
        return rows, dropped_total, chunks, broken, demo_summaries, present_lines
    pat = re.compile(r'demo_diag\s+(\{.*\})\s*$')
    for line in openany(path).splitlines():
        if 'demo_diag' not in line:
            if 'empty_sand_demo' in line:
                m = re.search(r'empty_sand_demo\s+(\{.*\})\s*$', line.strip())
                if m:
                    try:
                        demo_summaries.append(json.loads(m.group(1)))
                    except Exception:
                        broken += 1
            if 'present' in line and 'code' in line:
                m = re.search(r'\{.*\}', line)
                if m:
                    try:
                        present_lines.append(json.loads(m.group(0)))
                    except Exception:
                        pass
            continue
        m = pat.search(line.strip())
        if not m:
            continue
        payload = m.group(1)
        try:
            chunk = json.loads(payload)
        except Exception:
            # 截断容忍：补全括号
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
                chunk = json.loads(payload + ('"' if in_str else '') + ''.join(reversed(stack)))
            except Exception:
                broken += 1
                continue
        chunks += 1
        dropped_total = chunk.get('dropped', dropped_total)
        for row in chunk.get('rows', []):
            ev = row.get('event', {})
            ev['_seq'] = row.get('sequence')
            rows.append(ev)
    return rows, dropped_total, chunks, broken, demo_summaries, present_lines

def kinds(rows):
    out = {}
    for r in rows:
        k = r.get('kind', '?')
        out[k] = out.get(k, 0) + 1
    return out

def seq_gaps(rows):
    seqs = [r.get('_seq') for r in rows if isinstance(r.get('_seq'), int)]
    if not seqs:
        return []
    gaps = []
    for a, b in zip(seqs, seqs[1:]):
        if b - a > 1:
            gaps.append((a, b, b - a - 1))
    return gaps

def analyze_arm(arm):
    res = {'arm': arm}
    rows, dropped, chunks, broken, summaries, present_loose = parse_diag_chunks(os.path.join(ART, f'{arm}-full.txt'))
    res['diag'] = {'rows': len(rows), 'chunks': chunks, 'dropped': dropped, 'broken_lines': broken,
                   'kinds': kinds(rows), 'seq_gaps': seq_gaps(rows)[:10]}
    res['summaries'] = len(summaries)
    # ---- L1 RECEIVE ----
    touches = [r for r in rows if r.get('kind') == 'touch']
    ttypes = {}
    for t in touches:
        ttypes[t.get('source', '?')] = ttypes.get(t.get('source', '?'), 0) + 1
    res['L1_receive'] = {'touch_events': len(touches), 'types': ttypes,
                         'first': touches[0] if touches else None,
                         'sample': touches[:6]}
    # ---- L2 MAP/START ----
    starts = [r for r in rows if r.get('kind') == 'model.start']
    moves = [r for r in rows if r.get('kind') == 'model.move']
    stops = [r for r in rows if r.get('kind') == 'model.stop']
    res['L2_start'] = {'starts': len(starts), 'moves': len(moves), 'stops': len(stops),
                       'start_sample': starts[:4], 'stop_reasons': [s.get('reason') for s in stops[:10]]}
    # ---- L3 CLOCK ----
    ticks = [r for r in rows if r.get('kind') == 'tick']
    src = {}
    for t in ticks:
        src[t.get('source', 'none')] = src.get(t.get('source', 'none'), 0) + 1
    stale_hits = sum(1 for t in ticks if t.get('source') == 'stale')
    res['L3_clock'] = {'ticks': len(ticks), 'sources': src, 'stale_hits': stale_hits,
                       'max_stale_count': max((t.get('staleCount') or 0 for t in ticks), default=0)}
    # ---- L4 REQUEST ----
    reqs = [r for r in rows if r.get('kind') == 'dry.request']
    wet_reqs = [r for r in rows if r.get('kind', '').startswith('wet')]
    res['L4_request'] = {'dry_requests': len(reqs), 'wet_requests': len(wet_reqs),
                         'requested_total': sum(r.get('requested') or 0 for r in reqs),
                         'first': reqs[0] if reqs else None, 'last': reqs[-1] if reqs else None}
    # ---- L5 RECEIPT ----
    recs = [r for r in rows if r.get('kind') == 'dry.receipt']
    reasons = {}
    for r in recs:
        rr = r.get('reason', '?')
        reasons[rr] = reasons.get(rr, 0) + 1
    admitted_total = sum(r.get('admitted') or 0 for r in recs)
    res['L5_receipt'] = {'dry_receipts': len(recs), 'reasons': reasons, 'admitted_total': admitted_total,
                         'sample': recs[:4]}
    # ---- L6 PUBLICATION ----
    pub = {'summaries': len(summaries)}
    if summaries:
        last = summaries[-1]
        dry = last.get('dry') or {}
        met = last.get('metrics') or {}
        pub['last_dry'] = dry
        pub['last_metrics_subset'] = {k: met.get(k) for k in ('particleMass', 'depositedMass', 'expectedMass',
                                                              'actualMass', 'sourceEpoch', 'particleEvidenceKind',
                                                              'depositedFraction', 'suspFraction', 'airborneFraction') if k in met}
        pub['last_effectiveTiltDeg'] = last.get('effectiveTiltDeg')
        pub['last_input'] = last.get('input')
        pub['last_mediumMode'] = last.get('mediumMode')
    res['L6_publication'] = pub
    # ---- L7 RENDER ----
    presents = [r for r in rows if r.get('kind') == 'present'] + [
        r for r in present_loose if r.get('kind') == 'present']
    codes = {}
    for p in presents:
        c = p.get('code')
        codes[c] = codes.get(c, 0) + 1
    swap_true = sum(1 for p in presents if p.get('swap') is True)
    fails = sorted({p.get('reason') for p in presents if p.get('reason')})
    res['L7_render'] = {'present_events': len(presents), 'codes': codes, 'swap_true': swap_true,
                        'failure_reasons': fails[:6],
                        'last': presents[-1] if presents else None}
    # ---- 逐级 verdict ----
    v = {}
    v['L1'] = 'PASS' if len(touches) > 0 else 'FAIL'
    v['L2'] = 'PASS' if len(starts) > 0 else ('FAIL' if v['L1'] == 'PASS' else 'N/A')
    v['L3'] = 'PASS' if (ticks and stale_hits == 0) else ('FAIL' if v['L2'] == 'PASS' and stale_hits > 0 else ('PASS?' if ticks else 'N/A'))
    v['L4'] = 'PASS' if len(reqs) > 0 else ('FAIL' if v['L2'] == 'PASS' else 'N/A')
    v['L5'] = ('PASS' if any(r.get('reason') == 'ACCEPTED' for r in recs) else 'FAIL') if reqs else ('N/A' if v['L4'] != 'PASS' else 'FAIL')
    v['L6'] = 'PASS' if ((pub.get('last_metrics_subset', {}).get('particleMass') or 0) > 0 or
                         (pub.get('last_dry') or {}).get('particleCount', 0) > 0) else ('FAIL' if v['L5'] == 'PASS' else 'N/A')
    v['L7'] = 'PASS' if (presents and swap_true > 0 and not fails) else ('FAIL' if fails else ('N/A' if not presents else 'FAIL'))
    res['verdict'] = v
    # 首个可证失败点
    first_fail = next((k for k in ('L1', 'L2', 'L3', 'L4', 'L5', 'L6', 'L7') if v.get(k) == 'FAIL'), None)
    res['first_fail'] = first_fail
    return res

def main():
    if not os.path.isdir(ART):
        print(f'artifacts dir not found: {ART}')
        sys.exit(1)
    out = {'artifact_dir': ART,
           'build': {'commit': '545f45e', 'tree_fingerprint': '5b34bc45ce9cc5b7',
                     'hap': 'entry-default-s143-signed.hap',
                     'hap_sha256': 'babc44c65f8fae41daecfd8f911b0bb53d01261a12d6e9c11bb1395061c9d440'},
           'arms': {}}
    for arm in ARMS:
        if not os.path.exists(os.path.join(ART, f'{arm}-full.txt')):
            continue
        r = analyze_arm(arm)
        out['arms'][arm] = r
        print(f"\n===== {arm} =====")
        d = r['diag']
        print(f"  diag: rows={d['rows']} chunks={d['chunks']} dropped={d['dropped']} broken={d['broken_lines']} "
              f"gaps={len(d['seq_gaps'])} kinds={ {k: v for k, v in sorted(d['kinds'].items())} }")
        print(f"  L1 RECEIVE: {r['verdict']['L1']} touches={r['L1_receive']['touch_events']} types={r['L1_receive']['types']}")
        print(f"  L2 START:   {r['verdict']['L2']} starts={r['L2_start']['starts']} moves={r['L2_start']['moves']} stops={r['L2_start']['stops']} reasons={r['L2_start']['stop_reasons']}")
        print(f"  L3 CLOCK:   {r['verdict']['L3']} ticks={r['L3_clock']['ticks']} sources={r['L3_clock']['sources']} stale={r['L3_clock']['stale_hits']}")
        print(f"  L4 REQUEST: {r['verdict']['L4']} dry={r['L4_request']['dry_requests']} wet={r['L4_request']['wet_requests']} req_total={r['L4_request']['requested_total']:.3f}")
        print(f"  L5 RECEIPT: {r['verdict']['L5']} receipts={r['L5_receipt']['dry_receipts']} reasons={r['L5_receipt']['reasons']} admitted={r['L5_receipt']['admitted_total']:.3f}")
        print(f"  L6 PUB:     {r['verdict']['L6']} metrics={r['L6_publication'].get('last_metrics_subset')}")
        print(f"  L7 RENDER:  {r['verdict']['L7']} present={r['L7_render']['present_events']} codes={r['L7_render']['codes']} swapT={r['L7_render']['swap_true']} fails={r['L7_render']['failure_reasons']}")
        print(f"  first_fail = {r['first_fail']}")
    shots = sorted(os.path.basename(p) for p in glob.glob(os.path.join(ART, '*.jpeg')))
    out['screenshots'] = shots
    with open(os.path.join(ART, 't74-analysis.json'), 'w', encoding='utf-8') as fh:
        json.dump(out, fh, ensure_ascii=False, indent=1, default=str)
    print(f"\nwrote {os.path.join(ART, 't74-analysis.json')}")
    print(f"screenshots: {shots}")

if __name__ == '__main__':
    main()
