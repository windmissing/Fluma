"""T953 independent slot, GridView and particle observation checks."""
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

ROOT=Path(__file__).resolve().parents[2]
ART=ROOT/'test/reports'


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


T948=load('t948','test/t948/check.py')
need,raw,PLAN=T948.need,T948.raw,T948.PLAN
words,pair=T948.T922.words,T948.T922.pair
OLD=T948.T888.OLD
SLOT_ID=('run','nonce','fence','fenceGeneration','generation')
VIEW_ID=('run','root','token','commit','generation','geometry','stateBank','fence')


def schedule(e):
    proof=e['latch']
    need(all(proof[k]==e['identity'][k] for k in SLOT_ID) and proof['fenceVkResult']==0
         and proof['available'] is True,'LATCH_SCOPE')
    j=proof['J']
    need(j in (0,1,2,4,8,16,32) and e['rejectedAll']==(j==0),'LATCH_J')
    steps=e['steps']
    need(steps and e['controlStart']==1,'ORDER_START')
    at=1
    dispatch=1
    removed=0
    for s in steps:
        begin,end=s['begin'],s['end']
        need(begin==at==s['counterBefore'] and begin<end<=len(PLAN)
             and s['counterAfter']==end,'ORDER_CONTROL16')
        dispatch+=1
        if s['kind']=='COUNT_ONLY':
            need(j>0 and s['authoritativeLatch'] and s['generation']==proof['generation']
                 and s['writes']==s['events']==s['paths']==0,'EMPTY_EXTENT_WITNESS')
            for p in PLAN[begin:end]:
                need(p['role'] in ('AUTHORITATIVE','REPLAY') and p['micro']>=j>=1,
                     'EMPTY_EXTENT_OVERLAP')
            removed+=end-begin-1
        else:
            need(s['kind']=='DISPATCH' and end==begin+1,'ORDER_STEP')
        at=end
    need(at==e['sealedControl16']==len(PLAN) and e['dispatches']==dispatch==len(PLAN)-removed,
         'ORDER_SEAL')
    names=('BASE','ACTUAL','SELECTED','REPLAY','EVENT','PATH','REJECTED_PREFIX','UNTOUCHED','ORDER_CERTIFICATE')
    a,b=(e[key] for key in ('baseline','candidate'))
    need(set(a)==set(b)==set(names),'ORDER_COMPLETE_RAW')
    for n in names:
        need(raw(a[n])==raw(b[n]),f'ORDER_RAW_DIFF:{n}')
    need(e['replayFromBase'] and not e['replayAliasesActual'] and
         e['firstReject']==e['baselineFirstReject'] and e['mask']==e['baselineMask'],'ORDER_REPLAY_OR_REJECT')
    if j==0:
        need(e['firstReject'] is not None and e['acceptedRoot'] is None and e['currentBefore']==e['currentAfter']
             and not removed,'ALL_REJECTED_PUBLISHED')
    return dict(j=j,control16=at,logicalSlots=len(PLAN),physicalDispatches=dispatch,saved=removed)


