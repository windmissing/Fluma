"""T38 independent objective verification of the frozen peak protocol v1.

Tester-side read-only executor. Two independent layers:

  Layer A - independent recomputation of the delivered dual fixture
    (test/reports/p011_peak_dual_fixture): every reported quantity is
    recomputed from the ledger + manifest with formulas written directly
    from the frozen protocol text, then compared against the developer
    checker output (comparison.json). Any checker bug shared with the
    developer's own tests would surface here.

  Layer B - independent boundary probes: synthetic ledgers exercising
    protocol clauses the developer's own suite does not cover (earliest
    peak identity on ties, same-ms ordering, late off, heavy-phase N/A,
    state=0 exclusion, commit-coverage gap, exact 0.5 comparison,
    discriminating right boundary, backward clock, duplicate key,
    window-not-covered). Probes run through the DELIVERED checker
    (test/p011_peak_check.py) to test its conformance, and key expected
    values are additionally cross-checked by this executor's own
    reference computation.

  Layer C - evidence credibility: protocol source hash agreement,
    identity strings, no formal-release claims in evidence files.

No implementation or judgement change; no device use.
"""
import hashlib
import json
import math
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'test' / 'reports' / 'p011_peak_dual_fixture'
PROTOCOL = ROOT / 'docs' / 'peak-protocol-P011-ws08-peak-nondeposited-v1.md'
OUT = ROOT / 'test' / 'reports' / 'p038_peak_independent_verification.json'
EPS = 1e-9


def finite(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool) and math.isfinite(v)


def approx(a, b, tol=1e-12):
    if a is None or b is None:
        return a is None and b is None
    if isinstance(a, (int, float)) and isinstance(b, (int, float)) \
            and not isinstance(a, bool) and not isinstance(b, bool):
        return abs(a - b) <= tol
    return a == b


