"""T908/T907 offline fixtures for Demo evidence and large-angle GPU contracts."""
import argparse
from copy import deepcopy
from fractions import Fraction
from hashlib import sha256
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT/'test/reports'
spec = importlib.util.spec_from_file_location('t900',ROOT/'test/t900/check.py')
T900 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(T900)
T843 = T900.module('t843','test/t843/fixtures.py')
need, pair = T900.need,T900.pair
Q = 1<<20
SCOPE = ('run','geometry','root','slot','generation','image','lease')
manifest = json.loads((ROOT/'docs/analysis-T238-gpu-water-M1-engineering-manifest-v1.json').read_bytes())
MASKS = {event['id']:set(event['usedWordIndices']) for event in manifest['eventKinds']}


def land(e):
    t, frame = e['completion'],e['frame']
    raw = e['words']
    need(len(t)==256 and tuple(t[:4])==T900.COLD.SEAL and t[27:32]==[2,1,0,0,0]
         and (pair(t,6),pair(t,38),pair(t,4),t[35],pair(t,36),t[51]) ==
             tuple(frame[k] for k in ('run','geometry','root','slot','generation','arena'))
         and frame['fence']==0 and e['archiveBinding']=={k:frame[k] for k in (*SCOPE,'arena')},
         'LAND_SCOPE')
    start,end,capacity = t[52:55]
    need(0<=start<=end<=capacity and len(raw)==64*end
         and sha256(struct.pack(f'<{len(raw)}I',*raw)).hexdigest()==e['eventSHA'],'LAND_RANGE')
    bed, flight = (list(e['before'][k]) for k in ('S','P'))
    need(len(bed)==len(flight)==128 and min(bed+flight)>=0,'LAND_COLUMNS')
    particles = deepcopy(e['particles'])
    origins = e['origins']
    moved,count = 0,0
    for i in range(start,end):
        w = raw[i*64:(i+1)*64]
        kind = w[6]
        a,b = w[20:22]
        need(kind in MASKS and all(not v or j in MASKS[kind] for j,v in enumerate(w))
             and w[:2]==[1163088455,2] and pair(w,2)==pair(t,4) and w[4]==2
             and pair(w,7)==i+1 and pair(w,9)==pair(t,24) and w[11]==t[26]
             and pair(w,12)==pair(t,16) and pair(w,14)==pair(t,20)
             and 0<=a<128 and 0<=b<128,'LAND_EVENT_ID')
        if kind==9:
            q = w[32]-w[33]
            need(abs(a-b)==1 and q>0 and w[35]-w[34]==q and (bed[a],bed[b])==(w[32],w[34])
                 and w[33]>=0 and w[5]==3 and w[52]==2<<18,'BED_EDGE_ACCOUNTING')
            bed[a],bed[b]=w[33],w[35]
        elif kind==8:
            particle = particles.get(str(w[16]))
            origin = origins.get(str(pair(w,48)))
            q = w[22]-w[23]
            need(a==b and w[5]==2 and (w[52]>>18)&3==0 and q>0
                 and particle and (particle['generation'],particle['id'],particle['source'],particle['origin'],particle['mass'])
                 == (w[17],w[18],w[19],pair(w,48),w[22])
                 and origin and origin['source']==w[19] and origin['root']==pair(w,50)
                 and origin['firstId']<=w[18]<origin['firstId']+origin['births'], 'LAND_SOURCE')
            need((bed[a],flight[a])==(w[32],w[34]) and w[33]-w[32]==w[34]-w[35]==q
                 and w[36]-w[38]==w[40]-w[41]==q,'LAND_ACCOUNTING')
            bed[a],flight[a] = w[33],w[35]
            particle['mass']=w[23]
            moved+=q
            count+=1
        elif kind==5 and (w[52]>>18)&3==0:
            q = w[22]
            need(abs(a-b)==1 and q>0 and w[23]==q and (flight[a],flight[b])==(w[32],w[34])
                 and w[32]-w[33]==w[35]-w[34]==q,'PARTICLE_COLUMN_TRANSFER')
            flight[a],flight[b]=w[33],w[35]
    need(count>0,'LAND_MISSING')
    need(bed==e['after']['S'] and flight==e['after']['P']
         and sum(bed)+sum(flight)==sum(e['before']['S'])+sum(e['before']['P'])==pair(t,71)==pair(t,77)
         and pair(t,14)==pair(t,18),'LAND_CONSERVATION')
    return dict(count=count,massQ20=moved,bedEdgeCount=t[95])