def handoff(e):
    root,view=e['root'],e['grid']
    need(all(root[k]==view[k] for k in VIEW_ID) and root['root']==root['acceptedRoot']
         and root['acceptedRoot'] is not None and root['typed'] is True and root['actualFence']==root['submit']==0
         and 0<root['grantNs']<=root['fenceNs']<=root['promoteNs']<=root['deadlineNs']
         and root['deadlineNs']-root['grantNs']==250_000_000 and (root['J'] in (1,2,4)),
         'GRIDVIEW_LATE_OR_SCOPE')
    need(view['frame']=='BOX_X_RIGHT_Y_UP' and view['units']==dict(length='m',velocity='m/s',pressure='Pa',shear='Pa',
         density='kg/m3',viscosity='Pa*s',interval='s')
         and view['faceOrder']=='MAC_XFAST_U_LEFT_V_BOTTOM' and view['pressureGauge']=='PI_PLUS_PHI_MINUS_MEAN_PLUS_REF',
         'GRIDVIEW_COORDINATE_OR_UNITS')
    nx,ny=view['shape']
    need(nx>=2 and ny>=2 and len(view['origin'])==2 and len(view['spacing'])==2
         and all(math.isfinite(x) for x in (*view['origin'],*view['spacing'],view['rho'],view['mu'],view['pRef']))
         and min(*view['spacing'],view['rho'],view['mu'])>0
         and view['interval'][0]<view['interval'][1],'GRIDVIEW_EXTENT')
    arrays={k:words(raw(view[k])) for k in ('u','v','phi','pi','wallVelocity','wallShear')}
    need(len(arrays['u'])==(nx+1)*ny and len(arrays['v'])==nx*(ny+1) and len(arrays['phi'])==nx*ny
         and len(arrays['pi'])==2*nx*ny and len(arrays['wallVelocity'])==len(arrays['wallShear'])==2*(nx+ny),
         'GRIDVIEW_RAW_RANGE')
    decode=T948.T866.F.float_word
    phi=[decode(v) for v in arrays['phi']]
    mean=math.fsum(phi)/len(phi)
    pi=[struct.unpack('<q',struct.pack('<II',*arrays['pi'][i:i+2]))[0]/2**32 for i in range(0,len(arrays['pi']),2)]
    total=[a+b-mean+view['pRef'] for a,b in zip(pi,phi,strict=True)]
    u,v,wall,shear=([decode(y) for y in arrays[k]] for k in ('u','v','wallVelocity','wallShear'))
    need(all(math.isfinite(x) for x in u+v+wall+shear+total),'GRIDVIEW_NONFINITE')
    dx,dy=view['spacing']
    # Bottom/top tangential traction follows the declared wall-on-fluid sign.
    expected=[view['mu']*(wall[x]-.5*(u[x]+u[x+1]))/(dy/2) for x in range(nx)]
    expected += [view['mu']*(wall[nx+x]-.5*(u[(ny-1)*(nx+1)+x]+u[(ny-1)*(nx+1)+x+1]))/(dy/2) for x in range(nx)]
    expected += [view['mu']*(wall[2*nx+y]-.5*(v[y*nx]+v[(y+1)*nx]))/(dx/2) for y in range(ny)]
    expected += [view['mu']*(wall[2*nx+ny+y]-.5*(v[y*nx+nx-1]+v[(y+1)*nx+nx-1]))/(dx/2) for y in range(ny)]
    need(len(e['physicalPressure'])==nx*ny and all(abs(a-b)<=1e-5 for a,b in zip(total,e['physicalPressure'],strict=True)),
         'GRIDVIEW_PHYSICAL_PRESSURE')
    need(all(abs(a-b)<=1e-5 for a,b in zip(expected,shear,strict=True)),'GRIDVIEW_WALL_SHEAR')
    return dict(pressurePa=total,wallShearPa=shear,root=root['root'])


def demo_birth(prefix,s,down,source_qpc):
    need(down['kind']=='DOWN' and int(down['hold'])>0 and int(down['qpc'])<source_qpc,'DEMO_BEFORE_DOWN')
    root=pair(s,4)
    found=[]
    for row in prefix:
        if row[4:7]!=[1,1,1] or pair(row,2)!=root:
            continue
        need(row[17]>0 and row[18]>0 and pair(row,48)>0 and row[23]>0,'DEMO_BIRTH_WORDS')
        found.append(dict(slot=row[16],generation=row[17],id=row[18],origin=pair(row,48),q20=row[23],
                          xy=row[26:28]))
    need(len(found)==s[66] and found and sum(r['q20'] for r in found)==s[61]
         and len({(r['id'],r['generation'],r['origin']) for r in found})==len(found),'DEMO_BIRTH_COUNT')
    return found


