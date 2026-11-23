"""T631 reference samples and T828 case-specific offline controls."""
import json
import math as m
from pathlib import Path
from fractions import Fraction as F

import importlib.util
spec=importlib.util.spec_from_file_location('t837_gate',Path(__file__).with_name('check.py'))
gate=importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
ROOT, CASES, field_reference, selftest=gate.ROOT,gate.CASES,gate.field_reference,gate.selftest

def need(ok):
    if not ok:
        raise ValueError('fixture criterion')

def controls():
    # Analytic samples at declared MAC coordinates on the unit square.
    h=1/64
    x,y=16.5*h,20.5*h
    pi=m.pi
    dh=2*m.sin(pi*h/2)/h
    p=.25*pi/dh*m.cos(pi*x)*m.cos(pi*y)
    amp=lambda j,nu:.25*(1+nu*4*m.sin(pi*h)**2/h**2/(120*j))**(-30*j)
    n,nu,U=128,1/64,1/4
    a=nu*pi*pi/120
    startup=U*y+2*U/pi*sum((-1)**k*m.sin(k*pi*y)*m.exp(-a*k*k)/k for k in range(1,n+1))
    tail=U*m.exp(-a*n*n)/(pi*a*n*n)
    # T605 RAMP_SPIN, Omega max 1, T=2, evaluated at t=0.5.
    motion=(.25-1/(2*pi),.5,pi/2)
    def correct_motion(theta,omega,alpha):
        need(max(abs(v-w) for v,w in zip((theta,omega,alpha),motion))<1e-12)
    def transaction(before,after,accepted,replay):
        need((accepted and after==replay) or (not accepted and before==after==replay))
    def visible(token,frame,new):
        need(token==frame and new)
    def cost(frames,stale,seconds):
        need(seconds>0 and 0<=stale<=frames)
        return (frames-stale)/seconds
    def scalar(value,reference,tolerance):
        need(m.isfinite(value) and abs(value-reference)<=tolerance)
    def boundary(normal,rhs,nc,gauge,sign,current):
        need(normal==0 and abs(rhs)<=nc//2 and gauge==0 and sign==1 and not current)
    cases={}
    def test(id,reference,positive,negative):
        positive()
        try: negative()
        except (ValueError,AssertionError):
            cases[id]={'reference':reference,'positive':'PASS','negative':'REJECTED',
                       'T605':CASES[id]['name'],'actualPC':'NOT_OBSERVED'}
        else: raise AssertionError(id)
    test('F01',{'u':0,'v':0,'pi':0,'totalPressureFromPhi':.125},
         lambda:scalar(.125,.25-.125,2**-19),lambda:scalar(.25,.25-.125,2**-19))
    test('F02',{'pressureSample':p,'projectionSample':0,'scope':'PRIVATE_PROJECT'},
         lambda:scalar(p,p,2**-16),lambda:scalar(-p,p,2**-16))
    test('F03',{'amplitudeJ124':[amp(j,1/16) for j in (1,2,4)],'doubledNu':amp(4,1/8)},
         lambda:scalar(amp(4,1/8),amp(4,1/8),2**-16),lambda:scalar(amp(4,1/16),amp(4,1/8),2**-16))
    tg=.25*m.exp(-2*(2*pi)**2*(1/64)*.25)
    test('F04',{'amplitude':tg,'pressureAmplitude':tg*tg/4},
         lambda:scalar(tg*tg/4,tg*tg/4,2**-16),lambda:scalar(0,tg*tg/4,2**-16))
    u=.25*m.exp(-.25)*m.sin(pi*x)**2*m.sin(2*pi*y)
    v=-.25*m.exp(-.25)*m.sin(2*pi*x)*m.sin(pi*y)**2
    pressure=.125*m.exp(-.5)*m.cos(pi*x)*m.cos(pi*y)
    test('F05',{'u':u,'v':v,'pressure':pressure,'motions':['REST','RAMP_SPIN']},
         lambda:scalar(pressure,pressure,2**-16),lambda:scalar(0,pressure,2**-16))
    test('F06',{'startupSample':startup,'tailBound':tail,'steadyWallShear':nu*U},
         lambda:scalar(startup,startup,tail),lambda:scalar(U*y,startup,tail))
    def poiseuille(scale):
        u=[scale*(j+.5)*h*(1-(j+.5)*h) for j in range(64) for _ in range(65)]
        verdict=field_reference('F07',{'u32':u},{'shape':[64,64]},1/64,30)
        need(verdict['velocityPass'] and verdict['fluxPass'])
    test('F07',{'Q':str(F(1,6)),'halfCellBias':h*h/4},lambda:poiseuille(1),lambda:poiseuille(2))
    test('F08',{'fixtureControl':dict(zip(('theta','omega','alpha'),motion)),
                'scope':'RAMP_SPIN at t=0.5s'},
         lambda:correct_motion(*motion),lambda:correct_motion(motion[0],0,motion[2]))
    c,s=F(3,5),F(4,5)
    test('F09',{'restVelocity':0,'cayleyEnergy':25},
         lambda:need((c*3-s*4)**2+(s*3+c*4)**2==25),
         lambda:need((c*3-s*4)**2+(s*3+c*5)**2==25))
    def uniform(value):
        field_reference('F10',{'u32':[value],'v32':[-.125],'piQ32':[0]},{'shape':[64,64]},1/64,120)
    test('F10',{'u':.25,'v':-.125,'pi':0},lambda:uniform(.25),lambda:uniform(0))
    test('F11',{'normalWallRaw':0,'aliasInjection':'NOT_OBSERVABLE'},
         lambda:boundary(0,32,64,0,1,False),lambda:boundary(0,33,64,0,1,False))
    def budget(positive,after_negative):
        need(positive<=1 and after_negative==1-positive)
    test('F12',{'windowAcceptedQuanta':120,'positiveCap':1},lambda:budget(.75,.25),lambda:budget(.75,.5))
    test('F13',{'transaction':'reject leaves base and accepted clock untouched'},
         lambda:transaction((1,0),(1,0),False,(1,0)),lambda:transaction((1,0),(1,1),False,(1,0)))
    def nu_rule(L,rho,mu,reported):
        need(L>0 and rho>0 and m.isfinite(mu) and mu/(rho*L*L)==reported)
    test('F14',{'L0':2,'rho0':1000,'muSI':4,'nuStar':.001},
         lambda:nu_rule(2,1000,4,.001),lambda:nu_rule(2,1000,4,4))
    test('F15',{'scope':'accepted root same token and fresh identity'},
         lambda:visible('A','A',True),lambda:visible('A','B',True))
    test('F16',{'sampleFrames':120,'stale':12,'seconds':1,'freshFPS':108,'rootHardMs':250},
         lambda:need(cost(120,12,1)==108),lambda:need(cost(120,12,1)==120))
    return cases

if __name__=='__main__':
    import subprocess, sys
    def invalid_archive():
        from tempfile import TemporaryDirectory
        with TemporaryDirectory() as t:
            path=Path(t)/'bad.jsonl'
            path.write_text('{}\n')
            p=subprocess.run([sys.executable,str(ROOT/'test/t837/check.py'),'archive',str(path),str(path)],
                             capture_output=True,text=True)
            assert p.returncode==1 and json.loads(p.stdout)['state']=='INVALID'
            return json.loads(p.stdout)
    report={'baseline':selftest(),'caseFixtures':controls(),'truncatedArchive':invalid_archive(),
            'actualPC':'PENDING_T833','fixtureOnly':True}
    print(json.dumps(report,indent=2))