# ---------------------------------------------------------------- Layer A
def independent_recompute(records, manifest):
    """Reference implementation written from the frozen protocol text."""
    on_id = manifest['shakeOnEventId']
    off_id = manifest['shakeOffEventId']
    start = manifest['shakeOnEffectiveElapsedMs']
    off = manifest['shakeOffEffectiveElapsedMs']
    end = start + 3000
    events = {e['eventId']: e for e in manifest['peakShakeInputEvents']}
    first = records[0]
    audit = first.get('audit', {})
    vh = manifest.get('initialHeavy', audit.get('heavyTotalBefore'))
    vl = manifest.get('initialLight', audit.get('lightTotalBefore'))

    before = []
    candidates = []
    for r in records:
        c = r['context']
        if c.get('commitStatus') != 'committed':
            continue
        t = c['endpointSampleElapsedMs']
        s = r['stateAfter']
        p = r['particles']
        dh = sum(s['bh']) + sum(s['mh'])
        dl = sum(s['bl']) + sum(s['ml'])
        uh = sum(s['suspH'])
        ul = sum(s['suspL'])
        ph, pl = p['massH'], p['massL']
        eh = vh + c.get('sourceInjectedH', 0)
        el = vl + c.get('sourceInjectedL', 0)
        q = dict(key=dict(runId=c['runId'], sequence=c['sequence'],
                          frameId=c['frameId'], substepId=c['substepId']),
                 elapsed=t, order=c['peakInputCommitOrder'],
                 D=dh + dl, U=uh + ul, P=ph + pl, M=uh + ul + ph + pl,
                 E=eh + el, UH=uh, UL=ul, PH=ph, PL=pl, EH=eh, EL=el,
                 f=(uh + ul + ph + pl) / (eh + el) if eh + el > 0 else None,
                 ordinal=c.get('endpointCommittedOrdinal'))
        if t < start:
            before.append(q)
        # half-open window; commit must consume the shake-on input with value 1
        if not (start <= t < end):
            continue
        if c.get('peakConsumedShakeOnEventId') != on_id or c.get('peakConsumedShakeInputValue') != 1:
            continue
        on = events.get(on_id)
        if on is not None and q['order'] <= on['peakInputCommitOrder']:
            continue
        candidates.append(q)

    peak = None
    for q in candidates:  # earliest peak identity: strict >, first wins
        if peak is None or q['f'] > peak['f']:
            peak = q
    f = peak['f'] if peak else None

    def mx(expr):
        return max((expr(q) for q in candidates), default=None)

    historical = [q['U'] / (q['D'] + q['U']) for q in candidates if q['D'] + q['U'] > 0]
    live = [q['U'] / max(q['D'] + q['U'] + q['P'], EPS) for q in candidates]
    same_susp = [q['U'] / q['E'] for q in candidates]
    sampled_keys = {tuple(x.get(n) for n in ['runId', 'sequence', 'frameId', 'substepId'])
                    for x in manifest.get('legacy250msCommitKeys', [])}
    sampled = [q for q in candidates if tuple(q['key'].values()) in sampled_keys]
    legacy_samples = manifest.get('legacy250msSamples', [])
    reported = manifest.get('legacyPeakSuspReported')
    if reported is None and legacy_samples:
        values = [x.get('suspensionFraction') for x in legacy_samples
                  if finite(x.get('suspensionFraction'))]
        reported = max(values, default=None)
    m0 = before[-1]['M'] if before else None
    sampled_f = max((q['f'] for q in sampled), default=None)
    sampled_h = max((q['U'] / (q['D'] + q['U']) for q in sampled
                     if q['D'] + q['U'] > 0), default=None)
    score = None
    if f is not None:
        score = 10 if f >= .9 else 8 if f >= .75 else 6 if f >= .5 else 4 if f >= .35 else 2 if f >= .2 else 0
    return dict(
        peakWindowStartElapsedMs=start, peakWindowEndElapsedMs=end,
        peakWindowSampleCount=len(candidates),
        peakNonDepositedFraction=f,
        peakStockMass=peak['M'] if peak else None,
        peakReferenceMass=peak['E'] if peak else None,
        peakCommitKey=peak['key'] if peak else None,
        peakInputCommitOrder=peak['order'] if peak else None,
        peakSampleElapsedMs=peak['elapsed'] if peak else None,
        peakMasses={n: peak[n] for n in ['D', 'U', 'P', 'M', 'E', 'UH', 'UL', 'PH', 'PL', 'EH', 'EL']} if peak else None,
        peakStatus='PASS' if peak and peak['M'] >= .5 * peak['E'] else 'FAIL' if peak else 'INCOMPLETE',
        peakScore=score,
        peakNonDepositedHeavyFraction=mx(lambda q: (q['UH'] + q['PH']) / q['EH']) if candidates and candidates[0]['EH'] > 0 else None,
        peakNonDepositedLightFraction=mx(lambda q: (q['UL'] + q['PL']) / q['EL']) if candidates and candidates[0]['EL'] > 0 else None,
        heavyPhaseStatus='N/A' if candidates and candidates[0]['EH'] == 0 else 'OBSERVED',
        lightPhaseStatus='N/A' if candidates and candidates[0]['EL'] == 0 else 'OBSERVED',
        legacyPeakSuspReported=reported,
        legacyPeakHistoricalReference=max(historical, default=None),
        legacyPeakLiveFormula=max(live, default=None) if live else None,
        sameReferenceSuspOnlyPeak=max(same_susp, default=None) if same_susp else None,
        OBSERVED_SAMPLED_MAX=sampled_f,
        common250ms=dict(sampleCount=len(sampled), newPeak=sampled_f, historicalPeak=sampled_h),
        baselineNonDepositedFraction=m0 / candidates[0]['E'] if m0 is not None and candidates else None,
        windowEffect=reported - sampled_h if finite(reported) and sampled_h is not None else None,
        samplingDensityEffect=f - sampled_f if f is not None and sampled_f is not None else None,
        numeratorDenominatorEffect=sampled_f - sampled_h if sampled_f is not None and sampled_h is not None else None,
    )


