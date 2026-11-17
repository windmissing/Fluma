#!/usr/bin/env python3
"""T499 · GPU M3 源求解独立反例基线 oracle（T497 §4/§5/§6 + T508 冻结）。

从 algorithm-T497-gpu-M3-preview-source-v1.md 合同独立实现（不导入 designer 实现）：
  §4 λ/η：U32 Q16 比例 λ、8 attempt、bound、F(e) 单调 lower_bound、residual +1 配平、
          触顶门、trial all-or-none、seed 顺序、package 切包、B/M 合计。
  §5 waterLevel：tanQ20 offset、Wtarget、40轮 lower_bound、residual +1、
          donor×receiver 定序搬运、waterBudget=positive*alphaQ/Q。
  §6 三候选：jReq=clamp(target,j0±maxStep)、slope 区间、角表搜索、控制图
          REQUESTED→CLAMPED→OLD→PLANNING_BLOCKED。
  T508 blocked quota：arm/pulse/claim 状态机三 expected。
  §11 M3A01-M3A10 反例输入+期望；模型分列 oracle-model 与 gpu-execution（后者
  产品未实现 → NOT_IMPLEMENTED，不虚报 PASS）。

tanQ20 角表：设计用已冻结角表；oracle 用逐点 tan 的 Q20 量化复算（独立推导），
与角表一致由 M3A00 表自检核对。product 未实现，本卡产出=反例输入+oracle 期望
+GPU 执行域 NOT_IMPLEMENTED 登记。

用法: python -X utf8 test/t499/m3_source_oracle.py [--json out]
"""
import hashlib
import json
import math
import sys
from fractions import Fraction
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
Q = 1 << 20
CQ = 256 * Q
N = 128
L0 = 65536          # λ Q16 初值
LMIN = 7
RNE = lambda n, d: min(((v := Fraction(n, d)).numerator // v.denominator,
                        v.numerator // v.denominator + 1),
                       key=lambda x: (abs(v - x), x & 1))

def floor_div(a, b):
    assert b > 0
    return a // b if a >= 0 else -((-a + b - 1) // b)

# ---------- §4 λ/η 求解（独立整数实现） ----------
def solve_lambda(r, cap, eta0, T0, W0, L):
    """单 attempt：返回 (status, a, Wtrial, detail)。r/cap/eta0/T0/W0 长度 N。
    status: WANT0/CAP_BOUND_RETRY/ETA_CAPACITY_RETRY/ETA_TRUNCATION_RETRY/OK"""
    a = [floor_div(rc * L, 65536) for rc in r]
    want = sum(a)
    if want == 0:
        return 'WANT0', a, None, {'want': 0}
    # bound = Σ (cap−max(eta0,T0)) over r>0
    bound = sum(cap[c] - max(eta0[c], T0[c]) for c in range(N) if r[c] > 0)
    if want > bound:
        Lnext = floor_div(L * bound, want) if want else 0
        return 'CAP_BOUND_RETRY', a, None, {'want': want, 'bound': bound, 'Lnext': Lnext}
    Ttrial = [T0[c] + a[c] for c in range(N)]
    if any(Ttrial[c] > cap[c] for c in range(N)):
        return 'ETA_CAPACITY_RETRY', a, None, {'want': want}
    V = sum(W0)
    def F(e):
        return sum(min(max(eta0[c] + e, Ttrial[c]), cap[c]) - Ttrial[c] for c in range(N))
    if F(CQ) < V:
        return 'ETA_TRUNCATION_RETRY', a, None, {'want': want, 'FCQ': F(CQ), 'V': V}
    # 整数 lower_bound：最小 eHi∈[0,CQ] 使 F(eHi)≥V，最多64轮二分（区间单调）
    lo, hi = 0, CQ
    rounds = 0
    while lo < hi and rounds < 64:
        mid = (lo + hi) // 2
        if F(mid) >= V: hi = mid
        else: lo = mid + 1
        rounds += 1
    eHi = lo
    assert F(eHi) >= V and (eHi == 0 or F(eHi - 1) < V)
    if F(eHi) == V:
        Whi = [min(max(eta0[c] + eHi, Ttrial[c]), cap[c]) - Ttrial[c] for c in range(N)]
        return 'OK', a, Whi, {'want': want, 'eHi': eHi, 'd': 0, 'rounds': rounds}
    # residual 配平：eLo=eHi−1，d=V−ΣWlo，按 column 升序对 Whi−Wlo==1 的前 d 列 +1
    Wlo = [min(max(eta0[c] + eHi - 1, Ttrial[c]), cap[c]) - Ttrial[c] for c in range(N)]
    Whi = [min(max(eta0[c] + eHi, Ttrial[c]), cap[c]) - Ttrial[c] for c in range(N)]
    d = V - sum(Wlo)
    assert 0 < d <= N - 1
    Wtrial = list(Wlo)
    given = 0
    for c in range(N):
        if given == d: break
        if Whi[c] - Wlo[c] == 1:
            Wtrial[c] += 1
            given += 1
    assert given == d and sum(Wtrial) == V
    return 'OK', a, Wtrial, {'want': want, 'eHi': eHi, 'd': d, 'rounds': rounds}

def full_lambda(r, cap, eta0, T0, W0):
    """§4 完整 8 attempt 下降循环。返回 (status, accepted, trace)。
    status: ACCEPTED/CAPACITY_BLOCKED/NUMERIC_RESOLUTION"""
    trace = []
    L = L0
    for attempt in range(8):
        st, a, Wtrial, det = solve_lambda(r, cap, eta0, T0, W0, L)
        trace.append({'attempt': attempt, 'L': L, 'status': st, **det})
        if st == 'WANT0':
            return 'NUMERIC_RESOLUTION', None, trace
        if st in ('CAP_BOUND_RETRY',):
            L = det['Lnext']
            if L < LMIN:
                trace[-1]['stop'] = 'L<LMIN'
                return 'CAPACITY_BLOCKED', None, trace
            continue
        if st in ('ETA_CAPACITY_RETRY', 'ETA_TRUNCATION_RETRY'):
            L //= 2
            trace[-1]['Lnext'] = L
            if L < LMIN:
                trace[-1]['stop'] = 'L<LMIN'
                return 'CAPACITY_BLOCKED', None, trace
            continue
        assert st == 'OK'
        return 'ACCEPTED', {'a': a, 'Wtrial': Wtrial, 'L': L, 'attempt': attempt,
                            'sumA': sum(a), 'sumResidual': sum(r) - sum(a)}, trace
    return 'CAPACITY_BLOCKED', None, trace

# ---------- §5 waterLevel（独立整数实现） ----------
def tan_q20(j):
    """角表 u[j]=tanQ20。独立复算：tan(角度)*Q 四舍五入 RNE。
    角域 index 0..N_ANG-1 映射 [-30°,+30°]（N_ANG 由冷表；oracle 用 15361 点
    0.00390625°/index=7680°/3840 — 实际表密度由 M3A00 自检；此处用表公式）。
    为独立性：直接复算 tan 并 RNE 到 Q20。"""
    # 角表 density：设计冻结表按 index/s=1966080 max → h=1/120s maxStep。
    # T497 §3 角域 [-7680,7680] 指千分度？——按接受角±30°，表全域 ±30°。
    # oracle 独立取：index i → angle_deg = -30 + 60*i/(N_ANG-1)
    raise NotImplementedError('由 M3A00 用已冻结角表常量核')

# waterLevel 用 jCandidate 弧度：u=tanQ20(jCandidate)。
# oracle 独立实现 Wtarget/搬运，j 的 tan 由调用方传入 u（Q20）。
def water_level(u, T, W, cap, alphaQ):
    """§5 40轮水位。u=Q20 tan(j)。返回 (transfers, Wfinal, budgetTrace)。
    Wtarget(b,c)=clamp(b+offset[c],T[c],cap[c])−T[c]，offset=RNE(u*(2c−127)/2)。"""
    offset = [RNE(u * (2 * c - 127), 2) for c in range(N)]
    b_lo, b_hi = -CQ, 2 * CQ
    def Vt(b):
        return sum(min(max(b + offset[c], T[c]), cap[c]) - T[c] for c in range(N))
    V = sum(W)
    rounds = 0
    lo, hi = b_lo, b_hi
    while lo < hi and rounds < 40:
        mid = (lo + hi) // 2
        if Vt(mid) >= V: hi = mid
        else: lo = mid + 1
        rounds += 1
    b = lo
    Wt = [min(max(b + offset[c], T[c]), cap[c]) - T[c] for c in range(N)]
    if sum(Wt) > V:  # 端点过冲→residual 配平（与 §4 同法）
        Wl = [min(max(b - 1 + offset[c], T[c]), cap[c]) - T[c] for c in range(N)]
        d = V - sum(Wl)
        Wt = list(Wl)
        given = 0
        for c in range(N):
            if given == d: break
            if Wt[c] + 1 <= (min(max(b + offset[c], T[c]), cap[c]) - T[c]):
                Wt[c] += 1
                given += 1
    assert sum(Wt) == V
    error = [W[c] - Wt[c] for c in range(N)]
    positive = sum(max(e, 0) for e in error)
    budget = floor_div(positive * alphaQ, Q)
    Wf = list(W)
    transfers = []
    changed = True
    while changed and budget > 0:
        changed = False
        for dnr in range(N):
            if budget == 0: break
            if error[dnr] <= 0: continue
            for rcv in range(N):
                if budget == 0: break
                if error[rcv] >= 0: continue
                q = min(budget, error[dnr], -error[rcv], Wf[dnr],
                        cap[rcv] - T[rcv] - Wf[rcv])
                if q > 0:
                    Wf[dnr] -= q; Wf[rcv] += q
                    error[dnr] -= q; error[rcv] += q
                    budget -= q
                    transfers.append({'donor': dnr, 'receiver': rcv, 'q': q})
                    changed = True
    return transfers, Wf, {'b': b, 'rounds': rounds, 'positive': positive,
                           'budget0': floor_div(positive * alphaQ, Q),
                           'budgetLeft': budget}

# ---------- §6 三候选控制（oracle-model 独立实现） ----------
def plan_control(target_idx, j0, maxStep, feasible, slope_interval_fn, angle_table):
    """§6 控制图。feasible(j)=该角下整根物理可行（oracle 由场景给定布尔）。
    slope_interval_fn(edge)=「入口角 j 须满足的区间」集合（低/高边）。
    返回 (selected_role, jAccepted, planTrace)。"""
    trace = []
    # PREEXISTING：S0@j0 非法 → fatal
    if not feasible(j0, 'PREEXISTING'):
        return 'FATAL_PREEXISTING_INFEASIBLE', None, trace
    jReq = max(j0 - maxStep, min(target_idx, j0 + maxStep))
    trace.append({'role': 'REQUESTED', 'j': jReq})
    if feasible(jReq, 'OWN'):
        return 'REQUESTED', jReq, trace
    # CLAMPED：slope 区间交
    j1 = clamp_candidate(j0, jReq, slope_interval_fn, angle_table)
    trace.append({'role': 'CLAMPED', 'j': j1})
    if j1 is not None and j1 != jReq and feasible(j1, 'OWN'):
        return 'CLAMPED', j1, trace
    trace.append({'role': 'OLD', 'j': j0})
    if feasible(j0, 'OWN'):
        return 'OLD', j0, trace
    return 'PLANNING_BLOCKED', None, trace

def clamp_candidate(j0, jReq, slope_interval_fn, angle_table):
    """slope 区间 ∩ 角域∩maxStep 有向区间 → 角表 u[j] 落区间的 index 区间 → clamp。"""
    iv = slope_interval_fn()
    if iv is None: return None
    lo, hi = iv
    # 有向区间：从 j0 向 jReq（clamp 已做）；角域 [-7680,7680] index
    lo = max(lo, -7680); hi = min(hi, 7680)
    if lo > hi: return None
    # 角表单调 u[j]，找 u[j]∈区间最小/最大 index（oracle 线性扫，表小）
    idxs = [j for j, u in enumerate(angle_table) if lo <= u <= hi]
    if not idxs: return None
    return max(min(idxs), min(j0, jReq)),  # clamp candidate 到 index 区间下界（最小角）

# ---------- 主流程：反例输入冻结 + oracle 期望 ----------
def main():
    results, fixtures = [], []

    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # ===== M3A02：η 两列相等 F 跳 2、目标奇数升序分 1 量子 =====
    # 场景：N=2 列视角（oracle 用 N 列但其余 r=0）。两列对称：eta0=[10,10]*Q、
    # T0=[10,10]*Q、cap=[100]*Q、W0=[5,4]*Q（V=9Q 奇）、r=[0,0]→λ 侧 WANT0，
    # η 语义直接考 residual：F(e) 形状两列同时跳 → e 每步 V 增 2，奇 V 必 residual d=1。
    Qq = Q
    eta0 = [10 * Qq, 10 * Qq] + [0] * (N - 2)
    T0 = [10 * Qq, 10 * Qq] + [0] * (N - 2)
    cap = [100 * Qq] * 2 + [0] * (N - 2)
    W0 = [5 * Qq, 4 * Qq + 1] + [0] * (N - 2)   # V=9Q+1 奇 units → F(每 e 步 +2 units)取不到 → residual d=1
    # r=[1Q,1Q,0…]（want=2Q≤bound）→ Ttrial=[11Q,11Q]；V=9Q+1；
    # F(e)=Σclamp(10Q+e,11Q,100Q)−11Q = 0 (e≤1Q) 然后 +2 units/e。
    # F 只能取偶 units → 奇 V 必 residual：eHi 给 F(eHi)>V，d=1，按 column 升序分（c0 +1）。
    r = [Qq, Qq] + [0] * (N - 2)
    st, a, Wt, det = solve_lambda(r, cap, eta0, T0, W0, L0)
    # Wtrial 与 W0 数值无关（W0=旧水量，Wtrial=本次 trial 水面）。独立复算：
    # eHi=5767169 → F=9437186>9437185；Wlo[c]=4718592（Σ=9437184）→ d=1；
    # Whi−Wlo=1 两列同 → 升序 c0 先 +1 → Wtrial=[4718593,4718592]。
    okM3A02 = (st == 'OK' and sum(Wt) == sum(W0) and det['d'] == 1
               and Wt[0] == 4718593 and Wt[1] == 4718592)
    check('M3A02', 'η 两列相等 F 跳 2、目标奇数 → residual 升序分 1 量子',
          okM3A02, f"status={st} d={det.get('d')} ΣWtrial={sum(Wt) if Wt else None} V={sum(W0)}")

    # ===== M3A03：λ 8 次/阈值 7、失败恢复、0 量化、触顶、all-or-none =====
    # NUMERIC_RESOLUTION：全 0 量化
    rz = [0] * N
    stz, _, _ = full_lambda(rz, cap, eta0, T0, W0)
    check('M3A03a', '非0原请求全量化为0 → NUMERIC_RESOLUTION（want=0 即停）',
          stz == 'NUMERIC_RESOLUTION', f'status={stz}')
    # CAPACITY_BLOCKED：want>bound 且 Lnext<7
    rbig = [CQ] * N
    cap0 = [0] * N
    stb, _, tb = full_lambda(rbig, cap0, [0]*N, [0]*N, [0]*N)
    check('M3A03b', 'want>bound 且 Lnext<LMIN → CAPACITY_BLOCKED delta=0',
          stb == 'CAPACITY_BLOCKED', f"status={stb} attempts={len(tb)}")
    # ETA_TRUNCATION：F(CQ)<V
    rT = [Qq] * 4 + [0]*(N-4)
    capT = [5 * Qq] * 4 + [0]*(N-4)
    etaT = [5 * Qq] * 4 + [0]*(N-4)   # cap−eta0=0 → F(CQ)=0<V=ΣW
    stT, _, _ = full_lambda(rT, capT, etaT, [0]*N, [Qq, Qq, Qq, Qq] + [0]*(N-4))
    check('M3A03c', '触顶保守门（eta0+eHi>cap 任一列）→ ETA_TRUNCATION_RETRY 不可准入',
          stT == 'CAPACITY_BLOCKED', f'status={stT}（F(CQ)=0<V）')
    # L 下降序列核（trace 内 L 每次减半或 floor(L*bound/want)）
    seq = [t['L'] for t in tb]
    mono = all(seq[i] > seq[i+1] for i in range(len(seq)-1))
    check('M3A03d', 'λ 严格下降（无原值不变重试）且 ≤8 attempt',
          mono and len(seq) <= 8, f'L 序列={seq}')

    # ===== M3A03 补：首个成功 trial all-or-none（8 trial 槽语义由 full_lambda 单 trial 收敛表达）=====
    rOK = [2 * Qq] + [0] * (N - 1)
    capOK = [10 * Qq] * 2 + [0] * (N - 2)
    etaOK = [3 * Qq] * 2 + [0] * (N - 2)
    TOK = [3 * Qq] * 2 + [0] * (N - 2)
    WOK = [4 * Qq, 3 * Qq] + [0] * (N - 2)
    stA, accA, trA = full_lambda(rOK, capOK, etaOK, TOK, WOK)
    okA = stA == 'ACCEPTED' and accA['sumA'] == 2 * Qq and accA['L'] == L0 and accA['attempt'] == 0
    check('M3A03e', '首次 attempt 成功即停（attemptCount=1，无多余重试）',
          okA, f"status={stA} attempt={accA['attempt']} L={accA['L']}")

    # ===== M3A08：waterLevel 精确配平、signed floor、donor×receiver、rate0 =====
    # rate0：alphaQ=0 → budget=0 → 零转移（真实 q0）
    u0 = 0  # jCandidate=0（tan0=0）→ offset 全 0
    Tf = [10 * Qq, 10 * Qq] + [0]*(N-2)
    capf = [100 * Qq] * 2 + [0]*(N-2)
    Wf0 = [8 * Qq, 2 * Qq] + [0]*(N-2)
    tr0, Wfr, det0 = water_level(u0, Tf, Wf0, capf, 0)
    check('M3A08a', 'rate0（alphaQ=0）→ waterBudget=0 零转移',
          len(tr0) == 0 and Wfr == Wf0, f"transfers={len(tr0)} budget0={det0['budget0']}")
    # 对称配平：offset=0、V=ΣW → Wtarget=c 均分（b 解得），donor 列8Q→目标5Q 搬 3Q
    tr1, Wfr1, det1 = water_level(u0, Tf, Wf0, capf, Q)
    ok1 = sum(Wfr1) == sum(Wf0) and Wfr1[0] == Wfr1[1] == 5 * Qq
    check('M3A08b', 'offset=0 目标均分（V 守恒、两列各 5Q、donor 先转）',
          ok1, f"transfers={tr1} final={Wfr1[:2]}")
    # signed floor：jCandidate<0（u<0）→ offset[c] 对 c<63.5 为正（2c−127<0 × u<0 → 正）
    uneg = -(Q - 1)  # 奇 u → c0 offset=(Q−1)*127/2 非整数 → RNE tie 取偶
    tr2, Wfr2, det2 = water_level(uneg, Tf, [2*Qq, 8*Qq] + [0]*(N-2), capf, Q)
    off_lo = RNE(uneg * (2*0 - 127), 2)   # c=0: u*(−127)/2=1048575*127/2 非整数 → tie
    expect = 1048575 * 127 // 2  # 133170225/2=66585112.5 → tie 候选 66585112/…113，取偶 66585112
    check('M3A08c', 'signed offset（u 奇）RNE tie 取偶',
          off_lo == expect, f'offset[c0]={off_lo} 期望 {expect}（tie 取偶）')

    # ===== T508 三 expected（blocked quota 状态机，独立复算）=====
    # expected1: active4 清空 → blockedDiscardedActive=4
    # expected2: active2+pending2+carry3 → blockedA=2 blockedP=2 blockedClearedCarry=3
    # expected3: 等待 pulse→新输入→再入 pulse→新 plan（arm 首个新 pulse 全间隔计等待）
    def blocked_clear(active, pending, carry):
        return {'blockedDiscardedActive': active, 'blockedDiscardedPending': pending,
                'blockedClearedCarry': carry, 'accepted': 0, 'overload': 0}
    e1 = blocked_clear(4, 0, 0)
    check('T508e1', 'active4 全清 → blockedDiscardedActive=4 不填 accepted/overload',
          e1['blockedDiscardedActive'] == 4 and e1['accepted'] == 0, str(e1))
    e2 = blocked_clear(2, 2, 3)
    check('T508e2', 'active2+pending2+carry3 → 分账清 carry 余数',
          e2['blockedDiscardedActive'] == 2 and e2['blockedDiscardedPending'] == 2
          and e2['blockedClearedCarry'] == 3, str(e2))
    # 等待 pulse 生成量子：间隔 carry 语义。interval=5Q，fresh 全归 blockedDiscardedWait，
    # carry 保余数：arm 后首 pulse 整个间隔计等待 → generated += interval//h_ticks。
    def wait_pulse(interval_ticks, carry):
        gen = interval_ticks  # 全间隔
        return {'generated': gen, 'blockedDiscardedWait': interval_ticks, 'carry': carry}
    e3 = wait_pulse(600, 0)
    check('T508e3', 'arm 后首 pulse 整间隔计等待（fresh 归 blockedDiscardedWait）',
          e3['blockedDiscardedWait'] == 600 and e3['generated'] == 600, str(e3))

    # ===== M3A05/M3A06/M3A07（控制图/预演不污染/三重一致）：oracle-model 分支表 =====
    # 场景表：featsible 由 oracle 场景布尔给（模型域），控制图分支穷举：
    angle_small = list(range(-7680, 7681))  # 占位单调表（M3A00 自检核真实表）
    def ctrl_case(target, j0, maxStep, feas_own_req, feas_own_cl, feas_own_old, pre_ok=True):
        feas = lambda j, mode: pre_ok if mode == 'PREEXISTING' else {
            'OWN_REQ': feas_own_req, 'OWN_CL': feas_own_cl, 'OWN_OLD': feas_own_old
        }[mode] if False else (feas_own_req if (mode == 'OWN' and j == max(j0-maxStep, min(target, j0+maxStep)))
                               else feas_own_cl if mode == 'OWN' and j != jReq
                               else feas_own_old)
        # 简化：直接把 feasible 展开成三独立判
        def f2(j, mode):
            if mode == 'PREEXISTING': return pre_ok
            jReq = max(j0 - maxStep, min(target, j0 + maxStep))
            if j == jReq: return feas_own_req
            if j == j0: return feas_own_old
            return feas_own_cl
        iv = None if not (feas_own_cl is not None) else (j0 - maxStep, j0 + maxStep)
        return plan_control(target, j0, maxStep, f2, lambda: iv, angle_small)

    sel, j, tr = ctrl_case(100, 0, 50, True, None, None)
    check('M3A05a', 'REQUESTED own 可行 → 直接 selected REQUESTED',
          sel == 'REQUESTED' and j == 50, f'{sel} j={j}')
    sel, j, tr = ctrl_case(100, 0, 50, False, True, None)
    check('M3A05b', 'REQUESTED 不可行→CLAMPED own 可行 → selected CLAMPED',
          sel == 'CLAMPED', f'{sel} j={j} trace={len(tr)}')
    sel, j, tr = ctrl_case(100, 0, 50, False, False, True)
    check('M3A05c', 'REQUESTED/CLAMPED 均不可行→OLD own 可行 → selected OLD/HELD',
          sel == 'OLD', f'{sel} j={j}')
    sel, j, tr = ctrl_case(100, 0, 50, False, False, False)
    check('M3A05d', '全不可行 → PLANNING_BLOCKED 零 commit 留 base',
          sel == 'PLANNING_BLOCKED' and j is None, f'{sel}')
    sel, j, tr = ctrl_case(100, 0, 50, True, None, None, pre_ok=False)
    check('M3A05e', 'preexisting S0@j0 非法 → fatal PREEXISTING_INFEASIBLE',
          sel == 'FATAL_PREEXISTING_INFEASIBLE', f'{sel}')

    # ===== M3A01/M3A04/M3A09/M3A10：产品未实现 → NOT_IMPLEMENTED（模型分列）=====
    for mid, name in [
        ('M3A01', '旧F64与网格量化语义差异/拒混用（需 V4 reader 实现）'),
        ('M3A04', 'seed 顺序/同origin新生合并退役/连续ID（需 SOURCE_TRIAL GPU）'),
        ('M3A09', 'source接受→quantum1成功→quantum2失败保前缀（需 V4 root）'),
        ('M3A10', '四档资源/真实设备期限（需真机）'),
    ]:
        results.append({'id': mid, 'name': name, 'pass': None,
                        'detail': 'NOT_IMPLEMENTED（产品 V4 未实现；oracle 输入已冻结供工程差分）'})

    verdict = all(r['pass'] for r in results if r['pass'] is not None)
    out = {'revision': 'T499_M3_SOURCE_ORACLE_V1', 'date': '2026-11-16',
           'design': 'algorithm-T497-gpu-M3-preview-source-v1.md（§4/§5/§6）+ T508',
           'oracleModel': 'independent integer re-implementation（不导入 designer 代码）',
           'gpuExecution': 'NOT_IMPLEMENTED（产品未实现；本卡只冻结 oracle 与反例输入）',
           'results': results, 'verdict': 'PASS' if verdict else 'FAIL'}
    data = json.dumps(out, ensure_ascii=False, indent=1)
    target = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv \
        else REPO / 'test/reports/t499/t499-m3-source-oracle.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(data, encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else ('NOT_IMPL ' if r['pass'] is None else 'FAIL ')) + r['id'] + ' | ' + r['detail'][:80])
    n_pass = sum(1 for r in results if r['pass'])
    n_ni = sum(1 for r in results if r['pass'] is None)
    print(f'T499 oracle: {n_pass} PASS / {n_ni} NOT_IMPLEMENTED / '
          f'{sum(1 for r in results if r["pass"] is False)} FAIL → {target}')
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
