"""T897 offline checks for T895 originals and T894 D1 coverage rows."""
import argparse
from collections import Counter, defaultdict
from copy import deepcopy
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPORTS = ROOT/'test/reports'


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT/path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


T888 = module('t888', 'test/t888/check.py')
OLD, need, exactly, rows_at = T888.OLD, T888.need, T888.exactly, T888.rows_at


def read(path):
    rows, receipt = OLD.read_rows(path)
    need(not receipt['offerQuoteRepairLines'], 'STRICT_ORIGINAL_JSON')
    return rows


def empty_gravity(rows):
    empty, = [r for r in rows if r.get('state') == 'M1_EMPTY_PRESENT_QUEUED']
    ready = exactly(rows, 'M1_POINTER_READY')
    frame = empty['windowFrame']
    need(rows.index(empty) < rows.index(ready) and empty['geometry'] == ready['geometry'] == frame['geometry']
         and empty['surface'] == ready['surface'] == frame['surface'] and empty['pid'] == ready['pid']
         and frame['kind'] == 'EMPTY_READY' and frame['acceptedRoot'] is None
         and all(int(frame[k]) == 0 for k in ('sourceCommit','physicsCommit','sandStockQ','waterStockQ','particles',
                                               'initFenceVkResult',*T888.RATE.RESULTS))
         and min(int(frame[k]) for k in ('initFence','graphicsFence','hwnd')) > 0
         and 0 < int(frame['initFenceEnterQpc']) <= int(frame['initFenceExitQpc']) < int(frame['presentExitQpc'])
         and empty['pipelineVkResult'] == empty['m1PipelineVkResult'] == 0, 'EMPTY_FENCE_AND_READY')
    inputs = {}
    last_qpc = int(frame['presentExitQpc'])
    for row in rows_at(rows, 'M1_GRAVITY_INPUT'):
        seq, qpc = int(row['seq']), int(row['qpc'])
        angle = row['requestedAngle256']
        need(row['geometry'] == frame['geometry'] and seq > max(inputs, default=0) and qpc > last_qpc
             and row['result'] == ('ACCEPTED_INPUT' if abs(angle) <= 7680 else 'ANGLE_OUT_OF_RANGE'),
             'GRAVITY_INPUT')
        inputs[seq] = row
        last_qpc = qpc
    roots = OLD.accepted(rows)
    grants = []
    for row in rows:
        if row.get('state') != 'M1_QUANTUM_PRESENT_QUEUED':
            continue
        f, grant = row['windowFrame'], row['m1GravityGrant']
        root = f['acceptedRoot']
        words = roots[root]['words']
        angle = words[107] - (2**32 if words[107] >= 2**31 else 0)
        seq = int(grant['inputSequence'])
        sample = inputs.get(seq)
        want = grant['requestedAngle256']
        need(row['geometry'] == frame['geometry'] and f['run'] == frame['run']
             and root == grant['nonce'] and roots[root]['kind'] == 'QUANTUM'
             and str(OLD.COLD.pair(words,38)) == row['geometry']
             and str(OLD.COLD.pair(words,36)) == f['generation'] and words[35] == f['publicationSlot']
             and row['m1QuantumFenceVkResult'] == 0 and all(f[k] == 0 for k in T888.RATE.RESULTS)
             and grant['angleAcceptedIndex'] == angle and -7680 <= angle <= 7680
             and grant['grantedAngle256'] == want and grant['constrained'] == (angle != want),
             'GRAVITY_ACCEPTED_ROOT')
        need((seq == 0 and grant['inputQpc'] == '0' and want == 0) or
             (sample and sample['result'] == 'ACCEPTED_INPUT' and sample['qpc'] == grant['inputQpc']
              and int(sample['qpc']) < int(root) and sample['requestedAngle256'] == want), 'GRAVITY_GRANT_INPUT')
        grants.append(dict(root=root, seq=seq, requested=want, accepted=angle))
    return dict(empty='NULL_ROOT_FENCED_BEFORE_READY', run=frame['run'], inputs=list(inputs.values()),
                grants=grants, geometryChange='NOT_OBSERVED',
                failures=sorted({r['detail'] for r in rows if r.get('state') == 'FAILED'}))


