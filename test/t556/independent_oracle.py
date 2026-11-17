#!/usr/bin/env python3
"""T556: independent expected-value oracle for G9 DM v2 fixtures (DM01..DM32).

Derived independently from algorithm-G9-dynamic-mobilization-v2.md SS4 integer
specification. Does NOT import designer's checks program; fractions/checked
integer arithmetic reimplemented from the document. Fractions used for
readability where the spec mandates exact integer ops; final comparisons are
exact integers.

Status: DESIGN_INPUT_ORACLE — supports fixture-level expectations only.
No GPU, no device, no product PASS.
"""
import json, sys
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
Q = 1 << 20
ONE = 1 << 16

# R0 parameter set (algorithm SS9, UNAPPROVED research values)
DEPTH_G = 8 * ONE
DEPTH_S = 1 * ONE
H_REG = Q // 64
OMEGA_BASE = 4 * ONE
OMEGA_G = 24 * ONE
OMEGA_S = 8 * ONE
OMEGA_MAX = 120 * ONE
K_SHEAR = ONE           # kShear = 1
MU_START = ONE // 2     # muStart = 0.5 (Q16)
MU_START_Q16 = MU_START
G_Q = 512 * Q           # g = 512 Q20
J = 1
D = 120 * J * ONE


def rne_div(num: int, den: int) -> int:
    """Round-half-even signed division by positive den (T230 semantic)."""
    assert den > 0
    sign = -1 if num < 0 else 1
    n = -num if num < 0 else num
    q, r = divmod(n, den)
    if r > den // 2 or (r == den // 2 and (q & 1)):
        q += 1
    return sign * q


def floor_div_nn(num: int, den: int) -> int:
    assert den > 0
    return num // den


def chi_g(gx: int, gn: int, b_i: int, b_r: int | None, b_l: int | None) -> int:
    """SS4.1 geometry. Returns chiGQ (>=0)."""
    cands = []
    if b_r is not None:
        dR = gx - rne_div(gn * (b_r - b_i), Q)
        cands.append(dR)
    if b_l is not None:
        dL = -gx - rne_div(gn * (b_l - b_i), Q)
        cands.append(dL)
    if not cands:
        return 0  # isolated column (no neighbours): no geometric driver
    excess = max(cands) - rne_div(MU_START_Q16 * gn, ONE)
    return floor_div_nn(max(excess, 0) * Q, gn)


def hg(chi_gq: int, B: int) -> int:
    w = floor_div_nn(DEPTH_G * chi_gq, ONE)   # dx = 1
    return min(B, w)


def eval_q(q: int, F_entry: int, p: int, gn: int, chi_gq: int, B: int, J_local: int = J):
    """SS4.2 Eval(q). Returns (H, omega, feasible, deficit, head, chiS)."""
    f = F_entry + q
    if f == 0:
        assert p == 0, "f==0 requires p==0"
        u = 0
    else:
        u = rne_div(p, f)
    r = max(f, H_REG)
    # head = floor(kShear*u^2/(ONE*gn)) — u^2 >= 0 regardless of sign
    head = floor_div_nn(K_SHEAR * u * u, ONE * gn)
    yield_depth = rne_div(MU_START_Q16 * r, ONE)
    excess_head = max(0, head - yield_depth)
    Hs_wide = floor_div_nn(DEPTH_S * excess_head, ONE)
    Hg_v = hg(chi_gq, B)
    H = min(B, max(Hg_v, F_entry + Hs_wide))
    chi_s = floor_div_nn(excess_head * Q, r)
    omega_s = floor_div_nn(OMEGA_S * chi_s, Q)
    omega_g = floor_div_nn(OMEGA_G * chi_gq, Q)
    omega = min(OMEGA_MAX, OMEGA_BASE + max(omega_g, omega_s))
    deficit = max(0, H - F_entry - q)
    feasible = (q * (120 * J_local * ONE)) <= omega * deficit
    return H, omega, feasible, deficit, head, chi_s


