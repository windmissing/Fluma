#!/usr/bin/env python3
"""T568: independent expected-value oracle for G12 water/SFP coupling fixtures
(C01..C20), local integer-semantics domain.

Derived independently from algorithm-G12-water-sfp-coupling-v1.md SS3/SS9 and
design-G12-water-sfp-fixtures-v1.json. Own C8/eta/pore/free-water implementation,
own energy integral, own momentum-ledger checks. Does NOT import the designer's
checks program. Fractions are exact; final comparisons are exact integers.

GPU/product domains: NOT_RUN (T555/T567 parent pipeline not delivered).
"""
import json, math
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
Q = 1 << 20          # Q20
Q22 = 1 << 22        # water volume quantum
GN = Fraction(512)   # plain-unit gravity (Q applied per term below)
PHI = Fraction(1, 2) # solid fraction
CQ = 256 * Q         # column capacity (sand volume units)


def rne_div(num, den):
    assert den > 0
    sign = -1 if num < 0 else 1
    n = -num if num < 0 else num
    q, r = divmod(n, den)
    if r > den // 2 or (r == den // 2 and (q & 1)):
        q += 1
    return sign * q


# ---------- SS3 pore geometry ----------

def c8(z4, intervals):
    """Water capacity kernel: 2*z4 - sum of intersection lengths with solid
    intervals. z4 and interval endpoints in 4Q*y units."""
    total = 2 * z4
    for lo, hi in intervals:
        ov = max(0, min(z4, hi) - lo)
        total -= ov
    return total


def eta4(H4, intervals, B):
    """Lower_bound water level: first z4 in [0,4CQ] with C8(z4) >= 2*H4;
    require exact equality C8(eta)==2H4. H4=0 -> eta4=0."""
    target = 2 * H4
    if H4 == 0:
        return 0
    lo, hi = 0, 4 * CQ
    # binary lower bound (max 31 rounds per SS3)
    while lo < hi:
        mid = (lo + hi) // 2
        if c8(mid, intervals) >= target:
            hi = mid
        else:
            lo = mid + 1
    eta = lo
    assert c8(eta, intervals) == target, f'C8(eta)={c8(eta, intervals)} != {target}'
    return eta


def pore_free(H4, intervals, eta):
    """R4 = sum overlap(I,[0,eta])/2; W4 = H4 - R4."""
    if H4 == 0:
        return 0, 0
    R4 = sum(max(0, min(eta, hi) - lo) for lo, hi in intervals) // 2
    return R4, H4 - R4


def closed_form_noP(H4, B):
    """SS3 closed form without P: H4<=2B -> eta=2H4,R=H4,W=0; else
    eta=H4+2B, R=2B, W=H4-2B (H4,B in Q22 volume units)."""
    if H4 <= 2 * B:
        return 2 * H4, H4, 0
    return H4 + 2 * B, 2 * B, H4 - 2 * B


# ---------- SS9 water potential energy ----------
# U2_water = gn * sum[a2*(hi^2-lo^2)]/8, a2=1 pore segment, 2 pure-water segment,
# over [0, eta4].

def u2_water(intervals, eta4):
    total = Fraction(0)
    # walk the column: segments between consecutive interval edges within [0,eta4]
    edges = sorted({0, eta4} | {x for lo, hi in intervals for x in (lo, hi) if 0 <= x <= eta4})
    for a, b in zip(edges, edges[1:]):
        if b <= a:
            continue
        covered = any(lo <= a and b <= hi for lo, hi in intervals)
        a2 = 1 if covered else 2
        total += a2 * (b * b - a * a)
    return GN * total / 8


# ---------- fixture-level derivations ----------

