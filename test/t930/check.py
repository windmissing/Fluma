"""Independent T928/T929 offline reference and evidence gates."""
import argparse
from copy import deepcopy
from decimal import Decimal as D, localcontext
from hashlib import sha256
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
ABI = json.loads((ROOT/'docs/architecture-T928-abi-v1.json').read_bytes())


def need(condition, code):
    if not condition:
        raise ValueError(code)


def trig(x):
    sin,cos,st,ct = x,D(1),x,D(1)
    for k in range(1,26):
        st *= -x*x/((2*k)*(2*k+1))
        ct *= -x*x/((2*k-1)*(2*k))
        sin,cos = sin+st,cos+ct
    return sin,cos


def contact(raw, output=None, near=None):
    need(len(raw)==52,'CONTACT_INPUT_SIZE')
    *values,mode = struct.unpack('<12fI',raw)
    need(all(map(math.isfinite,values)),'CONTACT_NONFINITE')
    with localcontext() as ctx:
        ctx.prec=60
        m,r,gap,v,wall,a,e,h,x,y,d,omega = map(D.from_float,values)
        need(m>0 and r>0 and gap>=0 and h>0 and 0<=e<=D.from_float(struct.unpack('<f',struct.pack('<f',.15))[0])
             and mode in (0,1),'CONTACT_INPUT')
        if mode==1:
            need(x>0 and y>0 and d>r and omega>0 and omega*h<D('1.5707963267948966'),'ROTATING_DOMAIN')
            def clearance(t):
                s,c=trig(omega*t)
                return d-r-x*c-y*s
            s,c=trig(omega*h)
            need(x*s-y*c<0 and clearance(D(0))>0>clearance(h),'ROTATING_NO_BRACKET')
            lo,hi=D(0),h
            for _ in range(120):
                mid=(lo+hi)/2
                if clearance(mid)>0:
                    lo=mid
                else:
                    hi=mid
            t=(lo+hi)/2
            s,c=trig(omega*t)
            wall=inward=omega*(y*c-x*s)
            normal=[-c,-s]
        else:
            vn=v-wall
            disc=vn*vn-2*a*gap
            need(disc>0,'TRANSLATE_NO_HIT')
            inward=disc.sqrt()
            roots=[2*gap/(inward-vn)] if vn<0 else [(-vn-inward)/a] if a<0 else []
            need(roots and 0<=roots[0]<=h,'TRANSLATE_NO_HIT')
            t=roots[0]
            normal=[D(1),D(0)]
        j=m*(1+e)*inward
        result=dict(toi=float(t),normal=list(map(float,normal)),wallSpeed=float(wall),impulse=float(j),
                    velocity=float(wall+e*inward),wallWork=float(j*wall))
        if mode==1:
            # Report both signed near-root points from the identical uploaded FP32 parameters.
            result['nearRoot']=[dict(t=float(t+dt),gap=float(clearance(t+dt))) for dt in (D('-.000001'),D('.000001'))]
        if output is None:
            return result
        need(len(output)==36,'CONTACT_OUTPUT_SIZE')
        status,toi,impulse,velocity,work,vn0,vn1,blo,bhi=struct.unpack('<I8f',output)
        need(all(map(math.isfinite,(toi,impulse,velocity,work,vn0,vn1,blo,bhi))),'CONTACT_OUTPUT_NONFINITE')
        errors=[]
        if not (status==1 and 0<=toi<=float(h) and abs(toi-float(t))<=.00005):
            errors.append('TOI_TOLERANCE')
        if not (impulse>=0 and abs(velocity-result['velocity'])<=.01
                and math.isclose(impulse,result['impulse'],rel_tol=.01,abs_tol=1e-8)
                and math.isclose(work,result['wallWork'],rel_tol=.01,abs_tol=1e-8)
                and abs(vn0+float(inward))<=.01 and abs(vn1-float(e*inward))<=.01):
            errors.append('IMPACT_RESIDUAL')
        if mode==1:
            gaps=[float(clearance(D.from_float(b))) for b in (blo,bhi)]
            result['outputGapsM']=gaps
            if not (0<=blo<=toi<=bhi<=float(h) and gaps[0]>=0>=gaps[1]):
                errors.append('SIGNED_BRACKET')
            if near is not None:
                need(len(near)==2,'CONTACT_NEAR_POINTS')
                for sample in near:
                    at=D.from_float(float(sample['t']))
                    s,c=trig(omega*at)
                    speed=float(omega*(y*c-x*s))
                    if not (0<=at<=h and abs(at-t)<=D('.00005')
                            and abs(sample['gap']-float(clearance(at)))<=1e-7
                            and sample['gap']*float(clearance(at))>0
                            and len(sample['normal'])==2
                            and math.dist(sample['normal'],[-float(c),-float(s)])<=1e-4
                            and abs(sample['wallSpeed']-speed)<=1e-4):
                        errors.append('NEAR_ROOT_GEOMETRY')
                        break
                if not near[0]['gap']>0>near[1]['gap']:
                    errors.append('NEAR_ROOT_SIGN')
        return dict(result,observedTOI=toi,errorMs=(toi-float(t))*1000,errors=errors)