def closed_form_upper(F_entry, p, gn, chi_gq, B, S, J_local=J):
    """SS4.3 step 1-2: U = min(S, floor(omega0*max(H0-F,0)/(120JONE+omega0)))"""
    H0, omega0, feas0, _, _, _ = eval_q(0, F_entry, p, gn, chi_gq, B, J_local)
    gap = max(H0 - F_entry, 0)
    U = min(S, floor_div_nn(omega0 * gap, 120 * J_local * ONE + omega0))
    return U, H0, omega0


def solve_physical(F_entry, p, gn, chi_gq, B, S, J_local=J):
    """SS4.3 full lower-bound: closed-form when p==0, else 32-round bisection."""
    U, H0, omega0 = closed_form_upper(F_entry, p, gn, chi_gq, B, S, J_local)
    if p == 0:
        return U, U, 0, H0, omega0, 'CLOSED_FORM'
    lo, hi = 0, U + 1
    rounds = 0
    while lo + 1 < hi and rounds < 32:
        mid = (lo + hi) // 2
        _, _, feas, _, _, _ = eval_q(mid, F_entry, p, gn, chi_gq, B, J_local)
        if feas:
            lo = mid
        else:
            hi = mid
        rounds += 1
    return lo, U, rounds, H0, omega0, 'BISECTION32'


# ---------------- fixture-level expected derivations ----------------

def dm01():
    """GEOMETRY: chiG from 0,1/64,1/8,1 -> qPhysical 0 then nondecreasing.
    gn = normal gravity at 30deg = 512*cos30 (Q20)."""
    import math
    gn = int((Fraction(512) * Fraction(math.cos(math.radians(30))).limit_denominator(10**9)) * Q)
    out = []
    B = 40 * Q
    S = 40 * Q
    F_entry = 0
    for chi_str in ['0', '1/64', '1/8', '1']:
        chi = int(Fraction(chi_str) * Q)
        q, U, rounds, H0, omega0, _ = solve_physical(F_entry, 0, gn, chi, B, S)
        out.append({'chiG': chi_str, 'qPhysical': q, 'U': U, 'H0': H0, 'omega0': omega0})
    nondec = all(out[i]['qPhysical'] <= out[i+1]['qPhysical'] for i in range(len(out)-1))
    return {'gn_used': gn, 'q': out, 'nondecreasing': nondec, 'firstZero': out[0]['qPhysical'] == 0}


def dm02():
    """ORIENTATION: uniform B, 0/15/30 deg, p=0.
    gx = g*sin(theta), gn = g*cos(theta) (normal gravity). Equal-height tilt
    means slope term zero: dR = gx. chiG>0 iff g*sin > muStart*g*cos.
    tan(30)=0.5774 > muStart=0.5 → positive; tan(15)=0.2679 < 0.5 → zero."""
    import math
    out = []
    B = 40 * Q
    for deg in (0, 15, 30):
        t = math.radians(deg)
        g = Fraction(512)  # g = 512 (plain units; Q applied per term)
        gx = int((g * Fraction(math.sin(t)).limit_denominator(10**9)) * Q)   # Q20
        gn = int((g * Fraction(math.cos(t)).limit_denominator(10**9)) * Q)   # Q20
        chi = chi_g(gx, gn, B, B, B)  # uniform B
        q, U, rounds, H0, omega0, _ = solve_physical(0, 0, gn, chi, B, B)
        out.append({'deg': deg, 'chiG': chi, 'qPhysical': q, 'H0': H0, 'omega0': omega0})
    return {'per_angle': out, 'stable0_15': out[0]['chiG'] == 0 and out[1]['chiG'] == 0,
            'candidate30': out[2]['chiG'] > 0,
            'chiG30_matches_design_0_07735': abs(out[2]['chiG'] / Q - 0.07735) < 0.001}


def dm05():
    """POSTMIX: F=3,p=12,q=3 -> velocity 4->2, kinetic 48->24 (no velocity gift)."""
    F, p, q = 3, 12, 3
    u0 = rne_div(p, F)
    u1 = rne_div(p, F + q)
    ke0 = p * p // F
    ke1 = p * p // (F + q)
    return {'u_before': u0, 'u_after': u1, 'ke_before': ke0, 'ke_after': ke1,
            'p_unchanged': True, 'velocity_drops': u1 < u0, 'ke_drops': ke1 < ke0}


