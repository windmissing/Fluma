"""P-011 peak protocol v1 — tester-side execution evidence.

Executes, on synthetic fixtures and archived artifacts only:
  1. the frozen tier table and the 0.5 pass comparison (M >= 0.5E, no new tolerance);
  2. the eight design-section-6 counterexamples;
  3. the reachability necessary condition M0 + 300*tau_W >= 0.5E and the exact
     stock identity M(k) = M0 + cumAcceptedLift - cumSettledOrLanded;
  4. the phase-split rules (E_phase=0 -> N/A; independent phase peaks never add;
     the total peak commit records H/L sub-items simultaneously);
  5. the archived RED baseline (no shake event exists in any archive; the
     2026-10-13 baseline recorded shake BLOCKED/NOT_IMPLEMENTED);
  6. the delivered legacy checker's full-sequence max behavior;
  7. the same-ledger dual-calculation attribution chain (window / sampling
     density / numerator-denominator effects listed separately).

This script READS the delivered implementation (test/water_sand_check.py) and
archives. It never modifies them. It is NOT the production peak checker: the
production per-commit sampling and streaming peak belong to the developer.

Outputs:
  test/reports/peak_protocol_v1_execution.json

Usage:
  python test/p011_peak_protocol_v1_execution.py [--output <path>]
"""
from __future__ import annotations
import argparse
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from water_sand_check import evaluate_shake, parse_lines  # noqa: E402  (delivered legacy checker, read-only)

ROOT = Path(__file__).resolve().parents[1]
PROTOCOL_RELATIVE_PATH = 'docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md'
WS_CHECK_JSON = ROOT / 'test/artifacts/P003-P005-baseline-20261013/ws_check.json'
SLIP_LEDGER = ROOT / 'test/artifacts/p011-slip-overshoot-v1/ledger.ndjson'
ARTIFACT_TXT_GLOB = 'test/artifacts/*.txt'

ANCHORS = [(0.90, 10), (0.75, 8), (0.50, 6), (0.35, 4), (0.20, 2)]


def protocol_source_hash():
    """SHA-256 over the exact checked-in protocol bytes, including line endings."""
    path = ROOT / PROTOCOL_RELATIVE_PATH
    try:
        return hashlib.sha256(path.read_bytes()).hexdigest()
    except OSError:
        return None


def tier_score(f):
    """Highest applicable anchor; below 0.20 -> 0 (protocol section 1.3)."""
    for anchor, score in ANCHORS:
        if f >= anchor:
            return score
    return 0


def passes_half(M, E):
    """The 0.5 pass comparison is M >= 0.5*E, with no new downward tolerance."""
    return M >= 0.5 * E


def quantities(E, D, U, P):
    """Same-commit derived quantities; h uses the historical denominator D+U."""
    A = D + U + P
    return dict(E=E, D=D, U=U, P=P, M=U + P, A=A,
                f=(U + P) / E if E > 0 else None,
                h=U / (D + U) if (D + U) > 0 else None,
                l=U / max(A, 1e-9))


def approx(value, want, tol=1e-12):
    return value is not None and abs(value - want) <= tol


# --------------------------------------------------------------------------
# 1. Frozen tier table and pass comparison
# --------------------------------------------------------------------------

def verify_tiers():
    boundary = [
        (0.90, 10), (0.8999999999, 8), (0.75, 8), (0.7499999999, 6),
        (0.50, 6), (0.4999999999, 4), (0.35, 4), (0.3499999999, 2),
        (0.20, 2), (0.1999999999, 0), (0.0, 0), (1.0, 10)]
    rows = [dict(fraction=f, expectedScore=s, actualScore=tier_score(f),
                 matches=tier_score(f) == s) for f, s in boundary]
    comparison = [
        dict(M=500.0, E=1000.0, expected=True, actual=passes_half(500.0, 1000.0)),
        dict(M=500.0 - 1e-12, E=1000.0, expected=False, actual=passes_half(500.0 - 1e-12, 1000.0)),
        dict(M=500.0 + 1e-12, E=1000.0, expected=True, actual=passes_half(500.0 + 1e-12, 1000.0)),
        dict(M=0.0, E=1000.0, expected=False, actual=passes_half(0.0, 1000.0))]
    for row in comparison:
        row['matches'] = row['expected'] == row['actual']
    return dict(boundaryRows=rows, comparisonRows=comparison,
                allMatch=all(r['matches'] for r in rows + comparison))


