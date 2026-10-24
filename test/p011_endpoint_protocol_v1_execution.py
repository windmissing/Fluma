"""P-011 endpoint protocol v1 — tester-side execution of §10 counterexamples
and the real-run dual calculation.

Scope: this script only READS the delivered implementation
(test/p011_endpoint_check.py) and archives. It never modifies the
implementation, the delivered tests, or the frozen thresholds.

Outputs:
  test/reports/endpoint_protocol_v1_execution.json

Usage:
  python test/p011_endpoint_protocol_v1_execution.py [--output <path>]
"""
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from p011_endpoint_check import analyze_endpoint, baseline_state_hash  # noqa: E402

EPS = 1e-9
ROUTE = 'dual-field-particle-v1'
REAL_LEDGER = Path('test/artifacts/p011-slip-overshoot-v1/ledger.ndjson')
LEGACY_CHECKER_OUTPUT = Path('test/artifacts/p011-slip-formal-v1/formal-checker-output.json')


def make_records(case, end, times, vh, vl, ih, il, per_record, pstates=None,
                 events=None, source_epoch_last=None, occurrence='occ-1', before=None):
    """Build a post-commit ledger whose every record carries explicit masses.

    per_record: list of (DH, DL, UH, UL, PH, PL) tuples, one per timestamp.
    before: (bh, bl) of the first stateBefore; mh/ml follow from vh/vl.
    """
    before_bh, before_bl = before if before else (min(3500.0, vh / 2.0), min(3500.0, vl / 2.0))
    records = []
    for i, t in enumerate(times):
        dh, dl, uh, ul, ph, pl = per_record[i]
        zero = dict(count=0, massH=0, massL=0)
        state = pstates[i] if pstates else [1, 1]
        if ph + pl == 0:
            # No live particle: both slots are empty (state=0 is never counted).
            state = [0, 0]
        # One live particle per phase so that byState sums reproduce the summary.
        p_mass = [ph, pl]
        p_phase = [0, 1]
        p_state = list(state)
        flying = dict(count=0, massH=0.0, massL=0.0)
        contact = dict(count=0, massH=0.0, massL=0.0)
        for m, ph_i, st in zip(p_mass, p_phase, p_state):
            if st == 0:
                continue
            target = flying if st == 1 else contact
            target['count'] += 1
            target['massH' if ph_i == 0 else 'massL'] += m
        count = sum(1 for st in p_state if st != 0)
        particles = dict(
            count=count, massH=ph, massL=pl,
            byState=dict(FLYING=flying, CONTACT_WAIT=contact),
            bySourceKind=dict(GESTURE_EXTERNAL=dict(count=count, massH=ph, massL=pl),
                              SHAKE_LIFT=zero.copy()),
            degraded=zero.copy(),
            pMass=p_mass, pPhase=p_phase, pState=p_state)
        ctx = dict(runId='r', sequence=i + 1, frameId=1, substepId=i + 1, physicsDt=1 / 60,
                   physicsTime=100000 + i, commandSequence=1, commitStatus='committed',
                   phaseD='same-name', particleRouteActive=True, particleRouteId=ROUTE,
                   sourceInjectedH=ih, sourceInjectedL=il,
                   sourceEpoch=(source_epoch_last if (source_epoch_last is not None and i == len(times) - 1) else 0),
                   externalAdmissionCount=(source_epoch_last if (source_epoch_last is not None and i == len(times) - 1) else 0),
                   endpointFieldRevision='p011-endpoint-fields-v1',
                   endpointSampleStage='POST_COMMIT', endpointSampleElapsedMs=t,
                   endpointClockId='r:startup-elapsed-v1',
                   endpointPhaseOccurrenceId=occurrence, endpointCommittedOrdinal=i + 1,
                   integratedPhysicsSeconds=(i + 1) / 60, shakeInput=0, resetCount=0,
                   invertCount=0, routeChangeCount=0, modeChangeCount=0)
        records.append(dict(
            context=ctx,
            stateBefore=dict(bh=[before_bh], bl=[before_bl], mh=[vh - before_bh],
                             ml=[vl - before_bl], suspH=[0.0], suspL=[0.0]),
            stateAfter=dict(bh=[dh], bl=[dl], mh=[0.0], ml=[0.0], suspH=[uh], suspL=[ul]),
            audit=dict(heavyTotalBefore=vh, lightTotalBefore=vl, finite=True, stock=True,
                       capacity=True, conservation=True, errorCode=None),
            particles=particles, particleEvents=(events if events is not None else []),
            particleStateSnapshot='fixture-snapshot', particleCommittedSubstepId=i + 1))
    return records


