"""Endpoint protocol v1 / field rev2. Raw-ledger candidate checker.

No historical report is rewritten. Formal release remains unauthorized.
The existing safety checker remains authoritative and is supplied separately.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path

EPS = 1e-9
FIELD = 'p011-endpoint-fields-v1'
PROTOCOL = 'endpoint-terminal-nondeposited-v1'
ROUTE = 'dual-field-particle-v1'
PROTOCOL_SOURCE_RELATIVE_PATH = 'docs/endpoint-protocol-P011-terminal-nondeposited-v1.md'
EVIDENCE_MODES = ('CANDIDATE', 'RETROSPECTIVE_DIAGNOSTIC')


def protocol_source_hash():
    """SHA-256 over the exact checked-in protocol bytes, including line endings."""
    path = Path(__file__).resolve().parents[1] / PROTOCOL_SOURCE_RELATIVE_PATH
    try:
        return hashlib.sha256(path.read_bytes()).hexdigest()
    except OSError:
        return None


def finite(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def key(record):
    c = record.get('context', {})
    return {n: c.get(n) for n in ('runId', 'sequence', 'frameId', 'substepId')}


def key_tuple(record):
    return tuple(key(record).values())


def endpoint_state_hash(record):
    c = record['context']
    payload = dict(stateAfter=record['stateAfter'], particles=record.get('particles'),
                   sourceInjectedH=c.get('sourceInjectedH'), sourceInjectedL=c.get('sourceInjectedL'), sourceEpoch=c.get('sourceEpoch'))
    return hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()).hexdigest()


def baseline_state_hash(record):
    return hashlib.sha256(json.dumps(record['stateBefore'], sort_keys=True, separators=(',', ':'), allow_nan=False).encode()).hexdigest()


PARTICLE_EVENT_FIELDS = {
    'SPAWN': ('particleId', 'phase', 'mass', 'sourceKind', 'x', 'y', 'sourceId', 'requested', 'admitted'),
    'LIFT': ('particleId', 'phase', 'mass', 'fromLayer', 'column', 'requested', 'granted'),
    'LAND': ('particleId', 'phase', 'mass', 'column', 'requested', 'granted', 'deltaS',
             'remainder', 'remainderDisposition'),
    'CROSS_COLUMN': ('particleId', 'phase', 'mass', 'fromColumn', 'toColumn', 'budget', 'distance'),
    'DEGRADE': ('particleIds', 'phase', 'mass', 'toColumn', 'toSuspH', 'toSuspL', 'flag'),
    'MERGE': ('particleIds', 'phase', 'massBefore', 'massAfter', 'positionMomentPreserved'),
    'TO_SUSP': ('particleId', 'phase', 'mass', 'toColumn', 'suspDelta', 'reason'),
}


def _pool_snapshot(record, pointer, missing, hard):
    raw = record.get('particleStateSnapshot')
    if raw is None:
        missing.append(pointer + '/particleStateSnapshot')
        return None
    try:
        value = json.loads(raw) if isinstance(raw, str) else raw
    except (TypeError, json.JSONDecodeError):
        hard.append('PARTICLE_EVENT_INVALID_POOL_SNAPSHOT:' + pointer)
        return None
    if not isinstance(value, dict):
        hard.append('PARTICLE_EVENT_INVALID_POOL_SNAPSHOT:' + pointer)
        return None
    names = ('pId', 'pMass', 'pPhase', 'pState', 'pSource', 'pSourceId', 'pX', 'pY')
    arrays = [value.get(name) for name in names]
    if any(not isinstance(array, list) for array in arrays) or len({len(array) for array in arrays}) != 1:
        missing.append(pointer + '/particleStateSnapshot/' + ' or '.join(names))
        return None
    return value


def _live_particle(snapshot, particle_id):
    if snapshot is None:
        return None
    for i, value in enumerate(snapshot['pId']):
        if value == particle_id and snapshot['pState'][i] in (1, 2):
            return dict(mass=snapshot['pMass'][i], phase=snapshot['pPhase'][i],
                        state=snapshot['pState'][i], source=snapshot['pSource'][i],
                        sourceId=snapshot['pSourceId'][i], x=snapshot['pX'][i], y=snapshot['pY'][i])
    return None


def validate_particle_events(record, record_index, previous_record, missing, hard):
    """Independently check serialized event schema, commit identity, and particle-state deltas."""
    pointer = f'/records/{record_index}'
    raw_events = record.get('particleEvents')
    if not isinstance(raw_events, list):
        return
    if record.get('eventLevelValidationOwnership') != 'ENDPOINT_CHECKER':
        missing.append(pointer + '/eventLevelValidationOwnership')
    if record.get('transactionLayerEventValidationRole') != 'EMIT_AND_ATOMIC_COMMIT_ONLY':
        missing.append(pointer + '/transactionLayerEventValidationRole')
    if not raw_events:
        # Empty journals carry no particle transaction claim. The enclosing
        # commit/snapshot identity is checked by the ordinary endpoint gate.
        return
    context = record.get('context', {})
    committed_substep = context.get('substepId')
    after = _pool_snapshot(record, pointer, missing, hard)
    before = None
    before_pointer = pointer + '/particleStateBeforeSnapshot'
    if record.get('particleStateBeforeSnapshot') is not None:
        before = _pool_snapshot(dict(particleStateSnapshot=record.get('particleStateBeforeSnapshot')),
                                before_pointer, missing, hard)
    elif previous_record is not None and previous_record.get('context', {}).get('runId') == context.get('runId'):
        before = _pool_snapshot(previous_record, pointer + '/previousCommit', missing, hard)
    else:
        missing.append(before_pointer)
    seen_request_keys = set()
    for event_index, raw_event in enumerate(raw_events):
        event_path = f'{pointer}/particleEvents/{event_index}'
        try:
            event = json.loads(raw_event) if isinstance(raw_event, str) else raw_event
        except (TypeError, json.JSONDecodeError):
            hard.append('PARTICLE_EVENT_INVALID_JSON:' + event_path)
            continue
        if not isinstance(event, dict):
            hard.append('PARTICLE_EVENT_INVALID_SHAPE:' + event_path)
            continue
        kind = event.get('kind')
        if kind not in PARTICLE_EVENT_FIELDS:
            hard.append('PARTICLE_EVENT_UNKNOWN_KIND:' + event_path)
            continue
        absent = [name for name in PARTICLE_EVENT_FIELDS[kind] if name not in event]
        if absent:
            missing.extend(event_path + '/' + name for name in absent)
            continue
        if event.get('substepId') is None:
            missing.append(event_path + '/substepId')
        elif event.get('substepId') != committed_substep:
            hard.append('PARTICLE_EVENT_SUBSTEP_MISMATCH:' + event_path)
        request_key = event.get('requestKey')
        if request_key is not None:
            if request_key in seen_request_keys:
                hard.append('PARTICLE_EVENT_DUPLICATE_REQUEST:' + event_path)
            seen_request_keys.add(request_key)
        if isinstance(event.get('phase'), bool) or event.get('phase') not in (0, 1):
            hard.append('PARTICLE_EVENT_INVALID_PHASE:' + event_path)
        numeric_fields = ('mass', 'requested', 'granted', 'admitted', 'deltaS', 'remainder',
                          'distance', 'massBefore', 'massAfter', 'suspDelta', 'toSuspH', 'toSuspL')
        for name in numeric_fields:
            if name in event and (not finite(event[name]) or event[name] < 0):
                hard.append('PARTICLE_EVENT_INVALID_MASS:' + event_path + '/' + name)
        if kind == 'TO_SUSP':
            # The current dual-field revision explicitly does not enable this
            # schema-preserved path; a future route needs a new revision.
            hard.append('PARTICLE_EVENT_TO_SUSP_DISABLED:' + event_path)
            continue
        if kind == 'LAND':
            if abs(event['requested'] - event['granted'] - event['remainder']) > EPS or \
               abs(event['deltaS'] - event['granted']) > EPS or \
               abs(event['mass'] - event['requested']) > EPS:
                hard.append('PARTICLE_EVENT_LAND_MASS_CLOSURE:' + event_path)
            if event['remainder'] > EPS and event['remainderDisposition'] != 'RETAIN_PARTICLE':
                hard.append('PARTICLE_EVENT_LAND_REMAINDER_DISPOSITION:' + event_path)
            if event['remainder'] <= EPS and event['remainderDisposition'] != 'NONE':
                hard.append('PARTICLE_EVENT_LAND_REMAINDER_DISPOSITION:' + event_path)
            prior = _live_particle(before, event['particleId'])
            current = _live_particle(after, event['particleId'])
            if before is not None and prior is None:
                hard.append('PARTICLE_EVENT_LAND_UNKNOWN_BEFORE:' + event_path)
            elif prior is not None and (prior['phase'] != event['phase'] or abs(prior['mass'] - event['mass']) > EPS):
                hard.append('PARTICLE_EVENT_LAND_BEFORE_MISMATCH:' + event_path)
            if event['remainder'] > EPS and (current is None or current['state'] != 2 or
                current['phase'] != event['phase'] or abs(current['mass'] - event['remainder']) > EPS):
                hard.append('PARTICLE_EVENT_LAND_AFTER_MISMATCH:' + event_path)
            if event['remainder'] <= EPS and current is not None:
                hard.append('PARTICLE_EVENT_LAND_RELEASE_MISMATCH:' + event_path)
        elif kind in ('SPAWN', 'LIFT'):
            if kind == 'SPAWN' and abs(event['mass'] - event['admitted']) > EPS:
                hard.append('PARTICLE_EVENT_SPAWN_ADMISSION_MISMATCH:' + event_path)
            if kind == 'LIFT' and (event['granted'] > event['requested'] + EPS or event['mass'] + EPS < event['granted']):
                hard.append('PARTICLE_EVENT_LIFT_GRANT_MISMATCH:' + event_path)
            current = _live_particle(after, event['particleId'])
            prior = _live_particle(before, event['particleId'])
            if current is None or current['phase'] != event['phase'] or abs(current['mass'] - event['mass']) > EPS:
                hard.append('PARTICLE_EVENT_BIRTH_AFTER_MISMATCH:' + event_path)
            if prior is not None:
                hard.append('PARTICLE_EVENT_BIRTH_ID_ALREADY_LIVE:' + event_path)
        elif kind == 'CROSS_COLUMN':
            if abs(event['fromColumn'] - event['toColumn']) != 1:
                hard.append('PARTICLE_EVENT_CROSSING_NOT_ADJACENT:' + event_path)
            budget = event.get('budget')
            if not isinstance(budget, dict) or any(not finite(budget.get(name)) or budget[name] < 0
                for name in ('local', 'bottom', 'total')):
                missing.append(event_path + '/budget{local,bottom,total}')
            prior = _live_particle(before, event['particleId'])
            current = _live_particle(after, event['particleId'])
            if prior is not None and (current is None or current['phase'] != event['phase'] or
                abs(current['mass'] - event['mass']) > EPS or int(current['x']) != event['toColumn']):
                hard.append('PARTICLE_EVENT_CROSSING_AFTER_MISMATCH:' + event_path)
        elif kind == 'DEGRADE':
            ids = event['particleIds']
            if not isinstance(ids, list) or not ids or len(set(ids)) != len(ids):
                hard.append('PARTICLE_EVENT_DEGRADE_IDS_INVALID:' + event_path)
                continue
            if event['flag'] != 'PARTICLE_DEGRADED_TO_COLUMN':
                hard.append('PARTICLE_EVENT_DEGRADE_FLAG_INVALID:' + event_path)
            if abs(event['mass'] - event['toSuspH'] - event['toSuspL']) > EPS:
                hard.append('PARTICLE_EVENT_DEGRADE_SUSP_MISMATCH:' + event_path)
            before_mass = 0.0
            for particle_id in ids:
                prior = _live_particle(before, particle_id); current = _live_particle(after, particle_id)
                if before is not None and prior is None:
                    hard.append('PARTICLE_EVENT_DEGRADE_UNKNOWN_BEFORE:' + event_path)
                elif prior is not None:
                    if prior['phase'] != event['phase']:
                        hard.append('PARTICLE_EVENT_DEGRADE_PHASE_MISMATCH:' + event_path)
                    before_mass += prior['mass']
                if current is not None:
                    hard.append('PARTICLE_EVENT_DEGRADE_STILL_LIVE:' + event_path)
            if before is not None and abs(before_mass - event['mass']) > EPS:
                hard.append('PARTICLE_EVENT_DEGRADE_MASS_MISMATCH:' + event_path)
        elif kind == 'MERGE':
            ids = event['particleIds']
            if not isinstance(ids, list) or len(ids) < 2 or len(set(ids)) != len(ids):
                hard.append('PARTICLE_EVENT_MERGE_IDS_INVALID:' + event_path)
                continue
            if abs(event['massBefore'] - event['massAfter']) > EPS or event['positionMomentPreserved'] is not True:
                hard.append('PARTICLE_EVENT_MERGE_MASS_OR_MOMENT_INVALID:' + event_path)
            before_mass = 0.0; before_x_moment = 0.0; before_y_moment = 0.0
            for particle_id in ids:
                prior = _live_particle(before, particle_id)
                current = _live_particle(after, particle_id)
                if prior is None:
                    if before is not None: hard.append('PARTICLE_EVENT_MERGE_UNKNOWN_BEFORE:' + event_path)
                    continue
                if prior['phase'] != event['phase']:
                    hard.append('PARTICLE_EVENT_MERGE_PHASE_MISMATCH:' + event_path)
                before_mass += prior['mass']; before_x_moment += prior['mass'] * prior['x']; before_y_moment += prior['mass'] * prior['y']
                if particle_id == ids[0]:
                    if current is None or abs(current['mass'] - event['massAfter']) > EPS:
                        hard.append('PARTICLE_EVENT_MERGE_SURVIVOR_MISMATCH:' + event_path)
                elif current is not None:
                    hard.append('PARTICLE_EVENT_MERGE_ABSORBED_STILL_LIVE:' + event_path)
            if before is not None and abs(before_mass - event['massBefore']) > EPS:
                hard.append('PARTICLE_EVENT_MERGE_MASS_MISMATCH:' + event_path)
            survivor = _live_particle(after, ids[0])
            if before is not None and survivor is not None and before_mass > 0 and (
                abs(survivor['x'] * event['massAfter'] - before_x_moment) > EPS or
                abs(survivor['y'] * event['massAfter'] - before_y_moment) > EPS):
                hard.append('PARTICLE_EVENT_MERGE_MOMENT_MISMATCH:' + event_path)


def held_at(record, elapsed, manifest, rechecks):
    c = record['context']
    for proof in rechecks:
        if (proof.get('observedElapsedMs') == elapsed and proof.get('runId') == c.get('runId') and
            proof.get('endpointClockId') == manifest.get('endpointClockId') == c.get('endpointClockId') and
            proof.get('heldCommitKey') == key(record) and proof.get('heldCommittedOrdinal') == c.get('endpointCommittedOrdinal') and
            proof.get('heldStateHash') == endpoint_state_hash(record) and proof.get('sourceEpoch') == c.get('sourceEpoch') and
            proof.get('phaseOccurrenceId') == c.get('endpointPhaseOccurrenceId') and
            proof.get('modelDormant') is True and proof.get('modelFrozen') is False and
            proof.get('lastErrorCode') is None and proof.get('inputSequence') == c.get('commandSequence') and
            proof.get('routeId') == c.get('particleRouteId')):
            return True
    return False


def mass(record):
    s = record.get('stateAfter', {})
    values = {}
    for name in ('bh', 'bl', 'mh', 'ml', 'suspH', 'suspL'):
        arr = s.get(name)
        if not isinstance(arr, list):
            raise KeyError('stateAfter.' + name)
        if not arr or any(not finite(v) or v < 0 for v in arr):
            raise ValueError('INVALID_STOCK:' + name)
        values[name] = sum(arr)
    if len({len(s[n]) for n in values}) != 1:
        raise ValueError('COLUMN_LENGTH_MISMATCH')
    particles = record.get('particles')
    if not isinstance(particles, dict):
        raise KeyError('particles')
    for n in ('massH', 'massL', 'count'):
        if n not in particles:
            raise KeyError('particles.' + n)
        if not finite(particles[n]) or particles[n] < 0:
            raise ValueError('INVALID_PARTICLE_' + n)
    if int(particles['count']) != particles['count']:
        raise ValueError('INVALID_PARTICLE_COUNT')
    ph, pl = particles['massH'], particles['massL']
    if (particles['count'] == 0) != (ph + pl == 0): raise ValueError('PARTICLE_COUNT_MASS_MISMATCH')
    if not isinstance(record.get('particleEvents'), list):
        raise KeyError('particleEvents')
    sources = particles.get('bySourceKind')
    if not isinstance(sources, dict): raise KeyError('particles.bySourceKind')
    source_groups = [sources.get(name, {}) for name in ('GESTURE_EXTERNAL', 'SHAKE_LIFT')]
    for group in source_groups:
        if any(not finite(group.get(n)) or group[n] < 0 for n in ('count','massH','massL')):
            raise ValueError('INVALID_PARTICLE_SOURCE_SUMMARY')
    if any(abs(sum(g[n] for g in source_groups)-particles[n]) > EPS for n in ('count','massH','massL')):
        raise ValueError('PARTICLE_SOURCE_SUMMARY_MISMATCH')
    degraded = particles.get('degraded')
    if not isinstance(degraded, dict): raise KeyError('particles.degraded')
    if any(not finite(degraded.get(n)) or degraded[n] < 0 for n in ('count','massH','massL')):
        raise ValueError('INVALID_DEGRADED_DIAGNOSTIC')
    by_state = particles.get('byState')
    diagnostic = 'NOT_OBSERVABLE'
    if by_state is not None:
        groups = []
        for state in ('FLYING', 'CONTACT_WAIT'):
            group = by_state.get(state, {})
            if any(not finite(group.get(n)) or group[n] < 0 for n in ('count', 'massH', 'massL')):
                raise ValueError('INVALID_BY_STATE:' + state)
            if int(group['count']) != group['count']:
                raise ValueError('INVALID_BY_STATE_COUNT')
            groups.append(group)
        if any(abs(sum(g[n] for g in groups) - particles[n]) > EPS for n in ('count', 'massH', 'massL')):
            raise ValueError('BY_STATE_MISMATCH')
        diagnostic = 'OBSERVED'
    if 'pMass' in particles:
        arrays = [particles.get(n) for n in ('pMass', 'pPhase', 'pState')]
        if any(not isinstance(a, list) for a in arrays) or len({len(a) for a in arrays}) != 1:
            raise ValueError('INVALID_PARTICLE_ARRAYS')
        actual = [0.0, 0.0]
        count = 0
        for m, phase, state in zip(*arrays):
            if state == 0:
                continue
            if isinstance(state,bool) or isinstance(phase,bool) or state not in (1, 2) or phase not in (0, 1) or not finite(m) or m <= 0:
                raise ValueError('INVALID_LIVE_PARTICLE')
            actual[phase] += m
            count += 1
        if count != particles['count'] or abs(actual[0] - ph) > EPS or abs(actual[1] - pl) > EPS:
            raise ValueError('PARTICLE_SUMMARY_MISMATCH')
    return dict(DH=values['bh'] + values['mh'], DL=values['bl'] + values['ml'],
                UH=values['suspH'], UL=values['suspL'], PH=ph, PL=pl,
                diagnostic=diagnostic, byState=by_state)


def analyze_endpoint(records, manifest, safety=None):
    """Manifest owns the predeclared boundaries; never infer them from EOF."""
    rechecks = [r for r in records if r.get('kind') == 'DORMANT_STATE_RECHECK']
    records = [r for r in records if r.get('kind') != 'DORMANT_STATE_RECHECK']
    run = manifest.get('runId')
    missing, hard, baseline_errors, incomplete = [], [], [], []
    evidence_mode = manifest.get('evidenceMode', 'CANDIDATE')
    evidence_mode_valid = isinstance(evidence_mode, str) and evidence_mode in EVIDENCE_MODES
    evidence_mode_error = None if evidence_mode_valid else 'INVALID_EVIDENCE_MODE'
    source_hash = protocol_source_hash()
    if evidence_mode_error is not None:
        incomplete.append(evidence_mode_error)
    report = dict(endpointFieldRevision=FIELD, endpointMetricRevision='terminal-nondeposited-v1.1',
                  endpointProtocolId='p011-endpoint-terminal-nondeposited', endpointProtocolRevision=PROTOCOL,
                  endpointAuthority='APPROVED', endpointFieldAlignment='ALIGNED',
                  endpointProtocolEffective=False, evidenceMode=evidence_mode,
                  evidenceClassification=evidence_mode if evidence_mode_valid else 'INVALID',
                  evidenceModeError=evidence_mode_error,
                  protocolSourceHash=source_hash, protocolSourceHashAlgorithm='SHA-256',
                  protocolSourcePath=PROTOCOL_SOURCE_RELATIVE_PATH,
                  runId=run, formalReleaseAuthorized=False,
                  applicabilityReasons=['FORMAL_RELEASE_NOT_AUTHORIZED'], occurrences=[])
    if not records:
        baseline_errors.append('EMPTY_LEDGER')
        vh = vl = r_slip = None
    else:
        first = records[0]
        c = first.get('context', {})
        audit = first.get('audit', {})
        vh, vl = audit.get('heavyTotalBefore'), audit.get('lightTotalBefore')
        if not finite(vh) or not finite(vl) or min(vh, vl) < 0:
            baseline_errors.append('INVALID_INITIAL_MASS')
        if c.get('sourceInjectedH') != 0 or c.get('sourceInjectedL') != 0:
            baseline_errors.append('INITIAL_BASELINE_AFTER_ADMISSION')
        before = first.get('stateBefore', {})
        try:
            if any(not finite(v) or v < 0 for n in ('bh', 'bl') for v in before[n]):
                raise ValueError()
            r_slip = sum(before['bh']) + sum(before['bl'])
        except (KeyError, TypeError, ValueError):
            r_slip = None
            baseline_errors.append('INVALID_SLIP_BASELINE')
        for field in ('initialBaselineCommitKey', 'initialBaselineStateHash'):
            if manifest.get(field) is None:
                baseline_errors.append('MISSING_' + field)
        if manifest.get('initialBaselineCommitKey') != key(first):
            baseline_errors.append('BASELINE_KEY_MISMATCH')
        try:
            if manifest.get('initialBaselineStateHash') != baseline_state_hash(first):
                baseline_errors.append('BASELINE_HASH_MISMATCH')
            for initial, bed, mixed, susp in ((vh,'bh','mh','suspH'),(vl,'bl','ml','suspL')):
                values = before[bed] + before[mixed] + before[susp]
                if any(not finite(v) or v < 0 for v in values) or not finite(initial) or abs(sum(values)-initial)>EPS:
                    baseline_errors.append('BASELINE_MASS_MISMATCH')
        except (KeyError, ValueError, TypeError):
            baseline_errors.append('INVALID_BASELINE_STATE')
    if safety is None:
        incomplete.append('HARD_GATE_REPORT_MISSING')
    elif safety.get('passed') is not True:
        hard.extend(safety.get('failures') or ['EXISTING_HARD_GATE_FAILED'])
    elif safety.get('ledgerHash') != manifest.get('ledgerHash') or not manifest.get('ledgerHash'):
        incomplete.append('HARD_GATE_LEDGER_IDENTITY_MISMATCH')
    committed, failed_attempts, seen, last_ordinal, last_time = [], [], set(), None, None
    all_ordinals = []
    last_sources = None
    previous_particle_record = None
    for index, record in enumerate(records):
        c = record.get('context', {})
        k = key_tuple(record)
        if c.get('runId') != run:
            hard.append('RUN_IDENTITY_MISMATCH')
        if record.get('audit', {}).get('errorCode') is not None or c.get('commitStatus') in ('blocked', 'rolled_back', 'rejected'):
            failed_attempts.append(index)
            hard.append('FAILED_ATTEMPT')
            if c.get('endpointSampleStage') == 'POST_COMMIT':
                hard.append('FALSE_POST_COMMIT')
            continue
        if k in seen:
            hard.append('DUPLICATE_COMMIT_KEY')
        seen.add(k)
        if any(c.get(n) is None for n in ('runId', 'sequence', 'frameId', 'substepId')):
            hard.append('INVALID_COMMIT_KEY')
        for n in ('finite', 'stock', 'capacity', 'conservation'):
            if record.get('audit', {}).get(n) is not True:
                hard.append('AUDIT_' + n.upper())
        if c.get('particleRouteActive') is not True or c.get('particleRouteId') != ROUTE:
            missing.append('context.particleRouteActive/particleRouteId')
        if c.get('endpointFieldRevision') != FIELD:
            missing.append('context.endpointFieldRevision')
        if c.get('endpointSampleStage') != 'POST_COMMIT':
            missing.append('context.endpointSampleStage')
        if c.get('endpointSampleError'):
            missing.append('context.endpointSampleError:' + str(c.get('endpointSampleError')))
        if record.get('particles') is None:
            missing.append('particles')
        if not isinstance(record.get('particleEvents'), list):
            missing.append('particleEvents')
        else:
            validate_particle_events(record, index, previous_particle_record, missing, hard)
        if (record.get('particles') is not None and
            (record.get('particleStateSnapshot') is None or
             record.get('particleCommittedSubstepId') != c.get('substepId'))):
            hard.append('PARTICLE_SNAPSHOT_COMMIT_MISMATCH')
        elapsed = c.get('endpointSampleElapsedMs')
        if elapsed is None:
            missing.append('context.endpointSampleElapsedMs')
        elif not finite(elapsed):
            hard.append('INVALID_ELAPSED')
        elif elapsed < 0 or (last_time is not None and elapsed < last_time):
            hard.append('NON_MONOTONIC_ELAPSED')
        else:
            last_time = elapsed
        if not c.get('endpointClockId') or c.get('endpointClockId') != manifest.get('endpointClockId'):
            incomplete.append('CLOCK_DOMAIN_MISMATCH')
        if not c.get('endpointPhaseOccurrenceId'):
            incomplete.append('MISSING_PHASE_OCCURRENCE')
        ordinal = c.get('endpointCommittedOrdinal')
        if ordinal is None:
            incomplete.append('INVALID_COMMITTED_ORDINAL')
        elif not isinstance(ordinal, int) or isinstance(ordinal, bool) or ordinal < 1 or ordinal > 2**53-1:
            hard.append('INVALID_COMMITTED_ORDINAL')
        else:
            if last_ordinal is not None and ordinal != last_ordinal + 1:
                incomplete.append('MISSING_OR_REPEATED_COMMITTED_ORDINAL')
            last_ordinal = ordinal
            all_ordinals.append(ordinal)
        if not isinstance(c.get('sourceEpoch'), int) or c['sourceEpoch'] < 0:
            incomplete.append('INVALID_SOURCE_EPOCH')
        admission_count = c.get('externalAdmissionCount')
        if admission_count is None:
            missing.append('context.externalAdmissionCount')
        elif (not isinstance(admission_count, int) or isinstance(admission_count, bool) or
              admission_count < 0):
            hard.append('INVALID_EXTERNAL_ADMISSION_COUNT')
        elif isinstance(c.get('sourceEpoch'), int) and admission_count != c['sourceEpoch']:
            hard.append('EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH')
        try:
            quantities = mass(record)
            ih, il = c.get('sourceInjectedH'), c.get('sourceInjectedL')
            if ih is None or il is None:
                raise KeyError('context.sourceInjectedH/L')
            if any(not finite(v) or v < 0 for v in (ih, il)):
                raise ValueError('INVALID_SOURCE_MASS')
            epoch = c.get('sourceEpoch')
            if last_sources is not None and isinstance(epoch, int):
                if ih < last_sources[0] or il < last_sources[1] or epoch < last_sources[2]:
                    hard.append('SOURCE_LEDGER_REGRESSION')
                if epoch == last_sources[2] and (ih != last_sources[0] or il != last_sources[1]):
                    hard.append('SOURCE_CHANGED_WITHOUT_EPOCH')
            if isinstance(epoch, int): last_sources = (ih, il, epoch)
            if finite(vh) and finite(vl):
                quantities['EH'], quantities['EL'] = vh + ih, vl + il
                if abs(quantities['DH'] + quantities['UH'] + quantities['PH'] - quantities['EH']) > EPS or \
                   abs(quantities['DL'] + quantities['UL'] + quantities['PL'] - quantities['EL']) > EPS:
                    hard.append('PHASE_CONSERVATION')
            else:
                quantities['EH'] = quantities['EL'] = None
            committed.append((record, quantities))
            previous_particle_record = record
        except KeyError as error:
            missing.append(str(error.args[0]))
        except (ValueError, TypeError) as error:
            hard.append(str(error))
    occurrences = manifest.get('occurrences', [])
    if not occurrences:
        incomplete.append('FIXED_BOUNDARY_MANIFEST_MISSING')
    for occurrence in occurrences:
        local_incomplete = list(incomplete)
        oid, case = occurrence.get('endpointPhaseOccurrenceId'), occurrence.get('caseId')
        end, start = occurrence.get('endpointBoundaryElapsedMs'), occurrence.get('windowStartElapsedMs')
        if not finite(end) or not finite(start) or start != end - 1000:
            local_incomplete.append('INVALID_FIXED_WINDOW')
        if case not in ('TC-P011-SLIP-03', 'TC-WS-08', 'TC-WS-09'):
            local_incomplete.append('CASE_NOT_APPLICABLE')
        if case == 'TC-WS-08' and (not finite(occurrence.get('shakeOffEffectiveElapsedMs')) or
                                                 end != occurrence['shakeOffEffectiveElapsedMs'] + 8000):
            local_incomplete.append('INVALID_WS08_FIXED_BOUNDARY')
        if case == 'TC-P011-SLIP-03' and end != occurrence.get('phaseEndElapsedMs'):
            local_incomplete.append('SLIP_BOUNDARY_IDENTITY_MISMATCH')
        if case == 'TC-WS-09' and (not finite(occurrence.get('phaseStartElapsedMs')) or
            occurrence.get('phaseEndElapsedMs') != end or end-occurrence['phaseStartElapsedMs'] != 10000 or
            occurrence.get('legacyWindowStartElapsedMs') != end-5000):
            local_incomplete.append('WS09_ORIGINAL_10S_5S_IDENTITY_MISSING')
        selected = [(r, q) for r, q in committed if r['context'].get('endpointPhaseOccurrenceId') == oid and
                    finite(r['context'].get('endpointSampleElapsedMs')) and finite(end) and
                    r['context']['endpointSampleElapsedMs'] <= end]
        window = [(r, q) for r, q in selected if finite(start) and r['context']['endpointSampleElapsedMs'] >= start]
        anchors = [(r, q) for r, q in selected if finite(start) and r['context']['endpointSampleElapsedMs'] <= start]
        left_proven = bool(anchors) and (anchors[-1][0]['context']['endpointSampleElapsedMs'] == start or
                                        held_at(anchors[-1][0], start, manifest, rechecks))
        if left_proven and anchors[-1][0]['context']['endpointSampleElapsedMs'] < start:
            window.insert(0, anchors[-1])
        if not window or not left_proven:
            local_incomplete.append('WINDOW_LEFT_BOUNDARY_NOT_PROVEN')
        if not selected or (selected[-1][0]['context']['endpointSampleElapsedMs'] != end and
                            not held_at(selected[-1][0], end, manifest, rechecks)):
            local_incomplete.append('FIXED_END_NOT_PROVEN')
        epochs = {r['context'].get('sourceEpoch') for r, _ in window}
        if len(epochs) != 1 or None in epochs:
            local_incomplete.append('ENDPOINT_NOT_QUIET')
        for r, _ in window:
            c = r['context']
            if case != 'TC-P011-SLIP-03' and c.get('shakeInput') != 0:
                local_incomplete.append('ENDPOINT_NOT_QUIET:SHAKE')
            for n in ('resetCount', 'invertCount', 'routeChangeCount', 'modeChangeCount'):
                values = {row['context'].get(n) for row, _ in window}
                if None in values or len(values) != 1:
                    local_incomplete.append('INPUT_CONTINUITY_NOT_PROVEN:' + n)
        if case == 'TC-P011-SLIP-03' and any(r['context'].get('sourceInjectedH', 0) or
                                            r['context'].get('sourceInjectedL', 0) for r, _ in committed):
            local_incomplete.append('SLIP_EXTERNAL_ADMISSION_REQUIRES_RERUN')
        row = dict(occurrence, runId=run, endpointMetricRevision=report['endpointMetricRevision'],
                   evidenceMode=evidence_mode,
                   evidenceClassification=evidence_mode if evidence_mode_valid else 'INVALID',
                   endpointReferenceKind='SLIP_INITIAL_BH_BL' if case == 'TC-P011-SLIP-03' else 'EXPECTED_TOTAL',
                   legacyEndpointReferenceKind='SLIP_INITIAL_BH_BL' if case == 'TC-P011-SLIP-03' else 'HISTORICAL_DEPOSIT_PLUS_SUSP',
                   legacyReportIdentity='legacy-susp-only', endpointThresholdFraction=0.05, endpointEpsMass=EPS,
                   missingFields=sorted(set(missing)), hardGateFailures=sorted(set(hard)),
                   inputEligibilityReasons=sorted(set(local_incomplete)),
                   windowCommittedCount=len(window), windowCommittedOrdinals=[r['context'].get('endpointCommittedOrdinal') for r, _ in window],
                   failedAttemptCount=len(failed_attempts), R_SLIP=r_slip, VH0=vh if finite(vh) else None, VL0=vl if finite(vl) else None,
                   endpointFraction=None, windowPeakFraction=None, massWindowPass=None,
                   legacyEndpointFraction=None, legacyMetricSuspensionFraction=None,
                   endpointFractionDelta=None)
        invalid_denominator = False
        if selected and finite(vh) and finite(vl):
            endpoint, q = selected[-1]
            d, u, p = q['DH'] + q['DL'], q['UH'] + q['UL'], q['PH'] + q['PL']
            e, a = q['EH'] + q['EL'], d + u + p
            reference = r_slip if case == 'TC-P011-SLIP-03' else e
            legacy_reference = r_slip if case == 'TC-P011-SLIP-03' else d + u
            row.update(depositedMass=d, suspendedMass=u, particleMass=p, nondepositedMass=u+p,
                       expectedMass=e, actualMass=a, endpointReferenceMass=reference,
                       legacyEndpointReferenceMass=legacy_reference,
                       endpointCommitKey=key(endpoint), endpointCommittedOrdinal=endpoint['context'].get('endpointCommittedOrdinal'),
                       endpointSampleElapsedMs=endpoint['context']['endpointSampleElapsedMs'],
                       integratedPhysicsSeconds=endpoint['context'].get('integratedPhysicsSeconds'),
                       endpointBoundaryLagMs=end-endpoint['context']['endpointSampleElapsedMs'],
                       suspFraction=u/e if e > 0 else None, airborneFraction=p/e if e > 0 else None,
                       depositedFraction=d/e if e > 0 else None,
                       legacyEndpointFraction=u/legacy_reference if finite(legacy_reference) and legacy_reference > 0 else None,
                       legacyMetricSuspensionFraction=u/max(a, EPS),
                       particleResidueDiagnosticStatus=q['diagnostic'])
            for label,h,l in [('depositedMass',q['DH'],q['DL']),('suspendedMass',q['UH'],q['UL']),
                              ('particleMass',q['PH'],q['PL']),('nondepositedMass',q['UH']+q['PH'],q['UL']+q['PL']),
                              ('expectedMass',q['EH'],q['EL']),('actualMass',q['DH']+q['UH']+q['PH'],q['DL']+q['UL']+q['PL'])]:
                row[label+'Heavy'],row[label+'Light']=h,l
            row['conservationErrorHeavy']=row['actualMassHeavy']-q['EH']
            row['conservationErrorLight']=row['actualMassLight']-q['EL']
            if not finite(reference) or reference <= 0 or not finite(legacy_reference) or legacy_reference <= 0:
                invalid_denominator = True
            else:
                row['endpointFraction'] = (u+p)/reference
                row['endpointFractionDelta'] = row['endpointFraction']-row['legacyEndpointFraction']
                row['sameReferenceSuspOnlyFraction'] = u/reference
                row['sameReferenceParticleContribution'] = p/reference
                row['endpointMassLimit'] = 0.05*reference+EPS
                row['endpointMassGap'] = u+p-row['endpointMassLimit']
                gaps = []
                for record, quantities in window:
                    mass_ = quantities['UH']+quantities['UL']+quantities['PH']+quantities['PL']
                    ref = r_slip if case == 'TC-P011-SLIP-03' else quantities['EH']+quantities['EL']
                    if not finite(ref) or ref <= 0:
                        invalid_denominator = True
                    else:
                        gaps.append((mass_/ref, mass_-0.05*ref-EPS, key(record)))
                if gaps:
                    row['windowPeakFraction'] = max(g[0] for g in gaps)
                    worst = max(gaps, key=lambda g:g[1])
                    row['windowWorstMassGap'], row['windowWorstCommitKey'] = worst[1], worst[2]
                    row['massWindowPass'] = all(g[1] <= 0 for g in gaps)
            row['legacyEndpointResult'] = 'INVALID_DENOMINATOR' if invalid_denominator else ('PASS' if u <= 0.05*legacy_reference+EPS else 'FAIL')
            row['legacyMetricSuspensionFractionSource'] = 'RECOMPUTED_FROM_SAME_POST_COMMIT_STATE'
            row['legacyMetricSuspensionFractionObserved'] = None
            row['legacyMetricSuspensionFractionObservedDifference'] = None
        # Missing C particles never become an invented P=0. The historical
        # D+U/SLIP computation is nevertheless available independently.
        if not selected:
            legacy = [r for r in records if r.get('context', {}).get('endpointPhaseOccurrenceId') == oid or
                      (occurrence.get('phaseD') is not None and r.get('context', {}).get('phaseD') == occurrence['phaseD'])]
            if legacy:
                s = legacy[-1].get('stateAfter', {})
                try:
                    d = sum(sum(s[n]) for n in ('bh','bl','mh','ml'))
                    u = sum(sum(s[n]) for n in ('suspH','suspL'))
                    lr = r_slip if case == 'TC-P011-SLIP-03' else d+u
                    if finite(d) and finite(u) and finite(lr) and lr > 0:
                        row.update(legacyEndpointFraction=u/lr, legacyEndpointReferenceMass=lr,
                                   legacyEndpointResult='PASS' if u <= 0.05*lr+EPS else 'FAIL')
                except (KeyError, TypeError):
                    pass
        status = 'PASS' if row['massWindowPass'] and row.get('endpointMassGap', 1) <= 0 else 'FAIL'
        if local_incomplete: status = 'INCOMPLETE'
        if invalid_denominator: status = 'INVALID_DENOMINATOR'
        if baseline_errors: status = 'INVALID_BASELINE'
        if missing: status = 'NOT_IMPLEMENTED'
        if hard: status = 'INVALID_LEDGER'
        row.update(endpointStatus=status, endpointWindowStatus=status, revisedEndpointResult=status,
                   hardGateStatus='FAIL' if hard else ('PASS' if safety and safety.get('passed') is True else 'INCOMPLETE'),
                   endpointStatusDetail=dict(missingFields=sorted(set(missing)), hardGateFailures=sorted(set(hard)),
                                             baselineErrors=baseline_errors, incomplete=sorted(set(local_incomplete))),
                   formalReleaseAuthorized=False)
        row['eventLevelValidationOwnership'] = 'ENDPOINT_CHECKER'
        row['eventValidationFailures'] = [failure for failure in hard if failure.startswith('PARTICLE_EVENT_')]
        row['longTermParticleResidueStatus'] = 'NOT_OBSERVABLE'
        if selected:
            by_state = selected[-1][1]['byState']
            diagnostic_codes = ['NOT_OBSERVABLE'] if by_state is None else []
            if by_state is not None and by_state['CONTACT_WAIT']['count'] > 0:
                diagnostic_codes.append('PERSISTENT_PARTICLE_RESIDUE')
                row['particleResidueDiagnosticMeaning'] = \
                    'Endpoint CONTACT_WAIT present; no lifetime/permanent-suspension conclusion'
            row['particleResidueDiagnosticCodes'] = diagnostic_codes
        # Required report metadata is never fabricated. Missing artifact
        # identity is distinct from the raw numerical candidate result.
        context = selected[-1][0]['context'] if selected else {}
        # D-4: the checked ledger sample owns these report values. An
        # occurrence/manifest placeholder must not mask a missing sample.
        row['endpointSampleStage'] = context.get('endpointSampleStage')
        row['externalAdmissionCount'] = context.get('externalAdmissionCount')
        fields = ('routeId particleRouteActive particleEvidenceKind buildCommit sourceTreeHash packageHash '
                  'ledgerPath ledgerHash scheduleId scheduleHash renderSeed inputScriptHash commandPhaseId segmentId '
                  'sourceEpoch sourceInjectedH sourceInjectedL resetCount invertCount routeChangeCount modeChangeCount '
                  'externalAdmissionCount endpointClockId endpointClockApi endpointClockResolutionMs runStartClockMs '
                  'endpointSampleStage protocolSourceHash '
                  'phaseStartElapsedMs phaseEndElapsedMs shakeOffEffectiveElapsedMs initialBaselineCommitKey '
                  'initialBaselineStateHash boundaryEvidencePaths dormantRecheckPaths particleResidueEvidencePaths '
                  'residueObservedDurationMs otherCaseGatesStatus managerApplicabilityDecision effectReportPath objectiveReportPath').split()
        for name in fields:
            row.setdefault(name, context.get(name, occurrence.get(name, manifest.get(name))))
        # This identity is computed from the protocol source file itself; a
        # manifest or ledger value cannot substitute another document hash.
        row['protocolSourceHash'] = source_hash
        row['protocolSourceHashAlgorithm'] = 'SHA-256'
        row['protocolSourcePath'] = PROTOCOL_SOURCE_RELATIVE_PATH
        row['reportMissingFields'] = [name for name in fields if row.get(name) is None]
        for diagnostic_field in ('particleResidueDiagnosticStatus', 'particleResidueDiagnosticCodes'):
            if diagnostic_field not in row:
                row['reportMissingFields'].append(diagnostic_field)
        row['reportMissingFields'] = sorted(set(row['reportMissingFields']))
        row['reportIdentityStatus'] = 'INCOMPLETE' if row['reportMissingFields'] else 'COMPLETE'
        row['endpointFieldRevision']=FIELD
        row['endpointProtocolId']=report['endpointProtocolId'];row['endpointProtocolRevision']=PROTOCOL
        row['endpointProtocolEffective']=False;row['endpointAuthority']='APPROVED';row['endpointFieldAlignment']='ALIGNED'
        row['windowFirstCommitKey']=key(window[0][0]) if window else None
        row['windowLastCommitKey']=key(window[-1][0]) if window else None
        row['coverageStartElapsedMs']=window[0][0]['context']['endpointSampleElapsedMs'] if window else None
        row['coverageEndElapsedMs']=end if selected and held_at(selected[-1][0],end,manifest,rechecks) else (selected[-1][0]['context']['endpointSampleElapsedMs'] if selected else None)
        row['duplicateCommitKeys']=[] if 'DUPLICATE_COMMIT_KEY' not in hard else ['DUPLICATE_COMMIT_KEY']
        ordered = sorted(set(all_ordinals))
        intervals = [[a+1,b-1] for a,b in zip(ordered,ordered[1:]) if b>a+1]
        row['missingCommittedOrdinalRanges']=intervals
        count_missing=sum(b-a+1 for a,b in intervals)
        row['missingCommittedOrdinals']=[n for a,b in intervals for n in range(a,b+1)] if count_missing<=10000 else None
        row['missingCommittedOrdinalsTruncated']=count_missing>10000
        row['inputEligibilityStatus']='INCOMPLETE' if local_incomplete else 'ELIGIBLE'
        report['occurrences'].append(row)
    if evidence_mode_error is not None:
        report['status'] = 'INCOMPLETE'
    elif evidence_mode == 'RETROSPECTIVE_DIAGNOSTIC':
        report['status'] = 'RETROSPECTIVE_DIAGNOSTIC' if report['occurrences'] else 'INCOMPLETE'
    else:
        report['status'] = 'CANDIDATE_ONLY' if report['occurrences'] else ('INVALID_BASELINE' if baseline_errors else 'INCOMPLETE')
    report['peakNonDeposited'] = analyze_peak_nondeposited(records, manifest)
    return report


def check_files(ledger_path, manifest_path, safety=None):
    raw = Path(ledger_path).read_bytes()
    manifest = json.loads(Path(manifest_path).read_text(encoding='utf-8'))
    actual_hash = hashlib.sha256(raw).hexdigest()
    if manifest.get('ledgerHash') not in (None, actual_hash):
        raise ValueError('LEDGER_HASH_MISMATCH')
    manifest['ledgerHash'] = actual_hash
    report = analyze_endpoint([json.loads(line) for line in raw.decode('utf-8').splitlines() if line.strip()], manifest, safety)
    report.update(ledgerPath=str(ledger_path), ledgerHash=actual_hash, scheduleHash=manifest.get('scheduleHash'))
    return report


def analyze_peak_nondeposited(records, manifest):
    from p011_peak_check import check_peak
    return check_peak(records, manifest)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ledger', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--safety-report', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--candidate-only', action='store_true', help='Exit status reflects numerical diagnostics, never formal release')
    args = parser.parse_args()
    safety = json.loads(args.safety_report.read_text()) if args.safety_report else None
    result = check_files(args.ledger, args.manifest, safety)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False))
    return 0 if args.candidate_only and result['occurrences'] and all(r['endpointStatus']=='PASS' for r in result['occurrences']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
