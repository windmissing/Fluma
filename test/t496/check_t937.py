"""Independent T937 private-arm audit using the T938 raw-evidence reader."""
import json
from copy import deepcopy
from pathlib import Path

from check_pc_d1 import ART, load, need

T938=load('t938','test/t938/check.py')
BASE=ART/'t937/runs'
ARMS=('off-1-private','off-2-private','off-observe-private2')


def observation(rows,final,q):
    chain=[r for r in rows if r.get('stage','').startswith('COLD_COPY_')]
    need([r['stage'] for r in chain]==['COLD_COPY_SUBMITTED','COLD_COPY_WAIT','COLD_COPY_READBACK'],
         'OBSERVATION_COLD_CHAIN')
    s,w,r=chain
    auth=final['m1QuantumAuthority']
    b=final['m1ColdBacking']
    need(all((x['nonce'],x['run'],x['geometry'])==(auth['nonce'],auth['run'],auth['geometry'])
             and x['businessFence']==auth['fenceObject'] and x['businessFenceVkResult']==0
             and [x['businessFenceEnterQpc'],x['businessFenceExitQpc']]==auth['lastWait'][:2]
             and tuple(x[k] for k in ('baseBank','finalBank','publicationSlot','eventArena','eventStart','eventEnd','eventCapacity'))
             ==tuple(q[i] for i in (32,33,35,51,52,53,54)) for x in chain),'OBSERVATION_ROOT_FENCE')
    need(s['copyFence']==w['copyFence']==b['copyFence'] and int(w['copyFence'])>0
         and [w[k] for k in ('waitEnterQpc','waitExitQpc')]==[b[k] for k in ('waitEnterQpc','waitExitQpc')]
         and int(final['m1PrivateEvidenceCopy']['exitQpc'])<=int(s['submitEnterQpc'])<=int(s['submitExitQpc'])
         <=int(w['waitEnterQpc'])<=int(w['waitExitQpc'])
         and s['submitVkResult']==w['waitVkResult']==r['mapVkResult']==r['invalidateVkResult']==b['fence']==b['invalidate']==0,
         'OBSERVATION_COPY_WAIT')
    return dict(nonce=w['nonce'],fence=w['copyFence'],waitQpc=[w['waitEnterQpc'],w['waitExitQpc']],
                payloadSHA=final['m1PrivateObservationSHA'])


def arm(name,ref,packet):
    path=BASE/(name+'.jsonl')
    rows=list(map(json.loads,path.read_bytes().splitlines()))
    d=BASE/name
    observe=name==ARMS[2]
    argv=json.loads((BASE/(name+'-argv.json')).read_bytes())
    need((argv.get('exit')==0 or observe and 'exit' not in argv)
         and argv['command'][-1]==('ON' if observe else 'OFF')
         and Path(argv['command'][-2]).name==name and (d/'reference.bin').read_bytes()==packet,
         'ARM_INVOCATION_OR_REFERENCE')
    files={k:(d/(k+'.u32le')).read_bytes() for k in T938.SECTIONS}
    archives=list(d.glob('observe*.m1root'))
    need(len(archives)==int(observe),'ARM_ARCHIVE_INVENTORY')
    parsed=T938.private_arm(rows,files,ref,observe,archives[0].read_bytes() if observe else None)
    z=T938.evidence(rows,ref,observe)
    q=T938.words(files['quantum-completion'])
    cmd=T938.words(files['source-command'])
    hz=int(z['qpcFrequency'])
    need(T938.wide(cmd,22)==int(z['m1PulseNs'])>0
         and int(z['m1PulseNs'])*hz//1_000_000_000< int(z['m1SourceAuthority']['lastWait'][1])
         and z['m1AdmittedQ']==ref['q'] and z['m1QuantumDispatches']==127,'ARM_SOURCE_OR_DISPATCH')
    result=dict(run=parsed['run'],processExit=argv.get('exit'),traceSHA=T938.sha(path.read_bytes()),rawSHA=parsed['rawSHA'],
                admittedQ20=z['m1AdmittedQ'],carry=parsed['carry'],spareFinalizeDifferences=parsed['spareFinalizeDifferences'],
                sourcePulseNs=z['m1PulseNs'],copyFences=[z[key] for key in ('m1PrivateBaseCopy','m1PrivateEvidenceCopy')],
                roots={key:{k:z[key][k] for k in ('nonce','fenceObject','grantNs','deadlineNs','lastWait')}
                       for key in ('m1SourceAuthority','m1QuantumAuthority')},
                observation=observation(rows,z,q) if observe else None)
    return parsed,result


def comparison(a,b):
    first=T938.compare_private(a,b)
    if first['state']=='WORD_LENGTH_DIFF':
        return dict(first=first,allDifferences=None,physicalPayloadFirst=first)
    differences=[dict(section=k,word=i,left=x,right=y) for k in T938.SECTIONS
                 for i,(x,y) in enumerate(zip(a['words'][k],b['words'][k],strict=True)) if x!=y]
    clock={('quantum-candidate',7215),('quantum-candidate',7216),('quantum-completion',108),('quantum-completion',109)}
    physical=[r for r in differences if (r['section'],r['word']) not in clock]
    return dict(first=first,allDifferences=differences,physicalPayloadFirst=physical[0] if physical else None)


def main():
    packet=(BASE/ARMS[0]/'reference.bin').read_bytes()
    ref=T938.reference(packet)
    arms=[arm(name,ref,packet) for name in ARMS]
    name='off-observe-private'
    failed=list(map(json.loads,(BASE/(name+'.jsonl')).read_bytes().splitlines()))[-1]
    need(failed['state']=='FAILED' and failed['detail']=='M1_PRIVATE_REAL_PULSE'
         and failed.get('acceptedRoot') is None and not failed.get('m1PrivateEvidenceCopy'),'INITIAL_ON_FAILURE')
    comparisons={name:comparison(a,b) for name,a,b in (
        ('OFF_OFF',arms[0][0],arms[1][0]),('OFF_ON',arms[0][0],arms[2][0]))}
    rows=list(map(json.loads,(BASE/(ARMS[2]+'.jsonl')).read_bytes().splitlines()))
    bad=deepcopy(rows)
    next(r for r in bad if r.get('stage')=='COLD_COPY_WAIT')['copyFence']='0'
    q=T938.words((BASE/ARMS[2]/'quantum-completion.u32le').read_bytes())
    T938.rejects(lambda:observation(bad,T938.evidence(bad,ref,True),q),'OBSERVATION_COPY_WAIT')
    bad=deepcopy(rows)
    bad[-1]['acceptedRoot']='1'
    T938.rejects(lambda:T938.evidence(bad,ref,True),'PRIVATE_FAKE_PUBLICATION')
    print(json.dumps(dict(reference={k:ref[k] for k in ('sha','delta','q','pulses','frequency')},
                          arms={k:v[1] for k,v in zip(ARMS,arms,strict=True)},pairs=comparisons,
                          negatives=['OBSERVATION_COPY_WAIT','PRIVATE_FAKE_PUBLICATION'],
                          firstOnFailure=failed['detail'],formalFourArm='MISSING_COMPARABLE_ARMS',
                          t504='NOT_RELEASED'),indent=2))


if __name__=='__main__':
    main()