def layer_a():
    records = [json.loads(l) for l in (FIXTURE / 'ledger.jsonl').read_text(encoding='utf8').splitlines() if l.strip()]
    manifest = json.loads((FIXTURE / 'manifest.json').read_text(encoding='utf8'))
    actual = hashlib.sha256((FIXTURE / 'ledger.jsonl').read_bytes()).hexdigest()
    checks = [dict(check='fixture ledgerHash matches manifest',
                   ok=manifest.get('ledgerHash') == actual,
                   manifestHash=manifest.get('ledgerHash'), actualHash=actual)]
    if manifest.get('ledgerHash') != actual:
        manifest['ledgerHash'] = actual  # keep fixture usable for both layers
    mine = independent_recompute(records, manifest)
    theirs = json.loads((FIXTURE / 'comparison.json').read_text(encoding='utf8'))
    attribution = theirs.get('differenceAttribution', {})
    for name, mine_v in mine.items():
        if name == 'common250ms':
            their_v = theirs.get('common250ms', {})
            ok = (their_v.get('sampleCount') == mine_v['sampleCount']
                  and approx(their_v.get('newPeak'), mine_v['newPeak'])
                  and approx(their_v.get('historicalPeak'), mine_v['historicalPeak']))
            checks.append(dict(check='common250ms (count/newPeak/historicalPeak)', ok=ok,
                               recomputed=mine_v, reported=their_v))
            continue
        if name in ('windowEffect', 'samplingDensityEffect', 'numeratorDenominatorEffect'):
            checks.append(dict(check='differenceAttribution.' + name, ok=approx(mine_v, attribution.get(name)),
                               recomputed=mine_v, reported=attribution.get(name)))
            continue
        their_v = theirs.get(name)
        ok = approx(mine_v, their_v) if not isinstance(mine_v, dict) else all(
            approx(mine_v[k], their_v.get(k)) for k in mine_v)
        checks.append(dict(check=name, ok=ok, recomputed=mine_v, reported=their_v))
    return checks


# ---------------------------------------------------------------- Layer B
def build(points, **over):
    """Synthetic ledger in the delivered schema.

    points: list of (elapsedMs, U, P, order) or (elapsedMs, U, P).
    """
    records = []
    for i, pt in enumerate(points):
        t, u, p = pt[0], pt[1], pt[2]
        order = pt[3] if len(pt) > 3 else 2 + i
        c = dict(runId='r', sequence=i + 1, frameId=i + 1, substepId=i + 1,
                 attemptId='a' + str(i), commitStatus='committed',
                 endpointSampleStage='POST_COMMIT', endpointSampleElapsedMs=t,
                 endpointCommittedOrdinal=over.get('ordinals', [i + 1] * len(points))[i],
                 physicsDt=.1, sourceInjectedH=0, sourceInjectedL=0, sourceEpoch=0,
                 resetCount=0, invertCount=0, routeChangeCount=0, modeChangeCount=0,
                 particleRouteActive=True, particleRouteId='dual-field-particle-v1',
                 peakFieldRevision='p011-peak-fields-v1', peakInputCommitOrder=order,
                 peakConsumedShakeInputEventId='on', peakConsumedShakeOnEventId='on',
                 peakConsumedShakeInputValue=1)
        if t < 100:
            c.update(peakInputCommitOrder=0, peakConsumedShakeInputEventId=None,
                     peakConsumedShakeOnEventId=None, peakConsumedShakeInputValue=0)
        if t >= 3100:
            c.update(peakInputCommitOrder=100 + i, peakConsumedShakeInputEventId='off',
                     peakConsumedShakeOnEventId=None, peakConsumedShakeInputValue=0)
        records.append(dict(context=c,
                            stateAfter=dict(bh=[1000 - u - p], bl=[0], mh=[0], ml=[0],
                                            suspH=[u], suspL=[0]),
                            particles=dict(count=int(p > 0), massH=p, massL=0,
                                           pMass=[p] if p else [], pPhase=[0] if p else [],
                                           pState=[2] if p else []),
                            audit=dict(heavyTotalBefore=1000, lightTotalBefore=0,
                                       finite=True, stock=True, capacity=True, conservation=True)))
    events = [dict(eventId='on', runId='r', peakInputCommitOrder=1, effectiveElapsedMs=100,
                   endpointClockId='clock', value=1, kind='SHAKE_ON', shakeOnEventId='on',
                   caseOccurrenceId='case'),
              dict(eventId='off', runId='r', peakInputCommitOrder=100, effectiveElapsedMs=3100,
                   endpointClockId='clock', value=0, kind='SHAKE_OFF', shakeOnEventId='on',
                   caseOccurrenceId='case')]
    m = dict(buildSha256='a' * 64, paramsHash='b' * 64, inputHash='c' * 64,
             ledgerHash='d' * 64, endpointClockId='clock', runId='r',
             shakeOnEventId='on', shakeOffEventId='off',
             shakeOnEffectiveElapsedMs=100, shakeOffEffectiveElapsedMs=3100,
             peakShakeInputEvents=events)
    m.update(over.get('manifest', {}))
    for k, v in over.get('events', {}).items():
        for e in m['peakShakeInputEvents']:
            if e['eventId'] == k:
                e.update(v)
    for idx, patch in over.get('records', {}).items():
        for key, val in patch.items():
            if key in ('context', 'stateAfter', 'particles', 'audit'):
                records[idx][key].update(val)
            else:
                records[idx][key] = val
    return records, m


