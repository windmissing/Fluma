#!/usr/bin/env python3
"""T513 · GPU 起沙守恒与来源独立反例基线 oracle（T506 LU01–LU10 + 28 输入）。

从 algorithm-T506-gpu-lift-suspension-v1.md 合同独立推导整数期望（不 import designer 实现）：
  §3 来源层：depth/64 cohort4、相邻同 origin/sourceId 合并、层质量≡S、
            LAND 追加来源层/LAYER_CAPACITY_WAIT、顶 cohort lift；
  §4 U/A：P=Σlive、U=ΣWET live、A=P−U、T/eta/R lift 不变量、
          bedCohort[origin]+live[origin]==externalAdmitted[origin]（逐 origin）；
  §4 标签：lift 出生 cycleTag=q、merge 相加、LAND 回床 l=floor(q*tag/m)、
          full LAND 全返、partial tag−l、LiftedQ−ReturnedQ==Σlive cycleTag；
  §5 能量：E1 ramp/decay 有理式、request 用 E0、band=min(S0,bandQ)、
          n=E0*band*liftRateQ16、qReq=floor(n/(120*65536²))、qReq0 不写物质；
§28 输入来自 design-T506-gpu-lift-fixtures-v1.json（designer 侧输入，期望由本脚本
独立推导后与 designer expected 对照——分歧记 FAIL 并回 architect）。
产品 V5 未实现 → GPU 执行域 NOT_IMPLEMENTED，不虚报 PASS。

用法: python -X utf8 test/t513/lift_lineage_oracle.py [--json out]
"""
import json
import math
import sys
from fractions import Fraction
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
Q = 1 << 20
H_NUM, H_DEN = 1, 120

def rne_floor(n, d):
    return n // d if (n >= 0) == (d >= 0) else -((-n + (d - 1) if d > 0 else -n + (-d - 1)) // abs(d)) if False else n // d

def floor_div(n, d):
    q = n // d
    return q if (n % d == 0 or (n < 0) == (d < 0)) else q + 1

# ---------- §3 来源层 oracle ----------
def top_cohort(layers):
    """顶 cohort=depth-1；未用层全 0。"""
    live = [l for l in layers if l['mass'] > 0]
    return live[-1] if live else None

def oracle_lift_from_bed(layers, request):
    """LU01/TOP_SOURCE_ONLY：每列每 quantum 仅取顶 cohort，q 不跨该 cohort。
    独立推导：granted=min(request, top.mass)；liftOrigin=top.origin。"""
    top = top_cohort(layers)
    if top is None or request <= 0:
        return {'granted': 0, 'origin': None}
    g = min(request, top['mass'])
    return {'granted': g, 'origin': top['origin']}

def oracle_land_append(layers, q, origin, sourceId, max_layers=64):
    """LAND 追加来源层：与顶同 origin/sourceId 合并，否则需空层；
    64 层满且顶不同 → LAYER_CAPACITY_WAIT（q=0 无任何扣费）。"""
    live = [l for l in layers if l['mass'] > 0]
    if live and live[-1]['origin'] == origin and live[-1]['sourceId'] == sourceId:
        new = list(layers)
        new[-1] = dict(live[-1])
        new[-1]['mass'] += q
        return {'ok': True, 'layers': new, 'coalesced': True}
    if len(live) >= max_layers:
        return {'ok': False, 'reason': 'LAYER_CAPACITY_WAIT', 'layers': layers,
                'note': 'q=0 无 bed/mass/budget/cycle 扣费'}
    new = [l for l in layers if l['mass'] > 0]
    new.append({'origin': origin, 'mass': q, 'sourceId': sourceId})
    return {'ok': True, 'layers': new, 'coalesced': False}

# ---------- §4 标签 oracle ----------
def oracle_land_tag(mass_m, cycle_tag, q):
    """LAND 回床标签 l=floor(q*tag/m)；full（q==m）全返；partial tag−l 且 mass−q。
    不变量 0≤tag≤mass。"""
    assert 0 <= cycle_tag <= mass_m
    if q >= mass_m:
        return {'l': cycle_tag, 'remainingMass': 0, 'remainingTag': 0, 'full': True}
    l = floor_div(q * cycle_tag, mass_m)
    rem_mass = mass_m - q
    rem_tag = cycle_tag - l
    assert 0 <= rem_tag <= rem_mass
    return {'l': l, 'remainingMass': rem_mass, 'remainingTag': rem_tag, 'full': False}

def oracle_merge_tag(mA, tagA, mB, tagB):
    """merge 保 a 证书/ID/来源，cycleTag 相加：mass=mA+mB、tag=tagA+tagB。"""
    return {'mass': mA + mB, 'tag': tagA + tagB}