# --------------------------------------------------------------------------
# 2. Eight counterexamples (design section 6; protocol section 6)
# --------------------------------------------------------------------------

def counterexamples():
    results = []

    # P1 representation undercount: E=1000 D=480 U=320 P=200 -> h=40% l=32% f=52%
    q = quantities(1000, 480, 320, 200)
    results.append(dict(
        id='P1', title='representation undercount: E=1000, D=480, U=320, P=200',
        protocolExpected=dict(h='40%', l='32%', f='52%', newTier=6, newNumericallyPasses=True,
                              oldQuantitiesFail=True),
        actual=dict(h=q['h'], l=q['l'], f=q['f'], score=tier_score(q['f']),
                    passes=passes_half(q['M'], q['E'])),
        verdictMatches=approx(q['h'], 0.40) and approx(q['l'], 0.32) and approx(q['f'], 0.52)
                      and tier_score(q['f']) == 6 and passes_half(q['M'], q['E']),
        note=('Under the total-nondeposited target the old quantities read 40%/32% (false FAIL); '
              'under the old column-suspension semantics they remain correct old results and the '
              'historical report must not be rewritten.')))

    # P2 degradation rewards the old quantity: 200 all DEGRADE P->U
    before = quantities(1000, 480, 320, 200)
    after = quantities(1000, 480, 520, 0)
    results.append(dict(
        id='P2', title='degradation reward: 200 units DEGRADE from P into U',
        protocolExpected=dict(mUnchanged=True, oldChanged=True, newStays='52%'),
        actual=dict(beforeM=before['M'], afterM=after['M'], beforeF=before['f'], afterF=after['f'],
                    beforeH=before['h'], afterH=after['h'], beforeL=before['l'], afterL=after['l']),
        verdictMatches=before['M'] == after['M'] and approx(before['f'], after['f'])
                      and approx(before['f'], 0.52) and not approx(before['h'], after['h']),
        note=('The old conclusion changes (h 40%->52%) while the new quantity is unchanged at 52%; '
              'a changed old value is not evidence of improved lifting.')))

    # P3 adding maxima across commits: k1 U=400/P=0, k2 U=0/P=400
    k1 = quantities(1000, 600, 400, 0)
    k2 = quantities(1000, 600, 0, 400)
    cross_sum = max(k1['U'], k2['U']) / 1000 + max(k1['P'], k2['P']) / 1000
    results.append(dict(
        id='P3', title='adding per-commit maxima: k1 U=400/P=0, k2 U=0/P=400',
        protocolExpected=dict(correctPeak='40% (FAIL, score 4)', crossCommitSum='80% (false PASS)'),
        actual=dict(k1F=k1['f'], k2F=k2['f'], correctPeak=max(k1['f'], k2['f']),
                    correctScore=tier_score(max(k1['f'], k2['f'])),
                    crossCommitSum=cross_sum,
                    correctPasses=passes_half(max(k1['M'], k2['M']), 1000)),
        verdictMatches=approx(max(k1['f'], k2['f']), 0.40) and tier_score(0.40) == 4
                      and approx(cross_sum, 0.80) and not passes_half(400, 1000),
        note='Sum on the same commit first, then take the max; never add maxima from different commits.'))

    # P4 input-platform response lag: first shake commit f=10%, platform 2.4s f=55%
    first = quantities(1000, 900, 100, 0)
    later = quantities(1000, 450, 350, 200)
    results.append(dict(
        id='P4', title='input-platform response lag: first commit 10%, platform 2.4s 55%',
        protocolExpected=dict(inputPeakTimeMethod='10%', candidateB='55%', bScore=6),
        actual=dict(firstF=first['f'], laterF=later['f'], windowPeak=max(first['f'], later['f']),
                    peakIsLaterCommit=later['f'] > first['f'],
                    bScore=tier_score(later['f']), bPasses=passes_half(later['M'], later['E'])),
        verdictMatches=approx(first['f'], 0.10) and approx(later['f'], 0.55)
                      and tier_score(0.55) == 6 and passes_half(550, 1000),
        note=('The input-peak-instant method (10%) and the stock peak (55%) are different estimands; '
              'neither may retro-approve the other.')))

    # P5 full-sequence pollution: rest f=80%, shake max 40%
    rest = quantities(1000, 200, 800, 0)
    shake = quantities(1000, 600, 400, 0)
    results.append(dict(
        id='P5', title='full-sequence pollution: rest segment 80%, shake segment max 40%',
        protocolExpected=dict(fullSequenceMax='80% (false PASS, score 8)', fixedWindow='40% (FAIL, score 4)'),
        actual=dict(restF=rest['f'], shakeF=shake['f'],
                    fullSequenceMax=max(rest['f'], shake['f']),
                    fullSequenceScore=tier_score(max(rest['f'], shake['f'])),
                    fixedWindowPeak=shake['f'], fixedWindowScore=tier_score(shake['f']),
                    fixedWindowPasses=passes_half(400, 1000)),
        verdictMatches=approx(max(rest['f'], shake['f']), 0.80) and tier_score(0.80) == 8
                      and approx(shake['f'], 0.40) and tier_score(0.40) == 4
                      and not passes_half(400, 1000),
        note=('A full-sequence max can falsely PASS; the formal peak only takes the fixed window. '
              'If M0 itself is >=50% the stock peak may numerically pass without proving new '
              'lifting; M0 / net increase / LIFT evidence must then be reported.')))

    # P6 contact stock disguised as motion: E=1000 D=480 U=20 P_state2=500
    q = quantities(1000, 480, 20, 500)
    results.append(dict(
        id='P6', title='contact stock disguised as motion: U=20, P_state2=500',
        protocolExpected=dict(stockPeak='52% (numerically passes, score 6)',
                              misinterpretationRisk='52% must not be read as flying/natural suspension'),
        actual=dict(f=q['f'], score=tier_score(q['f']), passes=passes_half(q['M'], q['E']),
                    state2ShareOfM=500 / q['M'], flyingShareOfM=0.0),
        verdictMatches=approx(q['f'], 0.52) and tier_score(0.52) == 6 and passes_half(520, 1000),
        note=('The stock peak is correct but is not evidence of flight; the state split, lifecycle '
              'and subjective effect are reported separately and the peak alone never releases WS-08.')))

    # P7 250ms sampling misses the peak: true substep peak 52%, samples <=48%
    commit_f = [0.30, 0.45, 0.52, 0.48, 0.40]
    sample_idx = [0, 3, 4]
    sampled = [commit_f[i] for i in sample_idx]
    results.append(dict(
        id='P7', title='250ms sampling misses the peak: true substep peak 52%, samples max 48%',
        protocolExpected=dict(sampledMax='48% (false FAIL, score 4)', perCommitPeak='52% (PASS, score 6)'),
        actual=dict(sampledMax=max(sampled), sampledScore=tier_score(max(sampled)),
                    perCommitPeak=max(commit_f), perCommitScore=tier_score(max(commit_f)),
                    perCommitPasses=passes_half(520, 1000)),
        verdictMatches=approx(max(sampled), 0.48) and tier_score(0.48) == 4
                      and approx(max(commit_f), 0.52) and tier_score(0.52) == 6
                      and passes_half(520, 1000),
        note=('Sparse legacy sampling can falsely FAIL under the new target; the new quantity keeps '
              'every successful commit and never interpolates a peak.')))

    # P8 higher only after stop: shake max 48%, post-off 55%
    shake = quantities(1000, 520, 480, 0)
    post = quantities(1000, 450, 350, 200)
    results.append(dict(
        id='P8', title='higher only after stop: shake segment max 48%, post-off 55%',
        protocolExpected=dict(windowPeak='48% (FAIL, score 4)', extendedWindow='55% (must not be used)'),
        actual=dict(shakeF=shake['f'], postOffF=post['f'],
                    windowPeak=shake['f'], windowScore=tier_score(shake['f']),
                    windowPasses=passes_half(480, 1000),
                    extendedWindowPeak=post['f']),
        verdictMatches=approx(shake['f'], 0.48) and tier_score(0.48) == 4
                      and not passes_half(480, 1000) and approx(post['f'], 0.55),
        note=('The window is never extended into the rest segment to find a PASS; the endpoint '
              'residual is computed independently under the approved endpoint protocol.')))

    return results


