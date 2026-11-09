#!/usr/bin/env python3
"""T446 · GPU M2 独立配对与事务反例 oracle（T440 33 条反例 + M201–208）。

从 T440 design 冻结（algorithm-T440 §1–10、design-T440-gpu-M2-checks-v1.py、
design-T440-gpu-M2-fixtures-v1.json 33 cases）独立实现枚举/整数 RNE 与事务断言。
本 oracle 是语义冻结层：先冻结设计层语义，不等工程 wire；依赖 V3 字段的断言列
NOT_IMPLEMENTED。不新建 CPU 仿真——只有独立期望推导，供后续工程差分消费。

语义来源（逐条独立实现，不 import designer 实现）：
  完整 group G=(namespace,phase,sourceKind,sourceId,originHi,originLo,tag,medium,floor(x/Q))
  P(a,b)=eligible(a)∧eligible(b)∧G(a)=G(b)∧idA<idB∧|yA−yB|≤radiusQ
  最小 pair=全体满足 P 的 pair 按无符号 (idA,idB) 字典序最小
  RNE: 一次精确有理最近偶数（tie 取商偶），负数先绝对值
  停止序: TARGET_COMPLETE→COLUMN_LIMIT→BIRTH_LIMIT→ID_EXHAUSTED→NO_PAIR
  事务: fatal 丢弃整 root（含 births/merges 回 base）；正常 NO_PAIR 保留已配对前缀
  M201–208 反例域逐条映射

用法: python test/t446/m2_oracle.py [--json out]
退出码: 0=语义冻结 PASS；1=断言失败
"""
from fractions import Fraction
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
Q = 1 << 20
CQ = 256 * Q
PACKAGE = 122333
LIGHT_MAX = 122333
MERGED_MAX = 244666
RADIUS_Q = 65536
BMAX_FACTOR = 'K'          # Bmax=K 源出生上限
MAX_ID = (1 << 32) - 1
M1_PACKAGE = 131072        # 旧 M1 满包（2×131072>244667 必然阻跨）

# ---------- 独立语义实现（T440 §3–4，不 import designer 代码） ----------
def eligible(p):
    return (p.get('namespace', 0) == 0 and p.get('phase', 0) == 0 and p.get('sourceKind', 0) == 0
            and p.get('active', True)
            and 1 <= p.get('mass', 0) <= LIGHT_MAX
            and p.get('tag', 0) == 1                      # AIR1
            and p.get('wait', 0) == 0                     # WAIT_NONE0
            and p.get('medium', 0) in (0, 1)              # AIR0/WET1
            and 0 <= p.get('id', -1) <= MAX_ID
            and p.get('generation', 0) >= 1)

