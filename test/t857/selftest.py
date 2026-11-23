"""Generate OP17 write-set controls and rerun exact historical evidence."""
import argparse
from copy import deepcopy
from fractions import Fraction
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

from check import BUFFERS, ROOT, SEAL, SLOTS, TRACE_SEAL, check, delta, first_error, need, sha, snapshot, word


def fixture(push, mode, guard='', motion_bits=(), mms_bits=(), tick=120):
    _, scope, j, micro, _, ordinal = push
    tb = (2048, 10240)[scope]
    trace = tb+1024*(j+micro)+520
    freq = tb+64+64*({1:0,2:1,4:2}[j])
    ids = dict(pid=12, processStart='1234', owner=13, device='GPU1', generation='14',
               permit='15', nonce='4294967425')
    b = {n:[0]*s for n,s in zip(BUFFERS,(256,256,800,18688))}
    c, state = b['CTRL'], b['CANDIDATE']
    b['BASE'][18] = tick
    state[28] = word(2.)
    state[27] = word(1.)
    state[30] = dict(REST=0, UNIFORM=3, RAMP_SHORT=2, RAMP_END=2, MMS_ON=0, MMS_OFF=0)[mode]
    if mode.startswith('MMS'):
        state[204] = 3
        state[198], state[200] = (tick,tick+1) if mode == 'MMS_ON' else (0,tick)
    c[8:11] = [word(1.)]*3
    c[136:138] = [129,1]
    c[676], c[574], c[576] = word(.1), 98, 99
    c[688], state[160] = 92, 93
    if guard == 'FIELD':
        c[513:518] = [7,35,0,0,0]
        c[526],c[766] = 1,SEAL
    elif guard == 'SELECTED':
        c[518+scope] = j
    elif guard == 'SKIP':
        c[768+scope] = 1
    b['WORK'][1] = word(.5)
    b['WORK'][768:800] = [0xdead]*32
    layout = [0]*80
    layout[0:4] = [2,2,4,12]
    layout[5], layout[35] = 768,48
    identity = dict(identity=ids, push=push, bindings={
        n:dict(object=i+1,offset=0,bytes=len(b[n])*4) for i,n in enumerate(BUFFERS)})
    tag = 0x1717
    active = not guard
    stages = []
    status = 0

    def event(kind, **fields):
        stages.append(dict(identity, stage=kind, **fields))

    def fault(c, bits, function):
        nonlocal status
        for bit in bits:
            c[751] |= bit
            status |= bit
            if not c[514]:
                c[513:518] = [17,17,scope,j,micro]
                c[526], c[750], c[766] = 1, bit, SEAL
                c[733:737] = [129,1,function,1]
                c[737] = 812

    def seal(buf, phase, name, bits):
        buf['WORK'][768:800] = [tag,phase,129,1,*push,int(active),0,
                                int(bool(status)),status,0,0,0,0]+[0]*14
        event(name, after=snapshot(buf), mathBits=list(bits))

    m = deepcopy(b)
    c = m['CTRL']
    c[752], c[676] = ordinal, 0
    count = 0 if active else 1
    c[522+count] += 1
    c[freq+8+count] += 1
    if active:
        time = Fraction(tick,120)+Fraction(2*micro+1,240*j)
        c[629] = int(state[30] == 0 or mode == 'RAMP_END')
        if mode == 'UNIFORM':
            c[624:627] = [word(float(time)),word(1),0]
        elif mode == 'RAMP_SHORT':
            phase = math.pi*float(time)
            c[624:627] = [word((phase-math.sin(phase))/(2*math.pi)),
                          word(math.sin(phase/2)**2),word(math.pi/2*math.sin(phase))]
        elif mode == 'RAMP_END':
            c[624], c[629] = word(1), 1
        fault(c,motion_bits,3)
    else:
        c[trace:trace+8] = [17,32|dict(FIELD=8,SELECTED=2,SKIP=16)[guard],c[514],0,0,0,0,TRACE_SEAL]
    seal(m,1,'MOTION',motion_bits)
    event('BARRIER',dependency='COMPUTE_WRITE_TO_COMPUTE_READ_WRITE')

    s = deepcopy(m)
    c = s['CTRL']
    if active and mode == 'MMS_ON':
        t = float(Fraction(tick,120)+Fraction(2*micro+1,240*j))
        c[642:644] = [word(.25*math.exp(-t)),word(.125*math.exp(-2*t))]
    if active:
        fault(c,mms_bits,4)
    seal(s,2,'MMS',mms_bits)
    event('BARRIER',dependency='COMPUTE_WRITE_TO_COMPUTE_READ_WRITE')

    l = deepcopy(s)
    c, w = l['CTRL'], l['WORK']
    if active:
        at = 304
        k = word(.125)
        w[at] = k
        w[at+1:at+49] = c[544:592]
        w[at+64:at+84] = c[688:708]
        w[at+96:at+128] = state[160:192]
        if micro == 0 and tick % 120 == 0:
            for i,v in ((544,k),(574,0),(576,0),(578,k)):
                c[i] = v
        c[trace:trace+8] = [17,33,c[514],12,0,0,0,TRACE_SEAL]
    seal(l,3,'LEDGER',())
    event('FENCE',vkResult=0)
    ref = deepcopy(l)
    ref['WORK'][768:] = b['WORK'][768:]
    before = snapshot(b)
    return dict(identity, synthetic=True, events=stages, before=before, layoutWords=layout, privateWord=768,
                privateTag=tag, publicationBefore='base', publicationAfter='base', completionPublished=False,
                revoked=False, submitVkResult=0, mode=mode, reference=dict(push=push, layoutWords=layout,
                    inputSHA={n:v['sha256'] for n,v in before.items()}, after=snapshot(ref)))


