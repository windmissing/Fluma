"""Read T896 D1 originals and retain each independent acceptance boundary."""
import argparse
from copy import deepcopy
import json
from pathlib import Path

from check_pc_d1 import ROOT, D1, accepted, load, need, normalized_arm, read_rows

T897 = load('t897', 'test/t897/check.py')
T888 = T897.T888


def rejection(check):
    try:
        check()
    except ValueError as error:
        return str(error)
    raise AssertionError('UNEXPECTED_STRICT_PASS')


def arm(path, mode):
    rows, artifact = read_rows(path)
    need(not artifact['offerQuoteRepairLines'], 'STRICT_JSON')
    data = T897.d1(rows)
    need(data['mode']==mode, 'ARM_MODE')
    seal, drain_row, close = (T888.exactly(rows,s) for s in ('END_INTAKE','M1_D1_DRAIN','M1_D1_CLOSE'))
    drain = drain_row['m1D1Drain']
    need(rows.index(seal)<rows.index(drain_row)<rows.index(close),'DRAIN_RECEIPT_ORDER')
    authority = [r['m1QuantumAuthority'] for r in rows if 'm1QuantumAuthority' in r
                 and (r['m1QuantumAuthority']['run'],r['m1QuantumAuthority']['geometry']) ==
                     (seal['run'],seal['geometry'])]
    fence = T888.d1_drain(seal,drain,close,data['hz'],authority)
    if fence=='BUSINESS_SIGNALED':
        wrong = dict(drain,ownerFenceNonce='0')
        need(rejection(lambda:T888.d1_drain(seal,wrong,close,data['hz'],authority))=='DRAIN_BUSINESS_FENCE',
             'WRONG_BUSINESS_FENCE_NOT_REJECTED')
    if drain['ownerFenceKind']=='TOMBSTONE_RETIRED':
        need(any(a['nonce']==drain['ownerFenceNonce'] and a['fenceObject']==drain['ownerFence']
                 and a['retireEnterQpc']==drain['ownerFenceEnterQpc']
                 and a['retireExitQpc']==drain['ownerFenceExitQpc'] and a['retireVkResult']==0
                 and a['fencePending'] is False for a in authority), 'DRAIN_TOMBSTONE_QPC')
    wsi, = [r for r in rows if 'm1WsiRetireVkResult' in r]
    need(wsi['m1PublicationReleased'] is True and wsi['m1WsiResult']==wsi['m1WsiRetireVkResult']==0
         and wsi['geometry']==seal['geometry'] and str(wsi['ownerTid'])==drain['owner'], 'DRAIN_WSI_RELEASE')
    roots = accepted(rows)
    need(roots and all(str(T888.pair(r['words'],6))==seal['run'] and
                       str(T888.pair(r['words'],38))==seal['geometry'] for r in roots.values()),'ARM_ROOT_SCOPE')
    capture = [data['actions'][i-1][1] for i in (19,43)]
    pages = T888.rows_at(rows,'M1_D1_HOT_PAGE')
    need(not pages or (mode=='ON' and capture==['RESERVED','ENDED'] and all(
         p['request']==19 and int(p['safePoint'])>0 and roots[p['root']]['words']==p['completion'] for p in pages)),
         'ARM_HOT_JOIN')
    # Carry the actual owner outcome into the T865 projection.
    projection = normalized_arm(rows,dict(mode=mode,run=data['run'],drain=drain,sourceTerminals=data['source']))
    for item, action in zip(projection['actions'],data['actions'],strict=True):
        if item['action']['kind']=='PULSE' and action[3]:
            item['terminal'] = action[3][6].upper()
    strict = rejection(lambda:D1.arm(projection,mode))
    return data, projection, dict(artifact, mode=mode, run=data['run'], sessions=data['sessions'],
                                  source=data['source'], acceptedRoots=len(roots),
                                  drain=fence, retireKind=drain['ownerFenceKind'], wsiReleased=True,
                                  capture=capture, hotPages=len(pages), t865FirstReject=strict)


def replay(path):
    rows, artifact = read_rows(path)
    need(not artifact['offerQuoteRepairLines'],'STRICT_JSON')
    stops = [(i,r) for i,r in enumerate(rows,1) if r.get('state')=='FAILED']
    i,r = stops[0]
    drain = T888.exactly(rows,'M1_D1_DRAIN')['m1D1Drain']
    actions = len(T888.rows_at(rows,'M1_D1_ACTION'))
    need(r['stage']=='M1_QUANTUM_COLD_CAPTURE' and r['detail']=='M1_COLD_ROOT_BACKING'
         and actions==0 and drain['retired'] is False and drain['ownerCoverage']['complete'] is False
         and drain['state']=='OWNER_TERMINATED_BEFORE_DRAIN_PROOF', 'REPLAY_FAILURE_SHAPE')
    return dict(artifact,firstFailureLine=i,stage=r['stage'],detail=r['detail'],actions=actions,drain=drain['state'])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('base',type=Path,nargs='?',default=ROOT/'test/reports/t896')
    args = p.parse_args()
    names = ('off1','off2','on')
    a,b,on = [arm(args.base/f'b-{n}.jsonl',mode) for n,mode in zip(names,('OFF','OFF','ON'),strict=True)]
    rows,_ = read_rows(args.base/'b-off2.jsonl')
    wrong = deepcopy(rows)
    T888.exactly(wrong,'M1_D1_DRAIN')['m1D1Drain']['generation'] = '0'
    need(rejection(lambda:T897.d1(wrong))=='D1_OWNER_COVERAGE_RECEIPT','WRONG_GENERATION_NOT_REJECTED')
    pair = T897.compare(a[0],b[0])
    need(pair['state']!='OFF_INPUT_MATCH_STRICT_JOIN_PENDING', 'UNEXPECTED_OFF_EQUIVALENCE')
    replay_names = ('replay-off','replay-off2','replay-on')
    result = dict(arms={n:r[2] for n,r in zip(names,(a,b,on),strict=True)},offPair=pair,
                  replays={n:replay(args.base/f'b-{n}.jsonl') for n in replay_names},
                  t865FourArm=rejection(lambda:D1.compare({'OFF':a[1],'ON':on[1]})),
                  negativeControls=['DRAIN_BUSINESS_FENCE','D1_OWNER_COVERAGE_RECEIPT'],
                  handoff='T902_THEN_RECAPTURE')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
