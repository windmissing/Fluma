"""Compare only sealed, budgeted T692 replay originals from two complete arms."""
from pathlib import Path
import hashlib
import json
import struct
import re
import sys
from compare_actions import load
from verify import read

def hashes(directory):
    directory = Path(directory)
    launch = json.loads((directory/'launch.json').read_bytes())
    stem = directory / ('t693-replay-' + launch['attempt'])
    path = lambda suffix: Path(str(stem) + suffix)
    index = path('.root.ndjson').read_bytes()
    payload = path('.root.bin').read_bytes()
    completions = path('.completion.bin').read_bytes()
    assert len(index)+len(payload)+len(completions) <= 16*1024*1024, 'ROOT_BUDGET'
    *roots, terminal = map(json.loads,index.splitlines())
    assert roots and terminal == {'roots':len(roots),'payloadBytes':len(payload),'firstRejected':0}, 'ROOT_INCOMPLETE'
    assert len(completions)==len(roots)*1024, 'MISSING_COMPLETION'
    identities = []
    at = 0
    for i,r in enumerate(roots):
        assert r['result']==0 and r['sequence']==i+1 and r['offset']==at, 'ROOT_GAP_OR_REJECTION'
        assert r['bankRaw']==r['eventRaw']==0 and r['bankUnmapped']==1, 'BANK_READ'
        assert r['eventBytes']==256*r['eventEnd'] and 0<=r['eventStart']<=r['eventEnd'], 'EVENT_BOUND'
        assert not r['eventBytes'] or r['eventsUnmapped']==1, 'EVENT_READ'
        assert r['bankBytes']>0 and r['bankBytes']%4==0
        raw = completions[1024*i:1024*(i+1)]
        w = struct.unpack('<256I',raw)
        assert (w[27],w[33],w[51],w[52],w[53]) == tuple(r[k] for k in
             ('kind','bank','eventArena','eventStart','eventEnd')), 'COMPLETION_ROOT_KEY'
        assert w[4:8] == tuple(r[k] for k in ('nonceLo','nonceHi','runLo','runHi'))
        assert w[22:26] == tuple(r[k] for k in ('controlLo','controlHi','frameLo','frameHi'))
        assert w[35]==r['publication'], 'PUBLICATION_KEY'
        assert (w[18],w[19],w[20],w[21]) != (0,0,0,0), 'ZERO_COMMIT'
        bank = payload[at:at+r['bankBytes']]
        at += r['bankBytes']
        events = payload[at:at+r['eventBytes']]
        at += r['eventBytes']
        assert len(bank)==r['bankBytes'] and len(events)==r['eventBytes'], 'PAYLOAD_GAP'
        root_key = tuple(r[k] for k in ('sequence','kind','nonceLo','nonceHi','frameLo','frameHi',
                    'controlLo','controlHi','runLo','runHi','bank','publication','eventArena'))
        identities.append((root_key,raw,bank,events))
    assert at==len(payload) and {r['kind'] for r in roots}=={1,2}, 'ROOT_COVERAGE'
    return identities

def correlate(main, replay):
    arm = json.loads((Path(main)/'launch.json').read_bytes())['arm']
    assert load(main,arm)[-1]==load(replay,arm)[-1], 'REPLAY_INPUT_DIVERGENCE'

    def original(directory):
        directory = Path(directory)
        attempt = json.loads((directory/'launch.json').read_bytes())['attempt']
        events = [json.loads(line) for line in
                  (directory/f't496_log_{attempt}.ndjson').read_bytes().splitlines()]
        accepted = {}
        for event in events:
            if event.get('phase')!='ACCEPTED_OBSERVED':
                continue
            detail=event['detail']
            match=re.match(r'accepted=(\d+) source=(\d+) physics=(\d+) nonce=(\d+) ',detail)
            assert match,'ACCEPTED_ROW'
            key=tuple(map(int,match.groups()[1:]))
            assert key not in accepted
            accepted[key]=detail
        maps={}
        calls={}
        mapped_by_thread={}
        clean_unmap=set()
        for seq,_,_,tid,fn,edge,detail in read(str(directory/f'gltrace-{attempt}.ndjson.t693')):
            if fn==418 and edge in (1,2):
                record=calls.setdefault((tid,seq),{})
                record[edge]=detail
            if fn==419 and edge==2 and tid in mapped_by_thread:
                key=mapped_by_thread.pop(tid)
                assert detail.endswith(b' return=1'),'ORIGINAL_UNMAP_FAILED'
                clean_unmap.add(key)
            if edge!=3:
                continue
            call=calls.get((tid,seq),{})
            assert b'length=1024' in call.get(1,b'') and b'return=' in call.get(2,b'')
            assert not call[2].endswith(b'return=0'),'ORIGINAL_MAP_NULL'

            match=re.search(rb'sha256=([0-9a-f]{64}) hex=([0-9a-f]{2048})$',detail)
            assert match,'MAP_PAYLOAD'
            raw=bytes.fromhex(match[2].decode())
            assert hashlib.sha256(raw).hexdigest().encode()==match[1]
            w=struct.unpack('<256I',raw)
            if w[0:4]!=(1381192263,2,256,3235823838):
                continue
            key=(w[18]|w[19]<<32,w[20]|w[21]<<32,w[4]|w[5]<<32)
            assert key not in maps,'DUPLICATE_MAPPED_ROOT'
            assert tid not in mapped_by_thread,'ORIGINAL_MAP_WITH_NO_UNMAP'
            mapped_by_thread[tid]=key
            maps[key]=raw
        assert clean_unmap==maps.keys(), 'ORIGINAL_UNMAP_WITNESS'
        final = next(e['detail'] for e in events if e.get('phase')=='PRE_QUIT_SNAPSHOT')
        progress = next(e['detail'] for e in events if e.get('phase')=='PROGRESS_RAW')
        progress = json.loads('{' + progress[1:] + '}')
        for field in ('pageCallbackNs','pageReceivedNs'):
            del progress[field]
        return accepted,maps,(final,progress)
    a,original_maps,main_drain=original(main)
    b,_,replay_drain=original(replay)
    assert main_drain==replay_drain, 'REPLAY_DRAIN_DIFFERS'
    assert a==b, 'REPLAY_ACCEPTED_LEDGER_OR_TOKEN_DIFFERS'
    replay_maps={}
    for _,raw,_,_ in hashes(replay):
        w=struct.unpack('<256I',raw)
        key=(w[18]|w[19]<<32,w[20]|w[21]<<32,w[4]|w[5]<<32)
        assert key not in replay_maps
        replay_maps[key]=raw
    assert a.keys()==original_maps.keys()==replay_maps.keys(), 'UNBOUND_ROOT_KEY'
    assert original_maps==replay_maps, 'ORIGINAL_MAP_VS_ACCEPTED_BYTES'
    return len(replay_maps)

if __name__=='__main__':
    off,on,*main=sys.argv[1:]
    assert load(off,'D1_OFF')==load(on,'D1_ON'), 'REPLAY_INPUT_MISMATCH'
    a,b=hashes(off),hashes(on)
    assert a==b, 'REPLAY_RAW_BYTES_DIFFER'
    correlated = [correlate(m,r) for m,r in zip(main,(off,on))]
    print(json.dumps({'rootCount':len(a),'replayByteMatch':True,'correlatedWithMain':correlated,
          'requiresMainArmCorrelation':True,'productPASS':False}))
