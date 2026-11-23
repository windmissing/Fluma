"""T858 PC dry D1 plan, independent replay gate, and native hot-page reader."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time

ROOT = Path(__file__).resolve().parents[2]
SEAL = (1381192263, 2, 256, 3235823838)
MODES = ('OFF', 'ON', 'REPLAY_OFF', 'REPLAY_ON')
# Completion V2 positional pairs with explicit per-root identity evidence.
TAIL_IDS = {'nonce':4,'run':6,'owner':8,'context':10,'surface':12,'permit':22,'frame':24,'geometry':38}
COMMAND_IDS = {'run':4,'geometry':12}
DRAIN = ('offers', 'inFlight', 'leases', 'permits')


def need(ok, code):
    if not ok:
        raise ValueError(code)


def plan():
    result, tick, pulses = [], 1_000_000, 0
    for count, special in ((6,'SOURCE'),(10,'RESERVE'),(8,'SOURCE'),(12,'END'),(8,None)):
        for _ in range(count):
            tick += 16_000_000
            pulses += 1
            result.append(dict(actionId=len(result)+1, kind='PULSE', scriptNs=tick, pulseSeq=pulses))
        if special:
            tick += 16_000_000
            action = dict(actionId=len(result)+1, kind=special, scriptNs=tick)
            if special == 'SOURCE':
                action.update(phase='SAND', sourceSeq=1 if pulses==6 else 2, x=64.5,
                              y=64. if pulses==6 else 128.)
            result.append(action)
            pulses += 1
            result.append(dict(actionId=len(result)+1, kind='PULSE', scriptNs=tick, pulseSeq=pulses))
    need(len(result)==52 and pulses==48, 'PLAN')
    return result


def packed(words, count):
    need(len(words)==count and all(type(w) is int and 0<=w<2**32 for w in words),'RAW_WORDS')
    return struct.pack(f'<{count}I', *words)


def raw_bytes(words, digest, count):
    raw=packed(words,count)
    need(hashlib.sha256(raw).hexdigest()==digest,'RAW_SHA')
    return raw


def canonical(words, ids, offsets):
    out=words.copy()
    for name,at in offsets.items():
        uid=int(ids[name])
        need(uid>0 and uid==(words[at]|words[at+1]<<32),'RAW_IDENTITY')
        out[at:at+2]=[0,0]
    return out


def arm(row, mode):
    actions, roots = row['actions'], row['roots']
    expected=plan()
    need([a['action'] for a in actions]==expected,'ACTION_ORDER_OR_CONTENT')
    need(row['mode']==mode and len({a['terminalId'] for a in actions})==52,'TERMINAL_DUPLICATE')
    for item in actions:
        kind=item['action']['kind']
        allowed={'PULSE':('ACCEPTED',), 'SOURCE':('ACCEPTED','REJECTED','REVOKED','EXPIRED'),
                 'RESERVE':('RESERVED','NOT_BUSY'), 'END':('ENDED','NO_PAGE')}
        valid=('OFF_NOOP',) if kind in ('RESERVE','END') and mode.endswith('OFF') else allowed[kind]
        need(item['terminal'] in valid and item['actualNs']>=row['readyNs'],'ACTION_TERMINAL')
    need([a['actualNs'] for a in actions]==sorted(a['actualNs'] for a in actions),'ACTION_CLOCK')
    need(row['drain']['retired'] is True and row['drain']['fence'] == 0
         and all(row['drain'][k]==0 for k in DRAIN),'DRAIN')
    need(row['clockEquivalent'] is True and row['clockAnchorSHA']==row['contract']['clockAnchorSHA'],
         'CLOCK_EQUIVALENCE_UNPROVEN')
    values, witness, sources, nonces = [], [], set(), set()
    scope = None
    for root in roots:
        completion, ids = root['completion'], root['ids']
        raw_bytes(completion,root['completionSHA'],256)
        nonce = int(ids['nonce'])
        need(nonce not in nonces and tuple(completion[:4])==SEAL and completion[28]==1
             and root['ownerCode']==root['submit']==root['fence']==0,'ROOT_ACCEPTANCE')
        nonces.add(nonce)
        current_scope=tuple(str(ids[k]) for k in ('run','owner','context','surface','geometry'))
        need(scope is None or current_scope==scope,'D1_ROOT_SCOPE')
        scope=current_scope
        kind = root['kind']
        action_id = root['actionId']
        need(type(action_id) is int and 1<=action_id<=52,'ROOT_ACTION')
        action = actions[action_id-1]
        need(kind in ('SOURCE','QUANTUM') and completion[27]==(1 if kind=='SOURCE' else 2)
             and 0<=completion[52]<=completion[53]<=completion[54], 'ROOT_KIND_OR_RANGE')
        command=None
        if kind=='SOURCE':
            need(action['action']['kind']=='SOURCE' and action['terminal']=='ACCEPTED'
                 and root['externalProducer'] is True and completion[61]>0 and action_id not in sources,
                 'REAL_SOURCE')
            sources.add(action_id)
            raw_bytes(root['command'],root['commandSHA'],32)
            need(root['command'][:3]==[1129534023,2,0], 'COMMAND_LAYOUT')
            producer=bytes.fromhex(root['producerHex'])
            need(producer and hashlib.sha256(producer).hexdigest()==root['producerSHA'],
                 'PRODUCER_SHA')
            command=canonical(root['command'],ids,COMMAND_IDS)
        else:
            need(root['replayDone'] is True and root['publicationConsumed'] is True
                 and completion[53]>completion[52], 'QUANTUM_EVIDENCE')
        values.append([kind,action_id,canonical(completion,ids,TAIL_IDS),command])
        if mode.startswith('REPLAY'):
            need(root['copyFence']==root['mapResult']==root['invalidateResult']==0
                 and root['binding']==root['captureBinding'] and root['copyBeforeReuse'] is True,'REPLAY_BACKING')
            bank, events=bytes.fromhex(root['bankHex']),bytes.fromhex(root['eventHex'])
            need(len(bank)==29696 and len(events)==completion[53]*256
                 and sha_pair(bank,events)==[root['bankSHA'],root['eventSHA']], 'REPLAY_BYTES')
            witness.append((sha_pair(bank,events),len(bank)+len(events)))
        else:
            need(not any(k in root for k in ('bankHex','eventHex')),'HOT_READBACK')
    need(sources and sources=={a['action']['actionId'] for a in actions
                               if a['action']['kind']=='SOURCE' and a['terminal']=='ACCEPTED'}
         and any(v[0]=='QUANTUM' for v in values),'NONEMPTY_ROOT_CHAIN')
    if witness:
        credits=row['credits']
        need(0<=credits['apiBytes']<=12*2**20 and 0<=credits['indexBytes']<=4*2**20
             and sum(size for _,size in witness)<=credits['rawBytes']<=16*2**20, 'REPLAY_CREDIT')
    return values,[v for v,_ in witness]


def sha_pair(*parts):
    return [hashlib.sha256(p).hexdigest() for p in parts]


def compare(rows, synthetic=False):
    need(set(rows)==set(MODES),'ARM_SET')
    arms=[rows[m] for m in MODES]
    need(all(a['synthetic'] is synthetic for a in arms) and len({a['process'] for a in arms})==4
         and all(a['contract']==arms[0]['contract'] for a in arms),'ARM_IDENTITY')
    results=[arm(a,m) for a,m in zip(arms,MODES)]
    source_terminals=[[a['terminal'] for a in arm_['actions'] if a['action']['kind']=='SOURCE']
                      for arm_ in arms]
    need(all(t==source_terminals[0] for t in source_terminals),'SOURCE_TERMINAL_DIFF')
    need(all(x[0]==results[0][0] for x in results),'ROOT_WORD_DIFF')
    need(results[2][1]==results[3][1],'REPLAY_WORD_DIFF')
    return {'state':'SYNTHETIC_PASS' if synthetic else 'D1_MATCH',
            'actionsPerArm':52,'rootsPerArm':len(results[0][0])}


def hot(rows, accepted):
    reserved, ended = {}, set()
    result=[]
    last_safe=0
    for r in rows:
        stage=r.get('stage')
        if stage not in ('M1_HOT_CAPTURE_RESERVED','M1_HOT_CAPTURE_TERMINAL','M1_HOT_CAPTURE'):
            continue
        key=int(r['request'])
        need(key>0 and key not in ended,'HOT_DUPLICATE_OR_STALE')
        if stage.endswith('RESERVED'):
            need(key not in reserved and int(r['requestNs'])>0,'HOT_RESERVE')
            reserved[key]=int(r['requestNs'])
            continue
        need(key in reserved,'HOT_WITHOUT_RESERVE')
        ended.add(key)
        if stage.endswith('TERMINAL'):
            need(r['state'] in ('CANCELLED','TIMED_OUT','FAILED') and 'completion' not in r,'HOT_TERMINAL')
            result.append(r['state'])
            continue
        start, end, safe = int(r['requestedNs']),int(r['frozenNs']),int(r['safePoint'])
        need(start==reserved[key] and 0<=end-start<1_000_000_000 and safe>last_safe
             and r['blocker'] in (1,2,3,4),'HOT_TIME_OR_BLOCKER')
        last_safe=safe
        words=r['completion']
        raw=packed(words,256)
        nonce=str(words[4]|words[5]<<32)
        matches=[a for a in accepted if a.get('request')==r['request'] and a.get('root')==nonce
                 and a.get('run')==str(key) and a.get('accepted') is True
                 and a.get('completionSHA')==hashlib.sha256(raw).hexdigest()]
        need(tuple(words[:4])==SEAL and words[28]==1 and nonce==r['root']
             and (words[6]|words[7]<<32)==key and len(matches)==1,'HOT_ROOT_OR_SHA')
        result.append('FROZEN_HOT_PAGE')
    need(reserved and set(reserved)==ended,'HOT_PENDING')
    return {'terminal':result,'scope':'HOT_ONLY'}


def source_trace(rows):
    pending, terminal = {}, {}
    last = 0
    for row in rows:
        stage=row.get('stage')
        need(stage!='M1_OFFER_TRACE_INCOMPLETE','OFFER_TRACE_LOSS')
        if stage not in ('M1_OFFER_RETURN','M1_OFFER_TERMINAL'):
            continue
        serial=int(row['serial'])
        need(serial==last+1,'OFFER_TRACE_SERIAL')
        last=serial
        key=tuple(row[k] for k in ('run','geometry','offer'))
        code=row['result']
        identity=tuple(row[k] for k in ('sample','hold','pulseTicks','sampleTicks','pulseNs',
                                        'sampleNs','frequency','pixelX','pixelY','material'))
        if stage=='M1_OFFER_TERMINAL':
            need(key in pending and key not in terminal and pending[key]==identity and code in (2,3,4,7)
                 and (int(row['root'])>0)==(code==2),'OFFER_TRACE_TERMINAL')
            terminal[key]=code
        elif code==0:
            need(key not in terminal and (key not in pending or pending[key]==identity),'OFFER_TRACE_REPLAY')
            pending[key]=identity
        else:
            need(type(code) is int and 1<=code<=8,'OFFER_TRACE_RETURN')
    need(pending and set(pending)==set(terminal),'OFFER_TRACE_PENDING')
    return {'offers':len(terminal),'accepted':sum(c==2 for c in terminal.values()),
            'scope':'INGRESS_ONLY'}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('emit','drive','compare','hot','ingress'))
    p.add_argument('input',nargs='?',type=Path)
    p.add_argument('--synthetic',action='store_true')
    a=p.parse_args()
    try:
        if a.mode=='drive':
            start=time.monotonic_ns()
            for action in plan():
                delay=start+action['scriptNs']-time.monotonic_ns()
                if delay>0:
                    time.sleep(delay/1e9)
                print(json.dumps(action),flush=True)
            return
        if a.mode=='emit':
            out=plan()
        else:
            need(a.input is not None,'INPUT_REQUIRED')
            raw=a.input.read_bytes()
            if a.mode=='ingress':
                out=source_trace([json.loads(l) for l in raw.splitlines() if l.strip()])
            else:
                evidence=json.loads(raw)
                out=compare(evidence,a.synthetic) if a.mode=='compare' else hot(evidence['rows'], evidence['accepted'])
    except (OSError,KeyError,TypeError,ValueError,struct.error) as err:
        p.exit(1,f'{err}\n')
    print(json.dumps(out,indent=2))


if __name__=='__main__':
    main()