# --------------------------------------------------------------------------
# 3. Reachability: exact stock identity and the necessary condition
# --------------------------------------------------------------------------

def build_shake_ledger():
    """Synthetic same-ledger fixture used by the dual-calculation chain.

    E = 1000 (VH0=600, VL0=400), conservation holds on every commit. Rest
    [0,3000ms) at M=470 (h=47%); shake window W=[3000,6000ms) with 50ms
    commits; anchors (t, U, P) linearly interpolated; post [6000,8000)
    decaying. 250ms samples are flagged. The peak commit (t=4150, f=52%)
    splits 240/160 heavy and 80/40 light so that the total peak commit
    records H/L sub-items. The in-window per-commit historical-formula peak
    (44.5% at t=4100) sits strictly between the in-window sampled peak
    (41.5%) and the rest value (47%), so the window, sampling-density and
    numerator/denominator effects are all separately visible.
    """
    anchors = [(3000, 410, 0), (3500, 415, 0), (4000, 410, 0), (4100, 445, 0),
               (4150, 320, 200), (4250, 360, 100), (4400, 430, 0), (4500, 410, 0),
               (4750, 300, 0), (5000, 250, 0), (5250, 220, 0), (5500, 200, 0),
               (5750, 190, 0), (6000, 180, 0)]
    peak_split = dict(UH=240, PH=160, UL=80, PL=40)

    def interp(t):
        for i in range(len(anchors) - 1):
            t0, u0, p0 = anchors[i]
            t1, u1, p1 = anchors[i + 1]
            if t0 <= t <= t1:
                if t1 == t0:
                    return u0, p0
                r = (t - t0) / (t1 - t0)
                return u0 + r * (u1 - u0), p0 + r * (p1 - p0)
        return anchors[-1][1], anchors[-1][2]

    commits, samples = [], []
    t = 0.0
    ordinal = 0
    while t <= 8000.0 + 1e-9:
        if t < 3000.0:
            uh, ul, ph, pl = 282.0, 188.0, 0.0, 0.0
            dt = 0.05
        elif t <= 6000.0:
            u, p = interp(t)
            if abs(t - 4150.0) < 1e-9:
                uh, ph, ul, pl = peak_split['UH'], peak_split['PH'], peak_split['UL'], peak_split['PL']
            else:
                uh, ul = u * 0.6, u * 0.4
                ph, pl = p * 0.6, p * 0.4
            dt = 0.05
        else:
            uh = {6250.0: 90.0, 6500.0: 72.0, 6750.0: 60.0, 7000.0: 54.0,
                  7250.0: 48.0, 7500.0: 45.0, 7750.0: 42.0, 8000.0: 40.0}[t]
            ul = uh * (2.0 / 3.0)
            ph = pl = 0.0
            dt = 0.25
        ordinal += 1
        commits.append(dict(elapsedMs=t, physicsDt=dt, ordinal=ordinal,
                            UH=uh, UL=ul, PH=ph, PL=pl,
                            DH=600.0 - uh - ph, DL=400.0 - ul - pl,
                            inWindow=3000.0 <= t < 6000.0,
                            isSample=(t % 250.0) < 1e-9))
        if commits[-1]['isSample']:
            samples.append(commits[-1])
        t = round(t + (50.0 if t < 6000.0 else 250.0), 6)
    return commits, samples


