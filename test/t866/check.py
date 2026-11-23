"""T861: validate complete candidate slot coverage and independent field cases."""
import argparse
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[2]


def load(name,path,check_module=None):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    mod=importlib.util.module_from_spec(spec)
    if check_module is None:
        spec.loader.exec_module(mod)
    else:
        previous=sys.modules.get('check')
        sys.modules['check']=check_module
        try:
            spec.loader.exec_module(mod)
        finally:
            if previous is None:
                del sys.modules['check']
            else:
                sys.modules['check']=previous
    return mod


S=load('t857','test/t857/check.py')
F=load('t837','test/t837/check.py',load('t704','test/t704/check.py'))
need=S.need
OP17_ROLES=('OP17_PRIVATE_MOTION','OP17_PRIVATE_MMS','OP17_PRIVATE_LEDGER')
IDENTITY=('nonce','owner','context','generation','permit')


def owner_token(row):
    return tuple(str(row[k]) for k in IDENTITY)


def revoke(row):
    events=row['events']
    ident=owner_token(row['identity'])
    need(all(owner_token(e)==ident for e in events)
         and [e['qpc'] for e in events]==sorted(e['qpc'] for e in events),'REVOKE_EPOCH')
    names=[r['stage'] for r in events]
    if row['kind']=='BEFORE_SUBMIT':
        need(names==['PLAN','REVOKE','DISCARD'] and row['queueSubmitCount']==0,'REVOKE_PRE_SUBMIT')
    else:
        need(row['kind']=='AFTER_SUBMIT' and names==['PLAN','SUBMIT','REVOKE','FENCE','RETIRE']
             and row['queueSubmitCount']==1 and events[1]['vkResult']==events[3]['vkResult']==0
             and events[1]['fence']==events[3]['fence']==events[4]['fence'], 'REVOKE_IN_FLIGHT')
    need(row['acceptedRoot'] is None and row['completionPublished'] is False
         and row['publicationBefore']==row['publicationAfter'] and row['resourcesRetired'] is True,
         'REVOKE_PUBLISHED_OR_NOT_RETIRED')
    return row['kind']


def slots(e, synthetic=False):
    need(e['synthetic'] is synthetic,'EVIDENCE_MODE')
    need(e['productionMapping']==[16,820] and e['logicalDispatches']==820
         and e['physicalDispatches'] in (848,850),'DISPATCH_ACCOUNTING')
    need(e['physicalDispatches']==820+28+2*int(e['op7Split'])
         and isinstance(e['op7Split'],bool),'DISPATCH_ACCOUNTING')
    need([tuple(r['push']) for r in e['slots']]==sorted(S.SLOTS,key=lambda s:s[5]),'SLOT_COVERAGE')
    ident=owner_token(e['identity'])
    phases={}
    last_qpc=-1
    for r in e['slots']:
        need(owner_token(r['identity'])==ident,'SLOT_ROOT')
        state=S.check(r,synthetic)
        need(state.startswith(('ACTIVE_','NOOP_')),'SLOT_NOT_RETIRED')
        chain=r['commandTrace']
        need([c['stage'] for c in chain]==['DISPATCH','BARRIER','DISPATCH','BARRIER','DISPATCH']
             and len({c['commandBuffer'] for c in chain})==1 and len({c['queue'] for c in chain})==1
             and all(owner_token(c)==ident and c['push']==r['push'] and c['bindings']==r['bindings']
                     for c in chain),'RECORDING_IDENTITY')
        ticks=[c['qpc'] for c in chain]
        need(last_qpc<=ticks[0] and ticks==sorted(ticks),'RECORDING_ORDER')
        last_qpc=ticks[-1]
        for role,dispatch in zip(OP17_ROLES,chain[::2]):
            need(dispatch['role']==role and dispatch['moduleSHA']==e['modules'][role],'ROLE_SHA')
        for barrier in chain[1::2]:
            covers_work = (barrier.get('range')==r['bindings']['WORK']
                           or barrier.get('range')=='GLOBAL_MEMORY')
            need(barrier['srcStage']==barrier['dstStage']=='COMPUTE_SHADER'
                 and barrier['srcAccess']==['SHADER_WRITE']
                 and set(barrier['dstAccess'])=={'SHADER_READ','SHADER_WRITE'}
                 and covers_work,'ACTUAL_BARRIER')
        phases[str(r['push'][5])]=state
    checks=[revoke(r) for r in e['revocations']]
    need(sorted(checks)==['AFTER_SUBMIT','BEFORE_SUBMIT'],'REVOKE_COVERAGE')
    need(len({owner_token(r['identity']) for r in e['revocations']}|{ident})==3,'REVOKE_ROOT_REUSED')
    return {'slots':phases,'reference':'MONOLITH_WORDS_EQUAL'
            if all(s.endswith('_EQUAL') for s in phases.values()) else 'MISSING_PC_REFERENCE',
            'revocations':checks,'state':'SYNTHETIC_PASS' if synthetic else 'CANDIDATE_SLOT_EVIDENCE'}


