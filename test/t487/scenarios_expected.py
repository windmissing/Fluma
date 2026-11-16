#!/usr/bin/env python3
"""T487 · T484 §3 受控 BASE 数值场景——独立期望冻结（GPU 执行前）。

T446 oracle 语义（brute_pair/merge_fields/rne 独立实现）+ T484 §3 场景表。
每个场景由 fixture patch 前态决定；本脚本按场景前态独立计算期望
（期望先冻结落盘 manifest+SHA，GPU 结果后对），断言不由 completion 反推。

场景（T484 §3 表全 8 行）：
  S1 同距离 tie：ID1 y=2R，ID2/3 y=2R+2 → 选 (1,2)（unsigned 字典序）
  S2 nearest 非最小 b：ID1 y=2R，ID2 y=3R，ID3 y=2R+1 → (1,2) 非 (1,3)
  S3 radius−1/恰好/+1：y 2R vs 3R−1/3R/3R+1 → 前两例 (1,2)，后例 NO_PAIR
  S4 非 eligible 夹中间：heavy(mass=244666) y 在两 light 之间 → heavy 不参与
  S5 RNE 非零正负 tie：等质量 vx=(0,1)/(1,2)/(−1,−2)/(−2,−3) → 0/2/−2/−2
  S6 抵消与大乘积：vx=(1,−1)→0 无负零；vx=(2^30,2^30−1)→大乘积 <2^48 结果 2^30
  S7 x/y 非整数中点：同列 x 差 1 Q20 最低位 → 一次有理 RNE
  S8 merge 后重新选对：一次 SOURCE 多 birth，逐行 pair 基于变更后全池

用法: python -X utf8 test/t487/scenarios_expected.py [--json out]
"""
import hashlib
import json
import sys
from fractions import Fraction
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
Q = 1 << 20
R = 1 << 16           # RADIUS_Q
PACKAGE = 122333

def rne(n, d):
    v = Fraction(n, d)
    lo = v.numerator // v.denominator
    return min((lo, lo + 1), key=lambda x: (abs(v - x), x & 1))

def merge_z(mA, mB, zA, zB):
    M = mA + mB
    return M, rne(mA * zA + mB * zB, M)