def free_motion(e):
    track=e['frames']
    need(len(track)>=3 and e['downQpc']<track[0]['presentQpc'],'DEMO_BEFORE_DOWN')
    key=('id','generation','origin','run','geometry')
    previous=None
    for r in track:
        need(all(r[k]==e['birth'][k] for k in key) and r['root']==r['leaseRoot']==r['roiRoot']
             and r['slot']==r['roiSlot'] and r['publicationGeneration']==r['roiGeneration']
             and r['image']==r['roiImage'] and r['graphicsFence']==r['roiFence']
             and r['computeVkResult']==r['graphicsVkResult']==r['presentVkResult']==0
             and r['drawnAngle']==r['acceptedAngle'] and abs(r['acceptedAngle'])==5120
             and r['pixelClass']=='SAND' and r['colorMatched'] and not r['borderMaskHit'],'DEMO_ROI_OR_LINEAGE')
        pixels=bytes.fromhex(r['roiRGBA'])
        mask=bytes.fromhex(r['roiBorderMask'])
        x,y,w,h=r['roiRect']
        px,py=r['particleScreenPx']
        need(w>0 and h>0 and len(pixels)==4*w*h and len(mask)==w*h
             and sha256(pixels).hexdigest()==r['roiSHA'] and sha256(mask).hexdigest()==r['roiMaskSHA']
             and x<=px<x+w and y<=py<y+h,'DEMO_ROI_BYTES')
        index=(math.floor(py)-y)*w+math.floor(px)-x
        need(not mask[index] and list(pixels[4*index:4*index+4]) in e['sandPaletteRGBA'],'DEMO_PARTICLE_PIXEL')
        if previous:
            need(r['commit']==previous['commit']+1 and r['presentQpc']>previous['presentQpc']
                 and r['root']!=previous['root'] and r['eventKind']==3 and r['dry'] and not r['contact']
                 and r['tau']==[0,65536],'DEMO_FREE_STEP')
            vx,vy=map(int,r['deltaVQ20'])
            move=words(raw(r['moveEvent']))
            signed=lambda x:x-(1<<32) if x>=1<<31 else x
            need(len(move)==64 and move[4:7]==[2,2,3] and pair(move,2)==r['root']
                 and (move[17],move[18],pair(move,48))==(r['generation'],r['id'],r['origin'])
                 and move[56:58]==[0,65536]
                 and [signed(y)-signed(x) for x,y in zip(move[28:30],move[30:32],strict=True)]==[vx,vy],
                 'DEMO_MOVE_RAW_LINEAGE')
            need(all(signed(move[26+i])-signed(move[24+i])==
                     round(Fraction(signed(move[28+i])+signed(move[30+i]),240)) for i in (0,1)),
                 'DEMO_FREE_POSITION')
            if 'moveEvent' in previous:
                prev=words(raw(previous['moveEvent']))
                need(move[24:26]==prev[26:28] and move[28:30]==prev[30:32],'DEMO_TRACK_GAP')
            sign=1 if r['acceptedAngle']>0 else -1
            need(abs(vx-sign*1530172)<=1 and abs(vy+4204113)<=1,'DEMO_FREE_ACCELERATION')
            c,s=r['rotation']
            need(abs(c*c+s*s-1)<1e-5 and abs(c*vx+s*vy)<=.01*abs(vy)
                 and s*vx-c*vy>0,'DEMO_SCREEN_GRAVITY')
        previous=r
    return dict(frames=len(track),state='SAME_PARTICLE_SCREEN_FREE_FALL')


def sealed(directory):
    p,=directory.glob('*/envelope.json')
    e=json.loads(p.read_bytes())
    files={}
    for r in e['files']:
        b=(p.parent/r['name']).read_bytes()
        need(len(b)==r['bytes'] and sha256(b).hexdigest()==r['sha256'],'SEALED_FILE_SHA')
        files[r['name']]=b
    return e,files


