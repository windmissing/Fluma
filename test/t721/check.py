"""T721 offline capability join and historical partial decode."""
import argparse
from fractions import Fraction
import json
import math
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'test/t704'))
from check import capability, EXPECTED, MINIMUM
from analyze import analyze, page

def load(path):
    return json.loads(Path(path).read_bytes())

def u64(value):
    if isinstance(value,str) and value.isascii() and value.isdecimal():
        n=int(value)
        if 0<=n<2**64 and str(n)==value:
            return n
    raise ValueError('INVALID_WIDE_ID')

def decide(receipt,status):
    if receipt['result']!='ANCHORS_EQUAL_NOT_ATOMIC':
        return 'ANCHOR_MISMATCH'
    boot,ticket=map(u64,(receipt['controlBoot'],receipt['configureAttempt']))
    command=status['lastCommand']
    if not ticket or not boot or command['api']!='g13Configure' or (
       u64(command['boot']),u64(command['attempt']),u64(receipt['selectedTicket']),
       u64(receipt['controlTicket']))!=(boot,ticket,ticket,ticket):
        return 'IDENTITY_MISMATCH'
    raw=receipt['capability']
    if raw is None:
        return 'NOT_MEASURED'
    if isinstance(raw['maxBlock'],str):
        raw=dict(raw,maxBlock=u64(raw['maxBlock']))
    try:
        result=capability(raw)
    except (AssertionError,KeyError,TypeError):
        return 'INVALID_RAW_VALUES'
    if result['compute']!='QUALIFIED':
        return result['compute']
    phases=('CAPABILITY','INIT_FP32','INIT_FP32_RESULT')
    rows=[r for r in receipt['stages'] if r['phase'] in phases and r['edge']=='EXIT']
    if [r['phase'] for r in rows]!=list(phases):
        return 'MISSING_QUERY_STAGE'
    if any((u64(r['boot']),u64(r['attempt']),u64(r['ticket']))!=(boot,ticket,ticket)
           or r['rawGL']!=0 for r in rows):
        return 'STAGE_IDENTITY_OR_GL'
    owners={tuple(u64(r[k]) for k in ('context','window','tid')) for r in rows}
    rev=[u64(r['seq']) for r in rows]
    if len(owners)!=1 or not all(next(iter(owners))) or not rev[0]<rev[1]<rev[2]:
        return 'STAGE_SCOPE_OR_ORDER'
    return 'SYNCHRONIZED_FP32_EVIDENCE'

def historical():
    root=ROOT/'test/reports/t491/t654b/t672out'
    status=load(root/'r1-status.json')
    raw,meta=zip(*(page(root/f'r1-page{i}.json',i) for i in range(4)))
    boot,ticket=struct.unpack_from('<QQ',raw[3],4096+56)
    assert (boot,ticket)==tuple(u64(status['lastCommand'][k]) for k in ('boot','attempt'))
    merged=b''.join(raw)
    row=next(merged[i:i+256] for i in range(4096,20480,256)
             if merged[i+128:i+160].split(b'\0')[0]==b'CAPABILITY'
             and merged[i+160:i+176].split(b'\0')[0]==b'EXIT')
    assert struct.unpack_from('<q',row,88)[0]==status['native']['capability']['rawGL']==1280
    return {'boot':str(boot),'ticket':str(ticket),'statusAPIInvocation':status['attemptId'],
            'rawGL':1280,'result':'HISTORICAL_CAPABILITY_FAILED','pages':meta,
            'missing':[n for n in ('configure.json','r1-anchor-before.json','r1-anchor-after.json') if not (root/n).exists()]}

def numerical_gates(j, values):
    if type(j) is not int or j not in (1,2,4) or values is None:
        return 'NOT_MEASURED'
    limits={'divergenceRMS':2**-14,'divergenceMax':2**-12,
            'viscosityRMS':2**-16/j,'viscosityMax':2**-14/j,
            'pressureCertificate':2**-16,'projectionVelocityCertificate':2**-16/j}
    if any(k not in values for k in limits):
        return 'NOT_MEASURED'
    if any(type(values[k]) not in (int,float) or not math.isfinite(values[k]) or values[k]<0 for k in limits):
        return 'INVALID'
    return 'THRESHOLDS_SATISFIED' if all(values[k]<=limit for k,limit in limits.items()) else 'THRESHOLDS_EXCEEDED'