def roi(e):
    frames = e['frames']
    need(len(frames)>=2 and e['downNs']<e['upNs'],'ROI_WINDOW')
    visible, settled = [], []
    previous = None
    for f in frames:
        need(f['binding']=={k:f[k] for k in SCOPE} and f['shader'] and f['present']==0 and f['graphicsFenceResult']==0
             and len(f['columns'])==128 and f['width']>0 and f['height']>0 and
             (previous is None or (f['run'],f['geometry'])==(previous['run'],previous['geometry'])
              and f['physicsCommit']==previous['physicsCommit']+1 and f['root']!=previous['root']
              and f['ns']>previous['ns']), 'ROI_SAME_ROOT_WINDOW')
        data = bytes.fromhex(f['rgba'])
        mask = bytes.fromhex(f['particleMask'])
        need(len(data)==4*f['roiWidth']*f['roiHeight'] and len(mask)*4==len(data)
             and f['roiHeight']<=f['height'] and 0<=f['roiX']<=f['width']-f['roiWidth']
             and sha256(data).hexdigest()==f['sha'] and sha256(mask).hexdigest()==f['maskSHA'], 'ROI_BYTES')
        columns = []
        for col in range(63,66):
            physical = f['columns'][col]*f['height']/(256*Q)
            start,end=(k*f['width']//128-f['roiX'] for k in (col,col+1))
            need(0<=start<end<=f['roiWidth'],'ROI_COVERAGE')
            for x in range(start,end):
                indices = [y*f['roiWidth']+x for y in reversed(range(f['roiHeight']))]
                height = 0
                for i in indices:
                    if mask[i] or list(data[i*4:i*4+4])!=e['bedRGBA']:
                        break
                    height+=1
                columns.append(physical>=6 and height>=5 and abs(height-physical)<=1)
        if all(columns):
            visible.append(f)
            if f['ns']>=e['upNs'] and f['activeSand']==0:
                settled.append(f)
        previous = f
    need(visible and visible[0]['ns']<=e['downNs']+4_000_000_000,'ROI_BUILD_MISSING_OR_OCCLUDED')
    need(settled and settled[0]['ns']<=e['upNs']+3_000_000_000,'ROI_SETTLE_TIMEOUT')
    first = settled[0]
    retention = [f for f in frames if first['ns']<=f['ns']<=first['ns']+1_000_000_000]
    last = next((f for f in frames if f['ns']>=first['ns']+1_000_000_000),None)
    need(last and all(f in settled and f['columns']==first['columns'] for f in [*retention,last]),'ROI_RETENTION')
    return dict(firstVisibleRoot=visible[0]['root'],settledRoot=first['root'],retainedUntil=last['ns'])


def source_totals(path):
    rows,_ = T900.OLD.read_rows(path)
    roots = T900.OLD.accepted(rows)
    sand = [r['words'][61] for r in roots.values() if r['kind']=='SOURCE' and r['rows'][0]['m1SourceWords'][2]==0]
    return dict(count=len(sand),massQ20=sum(sand),sha=sha256(path.read_bytes()).hexdigest())


def load_plan():
    return dict(scenarios=[dict(seed=seed,active=active,material=material,source=source,draw=draw,mode=mode)
                           for seed,material,counts in [('same-sand-base','sand',(0,32,64,96)),
                                                        ('same-water-base','water',(0,32,64)),('high-static-bed','sand',(0,))]
                           for active in counts for source in (False,True) for draw in (False,True)
                           for mode in ('quantum-only','shared-grant')],
                phasePoints=['prepare','angle','sort','motion','bed-or-water','replay','pack','graphics','copy'],
                angleBranches={seed:[0,3840,7680,-3840,0] for seed in ('flat','gentle','guard-limited','mobile')},
                angleBranchesAre='independent same BASE branches, followed by zero recovery on each nonzero arm')


def g15(e):
    """Check integer support, mass, signed face flux and source ledger."""
    before,after,q = (e[k] for k in ('before','after','faces'))
    dimension = len(before['S'])
    need(len(q)==dimension+1 and q[0]==q[-1]==0 and abs(e['angleQ8'])<=11520
         and e['dx']>0 and e['gn']>0,'G15_DOMAIN')
    need(all(len(s[k])==dimension and min(s[k])>=0 for s in (before,after) for k in ('S','F','P')),'G15_MASS_RANGE')
    drive = []
    for dB in e['slopes']:
        d = e['gx']*e['dx']-e['gn']*dB
        an = e['gn']*e['dx']+e['gx']*dB
        need(5*abs(dB)<=3*e['dx'] and 4*an>=e['g']*e['dx'],'G15_SUPPORT')
        drive.append(1 if d*65536>e['muS']*an else -1 if -d*65536>e['muS']*an else 0)
    need(len(drive)==dimension-1,'G15_FACE_COUNT')
    for i in range(dimension):
        need(sum(after[k][i]-before[k][i] for k in ('S','F','P'))==q[i]-q[i+1],'G15_FACE_BALANCE')
    need(before['origins']==after['origins'] and sum(after['origins'].values())==sum(sum(after[k]) for k in ('S','F','P')),
         'G15_ORIGIN_BALANCE')
    return dict(staticDrive=drive,total=sum(after['origins'].values()))


def load_pair(e):
    a,b=e['light'],e['heavy']
    frozen=('device','driver','seedSHA','sourceSHA','shaderSHA','draw','mode','temperatureMode','queryPeriodNs','timestampBits')
    need(all(a[k]==b[k] for k in frozen) and a['root']!=b['root'] and a['active']<b['active'],'LOAD_PAIR_SCOPE')
    phases=load_plan()['phasePoints']
    samples=[]
    for r in (a,b):
        need(r['submit']==0 and r['sameFence'] and r['fence']==0
             and r['domain']=='GPU_TIMESTAMP' and 1<=r['timestampBits']<=64
             and math.isfinite(r['queryPeriodNs']) and r['queryPeriodNs']>0,'LOAD_GPU_EVIDENCE')
        need(set(r['spans'])==set(phases),'LOAD_SPANS')
        samples.append({p:((r['spans'][p][1]-r['spans'][p][0])%(1<<r['timestampBits']))*r['queryPeriodNs']
                        for p in phases if r['spans'][p] is not None})
    return dict(lightNs=samples[0],heavyNs=samples[1],missing=[p for p in phases if any(p not in s for s in samples)])


def isolated(case):
    """Analytic disk contact for a prescribed translating wall in world SI."""
    m,r,gap,vel,wall,acc,e = (case[k] for k in ('mass','radius','gap','normalVelocity','wallVelocity','acceleration','e'))
    need(m>0 and r>0 and gap>=0 and 0<=e<=0.15,'G16_CONTACT_INPUT')
    vn = vel-wall
    if gap==0 and abs(vn)<1e-14 and acc<0:
        dt = case['supportDt']
        need(dt>0,'G16_SUPPORT_DT')
        impulse=-m*acc*dt
        return dict(toi=None,impulse=impulse,velocity=wall,wallWork=impulse*wall)
    if gap==0 and vn>=0:
        return dict(toi=None,impulse=0,velocity=vel)
    if abs(acc)<1e-14:
        roots = [-gap/vn] if vn<0 else []
    else:
        disc = vn*vn-2*acc*gap
        roots = [(-vn+sign*math.sqrt(disc))/acc for sign in (-1,1)] if disc>=0 else []
    valid = [t for t in roots if t>=0 and vn+acc*t<0]
    if not valid:
        return dict(toi=None,impulse=0,velocity=vel)
    t = min(valid)
    normal = vn+acc*t
    impulse = -m*(1+e)*normal
    return dict(toi=t,impulse=impulse,velocity=vel+acc*t+impulse/m,wallWork=impulse*wall)


def rotating_wall():
    """Single vertical plane rotates about box center into a stationary disk."""
    x,y,r,offset,omega = 0.07,0.04,0.004,0.08,1.0
    # On this bracket the signed gap is strictly decreasing.
    def gap(t):
        return offset-r-x*math.cos(t)-y*math.sin(t)
    lo,hi = 0.0,0.25
    need(gap(lo)>0>gap(hi) and x*math.sin(hi)<y*math.cos(hi),'G16_ROTATING_BRACKET')
    for _ in range(48):
        m = (lo+hi)/2
        if gap(m)>0:
            lo=m
        else:
            hi=m
    mass=0.02
    normal=(-math.cos(hi),-math.sin(hi))
    wall_normal_speed=y*math.cos(hi)-x*math.sin(hi)
    impulse=1.1*mass*wall_normal_speed
    return dict(toi=hi,bracket=[lo,hi],normal=normal,wallNormalSpeed=wall_normal_speed,impulse=impulse,
                velocity=[impulse/mass*n for n in normal],wallWork=impulse*wall_normal_speed,omega=omega)


def angle_case(columns,requested):
    scene = sys.modules['scene']
    lo,hi = scene.angle_range(columns)
    need(lo<=hi,'ANGLE_EMPTY_DOMAIN')
    actual = min(hi,max(lo,requested))
    tangent = scene.angle_tangents()[actual+7680]
    excess = [max(0,abs(b-a-tangent)-557537) for a,b in zip(columns,columns[1:])]
    return dict(requested=requested,accepted=actual,range=[lo,hi],activeFaces=sum(x>0 for x in excess))


def fixtures():
    scene = sys.modules['scene']
    t = T900.T893.tail(nonce=9,geometry=2,generation=3)
    t[14],t[53],t[54],t[71],t[77] = 1,1,20,150,150
    w = [0]*64
    fields = {0:1163088455,1:2,2:9,4:2,5:2,6:8,7:1,9:1,11:1,14:2,17:1,18:2,19:1,
              22:100,23:60,32:50,33:90,34:100,35:60,36:40,40:40,48:1,50:7}
    for i,v in fields.items():
        w[i]=v
    frame = dict(run=1,geometry=2,root=9,slot=0,generation=3,arena=0,image=0,lease=1,fence=0)
    columns = lambda a:[a]+[0]*127
    land_case = dict(completion=t,frame=frame,words=w,eventSHA=scene.u32sha(w),
                     archiveBinding={k:frame[k] for k in (*SCOPE,'arena')},
                     before=dict(S=columns(50),P=columns(100)),after=dict(S=columns(90),P=columns(60)),
                     particles={'0':dict(generation=1,id=2,source=1,origin=1,mass=100)},
                     origins={'1':dict(source=1,root=7,firstId=2,births=1)})
    bed_only = deepcopy(land_case)
    event = [0]*64
    for i,v in {0:1163088455,1:2,2:9,4:2,5:3,6:9,7:1,9:1,11:1,14:2,21:1,
                32:50,33:40,35:10,52:2<<18}.items():
        event[i]=v
    bed_only.update(words=event,eventSHA=scene.u32sha(event))
    bed_only['completion'][95]=1
    pixels = bytes([200,150,80,255])*21
    clear = bytes(21)
    roi_frames = []
    for commit,ns in enumerate((1_000_000_000,2_000_000_000,3_000_000_000),1):
        f = dict(frame,root=9+commit,physicsCommit=commit,ns=ns,shader=True,present=0,graphicsFenceResult=0,
                 roiX=63,roiWidth=3,roiHeight=7,width=128,height=256,columns=[6*Q]*128,
                 rgba=pixels.hex(),sha=sha256(pixels).hexdigest(),particleMask=clear.hex(),
                 maskSHA=sha256(clear).hexdigest(),activeSand=int(commit==1))
        f['binding'] = {k:f[k] for k in SCOPE}
        roi_frames.append(f)
    roi_case = dict(frames=roi_frames,bedRGBA=[200,150,80,255],downNs=0,upNs=1_500_000_000)
    support = dict(angleQ8=11520,dx=100,g=100,gx=71,gn=71,muS=32768,slopes=[0],
                   faces=[0,5,0],before=dict(S=[40,40],F=[10,0],P=[0,0],origins={'a':90}),
                   after=dict(S=[40,40],F=[5,5],P=[0,0],origins={'a':90}))
    return land_case,bed_only,roi_case,support


def selftest():
    landing,bed_only,pixels,support = fixtures()
    gold = dict(land=land(landing),roi=roi(pixels),g15=g15(support))
    controls = []
    for label, gate, item, mutate, code in (
        ('bed-only',land,bed_only,lambda e:None,'LAND_MISSING'),
        ('wrong-generation',land,landing,lambda e:e['archiveBinding'].update(generation=99),'LAND_SCOPE'),
        ('wrong-source',land,landing,lambda e:e['origins']['1'].update(root=8),'LAND_SOURCE'),
        ('particle-occluded',roi,pixels,lambda e:[f.update(particleMask=(b'\1'*21).hex(),
                                  maskSHA=sha256(b'\1'*21).hexdigest()) for f in e['frames']],
         'ROI_BUILD_MISSING_OR_OCCLUDED'),
        ('wall-leak',g15,support,lambda e:e['faces'].__setitem__(0,1),'G15_DOMAIN'),
        ('source-drift',g15,support,lambda e:e['after']['origins'].update(a=89),'G15_ORIGIN_BALANCE'),
        ('unsupported-slope',g15,support,lambda e:e['slopes'].__setitem__(0,61),'G15_SUPPORT'),
    ):
        item = deepcopy(item)
        mutate(item)
        T900.rejects(lambda:gate(item),code)
        controls.append(label)
    seed = dict(mass=.02,radius=.004,gap=.025,normalVelocity=0,wallVelocity=0,acceleration=-9.81,e=.1)
    contacts = dict(fixed=isolated(seed),toward=isolated(dict(seed,wallVelocity=.1)),
                    receding=isolated(dict(seed,wallVelocity=-.1)),rotating=rotating_wall(),
                    free=isolated(dict(seed,normalVelocity=1,acceleration=0)),
                    supportPast90=isolated(dict(seed,gap=0,supportDt=1/120)))
    need(contacts['toward']['toi']<contacts['fixed']['toi']<contacts['receding']['toi']
         and contacts['free']['impulse']==0,'G16_CONTACT_FIXTURE')
    poses = {}
    for name in ('G15-positive','G15-negative','G16-invert'):
        path = ART/'t843'/f'{name}.json'
        doc = json.loads(path.read_bytes())
        T843.check(doc)
        samples = doc['samples']
        omega=max(abs(Fraction(r[2])) for r in samples)
        need(name.startswith('G16') or omega<=85,'G15_POSE_SPEED')
        poses[name]=dict(samples=len(samples),peakDegreesPerSec=float(omega),sha=sha256(path.read_bytes()).hexdigest())
    beds = dict(flat=[0]*128,gentle=[i*10_000 for i in range(128)],
                **{'guard-limited':[557546 if i==63 else 0 for i in range(128)],
                   'mobile':[330000 if i>=64 else 0 for i in range(128)]})
    branches = {key:[angle_case(bed,a) for a in (0,3840,7680,-3840,0)] for key,bed in beds.items()}
    originals = {p:source_totals(ART/'t901'/p) for p in ('a-longhold.jsonl','a-one-grant.jsonl')}
    need([v['count'] for v in originals.values()]==[2,37],'T901_SOURCE_COUNT')
    arm=dict(device='fixture_gpu',driver='fixture',seedSHA='fixture',sourceSHA='fixture',shaderSHA='fixture',
             draw=True,mode='quantum-only',temperatureMode='warm',queryPeriodNs=2,timestampBits=8,
             root=1,active=0,submit=0,sameFence=True,fence=0,domain='GPU_TIMESTAMP',
             spans={key:[250,4] for key in load_plan()['phasePoints']})
    pair_input=dict(light=arm,heavy=dict(arm,root=2,active=32))
    gold['load']=load_pair(pair_input)
    wrong=deepcopy(pair_input)
    wrong['heavy']['seedSHA']='wrong'
    T900.rejects(lambda:load_pair(wrong),'LOAD_PAIR_SCOPE')
    controls.append('mismatched-load-seed')
    return dict(synthetic=gold,controls=controls,contacts=contacts,poses=poses,angleBranches=branches,
                t901=originals,newGpuEvidence='PENDING')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('selftest','plan','land','roi','load','g15','contact'))
    p.add_argument('input',type=Path,nargs='?')
    args = p.parse_args()
    if args.mode in ('selftest','plan'):
        result = selftest() if args.mode=='selftest' else load_plan()
    else:
        if args.input is None:
            p.error('mode requires an input JSON')
        gate = {'land':land,'roi':roi,'load':load_pair,'g15':g15,'contact':isolated}[args.mode]
        result = gate(json.loads(args.input.read_bytes()))
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