def analyze_ledger(commits):
    """Per-commit derived quantities on the synthetic ledger."""
    rows = []
    for c in commits:
        U = c['UH'] + c['UL']
        P = c['PH'] + c['PL']
        D = c['DH'] + c['DL']
        E = 1000.0
        rows.append(dict(commit=c, D=D, U=U, P=P, M=U + P, E=E,
                         f=(U + P) / E, h=U / (D + U) if (D + U) > 0 else None,
                         l=U / max(D + U + P, 1e-9),
                         heavyF=(c['UH'] + c['PH']) / 600.0, lightF=(c['UL'] + c['PL']) / 400.0))
    return rows


def reachability():
    # Exact stock identity on the synthetic ledger: M(k) = M0 + cumLift - cumSettle.
    commits, _ = build_shake_ledger()
    rows = analyze_ledger(commits)
    window = [r for r in rows if r['commit']['inWindow']]
    m0_row = [r for r in rows if r['commit']['elapsedMs'] < 3000.0][-1]
    M0 = m0_row['M']
    cum_lift = cum_settle = 0.0
    identity_rows = []
    previous = M0
    for r in window:
        delta = r['M'] - previous
        if delta > 0:
            cum_lift += delta
        else:
            cum_settle += -delta
        expected = M0 + cum_lift - cum_settle
        identity_rows.append(dict(elapsedMs=r['commit']['elapsedMs'], M=r['M'],
                                  expected=expected, residual=r['M'] - expected))
        previous = r['M']
    tau_w = sum(r['commit']['physicsDt'] for r in window)
    peak_row = max(window, key=lambda r: r['f'])
    peak_m = peak_row['M']
    identity_ok = all(abs(row['residual']) <= 1e-9 for row in identity_rows)
    budget_ok = cum_lift <= 300.0 * tau_w + 1e-9
    peak_bound_ok = peak_m <= M0 + cum_lift + 1e-9
    # Frozen necessary-condition arithmetic (protocol section 6).
    frozen = [
        dict(case='M0=0, tau_W=3.0s -> most optimistic peak M <= 900 units; 50% then needs E <= 1800',
             maxEForFiftyPercent=2 * (0.0 + 300.0 * 3.0)),
        dict(case='E=8000 -> required lift integration time (seconds)',
             requiredTau=(0.5 * 8000.0) / 300.0)]
    necessary_ok = True
    for row in frozen:
        if 'maxEForFiftyPercent' in row:
            row['value'] = row['maxEForFiftyPercent']
            row['matches'] = approx(row['maxEForFiftyPercent'], 1800.0)
        else:
            row['value'] = row['requiredTau']
            row['matches'] = approx(row['requiredTau'], 40.0 / 3.0)
        necessary_ok = necessary_ok and row['matches']
    return dict(identity=dict(M0=M0, cumAcceptedLiftInW=cum_lift, cumSettledOrLandedInW=cum_settle,
                              peakM=peak_m, peakBoundHolds=peak_bound_ok,
                              tauW=tau_w, budgetHolds=budget_ok,
                              identityHolds=identity_ok,
                              identityMaxAbsResidual=max(abs(r['residual']) for r in identity_rows)),
                necessary=dict(rows=frozen, allMatch=necessary_ok),
                allMatch=identity_ok and budget_ok and peak_bound_ok and necessary_ok)


