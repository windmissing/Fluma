"""T898 PC acceptance checks for sustained SOURCE, autonomous LAND and recovery."""
import argparse
from copy import deepcopy
import importlib.util
import json
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]
REPORTS = ROOT/'test/reports'


def module(name,path):
    spec = importlib.util.spec_from_file_location(name,ROOT/path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


T888 = module('t888', 'test/t888/check.py')
sys.path.insert(0,str(ROOT/'test/t893'))
T893 = module('t893_fixture','test/t893/check.py')
COLD = module('t848','test/t848/check.py')
OLD, need, rows_at, pair = T888.OLD, T888.need, T888.rows_at, COLD.pair


def source_record(root, key, hz, inputs):
    row = root['rows'][0]
    tw = root['words']
    cmd = row['m1SourceWords']
    raw = OLD.D1.raw_bytes(cmd,row['m1SourceWordsSHA'],32)
    prod = row['m1ProducerWords']
    session,seq1,seq2,cb1,cb2,rx1,rx2,ns1,ns2 = row['m1ReservationPulses']
    delta = ns2-ns1
    down, up, sample = inputs
    stamp = lambda q:q*1_000_000_000//hz
    need(seq2 == seq1+1 and min(session,seq1) > 0 and cb1 < cb2 and cb1 <= rx1 < rx2
         and cb2 <= rx2 <= int(key) and ns1 == stamp(cb1) and ns2 == stamp(cb2)
         and 0 < delta <= 33_333_333 and stamp(rx2)-stamp(rx1) <= 33_333_333
         and int(row['m1SourceDeltaNs']) == delta, 'SOURCE_NATIVE_INTERVAL')
    need(len(prod)==25 and OLD.sha(struct.pack('<24Q',*prod[:-1])) == row['m1ProducerSHA']
         and (prod[0],prod[4],prod[9],prod[10],prod[12],prod[14],prod[15],prod[21],prod[23],prod[24])
         == (pair(tw,6),pair(tw,38),ns2,cb2,hz,int(down['hold']),int(sample['seq']),0,delta,int(key))
         and sample['kind'] in ('DOWN','MOVE') and sample['hold'] == down['hold'] and sample['geometry'] == up['geometry']
         and int(down['qpc']) <= int(sample['qpc']) <= cb2 < int(key) < int(up['qpc'])
         and int(row['m1FenceNs']) < stamp(int(up['qpc'])), 'SOURCE_HOLD_IDENTITY')
    need(cmd[:2] == [1129534023,2] and pair(cmd,4) == pair(tw,6) and pair(cmd,12) == pair(tw,38)
         and pair(cmd,28) == pair(tw,40) and cmd[14:16] == tw[59:61]
         and abs(struct.unpack_from('<d',raw,80)[0]-8*delta/1e9) < 1e-12
         and 0 < tw[61] == row['m1AdmittedQ'] and tw[61]+tw[62] == T888.TX.source_mass(delta)
         and pair(tw,18) == pair(tw,14)+1 and pair(tw,20) == pair(tw,16)+1 and tw[26]==0, 'SOURCE_MASS_OR_COMMIT')
    return dict(nonce=key,mass=tw[61],deltaNs=delta,sourceCommit=pair(tw,18),session=session,
                pulseSeq=seq2,commandSHA=row['m1SourceWordsSHA'])


def history(path, hold, strict=False):
    rows, receipt = OLD.read_rows(path)
    need(not receipt['offerQuoteRepairLines'],'HISTORY_STRICT_JSON')
    pointers = rows_at(rows,'WIN32_POINTER')
    down, = [r for r in pointers if r['kind']=='DOWN' and r['hold']==hold]
    d = int(down['qpc'])
    up = next(r for r in pointers if r['kind'] in ('UP','DOWN') and int(r['qpc'])>d)
    u = int(up['qpc'])
    need(up['kind']=='UP' and up['hold']=='0' and down['geometry']==up['geometry'],'HOLD_RELEASE')
    samples = {r['seq']:r for r in pointers}
    next_down = next((int(r['qpc']) for r in pointers if r['kind']=='DOWN' and int(r['qpc'])>u),2**64)
    roots = OLD.accepted(rows)
    hz, = {int(r['qpcFrequency']) for r in rows if 'qpcFrequency' in r}
    sources, frames = [], {}
    for key, root in sorted(roots.items(),key=lambda kv:int(kv[0])):
        if root['kind']!='SOURCE' or root['rows'][0]['m1Hold']!=hold:
            continue
        row = root['rows'][0]
        s = source_record(root,key,hz,(down,up,samples[row['m1PointerSeq']]))
        terms = [r for r in rows_at(rows,'M1_RESERVATION_TERMINAL') if r.get('root')==key]
        if terms:
            term, = terms
            need(term['result']==2 and term['hold']==hold and term['geometry']==down['geometry']
                 and term['run']==str(pair(root['words'],6)) and int(term['session'])==s['session']
                 and int(term['currentSeq'])==s['pulseSeq'] and int(term['deltaNs'])==s['deltaNs'], 'SOURCE_TERMINAL')
        s['observedTerminal'] = bool(terms)
        sources.append(s)
    need(len({(s['session'],s['pulseSeq']) for s in sources})==len(sources)
         and all(y['sourceCommit']==x['sourceCommit']+1 for x,y in zip(sources,sources[1:])), 'SOURCE_REUSED_INTERVAL')
    for row in rows:
        if row.get('state')!='M1_QUANTUM_PRESENT_QUEUED':
            continue
        f = row['windowFrame']
        key = f['acceptedRoot']
        tw = roots[key]['words']
        need(roots[key]['kind']=='QUANTUM' and str(pair(tw,38))==row['geometry']
             and str(pair(tw,6))==f['run'] and str(pair(tw,18))==f['sourceCommit']
             and str(pair(tw,20))==f['physicsCommit'] and str(pair(tw,36))==f['generation']
             and tw[35]==f['publicationSlot'] and all(f[k]==0 for k in T888.RATE.RESULTS), 'HISTORY_PRESENT_ROOT')
        if u < int(key) < next_down:
            frames[key] = dict(active=tw[88],stock=pair(tw,77),sourceCommit=pair(tw,18),bedEvents=tw[95],
                               presentBeforeNextDown=int(f['presentExitQpc']) < next_down)
    if sources:
        last = sources[-1]
        need(all(f['sourceCommit']==last['sourceCommit'] for f in frames.values())
             and len({f['stock'] for f in frames.values()})<=1, 'POST_UP_STOCK')
    qualified = len(sources)>=3 and all(s['observedTerminal'] for s in sources)
    if strict:
        need(qualified,'SUSTAINED_SOURCE_OR_TERMINAL_MISSING')
    return dict(receipt,hold=hold,durationMs=(u-d)*1000/hz,sources=sources,
                sustained='SUSTAINED_SOURCE_JOINED' if qualified else 'PARTIAL_EVIDENCE',postUp=frames)


def uninterrupted(inputs,start,end,code):
    # CAPTURE_LOST immediately after Up is a Win32 lifecycle receipt.
    need(not any(start < x['qpc'] <= end and x['kind']!='CAPTURE_LOST' for x in inputs),code)


def autonomous(e):
    proof = e['land']
    b,a = proof['before'],proof['after']
    up = proof['upQpc']
    edge = e['edge']
    samples = e['frames']
    need(edge['qpc']<=up<b['qpc']<a['qpc'] and edge['active']>0 and edge['P']==b['P']
         and edge['S']==b['S'] and edge['sourceCommit']==b['sourceCommit']
         and edge['geometry']==b['geometry'] and edge['run']==b['run']
         and samples and samples[-1]['qpc']>=a['qpc'], 'LIVE_AT_UP')
    uninterrupted(e['inputs'],up,samples[-1]['qpc'],'POST_UP_INPUT')
    need(not any(up < x <= samples[-1]['qpc'] for x in e['sourceAcceptQpc']), 'POST_UP_SOURCE')
    movement = T893.land(proof)
    need(proof['completion'][95]>0, 'LAND_COUNT_MISSING')
    previous = e['beforePhysicsCommit']
    for f in samples:
        need(f['run']==a['run'] and f['geometry']==a['geometry'] and f['sourceCommit']==a['sourceCommit']
             and f['S']+f['P']==a['S']+a['P'] and f['S']>=a['S'] and f['physicsCommit']==previous+1
             and f['render']=='SHADER' and f['current'] is True and f['present']==0
             and f['root']==f['leaseRoot'] and f['root']!=e['priorRoot'], 'AUTONOMOUS_FRAME')
        previous = f['physicsCommit']
    need(a['root']==samples[0]['root'] and samples[0]['physicsCommit']==pair(proof['completion'],20)
         and samples[-1]['P']==0 and samples[-1]['S']>0 and len(samples)>1
         and len({f['root'] for f in samples})==len(samples)
         and [f['qpc'] for f in samples]==sorted({f['qpc'] for f in samples}), 'LAND_RETAINED')
    return dict(state='AUTONOMOUS_LAND_JOINED',**{k:v for k,v in movement.items() if k!='state'})


def recovery(e):
    f,r,b,n = (e[k] for k in ('failed','retire','baseline','current'))
    hz = e['frequency']
    need(all(x['geometry']==f['geometry'] and x['run']==f['run'] for x in (r,b,n))
         and f['reason']=='ROOT_DEADLINE' and f['deadlineNs']-f['grantNs']==250_000_000
         and hz>0 and f['stopQpc']*1_000_000_000>=f['deadlineNs']*hz
         and f['accepted'] is False and f['nonce'] not in e['presentedRoots']
         and e['heldAtFailure'] is (e['downQpc']<=f['stopQpc']<e['upQpc']), 'RECOVERY_FAILED_ROOT')
    need(r['nonce']==f['nonce'] and r['fence']==f['fence'] and r['fence']>0
         and f['stopQpc']<=r['enterQpc']<=r['exitQpc']<r['idleQpc']
         and r['vkResult']==0 and r['idle'] is True and r['tombstonePoll']>0
         and r['pending']==r['inFlight']==r['pin']==0 and r['deviceLost'] is False
         and r['lastAcceptedRoot']==f['lastAcceptedRoot'] and r['lastPresentedRoot']==f['lastPresentedRoot'],
         'RECOVERY_SAME_FENCE_IDLE')
    c = e['credit']
    need(c['generated']==c['accepted']+c['overload']+c['revoked'] and c['carry']==0
         and c['revoked']-c['revokedBefore']==c['pendingBefore']+c['activeBefore']
         and c['clearedCarry']==c['carryBefore'], 'RECOVERY_CREDIT')
    need(r['idleQpc']<b['callbackQpc']<=b['receiptQpc']<=b['readyQpc']<n['callbackQpc']<=n['receiptQpc']
         <=n['grantQpc']<=n['fenceQpc']<=n['presentQpc']
         and b['session']!=f['session'] and b['session']==n['session'] and b['seq']+1==n['seq']
         and (n['callbackQpc']-b['callbackQpc'])*1_000_000_000<=33_333_333*hz
         and (n['receiptQpc']-b['receiptQpc'])*1_000_000_000<=33_333_333*hz
         and (n['grantQpc']-n['receiptQpc'])*1_000_000_000<=33_333_333*hz
         and (n['fenceQpc']-n['grantQpc'])*1_000_000_000<=250_000_000*hz,
         'RECOVERY_REAL_BASELINE')
    uninterrupted(e['inputs'],f['stopQpc'],n['presentQpc'],'RECOVERY_NEW_INPUT')
    need(not any(f['stopQpc'] < q <= n['presentQpc'] for q in e['sourceAcceptQpc']), 'RECOVERY_NEW_SOURCE')
    need(n['baseRoot']==r['lastAcceptedRoot'] and n['root'] not in (f['nonce'],r['lastPresentedRoot'],n['baseRoot'])
         and n['root']==n['leaseRoot'] and n['sourceCommit']==f['sourceCommit'] and n['stock']==f['stock']
         and n['physicsCommit']==f['physicsCommit']+1 and n['accepted'] is True and n['present']==0
         and n['fenceResult']==0 and n['state']=='CURRENT' and n['render']=='SHADER'
         and n['root'] in e['presentedRoots'], 'RECOVERY_FRESH_FRAME')
    tail = n['completion']
    need(len(tail)==256 and tuple(tail[:4])==COLD.SEAL and tail[27:32]==[2,1,0,0,0]
         and (str(pair(tail,4)),str(pair(tail,6)),str(pair(tail,38)))==(n['root'],n['run'],n['geometry'])
         and pair(tail,14)==pair(tail,18)==n['sourceCommit']
         and pair(tail,16)==f['physicsCommit'] and pair(tail,20)==n['physicsCommit']
         and pair(tail,71)==pair(tail,77)==n['stock'], 'RECOVERY_COMPLETION')
    return 'RECOVERY_WITHOUT_NEW_SOURCE'


def selftest():
    from scene import u32sha
    land = T893.a_cases()['land']
    land['before']['run'] = land['after']['run'] = '1'
    land['completion'][20] = 2
    land['completion'][95] = 1
    land['eventSHA'] = u32sha(land['eventWords'])
    a,b = land['after'],land['before']
    e = dict(land=land,edge=dict(b,qpc=8,active=1),beforePhysicsCommit=1,priorRoot='8',
             inputs=[dict(qpc=9,kind='UP')],sourceAcceptQpc=[7],
             frames=[dict(run='1',geometry=a['geometry'],sourceCommit=1,S=90,P=60,physicsCommit=2,qpc=12,
                          render='SHADER',current=True,present=0,root='9',leaseRoot='9'),
                     dict(run='1',geometry=a['geometry'],sourceCommit=1,S=150,P=0,physicsCommit=3,qpc=13,
                          render='SHADER',current=True,present=0,root='10',leaseRoot='10')])
    scope = dict(run='1',geometry='2')
    f = dict(scope,reason='ROOT_DEADLINE',grantNs=0,deadlineNs=250_000_000,nonce='bad',accepted=False,
             stopQpc=250,session=1,fence=7,lastAcceptedRoot='base',lastPresentedRoot='old',
             sourceCommit=2,physicsCommit=7,stock=100)
    r = dict(scope,nonce='bad',fence=7,enterQpc=251,exitQpc=255,idleQpc=256,idle=True,tombstonePoll=1,
             pending=0,inFlight=0,pin=0,deviceLost=False,vkResult=0,lastAcceptedRoot='base',lastPresentedRoot='old')
    baseline = dict(scope,session=2,seq=1,callbackQpc=260,receiptQpc=261,readyQpc=262)
    tail = T893.tail(nonce=42,geometry=2)
    tail[6],tail[14],tail[16],tail[18],tail[20],tail[71],tail[77] = 1,2,7,2,8,100,100
    frame = dict(scope,session=2,seq=2,callbackQpc=275,receiptQpc=276,grantQpc=277,fenceQpc=279,presentQpc=282,
                 baseRoot='base',root='42',leaseRoot='42',sourceCommit=2,physicsCommit=8,stock=100,accepted=True,
                 present=0,fenceResult=0,state='CURRENT',render='SHADER',completion=tail)
    y = dict(failed=f,retire=r,baseline=baseline,current=frame,downQpc=1,upQpc=240,heldAtFailure=False,
             inputs=[dict(qpc=240,kind='UP')],sourceAcceptQpc=[200],frequency=1000,presentedRoots=['old','42'],
             credit=dict(generated=7,accepted=3,overload=1,revoked=3,carry=0,revokedBefore=1,pendingBefore=1,
                         activeBefore=1,clearedCarry=20,carryBefore=20))
    held = dict(y,upQpc=300,heldAtFailure=True,inputs=[dict(qpc=300,kind='UP')])
    positive = dict(land=autonomous(e),released=recovery(y),held=recovery(held))
    negatives = []
    for gate,data,change,code in (
        (autonomous,e,lambda x:x['inputs'].append(dict(qpc=10,kind='DOWN')),'POST_UP_INPUT'),
        (autonomous,e,lambda x:x['sourceAcceptQpc'].append(10),'POST_UP_SOURCE'),
        (autonomous,e,lambda x:x['land']['completion'].__setitem__(95,0),'LAND_COUNT_MISSING'),
        (autonomous,e,lambda x:x['land']['eventWords'].__setitem__(6,1),'LAND_EVENT_RANGE'),
        (autonomous,e,lambda x:x['frames'][0].update(leaseRoot='wrong'),'AUTONOMOUS_FRAME'),
        (recovery,y,lambda x:x['retire'].update(fence=8),'RECOVERY_SAME_FENCE_IDLE'),
        (recovery,y,lambda x:x['retire'].update(pending=1),'RECOVERY_SAME_FENCE_IDLE'),
        (recovery,y,lambda x:x['retire'].update(deviceLost=True),'RECOVERY_SAME_FENCE_IDLE'),
        (recovery,y,lambda x:x['current'].update(geometry='3'),'RECOVERY_FAILED_ROOT'),
        (recovery,y,lambda x:x['failed'].update(accepted=True),'RECOVERY_FAILED_ROOT'),
        (recovery,y,lambda x:x['current'].update(baseRoot='old'),'RECOVERY_FRESH_FRAME'),
        (recovery,y,lambda x:x['current']['completion'].__setitem__(20,7),'RECOVERY_COMPLETION'),
        (recovery,y,lambda x:x['baseline'].update(session=1),'RECOVERY_REAL_BASELINE'),
        (recovery,y,lambda x:x['credit'].update(carry=1),'RECOVERY_CREDIT'),
        (recovery,y,lambda x:x['inputs'].append(dict(qpc=270,kind='DOWN')),'RECOVERY_NEW_INPUT'),
        (recovery,y,lambda x:x['sourceAcceptQpc'].append(270),'RECOVERY_NEW_SOURCE'),
    ):
        neg = deepcopy(data)
        change(neg)
        rejects(lambda:gate(neg),code)
        negatives.append(code)
    return dict(positive=positive,negatives=negatives)


def rejects(call,code):
    try:
        call()
    except ValueError as exc:
        need(str(exc)==code,f'EXPECTED_{code}: {exc}')
    else:
        raise AssertionError(code)


def originals(path=None):
    t892 = REPORTS/'t892'
    window = T888.long_window(t892/'bed-dual.jsonl')
    receipt = history(t892/'bed-dual.jsonl','1')
    raw, _ = OLD.read_rows(t892/'cold-busy.jsonl')
    archive = COLD.read_archive(t892/'m1pcold1.bin')
    cold = COLD.join(archive,raw)
    roi = json.loads((REPORTS/'t897/offline.json').read_bytes())['pixels']
    need(window['postUpNonzeroActive']>0 and window['bulkRetentionSupported']
         and archive[2][95]==archive[3][95]==0 and roi['layerState']=='ROI_OVERLAP'
         and roi['landState']=='MISSING_EVIDENCE','T892_T895_BOUNDARY')
    result = dict(t892=dict(source=receipt['sources'],postUpRoots=window['postUpNewRoots'],
                            firstZeroMs=window['firstZeroActiveRelativeToUpMs'],stock=window['postUpStockQ20'],
                            coldBytes=cold['decodedBytes'],coldLineage=cold['lineage'],land='MISSING_EVIDENCE'),
                  t895=dict(roi=roi['layerState'],land=roi['landState']),tests=selftest(),t899='PENDING_ORIGINALS')
    if path:
        run = history(path,'83')
        meta = json.loads((ROOT/'docs/architecture-T898-evidence-v1.json').read_bytes())
        need(run['sha256']==meta['sha256'] and len(run['sources'])==3 and len(run['postUp'])==8,'T898_ORIGINAL_SCOPE')
        rows,_ = OLD.read_rows(path)
        historical_recovery = []
        for row in rows:
            if row.get('state')!='M1_STALE_WAIT_NEW_INPUT' or 'm1Recovery' not in row:
                continue
            rec,authority = row['m1Recovery'],row.get('m1QuantumAuthority')
            if not authority or rec['reason']!='ROOT_DEADLINE' or authority.get('nonce')!=rec['failedNonce']:
                continue
            need(authority['retireVkResult']==0 and authority['fencePending'] is False
                 and rec['balanced'] is True and int(authority['fenceObject'])>0
                 and 0<int(authority['retireEnterQpc'])<=int(authority['retireExitQpc'])
                 and authority['retireExitQpc']==rec['oldRetiredQpc'], 'HISTORICAL_DEADLINE_RETIRE')
            historical_recovery.append(rec['failedNonce'])
        need(bool(historical_recovery),'HISTORICAL_DEADLINE_MISSING')
        source_nonces = {s['nonce'] for s in run['sources']}
        synthetic = []
        roots = OLD.accepted(rows)
        for nonce in sorted(source_nonces,key=int):
            row = roots[nonce]['rows'][0]
            session,_,seq,*_ = row['m1ReservationPulses']
            synthetic.append(dict(stage='M1_RESERVATION_TERMINAL',result=2,root=nonce,hold='83',
                                  geometry=row['geometry'],run=str(pair(roots[nonce]['words'],6)),
                                  session=str(session),currentSeq=str(seq),deltaNs=row['m1SourceDeltaNs']))
        with TemporaryDirectory() as tmp:
            fixture = Path(tmp)/'fixture.jsonl'
            def write(extra):
                fixture.write_text('\n'.join(map(json.dumps,rows+extra)),encoding='utf-8')
            write(synthetic)
            need(history(fixture,'83',strict=True)['sustained']=='SUSTAINED_SOURCE_JOINED','HOLD_FIXTURE')
            write(synthetic[:2])
            rejects(lambda:history(fixture,'83',strict=True),'SUSTAINED_SOURCE_OR_TERMINAL_MISSING')
        result['t898'] = run
        result['t898']['deadlineRetireNonces'] = sorted(set(historical_recovery),key=int)
        result['t898']['autonomousRecovery'] = 'MISSING_NEW_BASELINE_TO_CURRENT_JOIN'
        result['tests']['syntheticTerminalPositive'] = True
        result['tests']['negatives'].append('SUSTAINED_SOURCE_OR_TERMINAL_MISSING')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    modes = p.add_mutually_exclusive_group()
    modes.add_argument('--history',type=Path,help='Optional original T898 snapshot')
    modes.add_argument('--hold',nargs=2,metavar=('JSONL','HOLD'))
    modes.add_argument('--land',type=Path)
    modes.add_argument('--recovery',type=Path)
    args = p.parse_args()
    if args.hold:
        result = history(Path(args.hold[0]),args.hold[1],strict=True)
    elif args.land:
        result = autonomous(json.loads(args.land.read_bytes()))
    elif args.recovery:
        result = recovery(json.loads(args.recovery.read_bytes()))
    else:
        result = originals(args.history)
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