def root_pixels(rows, path):
    roots = OLD.accepted(rows)
    done, = [r for r in rows if r.get('state') == 'M1_OBSERVATION_COMPLETE']
    obs, frame = done['m1Observation'], done['windowFrame']
    root = obs['root']
    accepted, = [r for r in rows if r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED'
                 and r['windowFrame']['acceptedRoot'] == root]
    need(accepted['windowFrame'] == frame and accepted['m1Observation'] == obs, 'ROI_PRESENT_JOIN')
    entries = roots[root]['rows']
    backing, = [r['m1ColdBacking'] for r in entries if r.get('state') == 'M1_OBSERVATION_COMPLETE']
    # Reuse T496 raw root validator with the observation receipt's digest name.
    mapping = dict(roots[root], rows=[{'m1QuantumReplaySHA':done['m1ObservedColdSHA'], 'm1ColdBacking':backing}])
    archive = OLD.root_archive(path,{root:mapping})
    pixels = bytes(obs['data'])
    need(obs['state'] == 'FENCED_READBACK' and root == frame['acceptedRoot']
         and obs['slot'] == frame['publicationSlot'] and obs['generation'] == frame['generation']
         and obs['geometry'] == done['geometry'] and obs['imageIndex'] == frame['imageIndex']
         and int(obs['graphicsFence']) > 0 and int(obs['graphicsSerial']) > 0
         and obs['rowOrder'] == 'TOP_DOWN' and obs['format'] == frame['format']
         and OLD.sha(pixels) == obs['sha256'] and path.stat().st_size == done['m1ObservedColdBytes'],
         'ROI_FENCED_BYTES')
    rects = obs['rects']
    need(len(rects) == 3 and all(x >= 0 and y >= 0 and w > 0 and h > 0 and x+w <= frame['width']
                                and y+h <= frame['height'] for x,y,w,h in rects)
         and len(pixels) == sum(4*w*h for _,_,w,h in rects), 'ROI_PIXEL_RANGE')
    independent = all(x+w <= a or a+c <= x or y+h <= b or b+d <= y
                      for i,(x,y,w,h) in enumerate(rects) for a,b,c,d in rects[i+1:])
    return dict(archive=archive, pixelsSHA=obs['sha256'], rects=rects,
                layerState='REQUIRES_LAYER_ORACLE' if independent else 'ROI_OVERLAP',
                bedEventCount=roots[root]['words'][95], landState='MISSING_EVIDENCE')


