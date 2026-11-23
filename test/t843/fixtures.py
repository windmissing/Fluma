"""Emit deterministic PC input timelines for T829. No solver execution."""
import argparse
import hashlib
import json
from pathlib import Path
from fractions import Fraction

ROOT=Path(__file__).resolve().parents[2]
S=10**9
CASES={'G15-positive':([0,45,45,0],['C02','C06','C07']),
       'G15-negative':([0,-45,-45,0],['C02','C06','C07']),
       'G16-invert':([0,80,100,180,180],['C02','C09','C10','C11']),
       'G17-turns':([0,90,90,179,181,180,180,270,270,360,450,360,270,180,90,0],['C02','C13','C14','C15','C16'])}

def point(angles,t):
    span=min(t//S,len(angles)-2)
    x=Fraction(t-span*S,S)
    a,b=angles[span:span+2]
    d=b-a
    return [str(t),str(a+d*(10*x**3-15*x**4+6*x**5)),
            str(d*(30*x**2-60*x**3+30*x**4)),str(d*(60*x-180*x**2+120*x**3))]

def check(document):
    name=document['scenarioId']
    if document['axis']!=[0,0,1] or document['mode']!='PRESCRIBED_PLANAR_ROTATION':
        raise ValueError('AXIS_OR_MODE')
    angles,_=CASES[name]
    if document['knotsDeg']!=angles or document['samplePeriodNs']!=str(S//100):
        raise ValueError('FIXTURE_KNOTS')
    expected=[point(angles,t) for t in range(0,(len(angles)-1)*S+1,S//100)]
    if [r[0] for r in expected]!=[r[0] for r in document['samples']]:
        raise ValueError('TIME_GAP_OR_ORDER')
    if document['samples']!=expected:
        raise ValueError('KINEMATICS_OR_WRAPPED_PHASE')
    for n,a in enumerate(angles):
        assert point(angles,n*S)==[str(n*S),str(a),'0','0']

def main():
    p=argparse.ArgumentParser()
    p.add_argument('output',type=Path)
    p.add_argument('--check',action='store_true')
    args=p.parse_args()
    if args.check:
        doc=json.loads(args.output.read_bytes())
        check(doc)
        print(json.dumps({'scenario':doc['scenarioId'],'kinematics':'MATCH','physicalExecution':'NOT_RUN'}))
        return
    args.output.mkdir(parents=True,exist_ok=True)
    inputs=['docs/design-G15-G19-display-scenario-v1.md','docs/design-T673-research-cases-v1.json']
    provenance={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in inputs}
    result={}
    for name,(angles,cases) in CASES.items():
        doc={'scenarioId':name,'revision':1,'axis':[0,0,1],'axisOrigin':'BOX_CENTER','references':cases,
             'mode':'PRESCRIBED_PLANAR_ROTATION','geometryCandidateM':['0.10','0.20'],
             'materialAndSeed':'REFERENCE_REQUIRED','physicalExecution':'NOT_RUN',
             'columns':['time_ns','phase_deg','omega_deg_s','alpha_deg_s2'],
             'knotsDeg':angles,'samplePeriodNs':str(S//100),'frozenReferences':provenance,
             'samples':[point(angles,t) for t in range(0,(len(angles)-1)*S+1,S//100)]}
        check(doc)
        wire=(json.dumps(doc,separators=(',',':'))+'\n').encode()
        (args.output/(name+'.json')).write_bytes(wire)
        result[name]={'sha256':hashlib.sha256(wire).hexdigest(),'samples':len(doc['samples'])}
    bad=[(dict(doc,axis=[1,0,0]),'AXIS_OR_MODE'),(dict(doc,samples=doc['samples'][1:]),'TIME_GAP_OR_ORDER')]
    changed=[list(row) for row in doc['samples']]
    i=next(i for i,row in enumerate(changed) if Fraction(row[1])>180)
    changed[i][1]=str(Fraction(changed[i][1])-360)
    bad.append((dict(doc,samples=changed),'KINEMATICS_OR_WRAPPED_PHASE'))
    changed=[list(row) for row in doc['samples']]
    changed[100][2]='1'
    bad.append((dict(doc,samples=changed),'KINEMATICS_OR_WRAPPED_PHASE'))
    for item,expected in bad:
        try: check(item)
        except ValueError as error: assert str(error)==expected
        else: raise AssertionError(expected)
    print(json.dumps({'scenarios':result,'negativeControls':[code for _,code in bad],
                      'physicalExecution':'NOT_RUN'},indent=2))

if __name__=='__main__':
    main()
