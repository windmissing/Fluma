"""Static Vulkan role inventory and T886 candidate evidence gates."""
from hashlib import sha256
import json
from pathlib import Path
import struct

from scene import match, need

SPEC={
 'dm2':dict(common=240,push=20,source=12,quantum=2404,masks=(65535,262143),count=45,
            lib='pc_sand_dm2.lib',libSHA='24b26e02ff95f628ed16f12224958b515ae2663b790c1a6156cf5a1dc7b27b62',
            rolesSHA='3639e57f834308bab001ca783f321b6a503c232caa924e04b1d8f1d7e3333ca6'),
 'r1':dict(common=304,push=36,source=19,quantum=2543,masks=(851967,851967),count=49,
           lib='pc_sand_r1.lib',libSHA='48f28e28f8382243847fca6aebcb3ddecf46ba11ed71b8020cc3759257881b2f',
           rolesSHA='f7d812d64eba3c932fcfc41d70fbed48835349819f235b6d9d61fb58216641f3'),
}
SCOPE=('family','run','owner','context','device','geometry','generation')
SCENE=SCOPE+('window','surface','permission','swapchain','nonce','commit','visualGeneration','leaseSerial')


def sha(raw):
    return sha256(raw).hexdigest()


def digest_file(path, expected):
    need(sha(path.read_bytes())==expected,'ARTIFACT_SHA')


def reflect(raw, common, push):
    words=struct.unpack(f'<{len(raw)//4}I',raw)
    need(words[0]==0x07230203 and words[1]==0x00010300,'SPIRV_VULKAN11')
    at=5; decorations={}; members={}; pointer={}; variables={}; grid=[]
    while at<len(words):
        n,op=words[at]>>16,words[at]&65535
        need(n and at+n<=len(words),'SPIRV_INSTRUCTION')
        a=words[at+1:at+n]
        if op==71 and len(a)==3 and a[1] in (33,34):
            decorations[a[0],a[1]]=a[2]
        elif op==72 and len(a)==4 and a[2]==35:
            members[a[0],a[1]]=a[3]
        elif op==32:
            pointer[a[0]]=a[2]
        elif op==59:
            variables[a[1]]=(a[0],a[2])
        elif op==16 and len(a)==5 and a[1]==17:
            grid.append(a[2:])
        at+=n
    need(grid==[(64,1,1)],'SPIRV_WORKGROUP')
    bound={obj:b for (obj,d),b in decorations.items() if d==33}
    need(sorted(bound.values())==list(range(5)) and all(decorations.get((obj,34))==0 for obj in bound),
         'SPIRV_BINDINGS')
    uniform=next(obj for obj,b in bound.items() if b==4)
    need(variables[uniform][1]==2 and all(variables[o][1] in (2,12) for o,b in bound.items() if b<4),
         'SPIRV_STORAGE')
    pushed=[t for t,storage in variables.values() if storage==9]
    need(len(pushed)==1,'SPIRV_PUSH')
    for obj,offsets in ((pointer[variables[uniform][0]],common),(pointer[pushed[0]],push)):
        need(sorted((i,v) for (s,i),v in members.items() if s==obj)==list(enumerate(offsets)),
             'SPIRV_MEMBER_OFFSETS')


def read_family(family,folder):
    spec=SPEC[family]
    path=folder/'sand_assets'
    manifest=json.loads((path/'roles.json').read_bytes())
    need(manifest['schema']=='PC_SAND_VULKAN_ROLES_V1' and manifest['family']==family,'ROLE_FAMILY')
    role_sha=manifest['roleSetSHA']
    preimage={k:v for k,v in manifest.items() if k!='roleSetSHA'}
    need(sha(json.dumps(preimage,sort_keys=True,separators=(',',':')).encode())==role_sha==spec['rolesSHA'],
         'ROLE_SET_SHA')
    parent=path/'parent'/('pipeline.comp' if family=='dm2' else 'g11vis_r1.comp')
    need(sha(parent.read_bytes().replace(b'\r\n',b'\n'))==manifest['parentSHA'],'ROLE_PARENT_SHA')
    digest_file(folder/spec['lib'],spec['libSHA'])
    roles=manifest['roles']
    common=[r[2] for r in manifest['constants']]
    push=list(manifest['push'].values())
    need(manifest['commonBytes']==spec['common'] and push==list(range(0,spec['push'],4))
         and len({r['op'] for r in roles})==len(roles)==spec['count'],'ROLE_LAYOUT')
    registered=set(r['op'] for r in roles)
    for r in roles:
        need(r['state']=='STATIC_SPIRV' and r['sourceBytes']<=49152
             and r['spv']==f"role_{r['op']:08x}.spv",'ROLE_STATIC')
        source=(path/Path(r['spv']).with_suffix('.comp')).read_bytes()
        raw=(path/r['spv']).read_bytes()
        need((len(source),sha(source),len(raw),sha(raw))==
             (r['sourceBytes'],r['sourceSHA'],r['spvBytes'],r['spvSHA']),'ROLE_BYTES')
        reflect(raw,common,push)
    for kind,count,mask in zip(('Source','Quantum'),(spec['source'],spec['quantum']),spec['masks'],strict=True):
        plan=manifest['passes'][kind]
        need(len(plan)==count and manifest['masks'][kind]==mask and manifest['complete'][kind] is True
             and all(len(item)==6 and item[0] in registered for item in plan),'ROLE_PLAN')
    graphics=[r for group in manifest['graphics'].values() for r in group]
    need(len(graphics)==(6 if family=='r1' else 0),'ROLE_GRAPHICS_COUNT')
    for r in graphics:
        b=path/r['spv']
        digest_file(b,r['spvSHA'])
        digest_file(b.with_suffix(''),r['sourceSHA'])
        need(b.stat().st_size==r['spvBytes'] and b.with_suffix('').stat().st_size==r['sourceBytes'],
             'ROLE_GRAPHICS_BYTES')
    need(manifest['runtimeQualification']=='NOT_EXECUTED','ROLE_STATIC_CLASSIFICATION')
    return manifest