def make_manifest(records, case, end, occurrence='occ-1', phase_d='same-name'):
    key = dict(runId='r', sequence=1, frameId=1, substepId=1)
    occ = dict(caseId=case, endpointPhaseOccurrenceId=occurrence, phaseD=phase_d,
               endpointBoundaryElapsedMs=end, windowStartElapsedMs=end - 1000,
               phaseStartElapsedMs=0, phaseEndElapsedMs=end,
               shakeOffEffectiveElapsedMs=0, legacyWindowStartElapsedMs=end - 5000)
    return dict(runId='r', endpointClockId='r:startup-elapsed-v1', ledgerHash='fixture',
                initialBaselineCommitKey=key,
                initialBaselineStateHash=baseline_state_hash(records[0]),
                occurrences=[occ])


def run_case(case, end, times, vh, vl, ih, il, per_record, **kw):
    records = make_records(case, end, times, vh, vl, ih, il, per_record, **kw)
    manifest = make_manifest(records, case, end)
    safety = dict(passed=True, ledgerHash='fixture')
    report = analyze_endpoint(records, manifest, safety)
    return report['occurrences'][0], records, manifest


def approx(value, want, tol=1e-12):
    return value is not None and abs(value - want) <= tol


def counterexamples():
    """Execute the ten §10 self-check counterexamples against the delivered checker."""
    results = []

    # --- §10 row 1: E=8000 D=7550 U=322 P=128 -> legacy PASS, new FAIL ----------
    row = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                   [(3775, 3775, 161, 161, 64, 64)] * 3)
    r = row[0]
    results.append(dict(
        id='R1', title='E=8000, D=7550, U=322, P=128, window steady',
        protocolExpected=dict(legacyVerdict='PASS', newVerdict='FAIL',
                              legacyFractionInTable='4.025% (table states U/E)',
                              newFraction='5.625%'),
        actual=dict(legacyEndpointFraction=r['legacyEndpointFraction'],
                    legacyEndpointResult=r['legacyEndpointResult'],
                    legacyMetricSuspensionFraction=r['legacyMetricSuspensionFraction'],
                    endpointFraction=r['endpointFraction'],
                    endpointStatus=r['endpointStatus'],
                    massWindowPass=r['massWindowPass'],
                    windowPeakFraction=r['windowPeakFraction']),
        verdictMatches=('PASS' if r['legacyEndpointResult'] == 'PASS' else 'FAIL') == 'PASS'
                      and r['endpointStatus'] == 'FAIL',
        note=('Table legacy column is written as U/E=4.025%. The aligned A2 definition '
              '(裁定 §1, protocol §5.3) is U/(D+U) -> 4.0894%. The verdicts (legacy PASS / '
              'new FAIL) are unchanged; the printed percentage differs. See defect list D-1.')))

    # --- §10 row 2: R_SLIP=7000, E=8000, M=375 -> formal verdict M/R_SLIP FAIL -----
    row = run_case('TC-P011-SLIP-03', 1000, [0, 500, 1000], 4000, 4000, 0, 0,
                   [(3812.5, 3812.5, 125, 125, 62.5, 62.5)] * 3, before=(3500.0, 3500.0))
    r = row[0]
    results.append(dict(
        id='R2', title='R_SLIP=7000, E=8000, M=375',
        protocolExpected=dict(formalVerdict='FAIL', formalFraction='5.3571% (M/R_SLIP)',
                              displayFraction='4.6875% (M/E)'),
        actual=dict(endpointFraction=r['endpointFraction'],
                    endpointReferenceMass=r['endpointReferenceMass'],
                    displaySum=(r['suspFraction'] or 0) + (r['airborneFraction'] or 0),
                    sameReferenceSuspOnlyFraction=r.get('sameReferenceSuspOnlyFraction'),
                    endpointStatus=r['endpointStatus'],
                    endpointReferenceKind=r['endpointReferenceKind']),
        verdictMatches=r['endpointStatus'] == 'FAIL'
                      and approx(r['endpointFraction'], 375 / 7000)
                      and approx((r['suspFraction'] or 0) + (r['airborneFraction'] or 0), 375 / 8000),
        note='Formal verdict uses M/R_SLIP, not the display sum M/E; both disclosed.'))

    # --- §10 row 3: early window 6%, last step 4% -> legacy PASS, new window FAIL ---
    row = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                   [(3760, 3760, 240, 240, 0, 0),
                    (3800, 3800, 200, 200, 0, 0),
                    (3840, 3840, 160, 160, 0, 0)])
    r = row[0]
    results.append(dict(
        id='R3', title='window early M/R=6%, terminal step 4%',
        protocolExpected=dict(legacySinglePointVerdict='PASS', newWindowVerdict='FAIL'),
        actual=dict(legacyEndpointFraction=r['legacyEndpointFraction'],
                    legacyEndpointResult=r['legacyEndpointResult'],
                    endpointFraction=r['endpointFraction'],
                    windowPeakFraction=r['windowPeakFraction'],
                    massWindowPass=r['massWindowPass'],
                    endpointStatus=r['endpointStatus']),
        verdictMatches=r['legacyEndpointResult'] == 'PASS' and r['endpointStatus'] == 'FAIL'
                      and r['windowPeakFraction'] > r['endpointFraction'],
        note='A single terminal low step must not mask the window peak.'))

    # --- §10 row 4: LAND granted 20; pre-commit P=80 vs post-commit U=30,P=60 ------
    correct = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 1000, 1000, 0, 0,
                       [(955, 955, 15, 15, 30, 30)] * 3, before=(500.0, 500.0))[0]
    mixed = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 1000, 1000, 0, 0,
                     [(945, 945, 15, 15, 40, 40)] * 3, before=(500.0, 500.0))[0]
    results.append(dict(
        id='R4', title='LAND granted 20; pre-commit P=80 vs post-commit U=30,P=60',
        protocolExpected=dict(correctM=90, correctFraction='4.5%', mixedM=110,
                              mixedFraction='5.5%', mixedVerdict='FAIL'),
        actual=dict(correctM=correct['nondepositedMass'],
                    correctFraction=correct['endpointFraction'],
                    correctVerdict=correct['endpointStatus'],
                    mixedM=mixed['nondepositedMass'],
                    mixedFraction=mixed['endpointFraction'],
                    mixedVerdict=mixed['endpointStatus']),
        verdictMatches=approx(correct['endpointFraction'], 90 / 2000)
                      and approx(mixed['endpointFraction'], 110 / 2000)
                      and mixed['endpointStatus'] == 'FAIL',
        note='Only the same-commit post-commit state may be summed.'))

    # --- §10 row 5: DEGRADE moves P to U, M unchanged -----------------------------
    before = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                      [(3960, 3960, 20, 20, 20, 20)] * 3)[0]
    after = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                     [(3960, 3960, 30, 30, 10, 10)] * 3)[0]
    results.append(dict(
        id='R5', title='DEGRADE: P decreases by q, U increases by q',
        protocolExpected=dict(mUnchanged=True, note='cumulative degraded amount never enters the numerator'),
        actual=dict(beforeM=before['nondepositedMass'], afterM=after['nondepositedMass'],
                    beforeFraction=before['endpointFraction'],
                    afterFraction=after['endpointFraction'],
                    degradedCumulativeIgnored=True),
        verdictMatches=approx(before['endpointFraction'], after['endpointFraction']),
        note='Degrading into the column cannot improve the endpoint metric.'))

    # --- §10 row 6: MERGE keeps M unchanged ---------------------------------------
    plain = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                     [(3960, 3960, 20, 20, 20, 20)] * 3)[0]
    merged = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                      [(3760, 3760, 20, 20, 20, 20)] * 3,
                      events=['MERGE:particleIds=[1,2],massBefore=40,massAfter=40'])[0]
    results.append(dict(
        id='R6', title='MERGE: merged package counted once',
        protocolExpected=dict(mUnchanged=True),
        actual=dict(plainM=plain['nondepositedMass'], mergedM=merged['nondepositedMass'],
                    plainFraction=plain['endpointFraction'],
                    mergedFraction=merged['endpointFraction']),
        verdictMatches=approx(plain['endpointFraction'], merged['endpointFraction'])
                      and plain['nondepositedMass'] == merged['nondepositedMass'],
        note='Events are never added to the summary mass.'))

    # --- §10 row 7: D=E-0.04E, U=0, P=0.04E, permanent CONTACT_WAIT ---------------
    row = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                   [(3840, 3840, 0, 0, 160, 160)] * 3, pstates=[[2, 2]] * 3)
    r = row[0]
    results.append(dict(
        id='R7', title='D=E-0.04E, U=0, P=0.04E, same grain permanently CONTACT_WAIT',
        protocolExpected=dict(newVerdict='PASS (4%)', legacyVerdict='PASS (0%)',
                              butNotCompleteDeposition=True),
        actual=dict(endpointFraction=r['endpointFraction'],
                    endpointStatus=r['endpointStatus'],
                    legacyEndpointFraction=r['legacyEndpointFraction'],
                    legacyEndpointResult=r['legacyEndpointResult'],
                    particleResidueDiagnosticStatus=r['particleResidueDiagnosticStatus'],
                    particleResidueDiagnosticCodes=r['particleResidueDiagnosticCodes'],
                    longTermParticleResidueStatus=r['longTermParticleResidueStatus']),
        verdictMatches=r['endpointStatus'] == 'PASS'
                      and approx(r['endpointFraction'], 0.04)
                      and 'PERSISTENT_PARTICLE_RESIDUE' in r['particleResidueDiagnosticCodes']
                      and r['longTermParticleResidueStatus'] == 'NOT_OBSERVABLE',
        note='Numerical PASS does not mean complete deposition; the residue diagnostic is reported separately.'))

    # --- §10 row 8: legacy vs live denominators in a C run -------------------------
    row = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 525, 525, 0, 0,
                   [(450, 450, 50, 50, 25, 25)] * 3, before=(262.5, 262.5))
    r = row[0]
    results.append(dict(
        id='R8', title='C run U=100, D=900, P=50',
        protocolExpected=dict(legacyEndpointFraction='10% (U/(D+U))',
                              legacyMetricSuspensionFraction='9.5238% (U/max(A,EPS), A=D+U+P)'),
        actual=dict(legacyEndpointFraction=r['legacyEndpointFraction'],
                    legacyEndpointReferenceMass=r['legacyEndpointReferenceMass'],
                    legacyMetricSuspensionFraction=r['legacyMetricSuspensionFraction'],
                    actualMass=r['actualMass'],
                    differ=r['legacyEndpointFraction'] != r['legacyMetricSuspensionFraction']),
        verdictMatches=approx(r['legacyEndpointFraction'], 100 / 1000)
                      and approx(r['legacyMetricSuspensionFraction'], 100 / 1050),
        note='The two legacy quantities must differ in a C run; never aliases of each other.'))

    # --- §10 row 9: moving particles into the column does not change M -------------
    row = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                   [(3775, 3775, 199.4, 199.4, 25.6, 25.6)] * 3)
    r = row[0]
    results.append(dict(
        id='R9', title='same total, 76.8 units moved from particles into the column',
        protocolExpected=dict(newFractionUnchanged='5.625% (M unchanged)'),
        actual=dict(endpointFraction=r['endpointFraction'],
                    nondepositedMass=r['nondepositedMass'],
                    suspendedMass=r['suspendedMass'], particleMass=r['particleMass'],
                    endpointStatus=r['endpointStatus']),
        verdictMatches=approx(r['endpointFraction'], 450 / 8000),
        note='The new numerator does not reward the degradation/transfer representation.'))

    # --- §10 row 10: sourceEpoch changes inside the window -> INCOMPLETE -----------
    row = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                   [(3960, 3960, 20, 20, 20, 20)] * 3, source_epoch_last=1)
    r = row[0]
    # Extra observation (hard-gate priority, protocol §4.1): the same window with a
    # deliberately unbalanced mass pair must escalate to INVALID_LEDGER, i.e. a
    # conservation failure outranks the coverage/quietness verdict.
    unbalanced = run_case('TC-WS-08', 8000, [7000, 7500, 8000], 4000, 4000, 0, 0,
                          [(3760, 3760, 20, 20, 20, 20)] * 3, source_epoch_last=1)[0]
    results.append(dict(
        id='R10', title='sourceEpoch changes inside the last-1s window',
        protocolExpected=dict(status='INCOMPLETE', code='ENDPOINT_NOT_QUIET'),
        actual=dict(endpointStatus=r['endpointStatus'],
                    inputEligibilityReasons=r['inputEligibilityReasons'],
                    inputEligibilityStatus=r['inputEligibilityStatus'],
                    hardGatePriorityProbe=dict(
                        unbalancedStatus=unbalanced['endpointStatus'],
                        unbalancedHardGateFailures=unbalanced['hardGateFailures'])),
        verdictMatches=r['endpointStatus'] == 'INCOMPLETE'
                      and 'ENDPOINT_NOT_QUIET' in r['inputEligibilityReasons']
                      and unbalanced['endpointStatus'] == 'INVALID_LEDGER',
        note=('No extrapolation, no interpolation, no window re-selection. The probe also shows '
              'that a conservation failure inside the same window escalates to INVALID_LEDGER, '
              'i.e. the hard gate outranks the coverage/quietness verdict (protocol §4.1).')))

    return results