def rms(values):
    return math.sqrt(math.fsum(x*x for x in values)/len(values))


def project(e):
    n=e['n']
    need(n in (16,32,64) and e['mode']=='PRIVATE_PROJECT_DIAGNOSTIC' and e['acceptedRoot'] is None
         and e['qpcSubmit']<=e['qpcFence'] and e['queueSubmit']==e['fence']==0
         and e['pressureSolveObserved'] is True and e['analyticWrite'] is False,'F02_PRIVATE')
    h=1/n
    k=math.pi
    amp=.25*k/(2*math.sin(k*h/2)/h)
    raw=e['piQ32']
    need(len(raw)==n*n and all(type(v) is int and -(2**63)<=v<2**63 for v in raw)
         and abs(sum(raw))<=n*n//2,'F02_GAUGE')
    p=[v/2**32 for v in raw]
    ref=[amp*math.cos(k*(i+.5)*h)*math.cos(k*(j+.5)*h) for j in range(n) for i in range(n)]
    err=[a-b for a,b in zip(p,ref)]
    u,v=e['u32'],e['v32']
    need(len(u)==len(v)==n*(n+1),'F02_VELOCITY_LAYOUT')
    vel=[F.float_word(w) for w in u+v]
    need(e['J'] in (1,2,4) and all(math.isfinite(x) for x in vel) and max(map(abs,vel))<=2**-12
         and rms(vel)<=2**-16/e['J'],'F02_PROJECTED_VELOCITY')
    trunc=abs(amp-.25)/2
    need(rms(err)<=min(2**-16,trunc/16) and max(map(abs,err))<=2**-12,'F02_PRESSURE')
    continuous=[a-b*.25/amp for a,b in zip(p,ref)]
    return {'n':n,'pressureRMS':rms(err),'pressureMax':max(map(abs,err)),
            'pressureContinuousRMS':rms(continuous)}


def physical_case(case, fields, view, nu, accepted, j):
    n,m=view['shape']
    need(n==m and n in (32,64,128) and accepted==30,'PHYSICAL_FIXTURE')
    res=F.field_reference(case,fields,view,nu,accepted)
    if case=='F03':
        need(j in (1,2,4) and nu in (1/16,1/8),'F03_INPUT')
        amp=res['amplitude']
        a_discrete=res['discreteByJ'][str(j)]
        a_cont=.25*math.exp(-nu*(2*math.pi)**2/4)
        steps=30*j
        gamma=64*steps*2**-24
        need(abs(amp-a_discrete)<=steps*2**-16/j + gamma/(1-gamma)*.25,'F03_BE_AMPLITUDE')
        ref=[a_cont*math.sin(2*math.pi*(y+.5)/n) for y in range(n) for _ in range(n+1)]
        u,v=fields['u32'],fields['v32']
        err=max(abs(u0-r) for u0,r in zip(u,ref,strict=True))
        energy_ref=sum(x*x for x in ref)
        energy=sum(x*x for x in u)+sum(y*y for y in v)
        velocity_error=max(err,max(map(abs,v)))/a_cont
        energy_error=abs(energy-energy_ref)/energy_ref
        semidiscrete=.25*math.exp(-nu*4*(n*math.sin(math.pi/n))**2/4)
        return dict(res,n=n,j=j,nu=nu,continuousError=err,velocityRelMax=velocity_error,
                    energyRel=energy_error,timeError=abs(amp-semidiscrete),
                    continuousThresholdsSatisfied=velocity_error<=1/64
                    and energy_error<=1/32)
    need(case=='F05' and nu==1/64 and j==4 and view['motion'] in ('REST','RAMP_SPIN'),'F05_INPUT')
    need(n!=128 or res['finestThresholdsSatisfied'] is True,'F05_FINE_THRESHOLDS')
    return dict(res,n=n,motion=view['motion'])


def convergence(values, metric, threshold):
    ordered=sorted(values,key=lambda r:r['n'])
    need(len({r['n'] for r in ordered})==len(ordered),'MATRIX_DUPLICATE')
    orders=[]
    for a,b in zip(ordered,ordered[1:]):
        x,y=a[metric],b[metric]
        if x==y==0:
            continue
        need(x>0 and y>0,'CONVERGENCE_UNDEFINED')
        orders.append(math.log(x/y)/math.log(b['n']/a['n']))
    need(all(k>=threshold for k in orders),'SPACE_ORDER')
    return orders


def laplace(samples):
    """Measure the periodic shear operator from its original sampled output."""
    errors=[]
    for row in samples:
        n=row['n']
        need(n in (32,64,128) and row['scope']=='PRIVATE_OPERATOR' and row['queueSubmit']==row['fence']==0,
             'F03_OPERATOR_EVIDENCE')
        words=row['laplaceU32']
        need(len(words)==n*n and S.sha(S.encode(words))==row['outputSHA'],'F03_OPERATOR_RAW')
        k=2*math.pi
        reference=[-.25*k*k*math.sin(k*(y+.5)/n) for y in range(n) for _ in range(n)]
        values=list(map(F.float_word,words))
        need(all(math.isfinite(v) for v in values),'F03_OPERATOR_NONFINITE')
        errors.append(dict(n=n,rms=rms([a-b for a,b in zip(values,reference)])))
    return {'samples':errors,'spaceOrders':convergence(errors,'rms',1.8),
            'matrixComplete':sorted(x['n'] for x in errors)==[32,64,128]}


def full(manifest, archive, receipt):
    e=json.loads(manifest.read_bytes())
    config,raw,digest,grid=F.tail_from_archive(archive,receipt)
    need(e['archiveSHA']==digest and all(e['view'][a]==grid[b] for a,b in
         (('u32','u'),('v32','v'),('piQ32','piQ32'))),'FULL_GRID_RAW')
    accepted=F.inspect(e,config,raw)
    need(accepted['state']=='CASE_REFERENCE_PENDING' and e['case'] in ('F03','F05'),'FULL_ROOT_GATE')
    fields, units=F.field_view(e['view'],config)
    tail=struct.unpack('<256I',raw)
    return {'state':'FIELD_SAMPLE_VALID_MATRIX_PENDING','case':e['case'],'units':units,
            'reference':physical_case(e['case'],fields,e['view'],units['nu'],S.u64(tail,34),tail[36])}


def matrix(samples):
    shear=[r['reference'] for r in samples if r['case']=='F03']
    closed=[r['reference'] for r in samples if r['case']=='F05']
    f03={(r['n'],r['j'],r['nu']):r for r in shear}
    f05={(r['n'],r['motion']):r for r in closed}
    need(len(f03)==len(shear) and len(f05)==len(closed),'MATRIX_DUPLICATE')
    nx=(32,64,128)
    js=(1,2,4)
    nu=(1/16,1/8)
    motions=('REST','RAMP_SPIN')
    missing_f03=[k for k in ((n,j,v) for n in nx for j in js for v in nu) if k not in f03]
    missing_f05=[k for k in ((n,m) for n in nx for m in motions) if k not in f05]
    time_orders={}
    for n in nx:
        for v in nu:
            if all((n,j,v) in f03 for j in js):
                errs=[f03[n,j,v]['timeError'] for j in js]
                need(all(x>0 for x in errs),'F03_TIME_ORDER_UNDEFINED')
                time_orders[str((n,v))]=[math.log2(a/b) for a,b in zip(errs,errs[1:])]
    space_orders={}
    for m in motions:
        if all((n,m) in f05 for n in nx):
            space_orders[m]=convergence([f05[n,m] for n in nx],'velocityRelRMS',.8)
    return {'missingF03':missing_f03,'missingF05':missing_f05,'F03TimeOrders':time_orders,
            'F03TimeThresholdSatisfied':all(x>=.8 for xs in time_orders.values() for x in xs)
            if time_orders else None,
            'F03ContinuumMisses':[(r['n'],r['j'],r['nu']) for r in shear
                                 if not r['continuousThresholdsSatisfied']],
            'F05SpaceOrders':space_orders,'F03LaplaceSpatialOrder':'PENDING_RAW_OPERATOR_CONTROL'}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('slots','project','full','matrix','laplace'))
    p.add_argument('input',type=Path)
    p.add_argument('--archive',type=Path)
    p.add_argument('--receipt',type=Path)
    p.add_argument('--synthetic',action='store_true')
    a=p.parse_args()
    try:
        if a.mode=='full':
            need(a.archive and a.receipt,'FULL_INPUT_REQUIRED')
            out=full(a.input,a.archive,a.receipt)
        elif a.mode=='matrix':
            items=json.loads(a.input.read_bytes())
            need(isinstance(items,list) and items,'MATRIX_INPUT')
            out=matrix([full(*(Path(x[key]) for key in ('manifest','archive','receipt'))) for x in items])
        else:
            e=json.loads(a.input.read_bytes())
            if a.mode=='slots':
                out=slots(e,a.synthetic)
            elif a.mode=='laplace':
                need(e['synthetic'] is a.synthetic and e['samples'],'EVIDENCE_MODE')
                out=laplace(e['samples'])
            else:
                need(e['synthetic'] is a.synthetic,'EVIDENCE_MODE')
                samples=[project(row) for row in e['samples']]
                out={'samples':samples,'spaceOrders':convergence(samples,'pressureContinuousRMS',1.8),
                     'state':'SYNTHETIC_PASS' if a.synthetic else 'PRIVATE_PROJECT_SAMPLES',
                     'matrixComplete':sorted(x['n'] for x in samples)==[16,32,64]}
    except (OSError,KeyError,TypeError,ValueError,struct.error) as err:
        p.exit(1,f'{err}\n')
    print(json.dumps(out,indent=2))


if __name__=='__main__':
    main()