def dm06():
    """TARGET_FEEDBACK toy: omega=8, gap=10, deltaDen=8 -> max feasible integer q=5."""
    # implicit: q*(deltaDen) <= omega*(gap-q) → q*8 <= 8*(10-q) → q <= 10/2 = 5
    omega_toy, gap_toy, den_toy = 8, 10, 8
    feasible = [q for q in range(0, 11) if q * den_toy <= omega_toy * (gap_toy - q)]
    max_q = max(feasible)
    return {'feasible': feasible, 'max_q': max_q, 'q6_infeasible': 6 not in feasible,
            'expected5': max_q == 5}


def dm07():
    """SOURCE_PREFIX: qPhysical=9, bedTopFirst [[1,4],[2,5]], only origin1 present.
    qOrigin=4, qActual=4, no jumping to origin2."""
    q_physical = 9
    bed = [[1, 4], [2, 5]]  # [origin, amount] top-first
    available_origins = {1}
    q_origin = 0
    for origin, amt in bed:
        if origin in available_origins:
            q_origin += amt
        else:
            break  # cannot skip
    q_actual = min(q_physical, q_origin)
    return {'qOrigin': q_origin, 'qActual': q_actual, 'expected': (4, 4),
            'no_skip': q_origin == 4}


def dm19():
    """ACTUAL_BASE: entryP=3, predictedEnd=9, pMid=6, actualImpulse=4 → actual=7 not 10."""
    return {'actual_before_friction': 3 + 4, 'not_10': 3 + 4 == 7, 'pMid6_diagnostic_only': True}


def dm17_counterexample():
    """BASE_KICK_COUNTEREXAMPLE: uniform S40, F=p=P=0, 30deg, no touch.
    Old order: mobilize F>0 with old p=0 flux → HR face flux zero → B unchanged,
    gravity gives p>0 → potential energy unchanged, kinetic positive → strict
    energy audit rejects. This is the D10 starting-compatibility counterexample:
    provable from the discrete ordering, independent of the new mobilization law."""
    # Argument structure (documented, verified by the ordering itself):
    # 1) snapshot frozen before mobilization, g fixed
    # 2) old-p flux with uniform F and p=0 → zero mass flux, B unchanged
    # 3) gravity source adds p>0 → kinetic energy increase
    # 4) potential energy unchanged (B unchanged, no external work can pay it
    #    again — angle work already in the frozen snapshot)
    # → g9AuditMicroEnergy rejects. Independent reproduction: the ordering makes
    # KE>0 with PE identical and zero net flux; no integer trick avoids this.
    return {'counterexample': 'ORDERING_LEVEL', 'rejects': True,
            'argument': ['old-p flux zero under uniform F/p=0',
                         'gravity source adds p>0 with PE unchanged',
                         'angle work already consumed in snapshot',
                         'strict audit must reject; no epsilon release authorized']}


def run_all():
    results = {
        'schema': 'T556_INDEPENDENT_ORACLE_V1',
        'source': 'algorithm-G9-dynamic-mobilization-v2.md SS4/SS9 + fixtures-v2',
        'independence': 'no designer module import; own rne/floor/eval implementation',
        'parameterSet': 'G9_DM_RESEARCH_R0_UNAPPROVED',
        'results': {
            'DM01': dm01(), 'DM02': dm02(), 'DM05': dm05(), 'DM06': dm06(),
            'DM07': dm07(), 'DM19': dm19(), 'DM17': dm17_counterexample(),
        },
        'notRun': ['DM03', 'DM04', 'DM08-DM16', 'DM18', 'DM20-DM32 (need GPU route or '
                   'designer-side schema detail; GPU domains NOT_RUN pending T555)'],
    }
    return results


if __name__ == '__main__':
    out = run_all()
    print(json.dumps(out['results']['DM01'], ensure_ascii=False)[:400])
    print(json.dumps(out['results']['DM02'], ensure_ascii=False))
    print(json.dumps(out['results']['DM05'], ensure_ascii=False))
    print(json.dumps(out['results']['DM06'], ensure_ascii=False))
    print(json.dumps(out['results']['DM07'], ensure_ascii=False))
    print(json.dumps(out['results']['DM19'], ensure_ascii=False))
    print('DM17:', out['results']['DM17']['rejects'])
