"""T893 offline checks for T889 scene evidence and independent DM2/R1 roles."""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import struct
import subprocess
import sys

from scene import ROOT, BIND, Q, RESULTS, SCOPE, SEAL, angle_range, empty, gravity, land, need, roi, u32sha
from roles import COLD_PROFILES, SCENE, SCOPE as ROLE_SCOPE, SPEC, cold_plan, qualify, r1_lease, read_family, sha


def static(family,folder=None):
    return read_family(family,folder or ROOT/'.codex/local-test'/f't886-{family}-release')


def tail(nonce=9,geometry=2,generation=3):
    t=[0]*256
    t[:4]=[1381192263,2,256,SEAL]
    t[4],t[6],t[18],t[20],t[22],t[24],t[26],t[27],t[28],t[36],t[38]=(nonce,1,1,2,1,1,1,2,1,generation,geometry)
    t[14]=1
    return t


def a_cases():
    scope=dict.fromkeys(SCOPE,2)
    frame=dict(scope,**dict.fromkeys(RESULTS,0),renderer='SHADER',presentEnterQpc=20,presentExitQpc=21,
               kind='EMPTY_READY',root=None,generation=3,sourceCommit=0,acceptedSteps=0,permit=None,
               inventoryQ20=[0,0])
    zero=dict(init=dict(scope,fence=0,zeroInventory=True,zeroResourceGeneration=3,fenceQpc=15),
              frame=frame,ready=dict(scope,qpc=22),
              invalidatedInput=[dict(geometry=1,accepted=False,reason='STALE_GEOMETRY')])
    rows=[]
    for i,requested in enumerate((0,3840,-3840,0),1):
        geo=2 if i<3 else 3
        if i==3:
            rows.append(dict(phase='RESIZE',geometry=geo))
            rows.append(dict(phase='INPUT',geometry=2,seq=20,angle=0,result='STALE_GEOMETRY'))
        if i==2:
            rows.append(dict(phase='INPUT',geometry=geo,seq=i,angle=90*256,result='ANGLE_OUT_OF_RANGE'))
        rows.append(dict(phase='INPUT',geometry=geo,seq=i,angle=requested,result='ACCEPTED',qpc=10*i))
        rows.append(dict(phase='GRANT',geometry=geo,seq=i,angle=requested,permit=i,qpc=10*i+1,activeBefore=0))
        bed=[0]*128
        if i==2:
            bed[64]=600000
        lo,hi=angle_range(bed)
        got=min(hi,max(lo,requested))
        t=tail(nonce=20+i,geometry=geo)
        t[22]=i
        t[107]=got&0xffffffff
        rows.append(dict(phase='QUANTUM',geometry=geo,permit=i,root=str(20+i),completion=t,accepted=True,
                         fence=0,ordinal=1,qpc=10*i+2,bedBeforeQ20=bed,grantedAngle=requested,
                         constraintApplied=got!=requested))
    motion=dict(initialGeometry=2,records=rows)

    binding=dict(scope,root='9',generation=3,lease=3,imageIndex=0,graphicsFence='20')
    drawn=dict(binding,**dict.fromkeys(RESULTS,0),renderer='SHADER',presentEnterQpc=20,presentExitQpc=21,
               extent=[256,256],format='RGBA8',rowOrder='TOP_DOWN',srgbEncodings=1)
    w=[0]*7936
    w[7680:7684]=[1347637831,2,256,SEAL]
    for i,v in ((4,1),(12,1),(14,2),(18,2),(32,3),(34,1)):
        w[7680+i]=v
    word=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
    w[3*192]=1
    w[8*192],w[9*192],w[10*192]=[word(x) for x in (96.25,159.5,3)]
    w[5632+8*128+32]=word(4)
    bank=[0]*7424
    bank[0]=100
    bank[5120+256+32]=50
    t=tail()
    t[71]=t[77]=150
    gpu=dict(binding,copyFenceVkResult=0,invalidateVkResult=0,publicationWords=w,bankWords=bank,
             completion=t,publicationSHA=u32sha(w),bankSHA=u32sha(bank),completionSHA=u32sha(t))
    sand=[184,145,89,255]; background=[18,22,31,255]
    probes=[]
    for name,x,y,color in (('bed',64,252,sand),('particle',192,96,sand),('background',120,120,background)):
        data=bytes(color)
        probes.append(dict(binding,label=name,rect=[x,y,1,1],sample=[0,0],bytesHex=data.hex(),sha256=sha(data)))
    paired=dict(frame=drawn,gpu=gpu,probes=probes,palette=dict(sand=sand,background=background))
    b=dict(scope,qpc=10,S=50,P=100,sourceCommit=1)
    a=dict(scope,qpc=11,S=90,P=60,sourceCommit=1,root='9',accepted=True,fence=0)
    t=tail()
    t[71]=t[77]=150
    t[53]=1
    event=[0]*64
    for i,v in ((0,1163088455),(1,2),(2,9),(4,2),(6,8),(7,1),(9,1),(11,1),
                (17,1),(18,2),(32,50),(33,90),(34,100),(35,60),(36,40),(40,40)):
        event[i]=v
    landing=dict(before=b,after=a,upQpc=9,eventWords=event,eventSHA=u32sha(event),completion=t)
    return {'empty':zero,'gravity':motion,'roi':paired,'land':landing}


