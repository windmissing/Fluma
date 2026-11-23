"""Independent rational references for T829 goals 18 then 19."""
import argparse
from copy import deepcopy
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SOURCE=['docs/design-T673-research-cases-v1.json','docs/architecture-T679-constraints-v1.json',
        'docs/architecture-T829-v1.md','docs/design-G15-G19-display-scenario-v1.md']

def need(ok,reason):
    if not ok:
        raise ValueError(reason)

def layer(m,v):
    return {'mass':str(sum(m)),'momentum':str(sum(a*b for a,b in zip(m,v,strict=True))),
            'energy':str(sum(a*b*b/2 for a,b in zip(m,v,strict=True))),
            'secondMoment':str(sum(a*b*b for a,b in zip(m,v,strict=True))),
            'basal':str(v[0]),'top':str(v[-1]),'atRest':all(x==0 for x in v)}

def exchange(m,v,impulse):
    dv=[impulse/m[0],-impulse/m[1]]
    initial=layer(m,v)
    result=layer(m,[a+b for a,b in zip(v,dv,strict=True)])
    need(result['momentum']==initial['momentum'],'PAIRED_MOMENTUM')
    loss=F(initial['energy'])-F(result['energy'])
    need(loss>=0,'NEGATIVE_LOSS')
    return {'before':initial,'after':result,'dissipated':str(loss),'pairImpulse':[str(impulse),str(-impulse)]}

def plates(b,nz):
    # SI, closed-form no-slip laminar reference. Both wall tractions below
    # are the x component of wall-on-liquid traction.
    rho,mu,f=F(1000),F(1,100),F(3,2)
    nu=mu/rho
    depths=[F(2*k+1,2*nz)*b for k in range(nz)]
    values=[f*z*(b-z)/(2*nu) for z in depths]
    mean=f*b*b/(12*nu)
    face=-rho*f*b/2
    midpoint=sum(values)/nz
    need(midpoint-mean==mean/(2*nz*nz),'MIDPOINT_QUADRATURE')
    return {'b_m':str(b),'Nz':nz,'rho':str(rho),'mu':str(mu),'nu':str(nu),'force_m_s2':str(f),
            'z_m':list(map(str,depths)),'u_m_s':list(map(str,values)),'wallNormalVelocity':['0','0'],
            'exactMean_m_s':str(mean),'midpointMean_m_s':str(midpoint),'fluxPerWidth_m2_s':str(mean*b),
            'wallOnFluidShear_Pa':[str(face),str(face)],'wallVelocities':['0','0']}

def manufactured_depth():
    # Odd reflection of an imposed depth snapshot, not a solved flow.
    return {'zOverB':['0','1/4','1/2','3/4','1'],'w':['0','3/4','0','-3/4','0']}

def fixture():
    equal=[F(1,2)]*2
    swap=[layer(equal,[F(-1),F(1)]),layer(equal,[F(1),F(-1)])]
    thicknesses=[F(1,500),F(1,250),F(1,125)]
    profiles=[plates(b,n) for b in thicknesses for n in (8,16,32,64)]
    return {
      'C17':{'u=1':{'momentum':'1','energy':'1/2','secondMoment':'1','basal':'1','top':'1'},
             'u=2z':{'momentum':'1','energy':str(F(2,3)),'secondMoment':str(F(4,3)),'basal':'0','top':'2'},
             'halves':layer(equal,[F(0),F(2)])},
      'C18':{'massPerLayer':['1/2','1/2'],'velocities':[['-1','1'],['1','-1']],
             'runs':swap},
      'C19':{'massPerLayer':['1/2','1/2'],'initialSpeeds':['0','2'],'impulse':'1/4',
             'friction':exchange(equal,[F(0),F(2)],F(1,4)),
             'donorTransfer':{'beforeMasses':['1/2','1/2'],'beforeMomentum':['0','1'],
                              'massTransferred':'1/4','afterMasses':['3/4','1/4'],'afterMomentum':['1/2','1/2']}},
      'C20':{'reference':'REQUIRED','candidateCountFrozen':False},
      'C21':{'boundary':'periodic-x, no-slip z plates','profiles':profiles},
      'C22':{'scope':'manufactured reflection of a depth snapshot','sample':manufactured_depth()},
      'C23':{'comparison':'fixed rho/mu/f, vary b and Nz independently',
             'wallLawForGrains':'REFERENCE_REQUIRED','b_over_d':[str(b/F(3,5000)) for b in thicknesses],
             'doubleGapSpeedFactor':'4','doubleGapShearFactor':'2'},
      'C24':{'scope':'closed-box occupancy and load budget',
             'sand':'1/5','water':'4/5','pairedXImpulse':['1/8','-1/8'],
             'geometryAndCoupledSolver':'REFERENCE_REQUIRED'},
    }