def historical():
    data = json.loads((ROOT/'docs/architecture-T854-evidence-v1.json').read_bytes())
    results = []
    for arm in data['privateCaptures']:
        raw = (ROOT/arm['archive']).read_bytes()
        sizes = arm['segmentsBytes']
        need(len(raw) == 5*sum(sizes) and sha(raw) == arm['archiveSHA'], 'T854_ARCHIVE')
        start, before = 0,None
        for stage in arm['stages']:
            after = {}
            for name,size in zip(BUFFERS,sizes):
                after[name] = struct.unpack_from(f'<{size//4}I',raw,start)
                start += size
            c = after['CTRL']
            if before:
                need({n:sorted(d) for n,d in delta(before,after).items()} == stage['changesFromPrevious'],
                     'T854_RAW_DELTA')
            indices = (513,514,515,516,517,526,750,766)
            need([c[i] for i in indices] == stage['firstError']
                 and c[522:524] == (stage['logicalActive'],stage['logicalNoop'])
                 and c[755] == stage['auditMask'], 'T854_HISTORICAL_CTRL')
            before = after
        results.append({'case':arm['case'],'stages':5,'sha256':arm['archiveSHA']})
    return results


def suite():
    modes = ('REST','UNIFORM','RAMP_SHORT','RAMP_END','MMS_ON','MMS_OFF')
    rows = [fixture(s, modes[i%len(modes)], tick=240 if modes[i%6]=='RAMP_END' else 119 if i%2 else 120)
            for i,s in enumerate(sorted(SLOTS))]
    scope = sorted(SLOTS)[0]
    rows += [fixture(scope,'REST',guard=g) for g in ('SELECTED','SKIP','FIELD')]
    competition = fixture(scope,'MMS_ON',motion_bits=(2,1),mms_bits=(4,))
    rows += [competition, fixture(scope,'MMS_ON',mms_bits=(4,))]
    incomplete = deepcopy(rows[0])
    incomplete['reference'] = None
    cancelled = deepcopy(rows[0])
    cancelled['events'][3:] = [dict(cancelled['events'][0],stage='REVOKE'),cancelled['events'][-1]]
    cancelled.update(revoked=True,reference=None)
    rows += [incomplete,cancelled]
    answers = [check(r,True) for r in rows]
    need(answers.count('ACTIVE_MONOLITH_EQUAL') == 16 and answers.count('NOOP_MONOLITH_EQUAL') == 3
         and answers[-2:] == ['ACTIVE_WRITESETS_ONLY','REVOKED_RETIRED_NO_PUBLICATION'], 'EXPECTED_OUTCOMES')

    def change(row, event, buffer, index, value):
        snap = row['events'][event]['after']
        snap[buffer]['words'][index] = value
        row['events'][event]['after'] = snapshot({n:v['words'] for n,v in snap.items()})

    negatives = [
        ('OP17_SLOT', rows[0], lambda r:r.update(push=[17,0,4,3,0,999])),
        ('SCOPE_NONCE_OR_BINDINGS', rows[0], lambda r:r['events'][2].update(identity=dict(r['identity'],generation='55'))),
        ('PRIVATE_STAGE_NONCE_OR_PUSH', rows[0], lambda r:change(r,2,'WORK',770,0)),
        ('PRIVATE_STAGE_NONCE_OR_PUSH', rows[0], lambda r:change(r,2,'WORK',769,1)),
        ('MOTION_WRITESET', rows[0], lambda r:change(r,0,'CTRL',642,9)),
        ('MMS_WRITESET', rows[0], lambda r:change(r,2,'CTRL',642,9)),
        ('LEDGER_WRITESET', rows[0], lambda r:change(r,4,'WORK',304+49,9)),
        ('LEDGER_RAW_COPY', competition, lambda r:change(r,4,'WORK',304,0)),
        ('FIRST_ERROR_STAGE', competition, lambda r:change(r,0,'CTRL',750,1)),
        ('FIRST_ERROR_OVERWRITTEN', competition, lambda r:change(r,2,'CTRL',735,4)),
        ('MATH_ACCUMULATION', competition, lambda r:change(r,2,'CTRL',751,4)),
        ('REVOKED_CONSUMER_OR_RETIRE', rows[0], lambda r:r.update(revoked=True,reference=None)),
        ('PRIVATE_PUBLICATION', rows[0], lambda r:r.update(completionPublished=True)),
        ('BARRIER', rows[0], lambda r:r['events'][1].update(dependency='WRONG')),
        ('MOTION_BRANCH', rows[0], lambda r:change(r,0,'CTRL',629,0)),
        ('MONOLITH_WORD_DIFF', rows[0],
         lambda r:[change(r,i,'CTRL',820,123) for i in (0,2,4)]),
    ]
    rejected = []
    for reason,base,mutate in negatives:
        r = deepcopy(base)
        mutate(r)
        try:
            check(r,True)
        except ValueError as e:
            need(str(e) == reason, f'{reason}: {e}')
        else:
            raise AssertionError(reason)
        rejected.append(reason)
    # OP7 permits the later math call even when its field audit won the CAS.
    field = rows[16]['before']['CTRL']['words']
    drive = field.copy()
    drive[751] |= 4
    first_error(field, drive, 'OP7_DRIVE', [7,0,0,0,0,815], [4])
    drive[514] = 17
    try:
        first_error(field, drive, 'OP7_DRIVE', [7,0,0,0,0,815], [4])
    except ValueError as err:
        need(str(err) == 'FIRST_ERROR_OVERWRITTEN', str(err))
    else:
        raise AssertionError('OP7_FIRST_ERROR_OVERWRITTEN')
    rejected.append('OP7_FIRST_ERROR_OVERWRITTEN')
    return rows,answers,rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emit',type=Path)
    args = parser.parse_args()
    rows,answers,rejected = suite()
    if args.emit:
        args.emit.write_text(json.dumps(rows),encoding='utf-8')
    baseline={}
    for n,path in (('T844','test/t844/check.py'),('T837','test/t837/references.py')):
        p=subprocess.run([sys.executable,str(ROOT/path)],cwd=ROOT,check=True,capture_output=True,text=True)
        baseline[n]=json.loads(p.stdout)
    print(json.dumps(dict(synthetic=answers,negativeControls=rejected,historicalOP7=historical(),
                          baseline=baseline,actualOP17='PENDING_T833'),indent=2))


if __name__=='__main__':
    main()