def paths():
    pose=ABI['pose']
    path=ROOT/pose['fixture']['file']
    need(sha256(path.read_bytes()).hexdigest()==pose['fixture']['sha256'],'POSE_SHA')
    seeds=[dict(id=s['id'],sha=s['proof']['uploadSHA256'],massKg=s['proof']['massUg']/1e9) for s in ABI['seeds']]
    return [dict(seed=s,rate=rate,J=j,knots=pose[rate]['knotsDeg'],durationsNs=pose[rate]['durationsNs'])
            for s in seeds for rate in ('fast','slow') for j in pose['quantum']['BTrialJ']]


def multi(e):
    scenarios=paths()
    match=[r for r in scenarios if (r['seed']['id'],r['rate'],r['J'])==(e['seed'],e['rate'],e['J'])]
    need(len(match)==1 and e['seedSHA']==match[0]['seed']['sha'],'MULTI_IDENTITY')
    m,g,h=match[0]['seed']['massKg'],9.81,.2
    n=int(e['seed'].removeprefix('G16-B'))
    before,after=e['before'],e['after']
    need(len(before)==len(after)==n and len({p['id'] for p in before})==n
         and [p['id'] for p in before]==[p['id'] for p in after]
         and all((a['generation'],a['origin'],a['mass'],a['radius'],a['inertia'])==
                 (b['generation'],b['origin'],b['mass'],b['radius'],b['inertia'])
                 for a,b in zip(before,after,strict=True))
         and all(p['generation']>0 and p['origin']>0 and p['radius']>0 and p['inertia']>0 and p['mass']>0 for p in before)
         and math.isclose(sum(p['mass'] for p in before),m,rel_tol=1e-7),'MULTI_ORIGIN_MASS')
    def cross(a,b):
        return a[0]*b[1]-a[1]*b[0]
    def budget(particles):
        linear=[sum(r['mass']*r['velocity'][axis] for r in particles) for axis in (0,1)]
        angular=sum(r['mass']*cross(r['position'],r['velocity'])+r['inertia']*r['spin'] for r in particles)
        energy=sum(.5*r['mass']*sum(v*v for v in r['velocity'])+.5*r['inertia']*r['spin']**2+
                   r['mass']*g*r['position'][1] for r in particles)
        return linear,angular,energy
    before_budget,after_budget=budget(before),budget(after)
    wall=e['wall']
    contacts=e['contacts']
    for c in contacts:
        z=c['vNAfter']-c['vTarget']
        ln,lt=c['lambdaN'],c['lambdaT']
        need(ln>=0 and z>=-1e-3 and abs(ln*z)<=1e-3*ln+1e-10
             and abs(lt)<=c['mu']*ln+1e-10 and (abs(c['vTAfter'])<=1e-3 if c['static'] else lt*c['vTBefore']<=0),
             'MULTI_SOLVER_RESIDUAL')
    need(0<=e['maxPenetrationM']<=min(.00025,.2*min(p['radius'] for p in before)),'MULTI_PENETRATION')
    need(all(c['dissipation']>=0 for c in contacts),'MULTI_CONTACT_ENERGY')
    jg=e['gravityImpulse']
    dt=e['duration']
    need(dt>0 and abs(jg[0])<=1e-10 and math.isclose(jg[1],-m*g*dt,rel_tol=2e-7),'MULTI_GRAVITY')
    residual=[after_budget[0][k]-before_budget[0][k]-jg[k]-sum(w['impulse'][k] for w in wall)
              for k in (0,1)]
    angular=after_budget[1]-before_budget[1]-e['gravityAngularImpulse']-sum(cross(w['point'],w['impulse']) for w in wall)
    energy=after_budget[2]-before_budget[2]-sum(sum(i*v for i,v in zip(w['impulse'],w['speed'],strict=True)) for w in wall) \
           +sum(c['dissipation'] for c in contacts)
    need(math.hypot(*residual)<=1e-3*m*math.sqrt(g*h) and abs(angular)<=1e-3*m*h*math.sqrt(g*h)
         and abs(energy)<=1e-3*m*g*h,'MULTI_CONSERVATION')
    return dict(linearResidual=residual,angularResidual=angular,energyResidual=energy)


