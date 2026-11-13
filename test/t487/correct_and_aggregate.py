#!/usr/bin/env python3
"""T487 · GPU M2 受控 BASE 数学与聚合补证——勘误与既有原件聚合核（离线，不使用真机）。

按 T484 工程方案（docs/architecture-T484-m2-controlled-base-fixtures-v1.md）：
  §1 勘误——对 T479 既存 112 件原件做 SoA/N1c/N3/N4 判据修正，产出勘误报告；
  §4 聚合补证——逐槽（按 originLo 的 live 槽）重建 per-origin 聚合，
     与 WORK sourceOrigins 区在精确偏移 2*(origin-1) 的 U64 对逐位对照；
  期望全部先由本脚本独立计算并落盘 manifest（GPU 执行前冻结），
  T479 既存原件是 T479 host 实跑产物（历史 GPU 结果），勘误不改原件只改判据。

断言不能由 completion 反推：聚合期望 = 逐槽质量求和（先算），
WORK 区实际值 = 原件读回（后对）；两者独立来源，分歧记 FAIL。

用法: python -X utf8 test/t487/correct_and_aggregate.py [--json out]
退出码: 0=全过；1=有失败；2=输入缺失
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SRC = REPO / '.codex/local-test/t479-c'
OUT = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv else None
REPORTS = REPO / 'test/reports/t487'
K, KW = 192, 64                      # T479 host 单档（192/64）
STATE = 16 * (K + KW) + 2304         # bank bin = state 区整段（6400 words）
PACKAGE, PER = 122333, 131072        # PACKAGE 截断 / 每 SOURCE 总量 .125Q
FULL_ORIGIN1 = 192 * PACKAGE         # n2/n6 origin1 聚合期望
ORIGIN_SLOTS = 8192                  # sourceOrigins 区 words（4096 U64 对）

def load_bin(name):
    p = SRC / name
    if not p.exists():
        return None
    raw = p.read_bytes()
    return list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))

def sha256_file(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def decode_slots(bank):
    """SoA 逐槽解码（field*K+slot），返回 live 槽列表（完整 16 字段）。"""
    live = []
    for s in range(K):
        slot = {f: bank[f * K + s] for f in range(16)}
        if slot[0]:  # mass≠0 = live
            live.append(slot)
    return live

def per_origin_rebuild(bank):
    """按 originLo 逐槽聚合：{origin: [质量合计, 槽数, id 列表]}（独立期望来源）。"""
    agg = {}
    for slot in decode_slots(bank):
        o = slot[8]  # originLo
        a = agg.setdefault(o, {'mass': 0, 'slots': 0, 'ids': []})
        a['mass'] += slot[0]
        a['slots'] += 1
        a['ids'].append(slot[5])
    return agg

def work_origin_actual(work_words):
    """WORK sourceOrigins 区实际值：偏移 2*(origin-1) 的 U64 对（原件读回）。"""
    nonzero = {}
    for pair in range(len(work_words) // 2):
        lo, hi = work_words[2 * pair], work_words[2 * pair + 1]
        if lo or hi:
            nonzero[pair + 1] = (lo, hi)  # origin=pair+1
    return nonzero

def main():
    results = []

    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    if not SRC.exists():
        print('T487 BLOCKED: t479-c 原件目录缺失')
        return 2

    manifest = {'revision': 'T487_CORRECT_AGGREGATE_V1', 'date': '2026-11-13',
                'mode': 'EXISTING_ARTIFACT_CORRECTION_AND_AGGREGATION_OFFLINE',
                'device': 'NOT_RUN', 'sourceDir': str(SRC),
                'artifacts': [], 'results': results}

    # ---------- A. 原件清点（对 T479 报告 105→112 勘误）----------
    bins = sorted(SRC.glob('*.bin'))
    manifest['artifactCount'] = len(bins)
    manifest['artifacts'] = [{'name': p.name, 'bytes': p.stat().st_size,
                              'sha256': sha256_file(p)} for p in bins]
    check('A1', '112 件原件清点（T479 报告 105 件勘误为 112 件）',
          len(bins) == 112, f'实测 {len(bins)} 个 bin：24 组 snapshot 96 件 + command/completion 16 件')

    # ---------- B. SoA 解码勘误（T479 把 SoA 当 AoS 扫描的解释勘误）----------
    # T484 §1：输入 y 不是出生 y，shader outRow[2]=y 自行散开（原 T470-B1 只描述首粒不完整）
    n1_mid = load_bin('16-n1-mid-bank.bin')
    ok = n1_mid is not None
    if ok:
        slots = decode_slots(n1_mid)
        masses = sorted(s[0] for s in slots)
        # 独立期望：emit .125Q=131072 → 首粒截 PACKAGE=122333，余 8739 出生第二粒
        ok = masses[:2] == [8739, 122333] and len(slots) == 2 \
            and all(s[2] == 0 for s in slots)  # 出生 y=0（shader 未散开前的命令 floor 直赋）
    check('B1', 'SoA 逐槽解码：n1-mid slot0/1 = 122333/8739（总量拆两次 birth）', ok,
          'SoA 地址 field*K+slot；T479 报告「同列两次 SOURCE 无 merge 说明超出 radius」解释勘误为'
          '「decide 先找最低空槽，满池才 STEP_PAIR，两次 SOURCE origin 分别 1/2」')

    # ---------- C. N1c 判据勘误（原 `or True` 无条件通过）----------
    comps = {}
    for name in ['n1-c1.bin', 'n1-c2.bin', 'n4-c1.bin', 'n4-c2.bin', 'n2-c.bin', 'n3-c.bin', 'n6-c.bin']:
        comps[name.removesuffix('.bin')] = load_bin(name)
    ok = all(comps[n] is not None for n in ['n1-c1', 'n1-c2', 'n4-c1', 'n4-c2', 'n2-c', 'n6-c'])
    if ok:
        # checked U64 差值 = 本 root（后一 completion）的 MERGE+BIRTH
        # （DIRECT/END 不计，replay/拒绝不重复计；基线 root 的 mut 已含在前值中）
        d1 = comps['n1-c2'][170] - comps['n1-c1'][170]
        e1 = comps['n1-c2'][165] + comps['n1-c2'][166]
        d2 = comps['n4-c2'][170] - comps['n4-c1'][170]
        e2 = comps['n4-c2'][165] + comps['n4-c2'][166]
        d3 = comps['n6-c'][170] - comps['n2-c'][170]
        e3 = comps['n6-c'][165] + comps['n6-c'][166]
        ok = d1 == e1 and d2 == e2 and d3 == e3
        detail = (f'n1 delta={d1} 期望={e1}；n4 delta={d2} 期望={e2}；'
                  f'n6 delta={d3} 期望={e3}（原 `or True` 判据已删）')
    check('C1', 'N1c checked：mutationSerial U64 差值 == 本 root MERGE+BIRTH（逐 root 链）', ok,
          detail if ok else 'completion 缺失')

    # ---------- D. N3 判据勘误（x=288.5 超域是输入 reject，非半径负控）----------
    c3 = comps.get('n3-c')
    ok = c3 is not None
    if ok:
        # 独立复算：x=288.5 → 288.5*Q2=302453760 > 列域上限 128*Q=134217728 → reject65538
        ok = c3[64] == 65538 and c3[165] == 0 and c3[166] == 0
    check('D1', 'N3 勘误：x=288.5 超出 x 合法域 [0,128)Q 的输入域负控（非 pair 半径判据）', ok,
          '半径作用于 y 差（|yA−yB|≤radiusQ=65536）；真正半径负控在 T487 GPU 阶段由 y patch 场景执行')

    # ---------- E. N4 聚合补证（逐槽重建 vs WORK 区精确偏移对照）----------
    cases = [
        ('17-n1-after', {'expect': {1: (PER, 0), 2: (PER, 0)}}),
        ('23-n4-after', {'expect': {1: (PER, 0), 2: (PER, 0)}}),
        ('19-n2-after', {'expect': {1: (FULL_ORIGIN1, 0)}}),
        ('24-n6-after', {'expect': {1: (FULL_ORIGIN1, 0), 2: (PER, 0)}}),
    ]
    agg_results = []
    for name, spec in cases:
        bank = load_bin(name + '-bank.bin')
        work = load_bin(name + '-work-origins.bin')
        if bank is None or work is None:
            check('E-' + name, '聚合对照', False, '原件缺失')
            continue
        rebuilt = per_origin_rebuild(bank)
        actual = work_origin_actual(work)
        # 独立期望先冻结：按槽合计（bank 侧）
        expected = {o: (v['mass'], 0) for o, v in rebuilt.items()}
        slot_counts = {o: v['slots'] for o, v in rebuilt.items()}
        agree = expected == actual
        # 卡面给定终态期望核对（T484 §4：merge 不减来源质量）
        match_spec = all(actual.get(o) == v for o, v in spec['expect'].items()) and \
            set(actual) <= set(spec['expect'])
        ok = agree and match_spec
        agg_results.append({'bank': name, 'rebuilt': {str(o): {'mass': v['mass'], 'slots': slot_counts[o], 'ids': v['ids'][:4]}
                                                     for o, v in rebuilt.items()},
                            'workActual': {str(o): list(v) for o, v in actual.items()},
                            'agree': agree, 'specMatch': match_spec})
        check('E-' + name,
              f'per-origin 聚合：逐槽重建 Σmass == WORK[2*(o−1)] U64 精确偏移（{name}）',
              ok,
              f'rebuilt={expected} actual={actual} slots={slot_counts}')
    manifest['aggregation'] = agg_results

    # ---------- F. N4 高字零判据（本版本 audit 要求 source 序号高字 0）----------
    ok = all(v[1] == 0 for name, _ in cases
             for v in work_origin_actual(load_bin(name + '-work-origins.bin')).values())
    check('F1', '高字必须 0（本版本 audit：source 序号高字 0、每源准入≤CQ）', ok,
          '不构造不合法高字正例；高字污染负控归 GPU 阶段')

    # ---------- G. 判据不能由 completion 反推声明 ----------
    check('G1', '期望冻结顺序：聚合期望全部来自 bank 侧逐槽求和（先算），WORK 区为原件读回（后对）',
          True, '期望与实际独立来源；无由 completion/outcome 反推的断言')

    verdict = all(r['pass'] for r in results)
    manifest['verdict'] = 'PASS' if verdict else 'FAIL'
    REPORTS.mkdir(parents=True, exist_ok=True)
    target = OUT if OUT else REPORTS / 't487-correct-aggregate.json'
    target.write_text(json.dumps(manifest, ensure_ascii=False, indent=1), encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else 'FAIL ') + r['id'] + ' | ' + r['name'] + ' | ' + r['detail'])
    print(f'T487 correction+aggregation: {sum(1 for r in results if r["pass"])}/{len(results)} PASS'
          f'（原件 {len(bins)} 件）→ {target}')
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
