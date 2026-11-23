"""Independent T880 ingress and T881 drain/clock evidence checks."""
from hashlib import sha256
import struct

GAP = 33_333_333
DEADLINE = 250_000_000
Q20 = 1 << 20
DRAIN_FIELDS = ('offers','inFlight','leases','pending','active','pins','writers','readers','credits')


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def source_mass(delta):
    return (8 * delta * Q20) // 1_000_000_000


def input_trace(events):
    """Reference reservation ledger built from actual adjacent producer inputs."""
    hold = None
    current = None
    waiting = None
    claimed = None
    active = pending = carry = accepted_q = revoked_q = pulses_q = 0
    terminals = {}
    used_holds = set()
    admitted = 0
    last_ns = -1

    def finish(interval, code):
        if interval is not None:
            need(interval[0] not in terminals,'DUPLICATE_TERMINAL')
            terminals[interval[0]]=code

    def revoke():
        nonlocal hold,current,waiting,pending,active,revoked_q,carry
        finish(waiting,'REVOKED')
        revoked_q+=pending+active
        hold=current=waiting=None
        pending=active=carry=0

    for event in events:
        kind, now = event['kind'], event['ns']
        need(type(now) is int and now >= last_ns,'CLOCK_ORDER')
        last_ns=now
        if kind=='DOWN':
            need(hold is None and event['hold']>0 and event['hold'] not in used_holds,'DOWN_SCOPE')
            hold=event['hold']
            used_holds.add(hold)
            current=None
        elif kind=='PULSE':
            need(event['grantable'] in (True,False),'PULSE_FLAG')
            if event['hold']!=hold or hold is None:
                continue
            if current is not None:
                delta=now-current['ns']
                native_gap=event['nativeNs']-current['nativeNs']
                if not (0<delta<=GAP and 0<native_gap<=GAP and event['id']==current['id']+1):
                    revoke()
                    continue
                if event['grantable']:
                    tokens,carry=divmod(carry+120*native_gap,1_000_000_000)
                    room=min(tokens,4-pending)
                    pending+=room
                    pulses_q+=tokens
                    revoked_q+=tokens-room   # Explicitly shed excess while a grant is active.
                    finish(waiting,'SUPERSEDED')
                    waiting=(event['id'],hold,now,delta)
            current=event
        elif kind=='GRANT':
            need(active==0,'EARLY_GRANT')
            active,pending=pending,0
        elif kind=='QUANTUM':
            need(active>0,'QUANTUM_WITHOUT_GRANT')
            active-=1
            accepted_q+=1
        elif kind=='TAKE':
            need(active==0 and claimed is None,'ILLEGAL_BOUNDARY')
            if waiting is not None:
                if now-waiting[2]>GAP:
                    finish(waiting,'EXPIRED')
                else:
                    claimed=waiting
                waiting=None
            need(event['take']==(claimed[0] if claimed else None),'TAKE_WATERMARK')
        elif kind in ('UP','FOCUS_LOST','RESIZE','DEVICE_LOST','MATERIAL_SWITCH'):
            revoke()
        elif kind=='SOURCE_TAIL':
            need(claimed is not None and event['id']==claimed[0],'TAIL_WITHOUT_CLAIM')
            valid=(claimed[1]==hold and event['submitNs']<=event['fenceNs']<=now
                   and event['submitNs']>=claimed[2]
                   and now-event['submitNs']<=DEADLINE and event['fenceResult']==0)
            mass=source_mass(claimed[3]) if valid else 0
            need(event['admittedQ20']==mass and event['physicsSteps']==0,'SOURCE_LEDGER')
            admitted+=mass
            finish(claimed,'ACCEPTED' if valid else 'REVOKED_OR_LATE')
            claimed=None
        else:
            raise ValueError('INPUT_EVENT')
        need(accepted_q+revoked_q+active+pending==pulses_q,'QUANTUM_CONSERVATION')
    need(waiting is None and claimed is None,'UNFINISHED_RESERVATION')
    return dict(admittedQ20=admitted,acceptedQuanta=accepted_q,revokedOrShedQuanta=revoked_q,
                terminals=terminals,pending=pending,active=active,carry=carry)