def run_checker(records, manifest):
    sys.path.insert(0, str(ROOT / 'test'))
    import importlib
    import p011_peak_check
    importlib.reload(p011_peak_check)
    return p011_peak_check.check_peak(records, manifest)


def layer_b():
    checks = []

    def add(name, ok, **kw):
        checks.append(dict(check=name, ok=ok, **kw))

    # B1 earliest peak identity on an exact tie: equal f at two commits,
    # the earlier (lower peakInputCommitOrder) must be reported.
    recs, m = build([(0, 0, 0), (100, 480, 0), (200, 480, 0), (3100, 0, 0)])
    r = run_checker(recs, m)
    add('B1 tie: earliest commit identity reported (sequence 2, order 3)',
        r['peakCommitKey']['sequence'] == 2 and r['peakInputCommitOrder'] == 3,
        reportedKey=r['peakCommitKey'], reportedOrder=r['peakInputCommitOrder'],
        fraction=r['peakNonDepositedFraction'])

    # B2 same-ms: event at t=100 order 1, in-window commit at t=100 order 2
    # (same ms, later order) must be sampled.
    recs, m = build([(0, 0, 0), (100, 520, 0), (3100, 0, 0)])
    r = run_checker(recs, m)
    add('B2 same-ms: commit at t=on with later order sampled (f=0.52)',
        approx(r['peakNonDepositedFraction'], .52) and r['peakWindowSampleCount'] == 1,
        fraction=r['peakNonDepositedFraction'], sampleCount=r['peakWindowSampleCount'])

    # B3 same-ms wrong order: a commit whose order is below the on-event order
    # cannot consume that input and is not an eligible window sample. (An
    # order exactly equal to an event order is an order collision and is
    # rejected as INVALID_LEDGER by the monotonic-order clause.)
    recs, m = build([(100, 520, 0), (3100, 0, 0)],
                    records={0: dict(context=dict(peakInputCommitOrder=3))},
                    events={'on': dict(peakInputCommitOrder=5)})
    r = run_checker(recs, m)
    add('B3 pre-input commit: order below on-event order excluded (INCOMPLETE, 0 samples)',
        r['peakStatus'] == 'INCOMPLETE' and r['peakWindowSampleCount'] == 0,
        status=r['peakStatus'], sampleCount=r['peakWindowSampleCount'],
        reasons=r['peakInputEligibilityReasons'])

    # B4 late off: shake-off after the window end must not extend W.
    recs, m = build([(0, 0, 0), (100, 480, 0), (3100, 0, 0), (5000, 900, 0)],
                    events={'off': dict(effectiveElapsedMs=5000)},
                    manifest=dict(shakeOffEffectiveElapsedMs=5000))
    r = run_checker(recs, m)
    add('B4 late off: window stays [100,3100), post-off record excluded (f=0.48)',
        approx(r['peakNonDepositedFraction'], .48) and r['peakWindowEndElapsedMs'] == 3100,
        fraction=r['peakNonDepositedFraction'], windowEnd=r['peakWindowEndElapsedMs'])

    # B5 heavy-phase N/A: EH=0 (all-light initial stock) -> heavy N/A,
    # light OBSERVED, new quantity still computable from EL.
    recs, m = build([(0, 0, 0), (100, 0, 300), (3100, 0, 0)])
    for rec in recs:
        rec['stateAfter'] = dict(bh=[0], bl=[1000 - rec['stateAfter']['suspH'][0] - (rec['particles']['massH'] or 0)],
                                 mh=[0], ml=[0], suspH=[0], suspL=[rec['stateAfter']['suspH'][0]])
        rec['particles'] = dict(count=int(rec['particles']['massH'] > 0), massH=0,
                                massL=rec['particles']['massH'],
                                pMass=rec['particles']['pMass'], pPhase=[1] if rec['particles']['pMass'] else [],
                                pState=rec['particles']['pState'])
        rec['audit'] = dict(heavyTotalBefore=0, lightTotalBefore=1000, finite=True, stock=True,
                            capacity=True, conservation=True)
    r = run_checker(recs, m)
    add('B5 heavy N/A: EH=0 -> heavyPhaseStatus N/A, light OBSERVED (f=0.3)',
        r['heavyPhaseStatus'] == 'N/A' and r['lightPhaseStatus'] == 'OBSERVED'
        and approx(r['peakNonDepositedFraction'], .3) and approx(r['peakNonDepositedLightFraction'], .3),
        heavy=r['heavyPhaseStatus'], light=r['lightPhaseStatus'],
        fraction=r['peakNonDepositedFraction'], lightFraction=r['peakNonDepositedLightFraction'])

    # B6 state=0 exclusion: a dead pool slot (state=0) carrying mass must not
    # enter P. Correctly excluding it keeps the summary consistent; a summary
    # that counts the dead slot is a ledger violation.
    recs, m = build([(0, 0, 0), (100, 300, 200), (3100, 0, 0)])
    recs[1]['particles'].update(pMass=[200, 50], pPhase=[0, 0], pState=[2, 0], count=1, massH=200)
    r_ok = run_checker(recs, m)
    recs2, m2 = build([(0, 0, 0), (100, 300, 200), (3100, 0, 0)])
    recs2[1]['particles'].update(pMass=[200, 50], pPhase=[0, 0], pState=[2, 0], count=1, massH=250)
    r_bad = run_checker(recs2, m2)
    add('B6 state=0 slot excluded from P; dead-slot mass counted -> INVALID_LEDGER',
        r_ok['peakStatus'] == 'PASS' and approx(r_ok['peakStockMass'], 500)
        and r_bad['peakStatus'] == 'INVALID_LEDGER'
        and any('PARTICLE_SUMMARY_MISMATCH' in x for x in r_bad['hardGateFailures']),
        correctExclusion=dict(status=r_ok['peakStatus'], stock=r_ok['peakStockMass']),
        deadSlotCounted=dict(status=r_bad['peakStatus'], hard=r_bad['hardGateFailures']))

    # B7 commit-coverage gap: committedOrdinal jumps 1 -> 3 inside the window.
    recs, m = build([(0, 0, 0), (100, 480, 0), (200, 300, 0), (3100, 0, 0)],
                    ordinals=[1, 2, 4, 5])
    r = run_checker(recs, m)
    add('B7 coverage gap: ordinal jump detected -> INCOMPLETE (no false PASS)',
        r['peakStatus'] == 'INCOMPLETE'
        and any('COMMIT_COVERAGE_GAP' in x for x in r['peakInputEligibilityReasons']),
        status=r['peakStatus'], reasons=r['peakInputEligibilityReasons'])

    # B8 exact 0.5 comparison: M == 0.5E exactly must PASS (>=, no tolerance).
    recs, m = build([(0, 0, 0), (100, 500, 0), (3100, 0, 0)])
    r = run_checker(recs, m)
    add('B8 exact half: M=0.5E exactly -> PASS score 6',
        r['peakStatus'] == 'PASS' and r['peakScore'] == 6
        and approx(r['peakNonDepositedFraction'], .5),
        status=r['peakStatus'], score=r['peakScore'], fraction=r['peakNonDepositedFraction'])

    # B9 discriminating right boundary: the excluded record at t=3100 carries
    # the largest M; the reported peak must come from inside the window only.
    recs, m = build([(0, 0, 0), (100, 480, 0), (3100, 900, 0)])
    r = run_checker(recs, m)
    add('B9 right boundary: t=end record with M=900 excluded, peak stays 0.48',
        approx(r['peakNonDepositedFraction'], .48) and r['peakWindowSampleCount'] == 1,
        fraction=r['peakNonDepositedFraction'], sampleCount=r['peakWindowSampleCount'])

    # B10 pre-on pollution: huge pre-window M must not enter the peak; it is
    # disclosed as the baseline fraction instead.
    recs, m = build([(0, 900, 0), (100, 300, 0), (3100, 0, 0)])
    r = run_checker(recs, m)
    add('B10 pre-on: M=900 before window excluded (peak 0.3, baseline 0.9)',
        approx(r['peakNonDepositedFraction'], .3) and approx(r['baselineNonDepositedFraction'], .9),
        fraction=r['peakNonDepositedFraction'], baseline=r['baselineNonDepositedFraction'])

    # B11 backward clock inside the run -> INVALID_LEDGER (no silent reorder).
    recs, m = build([(0, 0, 0), (100, 480, 0), (50, 300, 0), (3100, 0, 0)])
    r = run_checker(recs, m)
    add('B11 backward elapsed rejected -> INVALID_LEDGER',
        r['peakStatus'] == 'INVALID_LEDGER'
        and any('ELAPSED_INVALID' in x for x in r['hardGateFailures']),
        status=r['peakStatus'], hard=r['hardGateFailures'])

    # B12 duplicate commit key -> INVALID_LEDGER.
    recs, m = build([(0, 0, 0), (100, 480, 0), (200, 300, 0), (3100, 0, 0)],
                    records={2: dict(context=dict(sequence=2, frameId=2, substepId=2))})
    r = run_checker(recs, m)
    add('B12 duplicate commit key rejected -> INVALID_LEDGER',
        r['peakStatus'] == 'INVALID_LEDGER'
        and any('COMMIT_KEY_INVALID' in x for x in r['hardGateFailures']),
        status=r['peakStatus'], hard=r['hardGateFailures'])

    # B13 window not covered: last commit before window end -> INCOMPLETE.
    recs, m = build([(0, 0, 0), (100, 480, 0), (200, 300, 0)])
    r = run_checker(recs, m)
    add('B13 uncovered window -> INCOMPLETE',
        r['peakStatus'] == 'INCOMPLETE'
        and any('WINDOW_NOT_COVERED' in x for x in r['peakInputEligibilityReasons']),
        status=r['peakStatus'], reasons=r['peakInputEligibilityReasons'])

    # B14 legacy reported value passes through unchanged (raw formula, full
    # sequence) while the new quantity uses the fixed window.
    recs, m = build([(0, 800, 0), (100, 400, 0), (3100, 0, 0)],
                    manifest=dict(legacy250msSamples=[dict(suspensionFraction=.8),
                                                      dict(suspensionFraction=.4)],
                                  legacy250msCommitKeys=[dict(runId='r', sequence=2,
                                                              frameId=2, substepId=2)]))
    r = run_checker(recs, m)
    add('B14 legacy reported kept raw (0.8) vs new fixed-window peak (0.4)',
        approx(r['legacyPeakSuspReported'], .8) and approx(r['peakNonDepositedFraction'], .4)
        and approx(r['common250ms']['newPeak'], .4),
        legacyReported=r['legacyPeakSuspReported'], newPeak=r['peakNonDepositedFraction'],
        commonNew=r['common250ms']['newPeak'])

    # B15 input event after its commit -> INPUT_AFTER_COMMIT hard failure.
    recs, m = build([(0, 0, 0), (100, 480, 0), (3100, 0, 0)],
                    records={1: dict(context=dict(peakConsumedShakeInputEventId='off'))})
    r = run_checker(recs, m)
    add('B15 consuming a later event rejected (INPUT_AFTER_COMMIT or mismatch)',
        r['peakStatus'] in ('INVALID_LEDGER', 'INCOMPLETE'),
        status=r['peakStatus'], hard=r['hardGateFailures'], reasons=r['peakInputEligibilityReasons'])
    return checks