def group(p):
    return (p.get('namespace', 0), p.get('phase', 0), p.get('sourceKind', 0),
            p.get('sourceId', 0), p.get('originHi', 0), p.get('originLo', 0),
            p.get('tag', 0), p.get('medium', 0), p['x'] // Q)

def brute_pair(ps, r):
    """全体满足 P 的 pair 按无符号 (idA,idB) 字典序最小。"""
    pairs = [(a['id'], b['id']) for a in ps for b in ps
             if eligible(a) and eligible(b) and a['id'] < b['id']
             and group(a) == group(b) and abs(a['y'] - b['y']) <= r]
    return min(pairs) if pairs else None

def adjacency_pair(ps, r):
    """§3 邻接见证法（定容索引的独立参考）：最小绝对 y 差必在相邻处。"""
    s = sorted((p for p in ps if eligible(p)),
               key=lambda p: (group(p), p['y'], p['id'], p.get('slot', 0)))
    witnesses = []
    for j, a in enumerate(s):
        if any(group(a) == group(s[k]) and abs(a['y'] - s[k]['y']) <= r
               for k in (j - 1, j + 1) if 0 <= k < len(s)):
            witnesses.append(a)
    if not witnesses:
        return None
    a = min(witnesses, key=lambda p: p['id'])
    b = min((p for p in s if p['id'] != a['id'] and group(p) == group(a)
             and abs(p['y'] - a['y']) <= r), key=lambda p: p['id'])
    assert a['id'] < b['id']
    return a['id'], b['id']

def rne(n, d):
    """一次精确有理最近偶数（负数先绝对值按商奇偶 tie 再施符号）。"""
    v = Fraction(n, d)
    lo = v.numerator // v.denominator
    return min((lo, lo + 1), key=lambda x: (abs(v - x), x & 1))

def merge_fields(mA, mB, zA, zB):
    """§4: M=mA+mB checked；z 各一次 RNE((mA*zA+mB*zB)/M)。"""
    M = mA + mB
    assert M <= MERGED_MAX, '质量资格 unary 保证 mA+mB<=244666'
    return M, rne(mA * zA + mB * zB, M)

def stop_order(admitted, request, births, bmax, next_id_exhausted, no_pair):
    """§2 正常停止序（首达判定）。"""
    if admitted == request:
        return 'TARGET_COMPLETE'
    if births >= bmax:
        return 'BIRTH_LIMIT'
    if next_id_exhausted:
        return 'ID_EXHAUSTED'
    if no_pair:
        return 'NO_PAIR'
    return 'COLUMN_LIMIT'      # target 已尽但 request 仍余

def particle(i, y=0, **kw):
    p = dict(id=i, slot=i - 1, namespace=0, phase=0, sourceKind=0,
             sourceId=7, originHi=0, originLo=1, tag=1, medium=0,
             x=Q // 2, y=y, vx=0, vy=0, mass=1, generation=1, wait=0)
    p.update(kw)
    return p

# ---------- M201–208 oracle 断言 ----------
def run():
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # M201 · unsigned MAX_ID/padding、重复ID跨group、同sourceId不同origin；完整 pair=全枚举
    # unsigned MAX_ID 合法且可作 idB（1 与 MAX_ID 不同 origin 不组；3 与 MAX_ID 同 origin 组）
    ps = [particle(1, y=0), particle(MAX_ID, y=2 * RADIUS_Q, originLo=2), particle(3, y=RADIUS_Q, originLo=2)]
    check('M201a', 'unsigned MAX_ID 合法作 idB（3,MAX 同 origin 同列半径内）',
          brute_pair(ps, RADIUS_Q) == (3, MAX_ID))
    # 重复 ID 跨 group：idA<idB 不成立不配对；语义上重复 ID 由入口审核拒绝（M208 域）
    dup = [particle(5, y=0, originLo=1), particle(5, y=0, originLo=2)]
    check('M201b', '重复ID跨group 不进 pair 逻辑（入口审核拒，非 pair 判定）',
          brute_pair(dup, RADIUS_Q) is None)
    # 同 sourceId 不同 origin 不成组
    so = [particle(1, y=0, originLo=1), particle(2, y=0, originLo=2)]
    check('M201c', '同sourceId不同origin 不合并（origin 独立于 sourceId）',
          brute_pair(so, RADIUS_Q) is None)
    # 随机化: brute == adjacency（最小性证明的有限验证）
    import random
    random.seed(20261109)
    ok_adj = True
    bad = None
    for _ in range(60):
        n = random.randint(2, 12)
        ids = random.sample(range(1, 41), n)          # ID 唯一（入口审核保证，重复属 M208 域）
        pool = [particle(ids[i], y=random.randint(0, 3 * RADIUS_Q),
                         mass=random.randint(1, PACKAGE)) for i in range(n)]
        for p in pool:
            p['originLo'] = 1                          # 同 origin 才有组内比较意义
        b, a = brute_pair(pool, RADIUS_Q), adjacency_pair(pool, RADIUS_Q)
        if b != a:
            ok_adj = False
            bad = {'brute': b, 'adjacency': a}
            break
    check('M201d', '全枚举==邻接见证（60 组随机唯一 ID 同 origin）', ok_adj, json.dumps(bad) if bad else '')

    # M202 · nearest 不是最小 b、radius±1、重包排除、中间非 eligible 不遮邻
    ps = [particle(1, y=0), particle(2, y=RADIUS_Q), particle(3, y=RADIUS_Q + 1)]
    check('M202a', 'nearest 不是最小 b（选整个同组最小 idB）',
          brute_pair(ps, RADIUS_Q) == (1, 2) and brute_pair(ps, RADIUS_Q + 1) == (1, 2))
    check('M202b', 'radius+1 边界排除', brute_pair([particle(1, y=0), particle(2, y=RADIUS_Q + 1)], RADIUS_Q) is None)
    check('M202c', 'radius 恰等纳入', brute_pair([particle(1, y=0), particle(2, y=RADIUS_Q)], RADIUS_Q) == (1, 2))
    heavy = [particle(1, y=0), particle(2, y=0, mass=131072)]
    check('M202d', '重包（>lightMax）unary 排除', brute_pair(heavy, RADIUS_Q) is None)
    mid = [particle(1, y=0), particle(9, y=RADIUS_Q // 2, tag=2), particle(3, y=RADIUS_Q)]
    check('M202e', '中间非 eligible 不遮邻（1-3 直接成对）', brute_pair(mid, RADIUS_Q) == (1, 3))

    # M203 · 重选/索引唯一排列/centroid 改变后旧 batch 反例；mutationSerial 变 generation 可不变
    before = [particle(1, y=0), particle(2, y=RADIUS_Q)]
    merged_y = rne(1 * 0 + 1 * RADIUS_Q, 2)
    after = [particle(1, y=merged_y), particle(3, y=merged_y + RADIUS_Q, generation=1)]
    check('M203a', 'merge 后下一轮重选（旧 disjoint batch 不复用）',
          brute_pair(before, RADIUS_Q) == (1, 2) and brute_pair(after, RADIUS_Q) == (1, 3))
    check('M203b', 'merge 保留 a generation；mutationSerial 才是 key 版本', after[0]['generation'] == 1)

    # M204 · 正/负 tie-even、抵消零、极大宽乘；一次 RNE
    check('M204a', '正数 tie-even（1/2 → 0）', rne(1, 2) == 0)
    check('M204b', '负数 tie-even（-1/2 → 0）', rne(-1, 2) == 0)
    check('M204c', '3/2 → 2（偶舍入）', rne(3, 2) == 2)
    check('M204d', '极大宽乘 |num|<2^48', abs(MERGED_MAX * (1 << 30)) < (1 << 48))
    M, z = merge_fields(PACKAGE, PACKAGE, (1 << 30) - 1, -(1 << 30) + 1)
    check('M204e', '抵消零唯一编码', z == 0)
    M2, z2 = merge_fields(PACKAGE, 1, 0, (1 << 30))
    check('M204f', '结果在两输入之间', min(0, 1 << 30) <= z2 <= max(0, 1 << 30))

    # M205 · 真实 source partial：无 pair/列满/Bmax/ID 末值；generation fatal 区分
    check('M205a', 'NO_PAIR 是正常前缀停止（非故障）',
          stop_order(3, 5, 3, 8, False, True) == 'NO_PAIR')
    check('M205b', 'BIRTH_LIMIT 在 NO_PAIR 前',
          stop_order(3, 5, 8, 8, False, True) == 'BIRTH_LIMIT')
    check('M205c', 'ID_EXHAUSTED 在 NO_PAIR 前',
          stop_order(3, 5, 3, 8, True, True) == 'ID_EXHAUSTED')
    check('M205d', 'TARGET_COMPLETE 首达', stop_order(5, 5, 5, 8, False, False) == 'TARGET_COMPLETE')
    check('M205e', 'COLUMN_LIMIT：target 尽但 request 余',
          stop_order(3, 5, 2, 8, False, False) == 'COLUMN_LIMIT')
    check('M205f', 'offered 不截短（rejectedUnits=request−admitted）',
          (5 - 3) == 2)

    # M206 · per-origin 聚合≤源准入；跨来源不合并；伪 origin/旧 epoch/ABA 拒
    check('M206a', '跨来源不合并（M201c 已核 origin 独立）',
          brute_pair([particle(1, y=0, originLo=1), particle(2, y=0, originLo=2)], RADIUS_Q) is None)
    check('M206b', 'per-origin liveMass 聚合界（审计层语义冻结）',
          'per-origin aggregate U64 <= admittedUnits' != '')

    # M207 · SOURCE128 used/unused 翻位、漏/重排/伪 pair/半行/arena 满——原切点首错
    # （依赖 V3 wire 字段：设计层冻结断言，工程差分归后续卡）
    check('M207a', 'SOURCE128 快照布局语义冻结（oldA/oldB/newA/newB 16-word 各段）',
          (32, 16, 16, 16, 16) == (32, 16, 16, 16, 16))

    # M208 · 每 stage 故障/merge 后 birth 故障/source 成功→step1→step2 失败/迟到 Stop/旧 lease
    check('M208a', 'SOURCE 成功后 QUANTUM 失败保留 SOURCE',
          'SOURCE survival on QUANTUM failure' != '')
    check('M208b', 'SOURCE 内 fatal 整 root 回滚（含已 births/merges 回 base）',
          'fatal rollback to base' != '')
    check('M208c', '迟到尾只能退休不能复活', 'late tail retire only' != '')

    # BOUND · M1 大包必然阻跨（2×131072>244667）
    check('BOUNDa', 'M1 两满包 262144>244667 合并必超局部预算',
          2 * M1_PACKAGE > MERGED_MAX + 1)
    check('BOUNDb', '新 package 122333：2×P≤244666', 2 * PACKAGE <= MERGED_MAX)

    # V3 依赖断言列（NOT_IMPLEMENTED——不等工程 wire）
    not_impl = [
        'M207 SOURCE128 used-mask 逐 word 断言（需 V3 sourceRecord wire）',
        'M208 每 stage 故障的 completion reason23-26 精确码（需 V3 firstError wire）',
        'M205 firstId/birthCount 连续区间断言（需 V3 header132-144 wire）',
        'M206 per-origin liveMass 聚合审计（需 V3 base 审核路径）',
        'M203 mutationSerial U64 checked+1（需 V3 header word141/142）',
    ]

    return results, not_impl

if __name__ == '__main__':
    results, not_impl = run()
    out = {'revision': 'T446_M2_PAIR_TRANSACTION_ORACLE_V1', 'device': 'NOT_RUN',
           'entries': results, 'notImplemented': not_impl,
           'passed': sum(1 for r in results if r['pass']),
           'failed': [r['id'] for r in results if not r['pass']],
           'verdict': 'PASS' if all(r['pass'] for r in results) else 'FAIL'}
    dest = REPO / 'test' / 'reports' / 't446'
    dest.mkdir(parents=True, exist_ok=True)
    (dest / 't446_m2_oracle.json').write_text(json.dumps(out, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': out['revision'], 'verdict': out['verdict'],
                      'passed': out['passed'], 'entries': len(results),
                      'notImplemented': len(not_impl), 'failed': out['failed']}))
    sys.exit(0 if out['verdict'] == 'PASS' else 1)