def real_run_dual():
    """Same-run dual calculation on an archived real SLIP-family run.

    The archived run predates the endpoint field revision, so the new caliber
    is reported as NOT_IMPLEMENTED with the exact missing-field list, while
    the legacy caliber is computed and cross-checked against the historical
    SLIP checker output for the very same ledger.
    """
    if not REAL_LEDGER.exists() or not LEGACY_CHECKER_OUTPUT.exists():
        return dict(available=False)
    records = [json.loads(line) for line in REAL_LEDGER.read_text(encoding='utf-8').splitlines() if line.strip()]
    legacy_out = json.loads(LEGACY_CHECKER_OUTPUT.read_text(encoding='utf-8'))
    order, groups = [], {}
    for r in records:
        phase = r['context'].get('phaseD')
        if phase not in groups:
            groups[phase] = []
            order.append(phase)
        groups[phase].append(r)
    occurrences = []
    for phase in order:
        group = groups[phase]
        occurrences.append(dict(caseId='TC-P011-SLIP-03', endpointPhaseOccurrenceId='legacy-' + phase,
                                phaseD=phase, endpointBoundaryElapsedMs=0, windowStartElapsedMs=-1000,
                                phaseStartElapsedMs=0, phaseEndElapsedMs=0,
                                shakeOffEffectiveElapsedMs=None, legacyWindowStartElapsedMs=-5000))
    manifest = dict(runId=records[0]['context'].get('runId'),
                    endpointClockId=None, ledgerHash=None, occurrences=occurrences,
                    initialBaselineCommitKey=None, initialBaselineStateHash=None)
    hard = legacy_out.get('hardConstraint', {})
    safety_fail = []
    if hard.get('passed') is not True:
        safety_fail.append('EXISTING_HARD_GATE_FAILED:tan30 '
                           f"(violatingEdges={hard.get('violatingEdges')},"
                           f"violatingSubsteps={hard.get('violatingSubsteps')})")
    safety = dict(passed=not safety_fail, failures=safety_fail, ledgerHash='archive-identity')
    with_safety = analyze_endpoint(records, manifest, safety)
    without_safety = analyze_endpoint(records, manifest, None)
    legacy_phase = legacy_out.get('phaseEndpoint', {})
    cross = []
    for row in with_safety['occurrences']:
        phase = row['phaseD']
        ref = legacy_phase.get(phase)
        if ref is None:
            continue
        legacy_u = ref.get('endSuspHPlusSuspL')
        r_slip = ref.get('initialDepositTotal')
        mine = row.get('legacyEndpointFraction')
        expect = (legacy_u / r_slip) if (legacy_u is not None and r_slip) else None
        cross.append(dict(phaseD=phase,
                          historicalEndSuspHPlusSuspL=legacy_u,
                          historicalInitialDepositTotal=r_slip,
                          historicalVerdict=ref.get('suspensionWithin5PercentOfInitialDeposit'),
                          recomputedLegacyEndpointFraction=mine,
                          recomputedLegacyEndpointResult=row.get('legacyEndpointResult'),
                          expectedFractionFromHistoricalFormula=expect,
                          bitwiseAgree=(mine is not None and expect is not None
                                        and abs(mine - expect) <= 1e-15)))
    return dict(available=True, ledgerPath=str(REAL_LEDGER), recordCount=len(records),
                phaseOrder=order, legacyCheckerOutput=str(LEGACY_CHECKER_OUTPUT),
                legacyCheckerOverall=legacy_out.get('overall'),
                legacyHardGatePassed=hard.get('passed'),
                legacyHardGateDetail={k: hard.get(k) for k in
                                       ('checkedEdges', 'violatingEdges', 'violatingSubsteps', 'passed')},
                withSafetyReport=with_safety, withoutSafetyReport=without_safety,
                legacyCrossCheck=cross)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=Path('test/reports/endpoint_protocol_v1_execution.json'))
    args = parser.parse_args()
    payload = dict(
        executionId='endpoint-protocol-v1-execution',
        protocolId='p011-endpoint-terminal-nondeposited',
        protocolRevision='endpoint-terminal-nondeposited-v1',
        endpointMetricRevision='terminal-nondeposited-v1.1',
        endpointAuthority='APPROVED',
        endpointProtocolEffective=False,
        formalReleaseAuthorized=False,
        note=('Tester-side execution of the delivered endpoint checker. No device run exists yet '
              'for the endpoint field revision, so the new caliber on the archived real run is '
              'NOT_IMPLEMENTED; the candidate numbers in §10 come from synthetic fixtures and are '
              'labelled as such.'),
        counterexamples=counterexamples(),
        realRun=real_run_dual())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    matched = sum(1 for c in payload['counterexamples'] if c['verdictMatches'])
    print(f"counterexamples={len(payload['counterexamples'])} verdictMatches={matched}")
    for c in payload['counterexamples']:
        print(f"  {c['id']}: {'MATCH' if c['verdictMatches'] else 'MISMATCH'} :: {c['title']}")
    print(f"output={args.output}")
    return 0 if matched == len(payload['counterexamples']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