def b_cases(manifests):
    qualification={}
    for family,m in manifests.items():
        scope=dict.fromkeys(ROLE_SCOPE,1)
        scope['family']=family
        qualification[family]=dict(family=family,scope=scope,backend='VULKAN_PC',roleSetSHA=m['roleSetSHA'],
            roles=[dict(scope,op=r['op'],moduleSHA=r['spvSHA'],shaderModuleVkResult=0,pipelineVkResult=0,
                        pipeline=r['op']+1,enterQpc=1,returnQpc=2) for r in m['roles']],
            graphics=[dict(scope,program=name,stage=r['stage'],moduleSHA=r['spvSHA'],shaderModuleVkResult=0,
                           pipelineVkResult=0,pipeline=i+1,enterQpc=1,returnQpc=2)
                      for i,(name,group) in enumerate(m['graphics'].items()) for r in group])
    cold=[]
    for family,mode in ((family,mode) for family in SPEC for mode in ('ROOT','FULL')):
        names=['BASE','FINAL','SELECTED','AUTHORITY']
        if mode=='FULL':
            names += [f'DISCOVERY_{i}' for i in range(6)]
        schema,state_bytes,arena_bytes=COLD_PROFILES[family]
        scope=qualification[family]['scope']
        at=32768
        segments=[]
        for i,name in enumerate(names):
            size=state_bytes if i<2 else arena_bytes
            segments.append(dict(scope,name=name,offset=at,bytes=size,copyFenceVkResult=0,invalidateVkResult=0,
                                 buffer=i+1,pinned=True,sha256='a'*64))
            at+=size
        cold.append(dict(family=family,mode=mode,schema=schema,preReserved=mode=='FULL',
                         scope=scope,segments=segments,expectedBytes=at))
    key=dict.fromkeys(SCENE,1)
    key['family']='r1'
    scene=dict(key,visualTicks=256,acceptedQuanta=1,graphicsFence='4')
    witness=dict(key,kind='G11_PHASE_WITNESS_V2',bytes=3149824,fence=0,invalidate=0,
                 beginOrdinal=699,applyOrdinal=702,role=2,micro=0,phase=3)
    return qualification,cold,dict(witness=witness,scene=scene,previousLeaseSerial=0,
           proof=dict(key,kind='G11_JOINT_PUBLICATION_V2',accepted=True),
           mapping=dict(key,graphicsFence='4',imageIndex=0,enterQpc=2,exitQpc=3,**dict.fromkeys(RESULTS,0)))