def g13(rows):
    queued = [r for r in rows if r.get('schema') == 'PC_G13_STEADY_OP66_V1']
    created = [r for r in rows if r.get('state') == 'ROLE_PIPELINE_CREATED']
    need(len(queued) == len(created) == 16 and len({r['role'] for r in queued}) == 16
         and all(r['queueSubmit'] == r['wait'] == 0 and r['acceptedRoot'] is None for r in queued), 'G13_OP66')
    pipeline_exits = [r for r in rows if r.get('schema') == 'PC_G13_PIPELINE_PROGRESS_V1'
                      and r.get('call') == 'vkCreateComputePipelines' and r.get('edge') == 'EXIT']
    identity = ('role','moduleSHA','pid','processStartTick','ownerTid')
    role_key = lambda r:tuple(r[k] for k in identity)
    need(Counter(map(role_key,queued)) == Counter(map(role_key,pipeline_exits))
         and all(r['vkResult'] == 0 for r in pipeline_exits), 'G13_PIPELINE_JOIN')
    wait = exactly(rows,'T877_ROOT_WAIT')
    diag, = [r for r in rows if r.get('state') == 'LATE_RETIRED_DIAGNOSTIC']
    drain = exactly(rows,'T877_DEVICE_DRAIN')
    keys = ('nonce','ticket','geometry','device','queue','fence','fenceGeneration','queryPool','writtenMask',
            'rootKind','stopQpc','firstStop')
    need(all(wait[k] == diag[k] for k in keys) and wait['submitVkResult'] == 0
         and diag['businessRejected'] is True and diag['acceptedRoot'] is None
         and diag['firstStop'] == 'GRANT_DEADLINE' and diag['fenceVkResult'] == diag['drainVkResult'] == drain['vkResult'] == 0
         and 0 < int(wait['submitExitQpc']) < int(wait['stopQpc']) < int(drain['enterQpc'])
         <= int(drain['exitQpc']) <= int(diag['retireConfirmEnterQpc']) <= int(diag['retireConfirmExitQpc']),
         'G13_RETIRED_JOIN')
    failed, = [r for r in rows if r.get('stage') == 'T833_ROOT_RETIRE_WAIT' and r['state'] == 'FAILED']
    hz = int(failed['qpcFrequency'])
    points = diag['points']
    names = ('UPLOAD_DONE','ACTUAL_BEGIN','ACTUAL_END','REPLAY_BEGIN','REPLAY_END',
             'ACTUAL_PRESSURE_SPAN_BEGIN','ACTUAL_PRESSURE_SPAN_END','ACTUAL_VISCOSITY_SPAN_BEGIN',
             'ACTUAL_VISCOSITY_SPAN_END','REPLAY_PRESSURE_SPAN_BEGIN','REPLAY_PRESSURE_SPAN_END',
             'REPLAY_VISCOSITY_SPAN_BEGIN','REPLAY_VISCOSITY_SPAN_END','COMPARE_DONE','PACK_DONE',
             'BEFORE_TAIL','BUSINESS_COPIES_DONE','COLD_COPIES_DONE','ROOT_BEGIN','COLD_COPIES_BEGIN')
    need(len({r['point'] for r in points}) == len(points) == int(diag['writtenMask']).bit_count()
         and {r['point'] for r in points} == {name for i,name in enumerate(names)
                                             if int(diag['writtenMask']) & (1<<i)}
         and all(r['available'] and r['vkResult'] == 0 for r in points), 'G13_QUERY_AVAILABILITY')
    ticks = {r['point']:int(r['tick']) for r in points}
    elapsed = ((ticks['BUSINESS_COPIES_DONE']-ticks['ROOT_BEGIN']) % (1<<diag['validBits']))*diag['periodNs']
    need(failed['vkResult'] == 2 and abs(elapsed-diag['spansNs']['root']) <= elapsed*0.00002, 'G13_ROOT_TIMESTAMPS')
    return dict(roles=len(created), root=None, firstStop=diag['firstStop'], originalRetireResult=2,
                lateDiagnostic=True, queryPoints=len(points), gpuRootNs=elapsed,
                hostSubmitToStopNs=(int(wait['stopQpc'])-int(wait['submitExitQpc']))*1e9/hz,
                flowAcceptance='NOT_REACHED')


