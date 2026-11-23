"""T841 diagnostic OP7 comparator. Snapshots are raw LE U32 buffers."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct
import sys
import math

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'test/t704'))
from check import EXPECTED

BUFFERS=('BASE','CANDIDATE','WORK','CTRL')
FIELD={522,676,752,755}
DRIVE=set(range(624,644))|set(range(820,828))
FAULT=set(range(513,518))|{526,750,766}|set(range(733,750))
PUSH=[7,0,0,0,0,815]
MODES=('REST','UNIFORM','RAMP_SHORT','RAMP_END','MMS_ON','MMS_OFF','FIELD_AND_DOMAIN_FAULT')
SEAL=0xc0de1311

def need(ok,reason):
    if not ok:
        raise ValueError(reason)

def encode(words):
    need(all(type(w) is int and 0<=w<2**32 for w in words),'U32')
    return struct.pack(f'<{len(words)}I',*words)

def snapshot(words):
    return {name:{'words':value,'sha256':hashlib.sha256(encode(value)).hexdigest()}
            for name,value in words.items()}

def decode(s,bindings):
    need(set(s)==set(BUFFERS),'MISSING_BUFFER')
    out={}
    for name in BUFFERS:
        words=s[name]['words']
        raw=encode(words)
        need(len(raw)==bindings[name]['bytes'] and hashlib.sha256(raw).hexdigest()==s[name]['sha256'],
             'SNAPSHOT_SIZE_OR_HASH')
        out[name]=words
    need(len(out['CTRL'])>=828 and min(len(out['BASE']),len(out['CANDIDATE']))>=256,'BUFFER_RANGE')
    return out

def float_word(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]

def raw_u64(words,at):
    return words[at]+(words[at+1]<<32)

def motion_inputs(before,mode):
    base,trial=before['BASE'],before['CANDIDATE']
    start=raw_u64(base,18)
    t0=raw_u64(trial,24)
    need(start>=t0,'TIME_ORIGIN')
    ticks=start-t0+1
    force=trial[204]==3 and raw_u64(trial,198)<=start<raw_u64(trial,200)
    if mode in ('MMS_ON','MMS_OFF'):
        need(trial[204]==3 and force==(mode=='MMS_ON'),'MMS_FORCE_WINDOW')
    expected={'REST':0,'UNIFORM':3,'RAMP_SHORT':2,'RAMP_END':2}
    if mode in expected:
        need(trial[30]==expected[mode],'MOTION_MODE')
    if mode in ('RAMP_SHORT','RAMP_END'):
        length=struct.unpack('<f',struct.pack('<I',trial[28]))[0]
        need(math.isfinite(length) and length>=2,'RAMP_DURATION')
        need((ticks>=120*length)==(mode=='RAMP_END'),'RAMP_BOUNDARY')
    return force

def words_diff(before,after):
    return {name:{i for i,(a,b) in enumerate(zip(before[name],after[name],strict=True)) if a!=b}
            for name in BUFFERS}

def scope_events(row):
    ids=('pid','processStart','owner','context','generation','permit','nonce')
    need(all(type(row['scope'][k]) is str and row['scope'][k].isascii() and
             row['scope'][k].isdecimal() and 0<int(row['scope'][k])<2**64 for k in ids),'SCOPE')
    bind=row['bindings']
    need(set(bind)==set(BUFFERS) and len({b['object'] for b in bind.values()})==4 and
         all(type(b['bytes']) is int and b['bytes']>0 and b['bytes']%4==0 and
             type(b['offset']) is int and b['offset']>=0 and b['offset']%4==0
             for b in bind.values()),'BINDING_LAYOUT')
    for e in row['events']:
        need(e['scope']==row['scope'] and e['bindings']==bind,'SCOPE_OR_BINDING_CHANGED')
        need(e['push']==PUSH,'PUSH')

def judge(row):
    need(row['mode'] in MODES,'MODE')
    scope_events(row)
    stages=[e['stage'] for e in row['events']]
    need(row['publicationBefore']==row['publicationAfter'],'PRIVATE_PUBLICATION')
    before=decode(row['before'],row['bindings'])
    force=motion_inputs(before,row['mode'])
    field=decode(row['field'],row['bindings'])
    first=words_diff(before,field)
    need(not any(first[n] for n in BUFFERS[:-1]) and first['CTRL']<=FIELD|FAULT,'FIELD_WRITESET')
    c0,c1=before['CTRL'],field['CTRL']
    need(c1[522]==c0[522]+1 and c1[523]==c0[523] and c1[752]==815,'LOGICAL_COUNT')
    if c0[514]:
        need(not first['CTRL']&FAULT,'FIELD_REWROTE_FIRST_ERROR')
    if c1[514]:
        need(c1[766]==SEAL and c1[513]==7,'FIELD_FIRST_ERROR')
    else:
        need(c1[755]==(c0[755]|6),'FIELD_AUDIT_MASK')
    if row['revoked']:
        need(stages==['FIELD','REVOKE'] and row['combined'] is None and
             row['reference'] is None and row['op8'] is None,'REVOKED_SECOND_ARM')
        return 'REVOKED_NO_PUBLICATION'
    need(stages==['FIELD','BARRIER','DRIVE','FENCE'] and row['submitVkResult']==row['fenceVkResult']==0,
         'EXECUTION_OR_BARRIER')
    need(set(row['op66'])=={'FIELD','DRIVE'} and
         all(r['words']==EXPECTED*2 and r['masks']==[0,0] and r['submit']==r['wait']==0 and
             r['moduleSHA']==row['modules'][role]
             for role,r in row['op66'].items()),'OP66')
    after=decode(row['combined'],row['bindings'])
    second=words_diff(field,after)
    legal=DRIVE|FAULT
    if not force:
        legal=legal-{642,643}
    need(not any(second[n] for n in BUFFERS[:-1]) and second['CTRL']<=legal,'DRIVE_WRITESET')
    c2=after['CTRL']
    if c1[514]:
        need(not (second['CTRL']&FAULT),'FIRST_ERROR_OVERWRITTEN')
    elif c2[514]:
        need(c2[514]==17 and c2[513]==7 and c2[766]==SEAL,'DRIVE_DOMAIN_ERROR')
    if row['reference'] is None:
        return 'WRITESETS_ONLY_NO_MONOLITH'
    ref=row['reference']
    need(ref['inputSHA']=={n:v['sha256'] for n,v in row['before'].items()} and ref['push']==PUSH,
         'REFERENCE_INPUT')
    need(decode(ref['output'],row['bindings'])==after,'MONOLITH_RAW_WORD_DIFF')
    if row['op8'] is None:
        return 'OP7_RAW_EQUAL_OP8_PENDING'
    op8=row['op8']
    next_state=decode(op8['split'],row['bindings'])
    need(op8['push']==[8,0,0,0,0,816] and op8['inputSHA']=={n:v['sha256'] for n,v in row['combined'].items()}
         and next_state==decode(op8['monolith'],row['bindings']),
         'OP8_STATE_TIME_DIFF')
    post=next_state['CANDIDATE']
    if c2[514] or not c2[518]:
        need(post==after['CANDIDATE'],'OP8_REJECT_MUTATION')
    else:
        advancing=c2[520]<50 and c2[762] not in (1,3)
        need(raw_u64(post,16)==(raw_u64(after['BASE'],16)+1)%(2**64) and
             raw_u64(post,18)==(raw_u64(after['BASE'],18)+advancing)%(2**64),'OP8_CLOCK')
    return 'PRIVATE_OP7_OP8_RAW_EQUAL'

def fixture(mode):
    scope={key:str(i+1) for i,key in enumerate(('pid','processStart','owner','context','generation','permit','nonce'))}
    before={n:[0]*(900 if n=='CTRL' else 256 if n in ('BASE','CANDIDATE') else 4) for n in BUFFERS}
    trial=before['CANDIDATE']
    trial[30]={'UNIFORM':3,'RAMP_SHORT':2,'RAMP_END':2}.get(mode,0)
    if trial[30]:
        trial[27:29]=[float_word(1),float_word(2)]
    if mode=='RAMP_END':
        before['BASE'][18]=239
    if mode in ('MMS_ON','MMS_OFF'):
        trial[204]=3
        trial[198:201]=[0,0,1] if mode=='MMS_ON' else [1,0,2]
    field=deepcopy(before)
    field['CTRL'][522]=1
    field['CTRL'][752]=815
    if mode=='FIELD_AND_DOMAIN_FAULT':
        for at,value in {513:7,514:35,526:1,766:SEAL}.items():
            field['CTRL'][at]=value
    else:
        field['CTRL'][755]=6
    after=deepcopy(field)
    after['CTRL'][624]=MODES.index(mode)+100
    if mode=='MMS_ON':
        after['CTRL'][642:644]=[101,102]
    bindings={n:{'object':i+1,'offset':0,'bytes':len(before[n])*4} for i,n in enumerate(BUFFERS)}
    events=[{'stage':stage,'scope':scope,'bindings':bindings,'push':PUSH}
            for stage in ('FIELD','BARRIER','DRIVE','FENCE')]
    result=snapshot(after)
    modules={'FIELD':'a'*64,'DRIVE':'b'*64}
    return dict(mode=mode,scope=scope,bindings=bindings,events=events,publicationBefore='base',
                publicationAfter='base',before=snapshot(before),field=snapshot(field),combined=result,
                reference=dict(inputSHA={n:v['sha256'] for n,v in snapshot(before).items()},
                               push=PUSH,output=deepcopy(result)),op8=None,revoked=False,
                submitVkResult=0,fenceVkResult=0,modules=modules,
                op66={r:dict(words=EXPECTED*2,masks=[0,0],moduleSHA=modules[r],submit=0,wait=0) for r in modules})

def selftest():
    positives={m:judge(fixture(m)) for m in MODES}
    bad=[]
    def mutation(name,func):
        row=deepcopy(fixture('FIELD_AND_DOMAIN_FAULT'))
        func(row)
        bad.append((name,row))
    def word(row,s,n,index,value):
        data={k:v['words'] for k,v in row[s].items()}
        data[n][index]=value
        row[s]=snapshot(data)
    mutation('SCOPE_OR_BINDING_CHANGED',lambda x:x['events'][2].update(bindings={}))
    mutation('LOGICAL_COUNT',lambda x:word(x,'field','CTRL',522,2))
    mutation('DRIVE_WRITESET',lambda x:word(x,'combined','WORK',0,1))
    mutation('FIRST_ERROR_OVERWRITTEN',lambda x:word(x,'combined','CTRL',514,17))
    mutation('DRIVE_WRITESET',lambda x:word(x,'combined','CTRL',642,1))
    mutation('PRIVATE_PUBLICATION',lambda x:x.update(publicationAfter='new'))
    mutation('MONOLITH_RAW_WORD_DIFF',lambda x:word(x,'combined','CTRL',820,1))
    mutation('REVOKED_SECOND_ARM',lambda x:x.update(revoked=True))
    mutation('OP66',lambda x:x['op66']['DRIVE'].update(masks=[0,1]))
    for expected,row in bad:
        try: judge(row)
        except ValueError as err: need(str(err)==expected,f'{expected}: {err}')
        else: raise AssertionError(expected)
    canceled=fixture('REST')
    canceled['events']=canceled['events'][:2]
    canceled['events'][1]['stage']='REVOKE'
    canceled.update(revoked=True,combined=None,reference=None)
    need(judge(canceled)=='REVOKED_NO_PUBLICATION','REVOKE_CONTROL')
    incomplete=fixture('REST')
    incomplete['reference']=None
    need(judge(incomplete)=='WRITESETS_ONLY_NO_MONOLITH','MISSING_REFERENCE')
    op8=fixture('REST')
    following=deepcopy(op8['combined'])
    op8['op8']={'push':[8,0,0,0,0,816],'inputSHA':{n:v['sha256'] for n,v in op8['combined'].items()},
               'split':following,'monolith':deepcopy(following)}
    need(judge(op8)=='PRIVATE_OP7_OP8_RAW_EQUAL','OP8_CONTROL')
    word(op8['op8'],'split','CANDIDATE',18,1)
    try: judge(op8)
    except ValueError as err: need(str(err)=='OP8_STATE_TIME_DIFF','OP8_NEGATIVE')
    else: raise AssertionError('OP8_STATE_TIME_DIFF')
    bad.append(('OP8_STATE_TIME_DIFF',op8))
    return dict(modeFixtures=positives,negativeControls=[code for code,_ in bad],
                revoked='REVOKED_NO_PUBLICATION',actualGPU='WAIT_T833',fixtureOnly=True)

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('capture',type=Path,nargs='?')
    p.add_argument('--emit',type=Path)
    a=p.parse_args()
    if a.emit:
        a.emit.write_text(json.dumps(fixture('REST'),indent=2)+'\n',encoding='utf8')
    print(json.dumps({'state':judge(json.loads(a.capture.read_bytes()))} if a.capture else selftest(),indent=2))