def c20(layers, dt):
    """Independent depth diffusion with a signed surface shear pulse."""
    nu=.12
    u=[0.]*layers
    n=round(3/dt)
    result=[]
    for k in range(n):
        t=(k+.5)*dt
        drive=1 if t<1 else 0 if t<2 else -1
        faces=[-2*nu*layers*u[0]]+[nu*layers*(u[i-1]-u[i]) for i in range(1,layers)]+[-drive]
        u=[v+dt*layers*(faces[i]-faces[i+1]) for i,v in enumerate(u)]
        if (k+1)%(n//30)==0:
            result.append([2*sum(u[:layers//2])/layers,2*sum(u[layers//2:])/layers,-2*nu*layers*u[0]])
    return result


def c20_local(mass, velocity, top_impulse, bottom_impulse, friction_limit):
    p=[mass[i]*velocity[i]+j for i,j in enumerate((bottom_impulse,top_impulse))]
    need(all(m>=0 for m in mass) and all(m or not x for m,x in zip(mass,p,strict=True))
         and friction_limit>=0,'C20_EMPTY_LAYER_FORCE')
    if min(mass)>0:
        slip=p[1]/mass[1]-p[0]/mass[0]
        transfer=math.copysign(min(friction_limit,abs(slip)/(1/mass[0]+1/mass[1])),slip)
        p=[p[0]+transfer,p[1]-transfer]
    return p


def c22(x,z,t,b=.004, length=.1):
    """Divergence-free moving manufactured field and analytic pressure."""
    k=2*math.pi/length
    angle=k*x
    s=z/b
    f=s*s*(1-s)**2
    dz=2*s*(1-s)*(1-2*s)/b
    dzz=(2-12*s+12*s*s)/(b*b)
    amp=1e-5*math.sin(2*t)
    amp_t=2e-5*math.cos(2*t)
    u=amp*math.sin(angle)*dz
    w=-amp*k*math.cos(angle)*f
    pressure=math.cos(angle)*math.sin(2*t)
    # Per-volume force for rho=1000 and mu=.010.
    ux=amp*k*math.cos(angle)*dz
    uz=amp*math.sin(angle)*dzz
    wx=amp*k*k*math.sin(angle)*f
    wz=-ux
    lap_u=-k*k*u+amp*math.sin(angle)*(-12+24*s)/(b**3)
    lap_w=-k*k*w-amp*k*math.cos(angle)*dzz
    fx=1000*(amp_t*math.sin(angle)*dz+u*ux+w*uz)-k*math.sin(angle)*math.sin(2*t)-.01*lap_u
    fz=1000*(-amp_t*k*math.cos(angle)*f+u*wx+w*wz)-.01*lap_w
    return dict(u=u,w=w,pressure=pressure,force=[fx,0,fz])


def c24(speed):
    """32×64×8 sealed box with a volume-preserving moving bed contrast."""
    nx,ny,nz=32,64,8
    dx,dy,dz=.1/nx,.2/ny,.004/nz
    dt=.001
    v=dx*dy*dz
    flux=speed*dy*dz
    # Bottom-strip left and right neighboring cells exchange equal volume.
    left=(nx//2-1)+(ny//2)*nx
    right=left+1
    cells=[dict(cell=left,before=.8*v,after=.8*v-dt*flux),dict(cell=right,before=.8*v,after=.8*v+dt*flux)]
    # Prescribed orthogonal stress components on the owned moving bed face.
    pressure=12.
    viscosity=.01*speed/dz
    area=dy*dz
    impulse=[viscosity*area*dt,pressure*area*dt]
    return dict(shape=[nx,ny,nz],dt=dt,cellVolume=v,cells=cells,faces=[dict(id=0,lo=left,hi=right,q=flux)],
                bedLoad=dict(face=0,pressure=pressure,viscosity=viscosity,area=area,
                             fluid=impulse,solid=[-j for j in impulse]))


def check24(e,reference):
    need(e['shape']==reference['shape'] and e['dt']==reference['dt'] and len(e['faces'])==1
         and e['faces'][0]['id']==reference['faces'][0]['id'],'C24_FACE_OWNERSHIP')
    face=e['faces'][0]
    need((face['lo'],face['hi'])==(reference['faces'][0]['lo'],reference['faces'][0]['hi']),
         'C24_CLOSED_COMPONENT')
    cells={c['cell']:c for c in e['cells']}
    need(len(cells)==len(e['cells'])==2 and set(cells)=={face['lo'],face['hi']},'C24_OCCUPANCY')
    allowed=max(1e-5*max(abs(reference['dt']*reference['faces'][0]['q']),1e-20),2**-23*reference['cellVolume'])
    for cell,sign in ((face['lo'],1),(face['hi'],-1)):
        c=cells[cell]
        need(0<=c['before']<=reference['cellVolume'] and 0<=c['after']<=reference['cellVolume']
             and abs(c['after']-c['before']+sign*e['dt']*face['q'])<=allowed,'C24_VOLUME')
    need(abs(face['q']-reference['faces'][0]['q'])*e['dt']<=allowed,'C24_PRESCRIBED_FLUX')
    load,ref=e['bedLoad'],reference['bedLoad']
    need(load['face']==face['id'] and all(abs(load[k]-ref[k])<=max(1e-14,.05*abs(ref[k]))
         for k in ('pressure','viscosity','area')),'C24_LOAD')
    need(all(abs(a+b)<=1e-14 and abs(a-r)<=max(1e-14,.05*abs(r))
             for a,b,r in zip(load['fluid'],load['solid'],ref['fluid'],strict=True)),'C24_REACTION')
    return 'C24_CLOSED_VOLUME_AND_LOAD'


def rms(a,b):
    need(len(a)==len(b) and bool(b) and all(math.isfinite(x) for x in a+b),'REFERENCE_SHAPE')
    scale=max(map(abs,b))
    need(scale>0,'ZERO_REFERENCE_SCALE')
    return math.sqrt(sum((x-y)**2 for x,y in zip(a,b,strict=True))/len(b))/scale


def check20(e):
    ref=c20(32,.0005)
    need(len(e['samples'])==len(ref) and all(len(row)==3 for row in e['samples']),'C20_SAMPLES')
    errors=[rms(list(a),list(b)) for a,b in zip(zip(*e['samples']),zip(*ref),strict=True)]
    need(errors[0]<=.05 and errors[1]<=.05 and errors[2]<=.10,'C20_RMS')
    for i in range(2):
        peak=max(abs(row[i]) for row in ref)
        need(all(a[i]*b[i]>0 for a,b in zip(e['samples'],ref,strict=True) if abs(b[i])>.01*peak),'C20_SIGN')
    dt=.1
    for index in range(2):
        for t in (10,20):
            segment=range(t,min(t+10,len(ref)))
            def switches(rows):
                return [i for i in segment if i>0 and rows[i-1][index]*rows[i][index]<0]
            a,b=switches(e['samples']),switches(ref)
            need(len(a)==len(b) and all(abs(x-y)*dt<=max(2*dt,.05) for x,y in zip(a,b,strict=True)),
                 'C20_REVERSAL_EVENT')
    need(all(any(abs(r[i])>1e-8 for r in e['samples']) for i in (0,1)),'C20_FALSE_REST')
    return errors


def depth_points(grid=8):
    return [(.1*i/grid,.004*z/grid,t) for t in (.15,.4,.8) for i in range(grid) for z in range(grid+1)]


def check22(e):
    grid=e.get('grid',8)
    need(grid in (8,16,32),'C22_GRID')
    ref=[c22(*p) for p in depth_points(grid)]
    need(len(e['samples'])==len(ref),'C22_SAMPLES')
    w=[r['w'] for r in e['samples']]
    gold=[r['w'] for r in ref]
    errors=[rms(w,gold),rms([r['pressure'] for r in e['samples']],[r['pressure'] for r in ref])]
    need(max(errors)<=.05 and all(w[i]*gold[i]>0 for i in (gold.index(min(gold)),gold.index(max(gold)))),
         'C22_DEPTH_OR_PRESSURE')
    stride=grid+1
    for off in range(0,len(ref),stride):
        observed=e['samples'][off:off+stride]
        need(all(abs(row['w']-mirror['w'])<=.05*max(map(abs,gold))
                 and abs(row['pressure']-mirror['pressure'])<=.05
                 for row,mirror in zip(observed,reversed(observed),strict=True)),'C22_DEPTH_REFLECTION')
    # Independent mirrored dry disk pair with e=.1 at a wall.
    dry=e['dry']
    speed=.02
    target=(.002-.0003)/speed
    expect=(1+.1)*.001*speed
    need(0<e['dt']<=.002 and len(dry)==2 and all(
         abs(r['toi']-target)<=2*e['dt'] and 0<=r['penetration']<=e['spaceQuantum']
         and abs(r['impulse']-s*expect)<=.01*expect for r,s in zip(dry,(1,-1),strict=True))
         and abs(sum(r['impulse'] for r in dry))<=.01*expect,'C22_DRY_MIRROR')
    return errors


def depth_fixture(grid=8, dt=.001):
    return dict(samples=[c22(*p) for p in depth_points(grid)],grid=grid,dt=dt,spaceQuantum=1e-6,
                dry=[dict(toi=.085,impulse=i*2.2e-5,penetration=0) for i in (1,-1)])


def multi_fixture():
    seed=ABI['seeds'][0]
    r=seed['radiusUm']/1e6
    mass=seed['massUg']/1e9
    particles=[]
    for y,first,pitch,columns in seed['rows']:
        for column in columns:
            i=len(particles)+1
            particles.append(dict(id=i,generation=1,origin=i,position=[(first+pitch*column)/1e6,y/1e6],
                                  mass=mass,radius=r,inertia=.5*mass*r*r,spin=0,velocity=[0,0]))
    dt=.001
    j=.13*9.81*dt
    x=sum(p['position'][0] for p in particles)/len(particles)
    return dict(seed=seed['id'],seedSHA=seed['proof']['uploadSHA256'],rate='fast',J=128,before=particles,
                after=deepcopy(particles),wall=[dict(point=[x,-.1],impulse=[0,j],speed=[0,0])],
                contacts=[dict(lambdaN=j,lambdaT=0,mu=0,static=True,vNAfter=0,vTarget=0,vTAfter=0,
                               vTBefore=0,dissipation=0)], maxPenetrationM=0,duration=dt,
                gravityImpulse=[0,-j],gravityAngularImpulse=-x*j)


def expect_rejection(fn,reason):
    try:
        fn()
    except ValueError as exc:
        need(str(exc)==reason,f'EXPECTED_{reason}: {exc}')
    else:
        raise AssertionError(reason)
    return reason


def selftest():
    original={}
    for name,r in ABI['isolatedReceipts'].items():
        input_=bytes.fromhex(r['inputPackedHex'])
        output=(ROOT/r['output']).read_bytes()
        need(sha256(input_).hexdigest()==r['inputSHA'] and sha256(output).hexdigest()==r['outputSHA'],'ORIGINAL_HASH')
        original[name]=contact(input_,output)
    need(original['g16-translate']['errors']==[]
         and original['g16-rotate']['errors']==['TOI_TOLERANCE','SIGNED_BRACKET'],'HISTORICAL_CONTACT')
    # Use adjacent representable FP32 times around the mathematical root.
    gold=original['g16-rotate']
    bits=struct.unpack('<I',struct.pack('<f',gold['toi']))[0]
    lo,hi=(struct.unpack('<f',struct.pack('<I',bits+i))[0] for i in (-1,1))
    data=struct.pack('<I8f',1,hi,gold['impulse'],gold['velocity'],gold['wallWork'],
                     -gold['wallSpeed'],gold['velocity']-gold['wallSpeed'],lo,hi)
    rotate=bytes.fromhex(ABI['isolatedReceipts']['g16-rotate']['inputPackedHex'])
    need(contact(rotate,data)['errors']==[],'CONTACT_GOLD')
    omega=struct.unpack('<12fI',rotate)[11]
    _,_,_,_,_,_,_,_,x,y,_,_,_=struct.unpack('<12fI',rotate)
    near=[]
    for sample in gold['nearRoot']:
        angle=omega*sample['t']
        near.append(dict(sample,normal=[-math.cos(angle),-math.sin(angle)],
                         wallSpeed=omega*(y*math.cos(angle)-x*math.sin(angle))))
    need(contact(rotate,data,near)['errors']==[],'CONTACT_NEAR_GOLD')
    wrong_near=deepcopy(near)
    wrong_near[0]['wallSpeed']+=.01
    need(contact(rotate,data,wrong_near)['errors']==['NEAR_ROOT_GEOMETRY'],'CONTACT_NEAR_NEGATIVE')
    malformed=bytearray(rotate)
    struct.pack_into('<f',malformed,40,.2)
    negatives=['NEAR_ROOT_GEOMETRY',expect_rejection(lambda:contact(malformed),'ROTATING_NO_BRACKET')]
    m=multi_fixture()
    multi_result=multi(m)
    wrong=deepcopy(m)
    wrong['contacts'][0]['lambdaN']=-1
    negatives.append(expect_rejection(lambda:multi(wrong),'MULTI_SOLVER_RESIDUAL'))
    wrong=deepcopy(m)
    wrong['wall'][0]['impulse'][1]=0
    negatives.append(expect_rejection(lambda:multi(wrong),'MULTI_CONSERVATION'))
    reference=c20(32,.0005)
    convergence={f'{a}/{b}':[rms(list(x),list(y)) for x,y in zip(zip(*c20(a,.001)),zip(*c20(b,.0005)),strict=True)]
                 for a,b in ((4,8),(8,16),(16,32))}
    need(all(x<y for x,y in zip(convergence['16/32'],convergence['4/8'])),'C20_REFERENCE_REFINEMENT')
    need(all(x<=y for x,y in zip(convergence['4/8'],(.025,.025,.05),strict=True)),'C20_REFERENCE_HALF_BUDGET')
    need(c20_local([.5,.5],[0,0],.25,0,0)==[0,.25]
         and c20_local([.5,.5],[0,2],0,0,.25)==[.25,.75]
         and c20_local([.5,.5],[0,0],-.25,0,0)==[0,-.25]
         and c20_local([0,.5],[0,0],.25,0,0)==[0,.25],'C20_LOCAL')
    negatives.append(expect_rejection(lambda:check20({'samples':[r[::-1] for r in reference]}),'C20_RMS'))
    negatives.append(expect_rejection(lambda:c20_local([0,.5],[0,0],0,.25,0),'C20_EMPTY_LAYER_FORCE'))
    depth=depth_fixture()
    depth_result=check22(depth)
    need(check22(depth_fixture(16,.0005))==[0,0],'C22_REFINED')
    wrong=deepcopy(depth)
    for row in wrong['samples']:
        row['w']=0
    negatives.append(expect_rejection(lambda:check22(wrong),'C22_DEPTH_OR_PRESSURE'))
    wrong=deepcopy(depth)
    wrong['dry'][1]['impulse']*=-1
    negatives.append(expect_rejection(lambda:check22(wrong),'C22_DRY_MIRROR'))
    models=[c24(s) for s in (0,.002,-.002)]
    for model in models:
        check24(model,model)
    for reason,mutate in (
        ('C24_FACE_OWNERSHIP',lambda e:e['faces'].append(e['faces'][0])),
        ('C24_CLOSED_COMPONENT',lambda e:e['faces'][0].update(hi=-1)),
        ('C24_VOLUME',lambda e:e['cells'][0].update(after=e['cells'][0]['before'])),
        ('C24_REACTION',lambda e:e['bedLoad'].update(solid=e['bedLoad']['fluid'])),
    ):
        wrong=deepcopy(models[1])
        mutate(wrong)
        negatives.append(expect_rejection(lambda:check24(wrong,models[1]),reason))
    for script in ('test/t928/fixtures.py','test/t846/fixtures.py'):
        subprocess.run([sys.executable,script],cwd=ROOT,check=True,capture_output=True)
    pose=ROOT/ABI['pose']['fixture']['file']
    subprocess.run([sys.executable,'test/t843/fixtures.py',str(pose),'--check'],cwd=ROOT,check=True,capture_output=True)
    return dict(original=original,g16Matrix=paths(),syntheticMulti=multi_result,c20Convergence=convergence,
                c22Errors=depth_result,c24Contrasts=len(models),negativeControls=negatives)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('selftest','contact','multi','c20','c22','c24','reference'))
    p.add_argument('input',type=Path,nargs='?')
    p.add_argument('output',type=Path,nargs='?')
    p.add_argument('--near',type=Path)
    args=p.parse_args()
    if args.mode=='selftest':
        result=selftest()
    elif args.mode=='reference':
        result=dict(c20={'samples':c20(32,.0005)},
                    c20Matrix=[dict(cells=c,layers=n,dt=dt) for c in (32,64,128) for n in (2,4,8)
                               for dt in (.001,.0005)],
                    c22=[depth_fixture(g,dt) for g,dt in ((8,.001),(16,.0005))],
                    c24={str(s):c24(s) for s in (0,.002,-.002)},g16Matrix=paths())
    elif args.mode=='contact':
        result=contact(args.input.read_bytes(),args.output.read_bytes() if args.output else None,
                       json.loads(args.near.read_bytes()) if args.near else None)
    else:
        e=json.loads(args.input.read_bytes())
        gate={'multi':multi,'c20':check20,'c22':check22,
              'c24':lambda d:check24(d['observed'],c24(d['speed']))}[args.mode]
        result=gate(e)
    print(json.dumps(result,indent=2))
    return int(isinstance(result,dict) and bool(result.get('errors')))


if __name__=='__main__':
    sys.exit(main())