def history():
    base=ART/'t949/runs'
    d=base/'goal9/raw-q-timing'
    q,f=sealed(d/'diagnostic')
    s,source=sealed(d/'accepted')
    rows=OLD.read_rows(base/'goal9/q-timing.jsonl')[0]
    root,=[r for r in rows if r.get('stage')=='ROOT' and r.get('kind')=='QUANTUM']
    gpu,=[r for r in rows if r.get('stage')=='ROOT_GPU_TIMESTAMPS' and r.get('kind')=='QUANTUM']
    need(all(root[k]==gpu[k] for k in ('run','nonce','fence','fenceGeneration'))
         and q['rootFence']==root['fence'] and q['rootFenceGeneration']==root['fenceGeneration']
         and s['acceptedRoot'] is not None and q['acceptedRoot'] is None and root['accepted'] is False
         and q['rootVkResult']==root['vkResult']==2 and root['tailRead'] is False
         and gpu['acceptedRoot'] is None and gpu['fenceVkResult']==0 and int(gpu['queryResetSerial'])>0
         and len(gpu['points'])==15 and all(r['available'] and r['vkResult']==0 for r in gpu['points']),
         'T949_SAME_FENCE_DIAGNOSTIC')
    need(f['CURRENT_BASE.bin']==f['BASE.bin']==source['ACTUAL.bin'] and f['INPUT128.bin']==f['GPU_INPUT128.bin'],
         'T949_CONTINUATION_BASE')
    controls = struct.unpack_from('<I',f['WORK.bin'],1166976+16*4)[0]
    base16=words(f['BASE.bin'])[16]
    actual16=words(f['ACTUAL.bin'])[16]
    need(controls==2404 and (base16,actual16)==(1,2) and q['selectedJ']==1
         and len(f['EVENT.bin'])==q['eventSealed']*256 and len(f['PATH.bin'])==q['pathSealed']*64,
         'T949_CONTROL16_LAYOUT')
    g=OLD.read_rows(base/'goal13/f02-split.jsonl')[0]
    roles={r['role']:r['moduleSHA'] for r in g if r.get('call')=='vkCreateComputePipelines' and r.get('edge')=='EXIT'
           and r['vkResult']==0 and r['role'] in ('OP48_VISCOSITY','OP49_PRESSURE','OP48_REMAINDER')}
    wait,=[r for r in g if r.get('stage')=='T877_ROOT_WAIT']
    lg,=[r for r in g if r.get('stage')=='T877_GPU_TIMESTAMPS' and r.get('state')=='LATE_RETIRED_DIAGNOSTIC']
    last,=[r for r in g if r.get('stage')=='T833_ROOT_RETIRE_WAIT' and r.get('state')=='FAILED']
    need(wait['firstStop']=='GRANT_DEADLINE' and lg['acceptedRoot'] is None and lg['businessRejected'] is True
         and lg['fenceVkResult']==0 and all(wait[k]==lg[k] for k in ('nonce','fence','fenceGeneration'))
         and last['vkResult']==2,'T949_F02_NO_ACCEPTANCE')
    path=ART/'t947/runs/pixel-birth'
    trace=OLD.read_rows(path/'demo.jsonl')[0]
    arc,=path.glob('root-*.m1root')
    mag,*sizes,digest=struct.unpack_from('<8s8I32s',arc.read_bytes())
    payload=arc.read_bytes()[72:]
    need(mag==b'M1PROOT1' and sum(sizes)==len(payload) and sha256(payload).digest()==digest,'DEMO_ARCHIVE')
    qwords=words(payload[:1024])
    ev=1024+sizes[1]
    arena=[words(payload[ev+i:ev+i+256]) for i in range(0,sizes[2],256)]
    row=next(r for r in trace if r.get('m1Nonce')=='16800600508902' and r.get('m1PointerSeq')=='8'
             and r.get('m1OwnerCode')==0 and r.get('m1Completion'))
    sc=row['m1Completion']
    cmd=OLD.D1.raw_bytes(row['m1SourceWords'],row['m1SourceWordsSHA'],32)
    down=next(r for r in trace if r.get('stage')=='WIN32_POINTER' and r['kind']=='DOWN' and r['hold']==row['m1Hold'])
    pointer=next(r for r in trace if r.get('stage')=='WIN32_POINTER' and r['seq']==row['m1PointerSeq'])
    need(int(pointer['qpc'])<=int(pair(sc,4)) and pointer['geometry']==down['geometry']==row['geometry']
         and pointer['hold']==down['hold'] and sc[27:29]==[1,1] and row['m1FenceVkResult']==0 and
         pair(qwords,18)==pair(sc,18),'DEMO_SOURCE_POINTER_JOIN')
    births=demo_birth(arena[:qwords[52]],sc,down,pair(sc,4))
    old=arena[4]
    need(old[4:7]==[2,2,3] and old[18]==5 and old[17]==1 and pair(old,48)==2,'DEMO_OLD_MOVE')
    capture=next(r for r in trace if r.get('windowFrame',{}).get('acceptedRoot')==str(pair(qwords,4))
                 and r.get('m1Observation'))
    obs=capture['m1Observation']
    view=capture['m1CompactView']
    wf=capture['windowFrame']
    need(obs['root']==wf['acceptedRoot']==view['nonce'] and obs['generation']==wf['generation']==view['publicationGeneration']
         and obs['graphicsFence']==view['graphicsFence'] and obs['imageIndex']==wf['imageIndex']
         and sha256(bytes(obs['data'])).hexdigest()==obs['sha256'],'DEMO_CAPTURE_JOIN')
    x,y,qty=struct.unpack_from('<ddd',cmd,64)
    # Horizontal compact view anchors the model at (64,32).
    xy=(view['center'][0]+view['scale']*(x-64),view['center'][1]-view['scale']*(y-32))
    need(math.dist(xy,(pointer['x']+.5,pointer['y']+.5))<.01
         and not any(a<=xy[0]<a+w and b<=xy[1]<b+h for a,b,w,h in obs['rects']),'DEMO_BIRTH_PIXEL_BOUNDARY')
    return dict(dm2=dict(control16=controls,stateHeaderLastNonce=[base16,actual16],selectedJ=q['selectedJ'],
                        sealedEvents=q['eventSealed'],sealedPaths=q['pathSealed'],businessAccepted=False),
                f02=dict(rolesObserved=roles,businessAccepted=False,firstStop=wait['firstStop']),
                demo=dict(births=births,sourceXY=[x,y],sourceAmount=qty,oldMoveId=old[18],
                          roiAtBirth=False,freeTrajectory='MISSING_ALIGNED_PIXELS'))


