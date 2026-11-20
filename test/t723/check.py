"""Audit the frozen T673/T679 case contract and recompute finite counterexamples."""
from fractions import Fraction as F
import argparse
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
DOCS=ROOT/'docs'
def load(name):
    return json.loads((DOCS/name).read_bytes())

def finite():
    mass=[1,3,5]
    assert sum(mass)==9 and sum(mass+mass)!=9
    # The material need for the example is 128 columns times 40 units.
    capacity=512*F(244666,2**20)
    assert capacity<F(5120)
    # Occupancy extends over disjoint intervals [0,1] and [2,3].
    height_first_moment=(1**2+3**2-2**2)/2
    assert height_first_moment==3 and F(2**2,2)==2
    # A stationary plane cannot provide suction at 180 degrees.
    reaction=[24,0,-24]
    assert min(reaction)<0
    # Moving wall, e=0.5, m=2, v=-1, wall=1.
    v0,wall,mass0,e=F(-1),F(1),F(2),F(1,2)
    v1=wall-e*(v0-wall)
    impulse=mass0*(v1-v0)
    loss=mass0*(1-e*e)*(v0-wall)**2/2
    assert mass0*(v1*v1-v0*v0)/2==impulse*wall-loss
    unwrap=181-179
    assert unwrap==2 and -179-179==-358
    profiles=[(F(1),F(1)),(F(1),F(4,3)),(F(1),F(2))]
    assert len({mean for mean,_ in profiles})==1 and len({moment for _,moment in profiles})==3
    counterflow=F(-1,2)+F(1,2)
    assert counterflow==0 and F(1,2)>0
    # Equal layer masses, selected paired impulse.
    p_before=F(1,2)*0+F(1,2)*2
    p_after=F(1,2)*F(1,2)+F(1,2)*F(3,2)
    assert p_before==p_after
    assert F(1,4)*(4-F(1,4)-F(9,4))==F(3,8)
    f,b,nu,rho=F(3,2),F(2),F(1,4),F(5)
    mean=f*b*b/(12*nu)
    shear=rho*f*b/2
    assert mean==2 and shear==F(15,2)
    assert (2*b)**2/b**2==4 and 2*shear==15
    assert F(4,5)+F(3,10)-1==F(1,10)
    nx,ny,nz=64,64,8
    faces=(nx+1)*ny*nz+nx*(ny+1)*nz+nx*ny*(nz+1)
    assert faces==103424 and 3*nx*ny*nz<faces
    n=64**3*8
    partial_bytes=3*(192*n+96*65**3)+2*16*27*n
    assert partial_bytes==3098990880 and partial_bytes>2**31-1
    values={
        'C01':{'mass':9,'duplicate':18},
        'C03':{'MACFaces':faces,'cellTimesThree':3*nx*ny*nz,'partialBytes':partial_bytes},
        'C04':{'incompatibleOverfill':'1/10'},
        'C05':{'signedForwardFactor':'(1-mu)/sqrt(2) at 45deg','muOne':'0'},
        'C08':{'originalFirstMoment':3,'collapsedFirstMoment':2},
        'C10':{'impulse':str(impulse),'loss':str(loss)},
        'C11':{'requiredReaction':reaction},
        'C12':{'K512Capacity':str(capacity),'required':5120},
        'C13':{'unwrapped':unwrap,'wrapped':-358},
        'C17':{'firstAndSecondMoments':[[str(a),str(b)] for a,b in profiles]},
        'C18':{'momentum':str(counterflow),'energy':'1/2'},
        'C19':{'pairedMomentum':str(p_after),'loss':'3/8'},
        'C21':{'mean':str(mean),'shear':str(shear)},
        'C23':{'doubleGapMeanFactor':4,'doubleGapShearFactor':2}}
    # Exact 45-degree factor after removing common g/sqrt(2).
    assert 1-F(1,4)==F(3,4) and 1-F(5,4)<0
    return values

POLICY={
  'C01':('exact integer inventory and transaction identity','none'),
  'C02':('analytic transformed position/velocity/force; exact declared phase','h/dt and measured work uncertainty'),
  'C03':('exact byte/row bounds and overflow rejection','actual device budget for selected layout'),
  'C04':('exact occupancy identity and closed-box compatibility','G14/G12 geometry and load residual budgets'),
  'C05':('analytic signed acceleration and integrated motion','selected h/dt error and declared absolute scale g'),
  'C06':('accepted phase and same-seed identity','material repeatability for centroid, front, recovery and final shape'),
  'C07':('identical fixture and parameters across resolutions','reference uncertainty and separated closure/discretization errors'),
  'C08':('exact interval mass and first moment','geometry approximation budget of selected representation'),
  'C09':('analytic ballistic phase and trajectory in no-contact window','gravity/length/time units and selected dt convergence'),
  'C10':('exact finite impulse/work identity','contact-time and restitution uncertainty'),
  'C11':('exact unilateral force sign','material recontact reference and contact tolerances'),
  'C12':('exact integer or rational capacity arithmetic','complete layout/work/overlap device measurement'),
  'C13':('exact phase/quadrant from declared motion','solver local convergence around wrap'),
  'C14':('exact cycle/control identity and no pause advance','torque, stop/reverse response and material uncertainty'),
  'C15':('exact accepted-part accounting through cycles','repeatability and prescribed phase-aligned comparison'),
  'C16':('exact declared supported axes and explicit rejection','finite-gap model and compatible reference'),
  'C17':('exact polynomial profile integrals','spatial error budget after representation choice'),
  'C18':('exact signed separate velocities and momentum','basal load and rest-threshold uncertainty'),
  'C19':('exact paired impulse and loss on finite fixture','selected material/shear closure uncertainty'),
  'C20':('same reference and cost envelope for all candidates','independent top/basal/shear measurements and convergence'),
  'C21':('analytic Newtonian profile, flux and shear','independent Nz/dt refinement; declared absolute scale'),
  'C22':('signed actual depth motion and exact axis identity','manufactured depth case, 3D reference accuracy'),
  'C23':('analytic b scaling for laminar fixture','granular wall reference and Nz/time resolution'),
  'C24':('exact inventory/reaction pairing and authority','validated 3D G13/G14/G12 interfaces and closure budget'),
}

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--case',choices=POLICY)
    args=parser.parse_args()
    source=load('design-T673-research-cases-v1.json')
    constraints=load('architecture-T679-constraints-v1.json')
    assert not constraints['algorithmChoiceFrozen'] and not constraints['MPMChosen'] and not constraints['twoLayerFChosen']
    cases=source['cases']
    assert [c['id'] for c in cases]==list(POLICY)
    results=finite()
    reviewed=[]
    for c in cases:
        case=c['id']
        absolute,relative=POLICY[case]
        assert c['input'] and c['observe'] and c['criterion'] and c['negativeControl'] and c['reference']
        reviewed.append(dict(c,absoluteToleranceSource=absolute,relativeToleranceSource=relative,
                             positiveControl=c['criterion'],finiteEvidence=results.get(case),
                             toleranceStatus='EXACT_FOR_FINITE_INVARIANTS; PRODUCT_PENDING_DESIGN',
                             device='NOT_RUN'))
    grouped={g:[c['id'] for c in reviewed if g in c['goals']] for g in range(15,20)}
    report={'caseCount':len(reviewed),'finiteCases':len(results),'goalOrder':grouped,
            'cases':reviewed,'device':'NOT_RUN','product':'PENDING_DESIGN'}
    if args.case:
        report=next(c for c in reviewed if c['id']==args.case)
    print(json.dumps(report,ensure_ascii=False,indent=2))