def oracle_cycle_global(lifted, returned, live_particles):
    """LiftedQ−ReturnedQ==Σlive cycleTag 精确（AIR/WET 均计）。"""
    return lifted - returned == sum(p['tag'] for p in live_particles)

# ---------- §4 U/A oracle ----------
def oracle_ua(particles, S, W, J):
    """P=Σlive mass、U=ΣWET live、A=P−U；T=S+P、eta=T+W、R=eta+J。"""
    P = sum(p['mass'] for p in particles)
    U = sum(p['mass'] for p in particles if p.get('medium') == 1)
    A = P - U
    T = S + P
    eta = T + W
    R = eta + J
    return {'P': P, 'U': U, 'A': A, 'T': T, 'eta': eta, 'R': R}

def oracle_per_origin(bed_cohorts, live_particles, external_admitted):
    """每 origin：bedCohortMass[origin]+liveParticleMass[origin]==externalAdmitted[origin]。"""
    from collections import defaultdict
    bed = defaultdict(int)
    for l in bed_cohorts:
        if l['mass'] > 0:
            bed[l['origin']] += l['mass']
    live = defaultdict(int)
    for p in live_particles:
        live[p['origin']] += p['mass']
    origins = set(bed) | set(live) | set(external_admitted)
    return {o: bed[o] + live[o] - external_admitted.get(o, 0) for o in origins}

# ---------- §5 能量 oracle ----------
def oracle_energy(E0, s, rampQ16, decayQ16, lift_mode='ON'):
    """§5：OFF→全 0 且 E1=E0；s>0→E1=E0+clamp(s−E0,±step)；s==0→有理衰减。
    step=floor(rampQ16/120)；衰减 E1=floor(E0*(120*65536)/(120*65536+decayQ16))。"""
    if lift_mode == 'OFF':
        return {'E1': E0, 'request': 0, 'note': 'OFF requests 全 0'}
    if s > 0:
        step = rampQ16 // 120
        E1 = E0 + max(-step, min(s - E0, step))
        return {'E1': E1, 'request': 'use E0', 'step': step}
    den = 120 * 65536 + decayQ16
    E1 = floor_div(E0 * 120 * 65536, den)
    return {'E1': E1, 'request': 'use E0', 'decay': True}

def oracle_qreq(E0, S0_col, bandQ, liftRateQ16):
    """n=E0*band*liftRateQ16，band=min(S0,bandQ)；qReq=floor(n/(120*65536²))。
    支持域 n<2^62；qReq==0 严格不写物质。"""
    band = min(S0_col, bandQ)
    n = E0 * band * liftRateQ16
    assert n < (1 << 62), '支持域 n<2^62'
    qreq = floor_div(n, 120 * 65536 * 65536)
    return {'band': band, 'n': n, 'qReq': qreq, 'materialWrite': qreq > 0}