# --------------------------------------------------------------------------
# 4. Phase-split rules
# --------------------------------------------------------------------------

def phase_split():
    # E_phase = 0 -> N/A (never EPS-fabricate a ratio).
    na = dict(EH=1000.0, EL=0.0,
              heavyFraction=quantities(1000.0, 400.0, 200.0, 0.0)['f'],
              lightFraction='N/A', lightReason='E_L=0 -> N/A, no EPS ratio')
    # Independent phase peaks never add; the total peak commit records H/L.
    commits, _ = build_shake_ledger()
    rows = analyze_ledger(commits)
    window = [r for r in rows if r['commit']['inWindow']]
    peak = max(window, key=lambda r: r['f'])
    heavy_peak = max(r['heavyF'] for r in window)
    light_peak = max(r['lightF'] for r in window)
    heavy_peak_row = max(window, key=lambda r: r['heavyF'])
    light_peak_row = max(window, key=lambda r: r['lightF'])
    return dict(phaseZeroE=na,
                totalPeak=dict(f=peak['f'], elapsedMs=peak['commit']['elapsedMs'],
                               heavyF=peak['heavyF'], lightF=peak['lightF']),
                independentPhasePeaks=dict(heavyF=heavy_peak, heavyAtElapsedMs=heavy_peak_row['commit']['elapsedMs'],
                                           lightF=light_peak, lightAtElapsedMs=light_peak_row['commit']['elapsedMs'],
                                           sum=heavy_peak + light_peak),
                rulesHold=not approx(heavy_peak + light_peak, peak['f'])
                        and heavy_peak_row['commit']['elapsedMs'] != light_peak_row['commit']['elapsedMs'],
                note=('The independent phase peaks (heavy and light) sum to more than the total peak '
                      'and occur at different commits; only the total peak commit records H/L '
                      'sub-items simultaneously, and phase peaks are never added.'))


# --------------------------------------------------------------------------
# 5. Archived RED baseline
# --------------------------------------------------------------------------

