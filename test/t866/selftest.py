"""Offline coverage of T861 slots, retirement evidence and frozen numerical fixtures."""
from copy import deepcopy
import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

from check import OP17_ROLES, ROOT, S, convergence, laplace, load, matrix, need, physical_case, project, slots

T857=load('t857_cases','test/t857/selftest.py',S)


def suite():
    traces=[T857.fixture(push,'REST') for push in sorted(S.SLOTS,key=lambda p:p[5])]
    for trace in traces:
        trace['identity']['context']='ctx-1'
    ids=traces[0]['identity']
    modules={r:str(i)*64 for i,r in enumerate(OP17_ROLES,1)}
    counter=0
    for record in traces:
        raw=[]
        for i in range(5):
            c=dict(ids,push=record['push'],bindings=record['bindings'],qpc=counter,
                   queue='queue',commandBuffer='cb',stage='DISPATCH' if i%2==0 else 'BARRIER')
            if i%2==0:
                c.update(role=OP17_ROLES[i//2],moduleSHA=modules[OP17_ROLES[i//2]])
            else:
                c.update(srcStage='COMPUTE_SHADER',dstStage='COMPUTE_SHADER',srcAccess=['SHADER_WRITE'],
                         dstAccess=['SHADER_READ','SHADER_WRITE'],range=record['bindings']['WORK'])
            counter+=1
            raw.append(c)
        record['commandTrace']=raw
        record['reference']=None
    retires=[]
    for kind,names in [('BEFORE_SUBMIT',['PLAN','REVOKE','DISCARD']),
                       ('AFTER_SUBMIT',['PLAN','SUBMIT','REVOKE','FENCE','RETIRE'])]:
        ident=dict(ids,nonce=str(100+len(retires)))
        events=[dict(ident,stage=s,qpc=i,vkResult=0,fence='f1') for i,s in enumerate(names)]
        retires.append(dict(kind=kind,identity=ident,events=events,
                            queueSubmitCount=int(kind=='AFTER_SUBMIT'),acceptedRoot=None,
                            completionPublished=False,publicationBefore='unchanged',publicationAfter='unchanged',
                            resourcesRetired=True))
    return dict(synthetic=True,productionMapping=[16,820],logicalDispatches=820,
                physicalDispatches=850,op7Split=True,identity=ids,modules=modules,
                slots=traces,revocations=retires)


def projection(n):
    h=1/n
    a=.25*math.pi/(2*math.sin(math.pi*h/2)/h)
    p=[round(a*math.cos(math.pi*(i+.5)*h)*math.cos(math.pi*(j+.5)*h)*2**32)
       for j in range(n) for i in range(n)]
    return dict(n=n,mode='PRIVATE_PROJECT_DIAGNOSTIC',acceptedRoot=None,queueSubmit=0,fence=0,
                qpcSubmit=1,qpcFence=2,pressureSolveObserved=True,analyticWrite=False,
                piQ32=p,u32=[0]*(n*(n+1)),v32=[0]*(n*(n+1)),J=1)


def shear(n,nu,j):
    eigenvalue=4*(n*math.sin(math.pi/n))**2
    a=.25*(1+nu*eigenvalue/(120*j))**(-30*j)
    return dict(u32=[a*math.sin(2*math.pi*(y+.5)/n) for y in range(n) for _ in range(n+1)],
                v32=[0.]*(n*(n+1)))


def closed(n):
    sin,cos=math.sin,math.cos
    p=math.pi
    a=.25*math.exp(-.25)
    return dict(u32=[a*sin(p*i/n)**2*sin(2*p*(y+.5)/n) for y in range(n) for i in range(n+1)],
                v32=[-a*sin(2*p*(i+.5)/n)*sin(p*y/n)**2 for y in range(n+1) for i in range(n)],
                piQ32=[.125*math.exp(-.5)*cos(p*(i+.5)/n)*cos(p*(y+.5)/n)
                       for y in range(n) for i in range(n)])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emit',type=Path)
    args=parser.parse_args()
    positive=suite()
    coverage=slots(positive,True)
    need(len(coverage['slots'])==14 and coverage['reference']=='MISSING_PC_REFERENCE','COVERAGE')
    projections=[projection(n) for n in (16,32,64)]
    operator=[]
    for n in (32,64,128):
        eig=4*(n*math.sin(math.pi/n))**2
        words=[S.word(-.25*eig*math.sin(2*math.pi*(y+.5)/n)) for y in range(n) for _ in range(n)]
        operator.append(dict(n=n,scope='PRIVATE_OPERATOR',queueSubmit=0,fence=0,laplaceU32=words,
                             outputSHA=S.sha(S.encode(words))))
    orders_spatial=laplace(operator)
    if args.emit:
        args.emit.mkdir(parents=True,exist_ok=True)
        for name,content in (('slots.json',positive),('project.json',dict(synthetic=True,samples=projections)),
                             ('laplace.json',dict(synthetic=True,samples=operator))):
            (args.emit/name).write_text(json.dumps(content),encoding='utf-8')
    p=[project(e) for e in projections]
    orders=convergence(p,'pressureContinuousRMS',1.8)
    s=[physical_case('F03',shear(n,nu,j),{'shape':[n,n]},nu,30,j)
       for n in (32,64,128) for nu in (1/16,1/8) for j in (1,2,4)]
    f=[physical_case('F05',closed(n),{'shape':[n,n],'motion':m},1/64,30,4)
       for n in (32,64,128) for m in ('REST','RAMP_SPIN')]
    aggregate=matrix([dict(case=case,reference=row) for case,rows in (('F03',s),('F05',f)) for row in rows])
    need(not (aggregate['missingF03'] or aggregate['missingF05'])
         and aggregate['F03TimeThresholdSatisfied'] is True
         and aggregate['F03ContinuumMisses'], 'MATRIX_CLASSIFICATION')
    faults=[]
    for reason,change in (
        ('SLOT_COVERAGE',lambda a:a['slots'].pop()),
        ('ACTUAL_BARRIER',lambda a:a['slots'][1]['commandTrace'][1].update(dstAccess=['SHADER_READ'])),
        ('RECORDING_IDENTITY',lambda a:a['slots'][1]['commandTrace'][0].update(queue='other')),
        ('REVOKE_PRE_SUBMIT',lambda a:a['revocations'][0].update(queueSubmitCount=1)),
        ('REVOKE_IN_FLIGHT',lambda a:a['revocations'][1]['events'][3].update(fence='wrong')),
        ('REVOKE_PUBLISHED_OR_NOT_RETIRED',lambda a:a['revocations'][1].update(acceptedRoot='unexpected')),
    ):
        a=deepcopy(positive)
        change(a)
        reject(lambda:slots(a,True),reason)
        faults.append(reason)
    bad=projection(16)
    bad['piQ32']=[-i for i in bad['piQ32']]
    reject(lambda:project(bad),'F02_PRESSURE')
    bad['analyticWrite']=True
    reject(lambda:project(bad),'F02_PRIVATE')
    reject(lambda:physical_case('F03',shear(64,1/16,4),{'shape':[64,64]},1/8,30,4),'F03_BE_AMPLITUDE')
    bad=closed(128)
    bad['piQ32']=[0]*128**2
    reject(lambda:physical_case('F05',bad,{'shape':[128,128],'motion':'REST'},1/64,30,4),'F05_FINE_THRESHOLDS')
    faults+=['F02_PRESSURE','F02_PRIVATE','F03_BE_AMPLITUDE','F05_FINE_THRESHOLDS']
    damaged=deepcopy(operator)
    damaged[0]['laplaceU32'][0]^=1
    reject(lambda:laplace(damaged),'F03_OPERATOR_RAW')
    faults.append('F03_OPERATOR_RAW')
    baseline=subprocess.run([sys.executable,str(ROOT/'test/t837/references.py')],
                            cwd=ROOT,check=True,capture_output=True,text=True)
    print(json.dumps({'slots':coverage,'F02Orders':orders,'F03Cases':len(s),'F05Cases':len(f),
                      'matrix':aggregate,'F03Laplacian':orders_spatial,'negatives':faults,
                      'T837':json.loads(baseline.stdout)['baseline'],
                      'actualCandidate':'PENDING_T833'},indent=2))


def reject(fn, reason):
    try:
        fn()
    except ValueError as error:
        need(str(error)==reason,f'{reason}: {error}')
    else:
        raise AssertionError(reason)


if __name__=='__main__':
    main()