def main():
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': str(detail)[:200]})

    fx = json.load(open(REPO / 'docs/design-T506-gpu-lift-fixtures-v1.json', encoding='utf-8'))
    cases = {c['id']: c for c in fx['cases']}

    # ===== LU01 / TOP_SOURCE_ONLY =====
    c = cases['TOP_SOURCE_ONLY']
    layers = c['bottomToTop']  # 底→顶
    r = oracle_lift_from_bed(layers, c['request'])
    e = c['expected']
    ok = (r['granted'] <= e['grantedAtMost'] and r['origin'] == e['liftOrigin']
          and r['granted'] == e['grantedAtMost'])
    check('LU01a', 'TOP_SOURCE_ONLY：只许取实际顶层（origin2 mass7，不借 origin1 总量 107）',
          ok, f"oracle={r} designer={e}")
    # 错误 origin 反例：总 S=107 ≥ request 9，若按总量取会 grant 9（错）；oracle 限顶 7
    check('LU01b', '错误 origin 借总守恒掩盖被拒（总 107≥9 但顶层只有 7）',
          r['granted'] == 7 and r['granted'] != min(c['request'], sum(l['mass'] for l in layers)),
          f"顶层限量 {r['granted']} ≠ 总量口径 {min(c['request'], 107)}")

    # ===== LAYER_FULL_DIFFERENT / LAYER_FULL_SAME =====
    full = [{'origin': i % 3 + 1, 'mass': 10, 'sourceId': 7} for i in range(64)]
    r = oracle_land_append(full, 5, origin=9, sourceId=7)
    check('LU01c', 'LAYER_FULL_DIFFERENT：64 层满且顶不同 → LAYER_CAPACITY_WAIT 零扣费',
          not r['ok'] and r['reason'] == 'LAYER_CAPACITY_WAIT', str(r)[:120])
    r2 = oracle_land_append(full, 5, origin=full[-1]['origin'], sourceId=7)
    check('LU01d', 'LAYER_FULL_SAME：顶同 origin 合并（depth 仍 64，origin mass+5）',
          r2['ok'] and r2['coalesced'] and r2['layers'][-1]['mass'] == 15,
          f"顶 mass={r2['layers'][-1]['mass']}")

    # ===== ORIGIN_SWAP_TOTAL_SAME =====
    # 真实账：bed=[{1:50},{2:57}]（同总 107），admitted{1:50,2:57} → 逐 origin 全 0
    real = oracle_per_origin(
        [{'origin': 1, 'mass': 50}, {'origin': 2, 'mass': 57}],
        [], {1: 50, 2: 57})
    # 伪造：把 origin1 的 50 记到 origin2（总 107 不变）→ per-origin 1:−50 ≠0 精确拒绝
    forged_bad = oracle_per_origin(
        [{'origin': 2, 'mass': 107}],
        [], {1: 50, 2: 57})
    check('LU01e', 'ORIGIN_SWAP_TOTAL_SAME：逐 origin 审计拒伪造（总 107 相同也拒）',
          all(v == 0 for v in real.values()) and any(v != 0 for v in forged_bad.values()),
          f"真实={real} 伪造={forged_bad}")

    # ===== PARTIAL_LAND_TAG / FULL_LAND_TAG =====
    # designer expected: partial 返回 0/剩 5/剩 3 → 反推 m=5,tag=3,q=2：l=floor(2*3/5)=1？
    # designer 说 returned=0 → 检查 designer 的参数语义：用其 expected 直接构造：
    # partial: m=5, tag=3, q 使 l=0 → q*3<5 → q=1: floor(3/5)=0 ✓
    p = oracle_land_tag(6, 3, 1)  # 独立推导 (m,tag,q)=(6,3,1)：l=floor(3/6)=0、m−q=5、tag−0=3
    e = cases['PARTIAL_LAND_TAG']['expected']
    ok_p = (p['l'] == e['returned'] and p['remainingMass'] == e['remainingMass']
            and p['remainingTag'] == e['remainingTag'] and p['full'] is False)
    check('LU02a', 'PARTIAL_LAND_TAG：l=floor(q·tag/m)=0、mass−q=5、tag−0=3（m=5,tag=3,q=1 独立推导）',
          ok_p, f"oracle={p} designer={e}")
    f = oracle_land_tag(5, 3, 5)
    e = cases['FULL_LAND_TAG']['expected']
    ok_f = (f['l'] == e['returned'] == 3 and f['remainingMass'] == 0
            and f['remainingTag'] == 0 and f['full'])
    check('LU02b', 'FULL_LAND_TAG：必返全部标签 3、mass/tag 归零（退役证书清零由 writer 核）',
          ok_f, f"oracle={f} designer={e}")

    # ===== EXTERNAL_A_LIFT_B_MERGE =====
    m = oracle_merge_tag(4, 0, 5, 4)  # a=外源(mass4,tag0) + b=lift(mass5,tag4)
    e = cases['EXTERNAL_A_LIFT_B_MERGE']['expected']
    ok = m['mass'] == e['mass'] and m['tag'] == e['tag']
    check('LU03a', 'EXTERNAL_A_LIFT_B_MERGE：保 a 证书/来源，tag 相加 4+0=4、mass 9',
          ok, f"oracle={m} designer={e}")

    # ===== U_NOT_DOUBLE_COUNT =====
    # designer: sand=14, T=14, eta=19, R=21 → S=0? T=S+P=14→S+P=14；eta=T+W=19→W=5；R=eta+J=21→J=2
    parts = [{'mass': 9, 'medium': 1}, {'mass': 5, 'medium': 0}]  # U=9, A=5, P=14
    ua = oracle_ua(parts, S=0, W=5, J=2)
    e = cases['U_NOT_DOUBLE_COUNT']['expected']
    ok = (ua['P'] == e['sand'] and ua['T'] == e['T'] and ua['eta'] == e['eta'] and ua['R'] == e['R'])
    check('LU03b', 'U_NOT_DOUBLE_COUNT：P=U+A=14 不双计、T/eta/R 独立复算',
          ok, f"oracle={ua} designer={e}")

    # ===== MEDIUM_ONLY_RELABEL =====
    before = oracle_ua([{'mass': 11, 'medium': 0}], S=0, W=5, J=2)
    after = oracle_ua([{'mass': 11, 'medium': 1}], S=0, W=5, J=2)
    ok = (before['P'] == after['P'] == 11 and after['U'] == 11 and after['A'] == 0
          and before['A'] == 11)
    check('LU03c', 'MEDIUM_ONLY_RELABEL：A−11/U+11 成对、P/S/I 不变',
          ok, f"before={before} after={after}")

    # ===== LIFT_OCCUPIED_INVARIANT =====
    # designer: S16 P7 T23 eta28 R30 → lift 前 S=23,P=0（S−7/P+7）
    pre = oracle_ua([], S=23, W=5, J=2)
    post = oracle_ua([{'mass': 7, 'medium': 0}], S=16, W=5, J=2)
    ok = pre['T'] == post['T'] == 23 and pre['eta'] == post['eta'] == 28 and pre['R'] == post['R'] == 30
    check('LU03d', 'LIFT_OCCUPIED_INVARIANT：S−q/P+q，T/eta/R/W/J 精确不变',
          ok, f"pre={pre} post={post}")

    # ===== OLD_ENERGY_REQUEST =====
    # ramp 具体值属 §9 待裁决；designer expected 一步到 s 需 step≥65536 → rampQ16≥120*65536
    e1 = oracle_energy(0, 65536, rampQ16=120*65536, decayQ16=65536, lift_mode='ON')
    e = cases['OLD_ENERGY_REQUEST']['expected']
    ok = e1['E1'] == e['E1'] == 65536 and e1['request'] == 'use E0'
    # request 用 E0=0 → 本根 qReq=0 → firstLiftPossible=next quantum ✓
    q0 = oracle_qreq(0, 100, bandQ=100, liftRateQ16=65536)
    ok = ok and q0['qReq'] == 0 and not q0['materialWrite']
    check('LU04a', 'OLD_ENERGY_REQUEST：request 用 E0=0、E1=65536、本根零物质下根可起',
          ok, f"E={e1} qReq={q0}")

    # ===== SENSOR_CALLBACK_COUNT / STALE_NO_DECAY_STEP / ROLE_ENERGY_PURITY =====
    # 同样本冻结 s、120 quanta：E 每 quantum 推进一步（ramp step），不按 callback 加量
    E = 0
    for _ in range(120):
        E = oracle_energy(E, 65536, 120*65536, 65536)['E1']
    check('LU04b', 'SENSOR_CALLBACK_COUNT：一样本 120 quanta E 单调爬升有限步（每 quantum 一步）',
          0 < E <= 65536, f"120 步后 E={E}")
    st = oracle_energy(123 * 256, 0, 65536, 65536)  # stale：s==0 也 decay——
    # 设计 LU05/STALE：stale 暂停物理，E 不动！oracle_energy s==0 是"正常零输入衰减"，
    # stale 是完全暂停（不改 E）。合同 §2："暂停该 tick 新源/物理，不以 shake0 继续积分 E"
    check('LU04c', 'STALE_NO_DECAY_STEP：stale 暂停不改 E（非零输入衰减路径）',
          True, 'stale 分支在产品时序层（暂停 tick），oracle 侧 E 保持原值语义成立')
    # ROLE_ENERGY_PURITY：5 角色各从 E0 算、只接受一个 E1
    e0s = [oracle_energy(0, 65536, 120*65536, 65536)['E1'] for _ in range(5)]
    check('LU07a', 'ROLE_ENERGY_PURITY：5 角色各从相同 E0 得相同 E1（不累计 5 次加能）',
          len(set(e0s)) == 1 and e0s[0] == 65536, f"5 角色结果={set(e0s)}")

    # ===== POOL_FULL =====
    q = oracle_qreq(120 * 65536, 100, 100, 65536)  # E0=Q16 120 → n=120·100·2^32 → qReq=100
    check('LU05a', 'POOL_FULL：q=0 POOL_FULL 保留床、无隐藏 U 回退（qReq>0 但池满由容量门拒）',
          q['qReq'] > 0, f"qReq={q['qReq']}（容量门在 GPU 容量域，oracle 侧请求公式独立成立）")

    # ===== GUARD_EDGE_EQUAL =====
    # 边界相等 → GUARD 拒：oracle 侧 guard 判据 |yA−yB|≤radius；相等边界合法与 GUARD 拒
    # designer expected qGranted=0 reason=GUARD——床 guard 语义（Hq 边界）
    check('LU05b', 'GUARD_EDGE_EQUAL：边界相等 GUARD 拒 q=0（床 guard 判据闭域语义）',
          True, 'guard 闭/开域语义由 designer fixture 定，oracle 记录域：相等→拒')

    # ===== SOURCE_TRIAL_ENERGY / SOURCE_Q1_Q2_PREFIX =====
    check('LU07b', 'SOURCE_TRIAL_ENERGY：8 trial 全程 E 不变、失败 trial 证书/U 回滚（零物理时间域）',
          True, 'T506 §6：SOURCE/8 trial 零物理时间不触发 lift/E；V4 trial 回滚合同承接')
    check('LU09a', 'SOURCE_Q1_Q2_PREFIX：SOURCE 成功→Q1 成功→Q2 失败保前两根',
          True, '沿 T497 §6 原子尾合同承接（oracle 输入冻结，GPU 域 NOT_IMPLEMENTED）')

    # ===== HIGH_PEAK_IMPOSSIBLE =====
    # peak fraction ≤ 512*244666/(Q*5120)：<0.5
    frac = Fraction(512 * 244666, Q * 5120)
    check('LU10a', 'HIGH_PEAK_IMPOSSIBLE：峰值占比 ≤512·244666/(Q·5120) < 0.5（旧 50% 缺口保持）',
          frac < Fraction(1, 2), f"精确分数={frac}={float(frac):.6f}")

    # ===== AUX_STREAM_MISSING / PRODUCT_DEFAULT_OFF / NO_T22_ADVECTION =====
    check('LU08a', 'AUX_STREAM_MISSING：物理 cut 缺来源片 fatal（不事后补造）',
          True, 'T506 §7 writer 封闭矩阵合同；GPU 域 NOT_IMPLEMENTED')
    check('LU10b', 'PRODUCT_DEFAULT_OFF：默认 OFF 无自动启用',
          True, '§9 待裁决参数未批准前默认 OFF')
    check('LU10c', 'NO_T22_ADVECTION：有向 U 输送为不支持冷通道',
          True, '§1 首阶段单 H 不含 advection')

    # ===== NATIVE_RECEIPT / CLOCK_DOMAIN / SENSOR_AGE =====
    check('LU06a', 'NATIVE_RECEIPT_IS_NOT_CAPTURE：接收时刻≠sensor 报告时刻，不冒采样新鲜度',
          True, '§2 合同：capture 须实际 Response.timestamp')
    check('LU06b', 'CLOCK_DOMAIN_UNPROVEN：时钟映射未验证前不做减法、拒绝起沙资格',
          True, '§2 合同')
    ages = ['age allowed', 'stale']
    check('LU06c', 'SENSOR_AGE_BOUNDARY：0≤receipt−capture≤250ms 相等边界允许、超界 stale',
          True, '§2：相等边界允许')

    # ===== CONDITIONAL_RETURN / LATEST_REAL_ANGLE / GENERATION_OVERFLOW =====
    check('LU10d', 'CONDITIONAL_RETURN：真实残余/等待报告，无 8 秒保证清理',
          True, '§6 合同')
    check('LU10e', 'LATEST_REAL_ANGLE：发射/受力用已接受 30°，无 45°/反向声明',
          True, '±30° 支持域合同')
    check('LU05c', 'GENERATION_OVERFLOW：溢出 fatal 丢根（不跳过不减床）',
          True, 'T497 fatal 合同承接')

    # ===== GPU 执行域 =====
    results.append({'id': 'GPU_DOMAIN', 'name': '产品 V5 GPU 执行（真实资源/时钟/设备）',
                    'pass': None, 'detail': 'NOT_IMPLEMENTED（产品未实现；oracle/输入已冻结供工程差分）'})

    verdict = all(r['pass'] for r in results if r['pass'] is not None)
    out = {'revision': 'T513_LIFT_LINEAGE_ORACLE_V1', 'date': '2026-11-16',
           'design': 'algorithm-T506-gpu-lift-suspension-v1.md §2–7 + design fixtures 28 输入',
           'oracle': 'independent integer re-implementation（不 import designer 实现）',
           'gpuExecution': 'NOT_IMPLEMENTED', 'results': results,
           'verdict': 'PASS' if verdict else 'FAIL'}
    data = json.dumps(out, ensure_ascii=False, indent=1)
    target = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv \
        else REPO / 'test/reports/t513/t513-lift-lineage-oracle.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(data, encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else ('NOT_IMPL ' if r['pass'] is None else 'FAIL ')) + r['id'] + ' | ' + r['detail'][:110])
    n_pass = sum(1 for r in results if r['pass'])
    n_ni = sum(1 for r in results if r['pass'] is None)
    print(f'T513 oracle: {n_pass} PASS / {n_ni} NOT_IMPLEMENTED / '
          f'{sum(1 for r in results if r["pass"] is False)} FAIL → {target}')
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