def archived_red():
    out = dict(wsCheckJson=dict(available=WS_CHECK_JSON.exists()))
    if WS_CHECK_JSON.exists():
        data = json.loads(WS_CHECK_JSON.read_text(encoding='utf-8'))
        shake = [item for item in data if item.get('name') == 'shake']
        out['wsCheckJson']['entries'] = [dict(name=i['name'], status=i['status'],
                                              note=i.get('note', '')) for i in data]
        out['wsCheckJson']['shakeEntry'] = shake[0] if shake else None
    # Scan top-level artifact text logs for an actual shake event.
    scanned, shake_events, ws_event_lines, ws_metric_lines = [], 0, 0, 0
    for path in sorted(ROOT.glob(ARTIFACT_TXT_GLOB)):
        try:
            text = path.read_text(encoding='utf-8', errors='replace')
        except OSError:
            continue
        scanned.append(path.name)
        shake_events += text.count('WS_EVENT shake')
        ws_event_lines += text.count('WS_EVENT')
        ws_metric_lines += text.count('WS_METRIC')
    out['artifactScan'] = dict(filesScanned=len(scanned), shakeEventsFound=shake_events,
                               wsEventLinesFound=ws_event_lines, wsMetricLinesFound=ws_metric_lines,
                               fileNames=scanned)
    # Archived SLIP-family ledger: not a WS-08 scenario and lacks v7 fields.
    out['slipLedger'] = dict(available=SLIP_LEDGER.exists())
    if SLIP_LEDGER.exists():
        records = [json.loads(line) for line in SLIP_LEDGER.read_text(encoding='utf-8').splitlines() if line.strip()]
        first = records[0]
        out['slipLedger'].update(
            recordCount=len(records),
            routeId=first['context'].get('observationRouteId'),
            phaseDs=sorted({r['context'].get('phaseD') for r in records}),
            hasParticlesBlock=('particles' in records[0]),
            hasEndpointSampleElapsedMs=('endpointSampleElapsedMs' in records[0]['context']),
            hasEndpointPhaseOccurrenceId=('endpointPhaseOccurrenceId' in records[0]['context']),
            hasIntegratedPhysicsSeconds=('integratedPhysicsSeconds' in records[0]['context']),
            applicableToWs08Peak=False,
            reason=('SLIP-family tilt run without a shake-on input window and without the v7 '
                    'particle/elapsed/occurrence fields; case family is TC-P011-SLIP-03, not TC-WS-08'))
    missing = ['context.particles.massH/massL (+ pMass/pPhase/pState arrays, byState)',
               'context.endpointSampleElapsedMs', 'context.endpointPhaseOccurrenceId',
               'context.integratedPhysicsSeconds',
               'shakeOnEffectiveElapsedMs / shakeOffEffectiveElapsedMs (on/off effective boundaries)',
               'per-substep consumed shake-on input identity',
               'peak window occurrence manifest (caseId, boundaries, coverage evidence)']
    out['newQuantityStatus'] = dict(status='NOT_IMPLEMENTED', missingFields=missing,
                                    observedSampledMax=None,
                                    observedSampledMaxNote=('No archived run contains a shake event, '
                                                            'so even the 250ms lower bound OBSERVED_SAMPLED_MAX '
                                                            'is not available; the 2026-10-13 baseline recorded '
                                                            'shake BLOCKED for exactly this reason.'),
                                    formalReleaseAuthorized=False)
    out['redStatus'] = 'NOT_IMPLEMENTED'
    return out


# --------------------------------------------------------------------------
# 6. Delivered legacy checker behavior (full-sequence max)
# --------------------------------------------------------------------------

def legacy_behavior():
    def metric(f):
        return 'WS_METRIC {"suspensionFraction":%r,"depositedHeavyFraction":0.9,"depositedLightFraction":0.9}' % f

    lines = ([metric(0.80)] * 3 +
             ['WS_EVENT shake {"shakeOn":true,"elapsedMs":3000}'] +
             [metric(0.40), metric(0.42), metric(0.38)] +
             [metric(0.05), metric(0.03)])
    records = parse_lines(lines)
    result = evaluate_shake(records)
    # Under the frozen protocol the fixed shake window only sees 0.40/0.42/0.38.
    window_values = [0.40, 0.42, 0.38]
    return dict(legacyResult=dict(name=result.name, status=result.status,
                                  measured=result.measured, threshold=result.threshold,
                                  evidenceCount=result.evidence_count),
                fixedWindowPeak=max(window_values),
                fixedWindowScore=tier_score(max(window_values)),
                fixedWindowPasses=passes_half(int(max(window_values) * 1000), 1000),
                fullSequenceTakesPreShakeValue=result.measured.get('peakSusp') == 0.80,
                tieredScoringImplemented=False,
                note=('The delivered evaluate_shake() takes max over every WS_METRIC row in the whole '
                      'sequence (pre-shake 0.80 included) and reports a binary PASS/FAIL without the '
                      'TIERED anchors; under the frozen protocol the fixed window peak is 0.42 '
                      '(score 4, FAIL). The residual column belongs to the endpoint protocol.'))