def fixtures():
    ident=dict(run=1,nonce=2,fence=3,fenceGeneration=4,generation=5)
    start=next(i for i,p in enumerate(PLAN[:-1]) if p['role']=='REPLAY' and p['micro']>=2
               and PLAN[i+1]['role']=='REPLAY' and PLAN[i+1]['micro']>=2)
    def order(j):
        e=dict(identity=ident,latch=dict(ident,fenceVkResult=0,available=True,J=j),rejectedAll=not j,
               controlStart=1,sealedControl16=2404,dispatches=2404-int(bool(j)),replayFromBase=True,
               replayAliasesActual=False,firstReject=None if j else [3,0],baselineFirstReject=None if j else [3,0],
               mask=7,baselineMask=7,currentBefore='stable',currentAfter='stable',acceptedRoot=2 if j else None)
        e['steps']=[]
        i=1
        while i<2404:
            trim=j and i==start
            n=i+1+int(bool(trim))
            e['steps'].append(dict(begin=i,end=n,counterBefore=i,counterAfter=n,
                                   kind='COUNT_ONLY' if trim else 'DISPATCH',authoritativeLatch=True,
                                   generation=5,writes=0,events=0,paths=0))
            i=n
        a={k:T948.artifact([i]) for i,k in enumerate(('BASE','ACTUAL','SELECTED','REPLAY','EVENT','PATH',
                                                     'REJECTED_PREFIX','UNTOUCHED','ORDER_CERTIFICATE'))}
        e.update(baseline=a,candidate=deepcopy(a))
        return e
    identity=dict(run=1,root=2,token=3,commit=4,generation=5,geometry=6,stateBank=7,fence=8)
    floats=lambda xs:T948.artifact(list(struct.unpack(f'<{len(xs)}I',struct.pack(f'<{len(xs)}f',*xs))))
    v=dict(identity,frame='BOX_X_RIGHT_Y_UP',units=dict(length='m',velocity='m/s',pressure='Pa',shear='Pa',
           density='kg/m3',viscosity='Pa*s',interval='s'),faceOrder='MAC_XFAST_U_LEFT_V_BOTTOM',
           pressureGauge='PI_PLUS_PHI_MINUS_MEAN_PLUS_REF',shape=[2,2],origin=[0,0],spacing=[.5,.5],
           rho=1000,mu=.01,pRef=11,interval=[0,.01],u=floats([0]*6),v=floats([0]*6),phi=floats([1,2,3,4]),
           pi=T948.artifact([0,0]*4),wallVelocity=floats([1]*8),wallShear=floats([.04]*8))
    g=dict(root=dict(identity,acceptedRoot=2,typed=True,actualFence=0,submit=0,grantNs=1,fenceNs=2,
                     promoteNs=3,deadlineNs=250000001,J=1),grid=v,physicalPressure=[9.5,10.5,11.5,12.5])
    particle=dict(id=1,generation=1,origin=1,run=1,geometry=1)
    angle=math.radians(20)
    pixel=bytes([240,180,90,255])
    frames=[dict(particle,commit=i,root=i,leaseRoot=i,roiRoot=i,slot=0,roiSlot=0,publicationGeneration=i,
                 roiGeneration=i,image=1,roiImage=1,graphicsFence=2,roiFence=2,drawnAngle=5120,acceptedAngle=5120,
                 presentQpc=i+10,computeVkResult=0,graphicsVkResult=0,presentVkResult=0,pixelClass='SAND',
                 borderMaskHit=False,colorMatched=True,dry=True,contact=False,eventKind=3,tau=[0,65536],
                 roiRGBA=pixel.hex(),roiSHA=sha256(pixel).hexdigest(),roiBorderMask='00',
                 roiMaskSHA=sha256(bytes(1)).hexdigest(),roiRect=[0,0,1,1],particleScreenPx=[.5,.5],
                 deltaVQ20=[1530172,-4204113],rotation=[math.cos(angle),math.sin(angle)]) for i in (1,2,3)]
    point=[64<<20,64<<20]
    velocity=[0,0]
    for f in frames:
        event=[0]*64
        event[2],event[4],event[5],event[6],event[17],event[18],event[48],event[57]=(f['root'],2,2,3,1,1,1,65536)
        event[24:26]=point
        event[28:30]=[v&0xffffffff for v in velocity]
        after=[a+b for a,b in zip(velocity,f['deltaVQ20'],strict=True)]
        point=[p+round(Fraction(a+b,240)) for p,a,b in zip(point,velocity,after,strict=True)]
        event[26:28]=point
        event[30:32]=[v&0xffffffff for v in after]
        velocity=after
        f['moveEvent']=T948.artifact(event)
    return order(2),order(0),g,dict(birth=particle,downQpc=10,frames=frames,sandPaletteRGBA=[list(pixel)])