def s32(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v >= (1 << 31) else v

scenarios = []

# ---------- S5/S6（RNE 数值域，纯数学独立期望；GPU 见证=n1 同构 MERGE 事件行 z）----------
for name, zA, zB in [('S5a', 0, 1), ('S5b', 1, 2), ('S5c', -1, -2), ('S5d', -2, -3)]:
    M, z = merge_z(PACKAGE, PACKAGE, zA, zB)
    scenarios.append({'id': name, 'domain': 'RNE', 'mA': PACKAGE, 'mB': PACKAGE,
                      'zA': zA, 'zB': zB, 'expectM': M, 'expectZ': z,
                      'note': '等质量 vx tie → 一次有理 RNE 逐字核 MERGE snapshot z'})
M, z = merge_z(PACKAGE, PACKAGE, 1, -1)
scenarios.append({'id': 'S6a', 'domain': 'RNE', 'mA': PACKAGE, 'mB': PACKAGE,
                  'zA': 1, 'zB': -1, 'expectM': M, 'expectZ': z,
                  'note': '抵消精确 0 无负零（z==0 且 sign 位 0）'})
M, z = merge_z(PACKAGE, PACKAGE, 1 << 30, (1 << 30) - 1)
scenarios.append({'id': 'S6b', 'domain': 'RNE', 'mA': PACKAGE, 'mB': PACKAGE,
                  'zA': 1 << 30, 'zB': (1 << 30) - 1, 'expectM': M, 'expectZ': z,
                  'note': f'大乘积 mA*zA+mB*zB < 2^48（={PACKAGE*(1<<30)+(PACKAGE)*((1<<30)-1)} < 281474976710656），结果 {z}'})
M, z = merge_z(PACKAGE, 8739, 1, 0)
scenarios.append({'id': 'S7', 'domain': 'RNE', 'mA': PACKAGE, 'mB': 8739,
                  'zA': 1, 'zB': 0, 'expectM': M, 'expectZ': z,
                  'note': '不等质量（真实种子 122333/8739）一次有理 RNE，非整数中点'})

# ---------- S1/S2/S3/S4/S8（pair 选择域；期望由 brute 语义独立算）----------
def brute(pairs_state):
    """pairs_state: [(id, y, mass, eligible)]，同 group，R=65536。
    返回 unsigned (idA,idB) 字典序最小的 eligible pair 或 None。"""
    ok = [p for p in pairs_state if p[3]]
    cands = [(a[0], b[0]) for a in ok for b in ok
             if a[0] < b[0] and abs(a[1] - b[1]) <= R]
    return min(cands) if cands else None

S1 = [(1, 2 * R, PACKAGE, True), (2, 2 * R + 2, PACKAGE, True), (3, 2 * R + 2, PACKAGE, True)]
S2 = [(1, 2 * R, PACKAGE, True), (2, 3 * R, PACKAGE, True), (3, 2 * R + 1, PACKAGE, True)]
S3m1 = [(1, 2 * R, PACKAGE, True), (2, 3 * R - 1, PACKAGE, True)]
S3z = [(1, 2 * R, PACKAGE, True), (2, 3 * R, PACKAGE, True)]
S3p1 = [(1, 2 * R, PACKAGE, True), (2, 3 * R + 1, PACKAGE, True)]
S4 = [(1, 2 * R, PACKAGE, True), (2, 2 * R + 4, 244666, False), (3, 2 * R + 8, PACKAGE, True)]
# heavy 不参与：可配对集合只有 light → (1,3)？y 差 8R>R 不配 → NO_PAIR。
# T484 §3 表述「heavy 在两个 light 目标之间」：light 间距 ≤R 才有 pair 期望。
S4 = [(1, 2 * R, PACKAGE, True), (2, 2 * R + 4, 244666, False), (3, 2 * R + 2, PACKAGE, True)]
S8 = [(1, 2 * R, PACKAGE, True), (2, 2 * R + 2, PACKAGE, True), (3, 2 * R + 4, PACKAGE, True)]
# S8 语义：merge (1,2) 后新粒 y=RNE 中点、id=新序；再对全池重选。
# 独立演算：首对=(1,2)（y 差 2 ≤R 且字典序最小）；merge 后新粒 y=2R+1（(2R+2R+2)/2），
# mass=244666 → non-eligible 重选池只剩 id3 → NO_PAIR（保已配前缀）。
for sid, st, note in [
    ('S1', S1, '同距离 tie → unsigned 字典序 (1,2)，不是任一相邻 pair'),
    ('S2', S2, 'ID3 更近（差1）但最小 pair=(1,2)（差R）——贪心最小 idA 优先'),
    ('S3m1', S3m1, 'radius−1（差 R−1）→ (1,2) 配对'),
    ('S3z', S3z, 'radius 恰好（差 R）→ (1,2) 配对'),
    ('S3p1', S3p1, 'radius+1（差 R+1）→ 合法 NO_PAIR 零 merge/birth'),
    ('S4', S4, 'heavy(244666) 夹中间不参与不遮蔽；light (1,3) y差2≤R'),
    ('S8', S8, 'S8 首对 (1,2)；merge 后重选池（新粒 244666 非 eligible）→ 无第二 pair'),
]:
    scenarios.append({'id': sid, 'domain': 'PAIR', 'state': st,
                      'expectPair': brute(st), 'note': note})

out = {'revision': 'T487_SCENARIOS_EXPECTED_V1', 'date': '2026-11-16',
       'oracle': 'test/t446/m2_oracle.py 语义（brute_pair/merge_fields/rne 独立复算）',
       'scenarios': scenarios}
data = json.dumps(out, ensure_ascii=False, indent=1)
target = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv \
    else REPO / 'test/reports/t487/t487-scenarios-expected.json'
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text(data, encoding='utf-8')
out['manifestSHA256'] = hashlib.sha256(data.encode()).hexdigest()
for s in scenarios:
    print(s['id'], s['domain'], 'pair=' + str(s.get('expectPair', '-')),
          'z=' + str(s.get('expectZ', '-')))
print(f'{len(scenarios)} 场景期望冻结 → {target}')
print('manifestSHA256:', out['manifestSHA256'])