# --------------------------------------------------------------------------
# 7. Same-ledger dual-calculation attribution chain
# --------------------------------------------------------------------------

def dual_calculation():
    commits, samples = build_shake_ledger()
    rows = analyze_ledger(commits)
    window = [r for r in rows if r['commit']['inWindow']]
    in_window_samples = [r for r in rows if r['commit']['isSample'] and r['commit']['inWindow']]
    all_samples = [r for r in rows if r['commit']['isSample']]

    def peak_of(rs, key):
        return max(rs, key=key)

    l1 = peak_of(all_samples, lambda r: r['h'])   # legacyPeakSuspReported: full sequence, samples
    l2 = peak_of(in_window_samples, lambda r: r['h'])  # window effect
    l3 = peak_of(window, lambda r: r['h'])        # density effect (per-commit, historical formula)
    n1 = peak_of(window, lambda r: r['f'])        # new caliber
    live = peak_of(window, lambda r: r['l'])      # legacyPeakLiveFormula
    susp_only = peak_of(window, lambda r: r['U'] / r['E'])  # sameReferenceSuspOnlyPeak
    baseline_row = [r for r in rows if r['commit']['elapsedMs'] < 3000.0][-1]
    observed_sampled_f = max(r['f'] for r in in_window_samples)

    chain = [
        dict(step='L1 legacyPeakSuspReported', definition='full sequence, 250ms samples, h=U/(D+U)',
             value=l1['h'], atElapsedMs=l1['commit']['elapsedMs'], effect='baseline (legacy reported)'),
        dict(step='L2 windowed samples', definition='fixed W, 250ms samples, h=U/(D+U)',
             value=l2['h'], atElapsedMs=l2['commit']['elapsedMs'],
             effect='window effect: %.6f -> %.6f (%.4f pp)' % (l1['h'], l2['h'], 100 * (l2['h'] - l1['h']))),
        dict(step='L3 windowed per-commit', definition='fixed W, every commit, h=U/(D+U)',
             value=l3['h'], atElapsedMs=l3['commit']['elapsedMs'],
             effect='sampling-density effect: %.6f -> %.6f (%.4f pp)' % (l2['h'], l3['h'], 100 * (l3['h'] - l2['h']))),
        dict(step='N1 peakNonDepositedFraction', definition='fixed W, every commit, (U+P)/E',
             value=n1['f'], atElapsedMs=n1['commit']['elapsedMs'],
             effect='numerator/denominator effect: %.6f -> %.6f (%.4f pp)' % (l3['h'], n1['f'], 100 * (n1['f'] - l3['h'])))]
    # Identity checks at the peak commit: f-l = P/E and f-h = P*D/(E*(D+U)).
    fl = n1['f'] - n1['l']
    fh = n1['f'] - n1['h']
    return dict(
        ledger=dict(commits=len(commits), windowCommits=len(window), samples=len(samples),
                    inWindowSamples=len(in_window_samples), E=1000.0, VH0=600.0, VL0=400.0,
                    conservationHolds=all(abs(r['D'] + r['U'] + r['P'] - r['E']) <= 1e-9 for r in rows)),
        attributionChain=chain,
        legacyToNewGapPp=100 * (n1['f'] - l1['h']),
        newQuantity=dict(peakNonDepositedFraction=n1['f'], peakStockMass=n1['M'],
                         peakReferenceMass=n1['E'], peakCommitElapsedMs=n1['commit']['elapsedMs'],
                         peakScore=tier_score(n1['f']), passes=passes_half(n1['M'], n1['E']),
                         heavyF=n1['heavyF'], lightF=n1['lightF'],
                         suspFraction=n1['U'] / n1['E'], airborneFraction=n1['P'] / n1['E'],
                         depositedFraction=n1['D'] / n1['E']),
        legacyPeakSuspReported=l1['h'],
        legacyPeakHistoricalReference=l3['h'],
        legacyPeakLiveFormula=live['l'],
        sameReferenceSuspOnlyPeak=susp_only['U'] / susp_only['E'],
        observedSampledMax=observed_sampled_f,
        observedSampledMaxIsLowerBound=observed_sampled_f <= n1['f'] + 1e-12,
        baselineNonDepositedFraction=baseline_row['M'] / 1000.0,
        peakNetIncreaseFraction=(n1['M'] - baseline_row['M']) / 1000.0,
        identityChecks=dict(fMinusL=fl, expectedPMinusE=n1['P'] / n1['E'],
                            fMinusH=fh, expectedPPMinusEH=n1['P'] * n1['D'] / (n1['E'] * (n1['D'] + n1['U'])),
                            fMinusLMatches=approx(fl, n1['P'] / n1['E']),
                            fMinusHMatches=approx(fh, n1['P'] * n1['D'] / (n1['E'] * (n1['D'] + n1['U'])))),
        note=('Window, sampling-density and numerator/denominator effects are listed separately; '
              'the legacy-to-new gap must not be attributed to physical improvement. The sampled '
              'OBSERVED_SAMPLED_MAX is only a lower bound of the true per-commit peak.'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=Path('test/reports/peak_protocol_v1_execution.json'))
    args = parser.parse_args()
    tiers = verify_tiers()
    counter = counterexamples()
    reach = reachability()
    phases = phase_split()
    red = archived_red()
    legacy = legacy_behavior()
    dual = dual_calculation()
    payload = dict(
        executionId='peak-protocol-v1-execution',
        protocolId='p011-ws08-peak-nondeposited',
        protocolRevision='ws08-peak-nondeposited-v1',
        peakMethodId='peakNonDepositedFraction',
        peakMetricRevision='shake-peak-nondeposited-v1',
        peakAuthority='APPROVED',
        peakProtocolEffective=False,
        formalReleaseAuthorized=False,
        protocolSourcePath=PROTOCOL_RELATIVE_PATH,
        protocolSourceHash=protocol_source_hash(),
        protocolSourceHashAlgorithm='SHA-256',
        note=('Tester-side execution of the frozen peak protocol on synthetic fixtures and archived '
              'artifacts. No device run exists for the peak field revision, so the new quantity on '
              'archived data is NOT_IMPLEMENTED; every candidate number below is synthetic and '
              'labelled as such. This script is not the production peak checker.'),
        tierTable=tiers,
        counterexamples=counter,
        reachability=reach,
        phaseSplit=phases,
        redBaseline=red,
        legacyCheckerBehavior=legacy,
        dualCalculation=dual)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    matched = sum(1 for c in counter if c['verdictMatches'])
    print(f"counterexamples={len(counter)} verdictMatches={matched}")
    for c in counter:
        print(f"  {c['id']}: {'MATCH' if c['verdictMatches'] else 'MISMATCH'} :: {c['title']}")
    print(f"tierTable={'MATCH' if tiers['allMatch'] else 'MISMATCH'}")
    print(f"reachability={'MATCH' if reach['allMatch'] else 'MISMATCH'}")
    print(f"phaseSplit={'MATCH' if phases['rulesHold'] else 'MISMATCH'}")
    print(f"redStatus={red['redStatus']} shakeEventsFound={red['artifactScan']['shakeEventsFound']}")
    print(f"legacyFullSequenceTakesPreShake={legacy['fullSequenceTakesPreShakeValue']}")
    print(f"dualChain={'MATCH' if dual['identityChecks']['fMinusLMatches'] and dual['identityChecks']['fMinusHMatches'] else 'MISMATCH'}")
    print(f"output={args.output}")
    ok = (matched == len(counter) and tiers['allMatch'] and reach['allMatch'] and phases['rulesHold']
          and legacy['fullSequenceTakesPreShakeValue']
          and dual['identityChecks']['fMinusLMatches'] and dual['identityChecks']['fMinusHMatches'])
    return 0 if ok else 1


if __name__ == '__main__':
    raise SystemExit(main())