def qualify(manifest, e):
    scope=e['scope']
    need(e['family']==manifest['family']==scope['family'] and e['roleSetSHA']==manifest['roleSetSHA']
         and e['backend']=='VULKAN_PC','RUNTIME_ROLE_FAMILY')
    expected={(r['op'],r['spvSHA']) for r in manifest['roles']}
    observed={(r['op'],r['moduleSHA']) for r in e['roles']}
    need(observed==expected and len(observed)==len(e['roles']),'RUNTIME_ROLE_INVENTORY')
    expected_graphics={(name,r['stage'],r['spvSHA']) for name,group in manifest['graphics'].items() for r in group}
    graphics=e.get('graphics',[])
    observed_graphics={(r['program'],r['stage'],r['moduleSHA']) for r in graphics}
    need(observed_graphics==expected_graphics and len(graphics)==len(observed_graphics),'RUNTIME_GRAPHICS_INVENTORY')
    for row in e['roles']+graphics:
        need(match(scope,row,SCOPE) and row['shaderModuleVkResult']==row['pipelineVkResult']==0
             and row['pipeline'] and 0<row['enterQpc']<=row['returnQpc'],'RUNTIME_ROLE_CREATION')
    return 'ROLE_CREATION_EVIDENCE'


COLD_PROFILES = {'dm2': ('G9_SPF_DM_COLD_V2', 591872, 12767232),
                 'r1': ('G9_SPF_COLD_V1', 591872, 11718656)}


def cold_plan(e):
    family=e['family']
    mode=e['mode']
    need(family in SPEC and mode in ('ROOT','FULL'),'COLD_FAMILY_MODE')
    expected=['BASE','FINAL','SELECTED','AUTHORITY']
    if mode=='FULL':
        expected += [f'DISCOVERY_{i}' for i in range(6)]
        need(e['preReserved'] is True,'FULL_DISCOVERY_NOT_RESERVED')
    rows=e['segments']
    need([r['name'] for r in rows]==expected,'COLD_SEGMENTS')
    schema,state_bytes,arena_bytes=COLD_PROFILES[family]
    sizes=[state_bytes]*2+[arena_bytes]*(len(rows)-2)
    need(e['schema']==schema and e['scope']['family']==family
         and e['expectedBytes']==32768+sum(sizes),'COLD_LAYOUT')
    offset=32768
    for r,size in zip(rows,sizes):
        need(match(e['scope'],r,SCOPE) and r['offset']==offset and r['bytes']==size
             and r['copyFenceVkResult']==r['invalidateVkResult']==0 and r['buffer'] and r['pinned'] is True
             and len(r['sha256'])==64,'COLD_BACKING')
        offset+=r['bytes']
    need(offset==e['expectedBytes'],'COLD_BYTE_TOTAL')
    return 'COLD_RANGE_EVIDENCE'


def r1_lease(e, manifest):
    witness=e['witness']; proof=e['proof']; scene=e['scene']; mapping=e['mapping']
    need(manifest['family']==proof['family']==scene['family']=='r1'
         and proof['kind']=='G11_JOINT_PUBLICATION_V2' and proof['accepted'] is True
         and match(proof,scene,SCENE) and match(scene,mapping,SCENE),'R1_LEASE_SCOPE')
    need(witness['kind']=='G11_PHASE_WITNESS_V2' and witness['bytes']==3149824
         and match(proof,witness,SCENE) and witness['fence']==witness['invalidate']==0,'R1_WITNESS')
    rows=manifest['passes']['Quantum']
    i,j=witness['beginOrdinal'],witness['applyOrdinal']
    need(0<=i<j<len(rows) and rows[i][0]==513 and rows[j][0]==514
         and rows[i][1:]==rows[j][1:] and rows[i][1]==witness['role']
         and rows[i][3]==witness['micro'] and rows[i][-1]==witness['phase'],'R1_WITNESS_SLOTS')
    need(scene['leaseSerial']>e['previousLeaseSerial'] and scene['visualTicks']==scene['acceptedQuanta']*256
         and mapping['graphicsFence']==scene['graphicsFence'] and mapping['graphicsFence']
         and mapping['imageIndex']>=0 and 0<mapping['enterQpc']<=mapping['exitQpc']
         and mapping['submit']==mapping['fence']==0
         and all(mapping[k] in (0,1000001003) for k in ('acquire','present','imagePresent')),'R1_PRESENT_LEASE')
    return 'R1_WITNESS_AND_LEASE_EVIDENCE'