def judge(case,observed,reference):
    # Rational fixture comparisons are exact. Production floating tolerances
    # and couplings are still governed by the original engineering contract.
    need(observed==reference,case+'_REFERENCE_DIFF')

def selftest(expected):
    failures=[]
    def check(case,mutate):
        altered=deepcopy(expected[case])
        mutate(altered)
        judge(case,expected[case],expected[case])
        try: judge(case,altered,expected[case])
        except ValueError as err: failures.append(str(err))
        else: raise AssertionError(case)
    check('C17',lambda d:d.update(halves=d['u=1']))
    check('C18',lambda d:d['runs'][0].update(atRest=True))
    check('C19',lambda d:d['donorTransfer'].update(afterMomentum=['0','1']))
    check('C20',lambda d:d.update(candidateCountFrozen=True))
    check('C21',lambda d:d['profiles'][0].update(wallOnFluidShear_Pa=['0','0']))
    check('C22',lambda d:d['sample'].update(w=['0']*5))
    check('C23',lambda d:d.update(doubleGapSpeedFactor='2'))
    check('C24',lambda d:d.update(pairedXImpulse=['1/8','1/8']))
    need(exchange([F(1,2)]*2,[F(0),F(2)],F(1,4))['dissipated']=='3/8','SHEAR_REFERENCE')
    transfer=expected['C19']['donorTransfer']
    for quantity in ('Masses','Momentum'):
        need(sum(map(F,transfer['before'+quantity]))==sum(map(F,transfer['after'+quantity])),
             'DONOR_TRANSFER_BALANCE')
    closed=expected['C24']
    need(F(closed['sand'])+F(closed['water'])==1 and sum(map(F,closed['pairedXImpulse']))==0,
         'CLOSED_BOX_BALANCE')
    a,b=(plates(F(1,k),8) for k in (250,125))
    need(F(b['exactMean_m_s'])==4*F(a['exactMean_m_s']) and
         F(b['wallOnFluidShear_Pa'][0])==2*F(a['wallOnFluidShear_Pa'][0]),'GAP_SCALING')
    return failures

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--check',type=Path,help='JSON object with case and observed keys')
    p.add_argument('--emit',type=Path)
    args=p.parse_args()
    expected=fixture()
    cases=json.loads((ROOT/SOURCE[0]).read_bytes())['cases']
    constraints=json.loads((ROOT/SOURCE[1]).read_bytes())['constraints']
    report=dict(schema='T846_PC_REFERENCE_FIXTURES_V1',order=[18,19],
                sources={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in SOURCE},
                caseContracts={row['id']:row for row in cases if row['id'] in expected},
                engineeringConstraints=constraints,fixtures=expected,negativeControls=selftest(expected),
                productExecution='NOT_RUN')
    if args.emit:
        args.emit.write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
    if args.check:
        data=json.loads(args.check.read_bytes())
        judge(data['case'],data['observed'],expected[data['case']])
        print(json.dumps({'case':data['case'],'result':'RATIONAL_FIXTURE_MATCH','productExecution':'NOT_RUN'}))
    else:
        print(json.dumps(report,indent=2))

if __name__=='__main__':
    main()