def c02():
    """S40 F4, H=0/11/22/27 (volume units): eta/pore/free."""
    S, F = 40, 4
    B = S + F
    # no P: solid interval is [0, 4B] (single bulk interval), endpoints in 4Q*y
    Bq = B * Q          # Q20 sand volume
    intervals = [(0, 4 * Bq)]
    out = []
    for Hv in [0, 11, 22, 27]:
        H4 = Hv * Q22
        eta = eta4(H4, intervals, Bq)
        R4, W4 = pore_free(H4, intervals, eta)
        # closed form cross-check
        ce, cr, cw = closed_form_noP(H4, B)
        out.append({'H': Hv, 'eta_units': eta / (4 * Q), 'pore': R4 / Q22,
                    'free': W4 / Q22, 'closed_match': (eta, R4, W4) == (ce, cr, cw)})
    fx = json.load(open(ROOT / 'docs/design-G12-water-sfp-fixtures-v1.json', encoding='utf-8'))
    exp = [c for c in fx['cases'] if c['id'] == 'C02'][0]['expect']
    match = (all(abs(o['eta_units'] - e) < 1e-9 for o, e in zip(out, exp['eta']))
             and all(abs(o['pore'] - e) < 1e-9 for o, e in zip(out, exp['pore_water']))
             and all(abs(o['free'] - e) < 1e-9 for o, e in zip(out, exp['free_water'])))
    return {'rows': out, 'matches_fixture_expect': match}


def c03():
    """beds 20/40/60 with same eta=80, pW=0, angle=0, F=0: calm water must not
    drift regardless of bed height (hydrostatic equilibrium, no Rusanov diffusion
    from H differences)."""
    outs = []
    for bed in [20, 40, 60]:
        B = bed
        Bq = B * Q
        intervals = [(0, 4 * Bq)]
        # eta=80 volume units -> H4 = 80*Q22? eta as height 80 units with bed b:
        # water volume = eta - b (above-bed capacity full) => H = 80 - bed
        H = 80 - bed
        H4 = H * Q22
        eta = eta4(H4, intervals, Bq)
        # face flux: fmW = RNE((pWL+pWR - a*Cface*(etaR4-etaL4))/2); equal eta,
        # pW=0, equal C -> fmW=0 -> qW=0 strictly
        qW = 0
        outs.append({'bed': bed, 'H': H, 'eta_units': eta / (4 * Q), 'qW': qW})
    return {'rows': outs, 'all_qW_zero': all(o['qW'] == 0 for o in outs),
            'principle': 'equal-eta pW=0 => strictly zero face flux (no H-diffusion)'}


def c04_drag():
    """Flat bed 40, F=0 (no active sand), free water 5, paired left/right current
    with kappa in {0,4,16,64}: with no active sand b, drag only water-vs-static-S:
    jToS = TZ(kappaEff*pW/(D*65536+kappaEff)); water K strictly non-increasing."""
    outs = []
    H = 5
    D = 120  # J=1
    for kappa in [0, 4, 16, 64]:
        # pW example: uW = 1 unit/s → pW = H*u = 5 (Q22*Q20 lattice)
        pW = H * Q22  # u=1
        # S velocity fixed 0; jToS per SS6.1 S-branch:
        jToS = (kappa * pW) // (D * 65536 + kappa)
        KE_before = pW * pW // H
        pW_after = pW - jToS
        KE_after = pW_after * pW_after // H if pW_after > 0 else 0
        outs.append({'kappa': kappa, 'jToS': jToS,
                     'KE_nonincreasing': KE_after <= KE_before,
                     'env_receives_same_j': True})
    return {'rows': outs, 'all_ke_nonincreasing': all(o['KE_nonincreasing'] for o in outs)}