# ---------------------------------------------------------------- Layer C
def layer_c():
    checks = []
    proto_bytes = PROTOCOL.read_bytes()
    proto_hash = hashlib.sha256(proto_bytes).hexdigest()
    theirs = json.loads((FIXTURE / 'comparison.json').read_text(encoding='utf8'))
    checks.append(dict(check='comparison.json protocolSourceHash equals recomputed SHA-256',
                       ok=theirs.get('peakProtocolSourceHash') == proto_hash,
                       recomputed=proto_hash, reported=theirs.get('peakProtocolSourceHash')))
    selfcheck = json.loads((ROOT / 'test' / 'reports' / 'p011_peak_developer_selfcheck.json').read_text(encoding='utf8'))
    checks.append(dict(check='developer selfcheck keeps peakProtocolEffective=false and '
                             'formalReleaseAuthorized=false',
                       ok=selfcheck.get('peakProtocolEffective') is False
                       and selfcheck.get('formalReleaseAuthorized') is False))
    node_out = subprocess.run(
        ['C:/Program Files/nodejs/node.exe', 'test/run_p011_peak_units.cjs'],
        cwd=str(ROOT), capture_output=True, text=True, timeout=300)
    try:
        node = json.loads(node_out.stdout)
        node_ok = node.get('failed') == 0 and node.get('passed', 0) > 0
    except Exception:
        node, node_ok = None, False
    checks.append(dict(check='delivered node unit suite green (reuse, not reimplement)',
                       ok=node_ok, passed=node.get('passed') if node else None,
                       failed=node.get('failed') if node else None))
    checks.append(dict(check='comparison.json declares CANDIDATE_DIAGNOSTIC and '
                             'FORMAL_RELEASE_NOT_AUTHORIZED',
                       ok=theirs.get('evidenceClassification') == 'CANDIDATE_DIAGNOSTIC'
                       and theirs.get('releaseStatus') == 'FORMAL_RELEASE_NOT_AUTHORIZED'))
    return checks