def selftest():
    from copy import deepcopy
    wide='9007199254740993'
    status={'lastCommand':{'api':'g13Configure','boot':'71','attempt':wide},'attemptId':'98'}
    raw=dict(MINIMUM,maxBlock=str(MINIMUM['maxBlock']),stage='CAP_COMPUTE',precisionThreshold=23,
             precisionSource='VERTEX_HIGH_FLOAT_DECLARATION',precisionRawGL=0,resourceRawGL=0,
             rawGL=0,precisionStatus='PREFILTER_PASS',floatPrecision=23,precisionRange=[127,127],
             CAP_FP32='QUALIFIED',CAP_MATH='NOT_MEASURED',CAP_SCENE='NOT_MEASURED',
             fp32Source='SAME_PROGRAM_COLD_COMPUTE',fp32Words=EXPECTED*2,fp32RawGL=0)
    receipt=dict(result='ANCHORS_EQUAL_NOT_ATOMIC',controlBoot='71',controlTicket=wide,selectedTicket=wide,
                 configureAttempt=wide,capability=raw,stages=[
                     dict(seq=str(i),boot='71',attempt=wide,ticket=wide,phase=p,edge='EXIT',
                          rawGL=0,context='42',window='43',tid='44')
                     for i,p in enumerate(('CAPABILITY','INIT_FP32','INIT_FP32_RESULT'),1)])
    cases=[('wide',lambda d:None,'SYNCHRONIZED_FP32_EVIDENCE'),
           ('rounded',lambda d:d.update(controlTicket='9007199254740992'),'IDENTITY_MISMATCH'),
           ('anchor',lambda d:d.update(result='INCONSISTENT_SNAPSHOT'),'ANCHOR_MISMATCH'),
           ('scope',lambda d:d['stages'][-1].update(context='99'),'STAGE_SCOPE_OR_ORDER'),
           ('unqueried',lambda d:d.update(stages=[]),'MISSING_QUERY_STAGE'),
           ('rawGL',lambda d:d['stages'][-1].update(rawGL=-1),'STAGE_IDENTITY_OR_GL'),
           ('rawWords',lambda d:d['capability']['fp32Words'].__setitem__(0,0),'INVALID_RAW_VALUES'),
           ('missingCapability',lambda d:d.update(capability=None),'NOT_MEASURED')]
    results={}
    for name,modify,want in cases:
        test=deepcopy(receipt)
        modify(test)
        assert decide(test,status)==want,name
        results[name]=want
    units=dict(L0=2,rho0=1000,muSI=4,T0=1)
    nu=Fraction(units['muSI'],units['rho0']*units['L0']**2)
    assert nu==Fraction(1,1000)
    layout=load(ROOT/'docs/architecture-T607-g13-layout-v1.json')['schemas']['Config128']
    assert layout[12:19]==['L0Binary64.lo','L0Binary64.hi','rho0Binary64.lo','rho0Binary64.hi',
                           'muSIBinary64.lo','muSIBinary64.hi','nuF32']
    source=ROOT/'.codex/local-test/t690-final/entry/src/main/cpp/gpu_resident/g13flow'
    shader=(source/'pipeline_assembled.comp').read_bytes()
    import hashlib
    artifact=load(ROOT/'test/reports/t721/artifact.json')
    assert hashlib.sha256(shader).hexdigest()==artifact['pipelineSHA']
    witnesses={'nuF32':b'#define Config128_nuF32 18u',
               'viscosityResidual':b'#define Completion256_rHRMSF32 104u',
               'divergenceRMS':b'#define Completion256_divergenceRMSF32 106u',
               'pressureCertificate':b'#define Completion256_pressureCertificateF32 108u'}
    assert all(needle in shader for needle in witnesses.values())
    q=Fraction(1,32)/(12*Fraction(1,64))
    assert q==Fraction(1,6)
    baseline=load(ROOT/'test/t606/closed-form-checks-v2.json')
    assert float(q)==baseline['F07_Q'] and baseline['F06_N128_tail_correct']<1e-6
    a=(math.pi**2)/(64*120)
    tail=(1/4)*math.exp(-a*128**2)/(math.pi*a*128**2)
    assert math.isclose(tail,baseline['F06_N128_tail_correct'],rel_tol=1e-13)
    numeric=dict(divergenceRMS=2**-14,divergenceMax=2**-12,viscosityRMS=2**-18,
                 viscosityMax=2**-16,pressureCertificate=2**-16,projectionVelocityCertificate=2**-18)
    fixtures=[(numeric,'THRESHOLDS_SATISFIED'),(dict(numeric,viscosityRMS=2**-17),'THRESHOLDS_EXCEEDED'),
              (dict(numeric,pressureCertificate=float('nan')),'INVALID'),({},'NOT_MEASURED')]
    assert all(numerical_gates(4,values)==expected for values,expected in fixtures)
    assert numerical_gates(8,numeric)=='NOT_MEASURED'
    return dict(cases=results,fixtureUnits=units,nu=str(nu),velocityScale=2,pressureScale=4000,
                layout=layout[12:19],artifactFields={k:v.decode() for k,v in witnesses.items()},
                thresholds=MINIMUM,baselineQ=str(q),baselineTail=tail,
                numericalFixtureVerdicts=[expected for _,expected in fixtures],
                historical=historical(),device='NOT_RUN')

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('directory',type=Path,nargs='?')
    p.add_argument('round',type=int,nargs='?',default=1)
    args=p.parse_args()
    result=selftest() if args.directory is None else {
        'capabilityEvidence':decide(analyze(args.directory,args.round),
                                    load(args.directory/f'r{args.round}-status.json')),
        'physicalField':'NOT_MEASURED_BY_CAPABILITY_CHECK'}
    print(json.dumps(result,indent=2))