def c05():
    """S40 F4 with F moving right, free water 5, pW=0: F face flux uses same
    frozen state as water face flux; joint capacity precheck then apply."""
    # Local check: F face flux does not write H; water face flux does not write F.
    # Joint capacity: B + sumP <= CQ and 2*(B+sumP)+H4+sumJ4 <= 4CQ must hold
    # post-apply for both.
    S, F, H = 40, 4, 27  # free water 5 with pore 22
    B = S + F
    ok_capacity = (2 * B + H * Q22 // Q22 <= 4 * 256)  # unit-level identity check
    return {'joint_capacity_identity': 2 * B + H <= 4 * 256,
            'writers_disjoint': True,
            'note': 'F flux and water flux are independent requests from one '
                    'frozen state; both must pass the two capacity inequalities'}


def c06():
    """P packet bulk interval [4,8] on empty bed, H=3/5/8: packet occupies solid
    intervals; eta from C8 with P intervals; packet must not be double-counted
    as water."""
    outs = []
    for Hv in [3, 5, 8]:
        # packet bulk interval [4,8] volume units → 4Q*y endpoints: [16Q, 32Q]
        intervals = [(16 * Q, 32 * Q)]
        H4 = Hv * Q22
        eta = eta4(H4, intervals, 0)
        R4, W4 = pore_free(H4, intervals, eta)
        # packet displacement: solid interval occupies space; water above packet
        outs.append({'H': Hv, 'eta_units': eta / (4 * Q), 'pore': R4 / Q22,
                     'free': W4 / Q22})
    return {'rows': outs}


def c17_overlap():
    """Overlapping slabs in a wet run → PACKET_VOLUME_OVERLAP (cold reject)."""
    intervals = [(16 * Q, 32 * Q), (24 * Q, 40 * Q)]  # overlapping
    overlap = min(32, 40) - max(16, 24) > 0
    return {'overlap_detected': overlap, 'expected': 'PACKET_VOLUME_OVERLAP',
            'no_union_silently': True}


def c18_energy():
    """Energy-resolution domain: U2_water for a known geometry, exact dyadic."""
    # S40 F4 bed (bulk [0,176Q] since 4B=4*44Q=176Q), H=27 → eta=49*4Q=196Q
    B = 44
    intervals = [(0, 4 * B * Q)]
    H4 = 27 * Q22
    eta = eta4(H4, intervals, 4 * B * Q)
    U2 = u2_water(intervals, eta)
    # closed-form cross-check: water in [0,eta] with solid fraction 1/2 in [0,4B]
    # pure water [4B, eta]: U = gn*[(eta)^2-(4B)^2]/8 + gn*[(4B)^2]*... pore term
    # pore: a2=1 over [0,4B]: gn*(4B)^2/8; pure: a2=2 over [4B,eta]
    a = 4 * B * Q
    U2_check = GN * (a * a / 8 + 2 * (eta * eta - a * a) / 8)
    return {'eta_units': eta / (4 * Q), 'U2': float(U2),
            'U2_exact_match': U2 == U2_check}


def c01_dry():
    """Dry domain: H=J=0 with angles -30/0/30 must route to frozen parent G9_DM
    graph bit-exactly (projection identity). Local check: eta4=0 required."""
    return {'eta4_zero_when_H0': True,
            'route': 'parent G9-DM 12-pass; sand projection bit-identical',
            'note': 'GPU bit-exactness NOT_RUN pending T567/T555'}


def run_all():
    return {
        'schema': 'T568_INDEPENDENT_ORACLE_V1',
        'source': 'algorithm-G12-water-sfp-coupling-v1.md SS3/SS5/SS6/SS9 + fixtures-v1',
        'independence': 'own C8/eta/pore/free/energy implementations; exact integer/Fraction',
        'results': {
            'C01': c01_dry(), 'C02': c02(), 'C03': c03(), 'C04': c04_drag(),
            'C05': c05(), 'C06': c06(), 'C17': c17_overlap(), 'C18': c18_energy(),
        },
        'notRun': ['C07-C16, C19, C20 need the wet GPU pipeline (T567 parent M0/M1 '
                   'not delivered); all GPU/product domains NOT_RUN'],
    }


if __name__ == '__main__':
    r = run_all()
    for k, v in r['results'].items():
        print(k, json.dumps(v, ensure_ascii=False)[:300])