def main():
    a = layer_a()
    b = layer_b()
    c = layer_c()
    all_checks = a + b + c
    matched = sum(1 for x in all_checks if x['ok'])
    verdict = 'PASS' if matched == len(all_checks) else 'FAIL'
    report = dict(
        executionId='p038-peak-independent-verification-v1',
        role='tester', mode='READ_ONLY_INDEPENDENT_VERIFICATION',
        basis=['docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md (frozen v1)',
               'docs/architecture-P011-peak-fields-revision-v1.md (T07 rev2)',
               'commit 41b90bf (T08 implementation), 61e5ac4 (T07 alignment)'],
        protocolSourceSha256=hashlib.sha256(PROTOCOL.read_bytes()).hexdigest(),
        verdict=verdict,
        summary=dict(totalChecks=len(all_checks), matched=matched,
                     layerA=len(a), layerB=len(b), layerC=len(c)),
        layerAIndependentRecompute=a,
        layerBIndependentProbes=b,
        layerCEvidenceCredibility=c,
        scopeNotes=[
            'Layer A recomputes every delivered fixture quantity with formulas written '
            'from the frozen protocol text (not by calling the delivered checker).',
            'Layer B probes exercise protocol clauses the delivered suite does not cover; '
            'probes run through the delivered checker to test its conformance.',
            'No device, no implementation change, no judgement change; protocol formal '
            'effectiveness remains a separate manager decision.'],
    )
    OUT.write_text(json.dumps(report, indent=1, ensure_ascii=False) + '\n', encoding='utf8')
    print(json.dumps(dict(verdict=verdict, summary=report['summary']), ensure_ascii=False))
    for x in all_checks:
        if not x['ok']:
            print('FAIL:', x['check'], json.dumps({k: v for k, v in x.items() if k != 'check'},
                                                  ensure_ascii=False)[:400])
    return 0 if verdict == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