def selftest():
    a,b,c,d=fixtures()
    result=dict(order=schedule(a),allRejected=schedule(b),grid=handoff(c),motion=free_motion(d))
    negative=[]
    for func,e,mutate,reason in (
        (schedule,a,lambda x:x['latch'].update(fence=9),'LATCH_SCOPE'),
        (schedule,a,lambda x:x['steps'][15].update(counterAfter=999),'ORDER_CONTROL16'),
        (schedule,a,lambda x:x['candidate'].update(REJECTED_PREFIX=T948.artifact([123])),'ORDER_RAW_DIFF:REJECTED_PREFIX'),
        (schedule,a,lambda x:x.update(replayAliasesActual=True),'ORDER_REPLAY_OR_REJECT'),
        (schedule,b,lambda x:x.update(currentAfter='changed'),'ALL_REJECTED_PUBLISHED'),
        (handoff,c,lambda x:x['root'].update(promoteNs=250000002),'GRIDVIEW_LATE_OR_SCOPE'),
        (handoff,c,lambda x:x['grid'].update(generation=6),'GRIDVIEW_LATE_OR_SCOPE'),
        (handoff,c,lambda x:x['grid'].update(frame='WORLD'),'GRIDVIEW_COORDINATE_OR_UNITS'),
        (handoff,c,lambda x:x['grid']['units'].update(shear='N'),'GRIDVIEW_COORDINATE_OR_UNITS'),
        (handoff,c,lambda x:x['physicalPressure'].__setitem__(0,0),'GRIDVIEW_PHYSICAL_PRESSURE'),
        (handoff,c,lambda x:x['grid'].update(wallShear=T948.artifact([0]*8)),'GRIDVIEW_WALL_SHEAR'),
        (free_motion,d,lambda x:x.update(downQpc=99),'DEMO_BEFORE_DOWN'),
        (free_motion,d,lambda x:x['frames'][1].update(generation=2),'DEMO_ROI_OR_LINEAGE'),
        (free_motion,d,lambda x:x['frames'][1].update(borderMaskHit=True),'DEMO_ROI_OR_LINEAGE'),
        (free_motion,d,lambda x:x['frames'][1].update(moveEvent=T948.artifact([0]*64)),'DEMO_MOVE_RAW_LINEAGE'),
    ):
        e=deepcopy(e)
        mutate(e)
        negative.append(T948.T905.rejects(lambda:func(e),reason))
    return dict(synthetic=result,negative=negative,originals=history())


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('selftest','schedule','gridview','motion'))
    p.add_argument('input',type=Path,nargs='?')
    args=p.parse_args()
    result=selftest() if args.mode=='selftest' else {'schedule':schedule,'gridview':handoff,'motion':free_motion}[args.mode](
        json.loads(args.input.read_bytes()))
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
