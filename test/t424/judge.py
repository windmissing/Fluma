#!/usr/bin/env python3
"""T424 · 判读器：读 test/reports/t424/matrix.json，按 T420 §6 输出 verdict。

对照:
  - T420 §2 真机基准: T368 停源高接触窗 callback2017-2144 N512/L157/Q2/A0/R942
    substep R471/wall p50 411ms；T352/T363 停源慢窗 N512/L101/Q2/A0/R534-606。
  - 哨兵: T403 LEVEL0/TILT25 grants/bedFinal 逐位（基线内 assert 已核）。
  - 判读字段: 逐帧 p50/p95 wall、Q/A/R 分布、role/cut 阶段成本、注入与休眠对照。
  - 本基线为 host 离线（性能数量级≠真机），只判工作界结构与可复跑性，
    不判 33.3ms 达标（那是真机 A/B 卡的范围）。
用法: python test/t424/judge.py [matrix.json]
退出码: 0=结构判读 PASS；1=发现结构缺口
"""
import json
import statistics
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
STAGES = ['callbackTail','input','injection','step','preview','collect','hotApply','field',
          'fillCopy','publicVerify','authorityCompare','seal','consumerCopy','sceneSetup',
          'pack','nativeDraw','structure','exact70','strictDomain','reserved']


def pctl(a, p):
    a = sorted(a)
    return a[min(len(a) - 1, int(p * len(a)))] if a else 0


def judge(path):
    d = json.loads(Path(path).read_text(encoding='utf-8-sig'))
    out = {'revision': 'T424_JUDGE_V1', 'source': str(path), 'entries': [], 'verdict': 'PASS'}
    budgets = d['budgets']

    # 哨兵判定
    ph = d['sentinels']['t403_physics']
    ok403 = (ph[0]['grants'] == [0.1, 0.1, 0.09999999999999998] and
             ph[1]['grants'] == [0.1, 0.032121583881089655, 0] and
             ph[0]['retirements'] == 3 and ph[1]['retirements'] == 1)
    out['entries'].append({'entry': 'SENTINEL_T403', 'pass': ok403,
                           'grants': [x['grants'] for x in ph]})
    if not ok403:
        out['verdict'] = 'FAIL'

    # 逐场景工作界
    for name, wrap in budgets.items():
        b = wrap['budget']
        rows = [r for r in b['rows'] if r[0] >= 4]
        subs = b['substeps']
        entry = {'scenario': name, 'frames': len(rows), 'substeps': len(subs),
                 'incomplete': b['incomplete'], 'invalid': b['invalid']}
        if rows:
            entry['wallP50'] = pctl([r[19] for r in rows], .5)
            entry['wallP95'] = pctl([r[19] for r in rows], .95)
            entry['QSum'] = sum(r[7] for r in rows)
            entry['ASum'] = sum(r[8] for r in rows)
            entry['RSum'] = sum(r[9] for r in rows)
            entry['fillCopySum'] = sum(r[10] for r in rows)
            entry['authoritySum'] = sum(r[16] for r in rows)
            stages = {}
            for i, s in enumerate(STAGES):
                vals = [r[24 + i] for r in rows]
                if any(v > 0 for v in vals):
                    stages[s] = {'p50': pctl(vals, .5), 'sum': sum(vals)}
            entry['stagesActive'] = {k: v for k, v in stages.items() if v['sum'] > 0}
        if subs:
            entry['subRSum'] = sum(s[9] for s in subs)
            entry['subWallP50'] = pctl([s[6] for s in subs], .5)
        entry['admission'] = wrap['admission']
        # 结构缺口: incomplete/invalid 必须为 0（互斥闭合）
        if b['incomplete'] or b['invalid']:
            entry['verdict'] = 'FAIL'
            out['verdict'] = 'FAIL'
        else:
            entry['verdict'] = 'PASS'
        out['entries'].append(entry)

    # T368 对照（真机基准锚，仅数量级参考）
    ref = REPO / 'test' / 'reports' / 't368' / 'v2-budget-t368.json'
    if ref.exists():
        t = json.loads(ref.read_text(encoding='utf-8-sig'))['budget']
        subs = t['substeps']
        out['t368Reference'] = {'window': 'callback2017-2144 N512/L157/Q2/A0/R942',
                                'substeps': len(subs),
                                'subWallP50DeviceMs': pctl([s[6] for s in subs], .5),
                                'subRP50Device': pctl([s[9] for s in subs], .5),
                                'note': '真机数量级锚；host 基线不可直接比 ms，只比工作界结构'}
    dest = REPO / 'test' / 'reports' / 't424' / 'judgement.json'
    dest.write_text(json.dumps(out, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': out['revision'], 'verdict': out['verdict'],
                      'entries': len(out['entries']), 'dest': str(dest)}))
    return 0 if out['verdict'] == 'PASS' else 1


if __name__ == '__main__':
    sys.exit(judge(sys.argv[1] if len(sys.argv) > 1 else REPO / 'test' / 'reports' / 't424' / 'matrix.json'))