def tests(manifests):
    checks=a_cases()
    functions=dict(empty=empty,gravity=gravity,roi=roi,land=land)
    a={k:functions[k](e) for k,e in checks.items()}
    q,cold,lease=b_cases(manifests)
    b={'static':{f:dict(compute=len(m['roles']),graphics=sum(map(len,m['graphics'].values())),
                         roleSetSHA=m['roleSetSHA'],runtime=m['runtimeQualification']) for f,m in manifests.items()},
       'syntheticRoleCreation':{f:qualify(manifests[f],e) for f,e in q.items()},
       'syntheticCold':[cold_plan(e) for e in cold],'syntheticR1':r1_lease(lease,manifests['r1'])}
    negatives=[]
    def neg(fn, sample, mutate, why):
        candidate=deepcopy(sample)
        mutate(candidate)
        try:
            fn(candidate)
        except ValueError as exc:
            need(str(exc)==why,f'{why}: {exc}')
        else:
            raise AssertionError(why)
        negatives.append(why)
    for name,changes,why in (
        ('empty',lambda e:e['frame'].update(root='9'),'EMPTY_NULL_ROOT'),
        ('empty',lambda e:e['ready'].update(geometry=5),'EMPTY_EPOCH'),
        ('gravity',lambda e:e['records'][3].update(result='ACCEPTED',angle=99999),'GRAVITY_INPUT_REJECT'),
        ('gravity',lambda e:e['records'][-1].update(geometry=2),'GRAVITY_QUANTUM'),
        ('roi',lambda e:e['probes'][1].update(root='5'),'ROI_FRAME_IDENTITY'),
        ('roi',lambda e:e['probes'][1].update(rect=[255,96,2,1]),'ROI_RECT'),
        ('roi',lambda e:e['gpu'].update(copyFenceVkResult=2),'ROI_GPU_JOIN'),
        ('land',lambda e:e.update(upQpc=11),'LAND_AFTER_UP_ROOT'),
        ('land',lambda e:e.update(eventSHA='0'*64),'LAND_EVENT_RANGE'),
    ):
        neg(functions[name],checks[name],changes,why)
    neg(lambda e:qualify(manifests['dm2'],e),q['r1'],lambda e:None,'RUNTIME_ROLE_FAMILY')
    neg(lambda e:qualify(manifests['dm2'],e),q['dm2'],lambda e:e['roles'][0].update(generation=2),'RUNTIME_ROLE_CREATION')
    neg(cold_plan,cold[1],lambda e:e.update(preReserved=False),'FULL_DISCOVERY_NOT_RESERVED')
    neg(cold_plan,cold[0],lambda e:e['segments'][0].update(family='r1'),'COLD_BACKING')
    neg(cold_plan,cold[2],lambda e:e['segments'][0].update(bytes=4),'COLD_BACKING')
    neg(lambda e:r1_lease(e,manifests['r1']),lease,lambda e:e['mapping'].update(geometry=2),'R1_LEASE_SCOPE')
    neg(lambda e:r1_lease(e,manifests['r1']),lease,lambda e:e['witness'].update(applyOrdinal=1968),'R1_WITNESS_SLOTS')
    neg(lambda e:r1_lease(e,manifests['r1']),lease,lambda e:e['mapping'].update(graphicsFence='5'),'R1_PRESENT_LEASE')
    return dict(sceneFixtures=a,roleFixtures=b,negatives=negatives)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('selftest','static','empty','gravity','roi','land','cold','roles','r1-lease'))
    p.add_argument('path',type=Path,nargs='?')
    p.add_argument('--family',choices=tuple(SPEC))
    p.add_argument('--build-dir',type=Path)
    args=p.parse_args()
    if args.mode=='selftest':
        out=tests({k:static(k) for k in SPEC})
        previous=json.loads((ROOT/'test/reports/t888/offline.json').read_bytes())
        need(previous['A']['empty-idle']['queued']==0 and previous['A']['dual-release']['fieldState']=='MISSING_EVIDENCE',
             'T888_EXISTING_SCOPE')
        out['originalT884']={'emptyFrames':0,'LAND':'MISSING_EVIDENCE','ROI':'MISSING_EVIDENCE'}
        for name,params in (('t836/check.py',['selftest']),('t883/check.py',['selftest']),
                            ('t888/check.py',['--selftest'])):
            subprocess.run([sys.executable,str(ROOT/'test'/name),*params],cwd=ROOT,check=True,capture_output=True)
        out['regressions']=['T836','T883','T888']
        out['actualRoles']='PENDING_T890'
    elif args.mode=='static':
        m=static(args.family,args.build_dir)
        out=dict(family=m['family'],roleSetSHA=m['roleSetSHA'],roles=m['roles'],graphics=m['graphics'],
                 runtimeQualification=m['runtimeQualification'])
    else:
        e=json.loads(args.path.read_bytes())
        if args.mode=='roles':
            out=qualify(static(args.family,args.build_dir),e)
        elif args.mode=='r1-lease':
            out=r1_lease(e,static('r1',args.build_dir))
        else:
            out={'empty':empty,'gravity':gravity,'roi':roi,'land':land,'cold':cold_plan}[args.mode](e)
    print(json.dumps(out,indent=2))


if __name__=='__main__':
    main()