def d1(rows):
    ready, seal = (exactly(rows,s) for s in ('M1_D1_READY','END_INTAKE'))
    drain = exactly(rows,'M1_D1_DRAIN')['m1D1Drain']
    coverage = drain['ownerCoverage']
    scope = (seal['run'],seal['geometry'])
    actions = rows_at(rows,'M1_D1_ACTION')
    plan = OLD.D1.plan()
    need([(r['id'],OLD.KINDS[r['kind']],int(r['scriptNs'])) for r in actions] ==
         [(s['actionId'],s['kind'],s['scriptNs']) for s in plan], 'D1_ACTION_TABLE')
    need(ready['run'] == drain['run'] == scope[0] and drain['generation'] == scope[1]
         and drain['traceLoss'] == coverage['loss'] == 0 and coverage['complete'] is True
         and coverage['revision'] == 'M1_D1_OWNER_COVERAGE_V1', 'D1_OWNER_COVERAGE_RECEIPT')
    hz = int(ready['qpcFrequency'])
    sent, masks, by_action = {}, defaultdict(Counter), {}
    need(hz > 0, 'D1_CLOCK_FREQUENCY')
    for r in actions:
        if OLD.KINDS[r['kind']] != 'PULSE':
            if ready['mode'] == 'OFF' and OLD.KINDS[r['kind']] != 'SOURCE':
                need(r['state'] in ('OFF_NOOP','REVOKED'), 'D1_OFF_CAPTURE_STATE')
            continue
        k = (r['session'],r['nativeSeq'])
        if r['state'] != 'DELIVERED':
            need(k == ('0','0') and r['state'], 'D1_UNQUEUED')
            continue
        callback, receipt = int(r['callbackQpc']), int(r['nativeReceiptQpc'])
        need(k not in sent and int(k[0]) > 0 and 1 <= int(k[1]) <= 64
             and 0 < callback <= receipt and int(r['actualNs']) == callback*1_000_000_000//hz,
             'D1_PRODUCER_KEY')
        sent[k] = r
    used = set()
    anchor = (min((int(r['callbackQpc']) for r in sent.values()), default=0),
              min((int(r['nativeReceiptQpc']) for r in sent.values()), default=0))
    for r in rows_at(rows,'M1_D1_PULSE_OWNER'):
        k = (r['session'],r['seq'])
        need((r['run'],r['geometry']) == scope and k in sent and k not in used, 'D1_OWNER_UNIQUE_SCOPE')
        p = sent[k]
        callback, receipt, owner = (int(r[f]) for f in ('callbackQpc','nativeReceiptQpc','ownerQpc'))
        need((p['id'],p['callbackQpc'],p['nativeReceiptQpc']) ==
             (r['actionId'],r['callbackQpc'],r['nativeReceiptQpc']) and receipt <= owner
             and owner*1_000_000_000//hz <= int(drain['drainNs']), 'D1_DUAL_CLOCK_JOIN')
        kind = r['outcome'].lower()
        rate = ('generated','pending','carry','overload')
        need(kind in ('accepted','rejected','revoked') and bool(r['state'])
             and (kind == 'accepted') == (r['state'] == 'PAGE_PULSE_ACCEPTED')
             and all((f in r) == (kind != 'revoked') for f in rate), 'D1_OWNER_OUTCOME')
        masks[k[0]][kind] |= 1 << (int(k[1])-1)
        used.add(k)
        by_action[p['id']] = (int(k[0]),int(k[1]),callback-anchor[0],receipt-anchor[1],receipt-callback,
                              owner-receipt,kind,r['state'],*(r.get(f) for f in rate))
    need(used == sent.keys() and len(sent) == coverage['producerQueued'], 'D1_OWNER_TERMINALS')
    sessions = {}
    for r in coverage['sessions']:
        session = r['session']
        need(session in masks and session not in sessions, 'D1_SESSION_RECEIPT')
        m = masks[session]
        mask = m['accepted'] | m['rejected'] | m['revoked']
        need(mask == int(r['queuedMask']) == int(r['terminalMask']) and mask.bit_count() == r['queued']
             and all(m[k].bit_count() == r[k] for k in ('accepted','rejected','revoked')), 'D1_SESSION_MASKS')
        sessions[session] = r
    need(sessions.keys() == masks.keys(), 'D1_SESSION_RECEIPT')
    last = next((r for r in reversed(actions) if r['state'] == 'DELIVERED'),None)
    need((last['session'],last['nativeSeq']) == (seal['lastPulseSession'],seal['lastPulseSeq'])
         if last else (seal['lastPulseSession'],seal['lastPulseSeq']) == ('0','0'), 'D1_FINAL_WATERMARK')
    source = OLD.offers(rows,ready['run'],plan,OLD.accepted(rows))
    for r in rows_at(rows,'M1_D1_SOURCE_TERMINAL'):
        a = r['actionId']
        need(a in source and r['state'] in ('NO_ADJACENT_QUEUED_PULSE','REVOKED_END_INTAKE','NOT_ADMITTED'),
             'D1_SOURCE_TERMINAL')
        if source[a]['outcome'] == 'MISSING_OFFER':
            source[a] = dict(outcome=r['state'], offerResult=r.get('offerResult'))
        else:
            need(r['state'] == 'NOT_ADMITTED' and source[a]['outcome'] in ('REVOKED','REJECTED','EXPIRED')
                 and 'offerResult' not in source[a], 'D1_SOURCE_TERMINAL')
            source[a]['offerResult'] = r['offerResult']
    for row in actions:
        if OLD.KINDS[row['kind']] == 'SOURCE' and row['state'] == 'REVOKED':
            need(source[row['id']]['outcome'] == 'MISSING_OFFER', 'D1_SOURCE_TERMINAL')
            source[row['id']] = dict(outcome='REVOKED_BEFORE_PRODUCER')
    need(all(r['outcome'] != 'MISSING_OFFER' for r in source.values()), 'D1_SOURCE_TERMINAL')
    for value in source.values():
        if value['outcome'] == 'ACCEPTED':
            need(value['admittedQ20'] == T888.TX.source_mass(value['deltaNs']), 'D1_SOURCE_MASS')
    for r in rows_at(rows,'M1_OFFER_TERMINAL'):
        if source[r['actionId']]['outcome'] == 'EXPIRED':
            need(r.get('reason') and int(r['ageNs']) == int(r['checkedNs'])-int(r['pulseNs']) >= 0,
                 'D1_SOURCE_EXPIRY')
            source[r['actionId']].update(reason=r['reason'], ageNs=int(r['ageNs']))
    return dict(run=scope[0],hz=hz, mode=ready['mode'], sessions=sessions, source=source,
                actions=[(r['id'],r['state'],int(r['actualNs'])-int(ready['readyNs']),by_action.get(r['id']))
                         for r in actions])


def compare(a,b):
    need(a['run'] != b['run'] and a['mode'] == b['mode'] == 'OFF' and a['hz'] == b['hz'], 'D1_OFF_PAIR')
    for x,y in zip(a['actions'],b['actions'],strict=True):
        if x != y:
            return dict(state='CLOCK_OR_INPUT_DIFFERENCE',action=x[0],left=x,right=y)
        if x[0] in a['source']:
            fields = ('outcome','deltaNs','requested','admittedQ20','reason','ageNs','offerResult')
            l,r = ([arm['source'][x[0]].get(f) for f in fields] for arm in (a,b))
            if l != r:
                return dict(state='SOURCE_INPUT_DIFFERENCE',action=x[0],left=l,right=r)
    return dict(state='OFF_INPUT_MATCH_STRICT_JOIN_PENDING' if any(v['outcome'] == 'ACCEPTED'
                for v in a['source'].values()) else 'OFF_SOURCE_POSITIVE_MISSING')


def selftest(a, archive, full):
    def fixture(run):
        pulse = 0
        rows = [dict(stage='M1_D1_READY',run=run,readyNs='10000',mode='OFF',qpcFrequency='10000000')]
        for s in OLD.D1.plan():
            kind = s['kind']
            aid = s['actionId']
            tick = 100+aid*100
            session,seq = 0,0
            state = 'OFF_NOOP'
            if kind == 'PULSE':
                session,seq = pulse//24+1,pulse%24+1
                pulse += 1
                state = 'DELIVERED'
                rev = pulse == 24
                rows.append(dict(stage='M1_D1_PULSE_OWNER',actionId=aid,run=run,geometry='2',
                                 session=str(session),seq=str(seq),callbackQpc=str(tick),
                                 nativeReceiptQpc=str(tick+1),ownerQpc=str(tick+2),
                                 outcome='REVOKED' if rev else 'ACCEPTED',
                                 state='REVOKED_REANCHOR' if rev else 'PAGE_PULSE_ACCEPTED',
                                 **({} if rev else dict(generated=1,pending=1,carry=0,overload=0))))
            if kind == 'SOURCE':
                state = 'AWAIT_PULSE'
                rows.append(dict(stage='M1_D1_SOURCE_TERMINAL',actionId=aid,state='NO_ADJACENT_QUEUED_PULSE'))
            rows.append(dict(stage='M1_D1_ACTION',id=aid,kind=OLD.KINDS.index(kind),
                             scriptNs=str(s['scriptNs']),actualNs=str(tick*100),state=state,
                             session=str(session),nativeSeq=str(seq),callbackQpc=str(tick),
                             nativeReceiptQpc=str(tick+1) if seq else '0'))
        sessions = [dict(session=str(i+1),queued=24,accepted=23+i,rejected=0,revoked=1-i,
                         queuedMask=str((1<<24)-1),terminalMask=str((1<<24)-1)) for i in range(2)]
        rows.extend([dict(stage='END_INTAKE',run=run,geometry='2',lastPulseSession='2',lastPulseSeq='24'),
                     dict(stage='M1_D1_DRAIN',m1D1Drain=dict(run=run,generation='2',traceLoss=0,drainNs='900000',
                          ownerCoverage=dict(revision='M1_D1_OWNER_COVERAGE_V1',complete=True,loss=0,
                                             producerQueued=48,sessions=sessions)))])
        return rows

    rows = fixture('1')
    left,right = d1(rows),d1(fixture('2'))
    need(compare(left,right)['state']=='OFF_SOURCE_POSITIVE_MISSING','FIXTURE_SOURCE_POSITIVE')
    changed = fixture('2')
    next(r for r in changed if r.get('actionId') == 1)['ownerQpc'] = '205'
    need(compare(left,d1(changed))['action']==1,'FIXTURE_CLOCK_FIRST_DIFF')
    first_source = min(left['source'])
    accepted = dict(outcome='ACCEPTED',deltaNs=15_000_000,requested=0.12,admittedQ20=125829)
    positive_left, positive_right = deepcopy(left),deepcopy(right)
    positive_left['source'][first_source] = accepted
    positive_right['source'][first_source] = dict(accepted)
    need(compare(positive_left,positive_right)['state']=='OFF_INPUT_MATCH_STRICT_JOIN_PENDING','FIXTURE_INPUT_MATCH')
    positive_right['source'][first_source]['deltaNs'] += 1
    need(compare(positive_left,positive_right)['state']=='SOURCE_INPUT_DIFFERENCE','FIXTURE_SOURCE_FIRST_DIFF')
    negatives = []
    first = lambda r: rows_at(r,'M1_D1_PULSE_OWNER')[0]
    cases = (
        (d1,rows,lambda r:r.append(first(r)),'D1_OWNER_UNIQUE_SCOPE'),
        (d1,rows,lambda r:r.remove(first(r)),'D1_OWNER_TERMINALS'),
        (d1,rows,lambda r:first(r).update(geometry='3'),'D1_OWNER_UNIQUE_SCOPE'),
        (d1,rows,lambda r:first(r).update(nativeReceiptQpc='0'),'D1_DUAL_CLOCK_JOIN'),
        (d1,rows,lambda r:exactly(r,'M1_D1_DRAIN')['m1D1Drain']['ownerCoverage']['sessions'][0].update(terminalMask='0'),
         'D1_SESSION_MASKS'),
        (lambda r:root_pixels(r,archive),a,
         lambda r:next(x for x in r if x.get('state')=='M1_OBSERVATION_COMPLETE')['m1Observation'].update(imageIndex=99),
         'ROI_PRESENT_JOIN'),
        (g13,full,lambda r:next(x for x in r if x.get('state')=='LATE_RETIRED_DIAGNOSTIC').update(fence='0'),
         'G13_RETIRED_JOIN'))
    for check,raw,mutate,code in cases:
        raw = deepcopy(raw)
        mutate(raw)
        try:
            check(raw)
        except ValueError as exc:
            need(str(exc)==code,f'EXPECTED_{code}: {exc}')
        else:
            raise AssertionError(code)
        negatives.append(code)
    return dict(sessionCoverage=left['sessions'], negatives=negatives, firstClockDiffAction=1)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--off',nargs=2,type=Path)
    args = p.parse_args()
    if args.off:
        a,b = (d1(read(path)) for path in args.off)
        result = dict(pair=compare(a,b),left=a,right=b)
    else:
        base = REPORTS/'t895'
        a = read(base/'a-empty-gravity.jsonl')
        b = read(base/'a-gravity-limit.jsonl')
        archive, = base.glob('a-obs-*.m1root')
        g = {arm:read(base/f'b-{arm}.jsonl') for arm in ('full','hot')}
        result = dict(emptyGravity=empty_gravity(a), gravityLimit=empty_gravity(b),pixels=root_pixels(a,archive),
                      g13={arm:g13(rows) for arm,rows in g.items()}, syntheticD1=selftest(a,archive,g['full']),
                      originalSHA256={path.name:OLD.sha(path.read_bytes()) for path in sorted(base.glob('*.jsonl'))},
                      actualOff='PENDING_T496_ORIGINALS')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