def drain(rows):
    names=[x['stage'] for x in rows]
    need(names in (['END_INTAKE','FENCE_SIGNALED','OWNER_POLL','WSI_RETIRE','ACCOUNT','DRAIN_DONE','CLOSE'],
                   ['END_INTAKE','FENCE_PENDING','FENCE_SIGNALED','OWNER_POLL','WSI_RETIRE','ACCOUNT','DRAIN_DONE','CLOSE']),
         'DRAIN_ORDER')
    keys=('run','owner','generation','device')
    need(all(all(r[k]==rows[0][k] for k in keys) for r in rows)
         and [r['qpc'] for r in rows]==sorted(r['qpc'] for r in rows),'DRAIN_SCOPE')
    by={r['stage']:r for r in rows}
    signaled,poll=by['FENCE_SIGNALED'],by['OWNER_POLL']
    need(signaled['vkResult']==0 and signaled['fence']==poll['fence']
         and signaled['nonce']==poll['nonce'] and poll['tombstone'] is True
         and poll['acceptedRoot'] is None and poll['ownerIdle'] is True,'FENCE_OWNER_RETIRE')
    if 'FENCE_PENDING' in by:
        old=by['FENCE_PENDING']
        need(old['vkResult']==2 and all(old[k]==signaled[k] for k in ('fence','nonce')),'DRAIN_PENDING_FENCE')
    need(by['WSI_RETIRE']['vkResult']==0,'WSI_RETIRE')
    a=by['ACCOUNT']
    need(a['generated']==a['accepted']+a['revoked']+a['shed']
         and a['finalOfferSerial']==rows[0]['lastOfferSerial'] and a['traceLoss']==0,'DRAIN_LEDGER')
    done=by['DRAIN_DONE']
    need(all(done[k]==0 for k in DRAIN_FIELDS) and done['acceptedLate']==0 and done['retired'] is True,
         'DRAIN_RESOURCES')
    return 'DRAINED_AFTER_PENDING' if 'FENCE_PENDING' in by else 'DRAINED_ALREADY_SIGNALED'


def sha_words(words):
    return sha256(struct.pack(f'<{len(words)}I',*words)).hexdigest()


def clock_pair(left, right, expected):
    need(left['mode']==right['mode']=='OFF' and left['run']!=right['run']
         and left['frozenContract']==right['frozenContract'],'OFF_PAIR_IDENTITY')
    for arm in (left,right):
        need([row['action'] for row in arm['actions']]==expected,'ACTION_TABLE')
        need(len({row['terminalId'] for row in arm['actions']})==52,'ACTION_TERMINAL_ID')
        need(drain(arm['drain']).startswith('DRAINED'),'DRAIN')
        for row in arm['actions']:
            need(arm['readyQpc']<=row['producerQpc']<=row['receiptQpc']<=row['terminalQpc']
                 and row['terminal'] in ('ACCEPTED','EXPIRED','REVOKED','OFF_NOOP'),'ACTION_ACTUAL_TERMINAL')
            if row['action']['kind']=='SOURCE' and row['terminal']=='ACCEPTED':
                need(0<row['deltaNs']<=GAP and row['q20']==source_mass(row['deltaNs'])
                     and len(row['command'])==32 and row['commandSHA']==sha_words(row['command']),
                     'SOURCE_ACTUAL_WORDS')
        ticks=[r['producerQpc'] for r in arm['actions']]
        need(ticks==sorted(ticks) and arm['qpcFrequency']>0,'OFF_CLOCK')
    for a,b in zip(left['actions'],right['actions'],strict=True):
        for key in ('terminal','deltaNs','q20','acceptedPulse'):
            if a.get(key)!=b.get(key):
                return dict(state='CLOCK_EQUIVALENCE_UNPROVEN',action=a['action']['actionId'],field=key,
                            left=a.get(key),right=b.get(key),leftSHA=a.get('commandSHA'),rightSHA=b.get('commandSHA'))
        for key in ('producerQpc','receiptQpc','terminalQpc'):
            x,y=a[key]-left['readyQpc'],b[key]-right['readyQpc']
            if x*right['qpcFrequency']!=y*left['qpcFrequency']:
                return dict(state='CLOCK_EQUIVALENCE_UNPROVEN',action=a['action']['actionId'],field=key,
                            left=x,right=y,leftSHA=a.get('commandSHA'),rightSHA=b.get('commandSHA'))
    return dict(state='OFF_CLOCK_MATCH_FOUR_ARMS_PENDING')
