"""Verify both T728 build closures before handing either to the existing signer."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'test/offline_handoff'))
from package import build_id

def require(ok,reason):
    if not ok:
        raise ValueError(reason)

def content(path,digest):
    raw=path.read_bytes()
    require(hashlib.sha256(raw).hexdigest()==digest,'SHA_MISMATCH '+str(path))
    return raw

def relative(base,name):
    path=(base/name).resolve()
    require(path.is_relative_to(base),'PATH_OUTSIDE_ROOT')
    return path

def read_arm(path,variant):
    item=json.loads(path.read_bytes())
    require(item['variant']==variant,'VARIANT_MISMATCH')
    project=Path(item['project']).resolve()
    require(project.is_relative_to(ROOT/'.codex/local-test'),'PROJECT_PATH')
    parent=Path(item['parentReceipt']).resolve()
    content(parent,item['parentSHA256'])
    hap=relative(project,item['unsigned'])
    require(hap.name.endswith('unsigned.hap'),'UNSIGNED_NAME')
    unsigned=content(hap,item['unsignedSHA256'])
    import io
    with zipfile.ZipFile(io.BytesIO(unsigned)) as archive:
        native=archive.read('libs/arm64-v8a/libgrs_g13.so')
    require(len(native)>20 and native[:4]==b'\x7fELF' and
            int.from_bytes(native[18:20],'little')==183,'G13_ELF_ARCH')
    require(hashlib.sha256(native).hexdigest()==item['nativeSHA256'] and
            build_id(native)==item['buildId'] and bool(item['buildId']),'G13_ELF_IDENTITY')
    seen=set()
    compute={}
    for role,record in item['roles'].items():
        source=relative(project,record['path'])
        if record.get('sourceNewlines')=='CPP_TRANSLATION_LF':
            raw=content(source,record['sourceSHA256']).replace(b'\r\n',b'\n')
            require(hashlib.sha256(raw).hexdigest()==record['sha256'],'ROLE_CANONICAL_SHA '+role)
        else:
            raw=content(source,record['sha256'])
        raw.decode('utf8',errors='strict')
        require(len(raw)==record['bytes'] and (raw+b'\0') in native,'ROLE_BYTES '+role)
        if record['kind']=='compute':
            ops=record['opcodes']
            require(isinstance(ops,list) and ops and all(type(op) is int and op>=0 for op in ops)
                    and len(set(ops))==len(ops) and not set(ops)&seen,'OPCODE_OVERLAP')
            seen.update(ops)
            compute[role]=set(ops)
        else:
            require(record['kind']=='graphics' and not record['opcodes'],'ROLE_KIND')
        if variant=='SPLIT' and record['reworked']:
            require(len(raw)<=49152,'T727_SHADER_SIZE '+role)
    require({64,65,66}<=seen and len(seen)>3,'MISSING_BOOT_OR_STEP')
    if variant=='MONOLITH':
        require(len(compute)==1,'MONOLITH_COMPUTE_ROLE_COUNT')
    else:
        boot=[role for role,ops in compute.items() if ops=={64,65,66}]
        require(len(compute)>1 and len(boot)==1 and item['roles'][boot[0]]['reworked'],
                'SPLIT_BOOT_CLOSURE')
    return item,project,seen

def pair(monolith,split):
    a,b=[read_arm(path,v) for path,v in ((monolith,'MONOLITH'),(split,'SPLIT'))]
    x,y=a[0],b[0]
    require(x['parentSHA256']==y['parentSHA256'],'PARENT_SHA_MISMATCH')
    require(x['buildId']!=y['buildId'] and x['unsignedSHA256']!=y['unsignedSHA256'],'DUPLICATE_ARMS')
    require(a[2]==b[2],'OPCODE_COVERAGE_MISMATCH')
    graphics=lambda item:{k:r['sha256'] for k,r in item['roles'].items() if r['kind']=='graphics'}
    require(graphics(x) and graphics(x)==graphics(y),'GRAPHICS_SOURCE_MISMATCH')
    return [a,b]

def main():
    p=argparse.ArgumentParser()
    p.add_argument('monolith',type=Path)
    p.add_argument('split',type=Path)
    p.add_argument('output',type=Path)
    p.add_argument('--verify-only',action='store_true')
    args=p.parse_args()
    require(not args.output.exists(),'DEPLOYMENT_RECEIPT_EXISTS')
    arms={}
    for item,project,_ in pair(args.monolith,args.split):
        variant=item['variant']
        arm={'unsignedSHA256':item['unsignedSHA256'],'nativeSHA256':item['nativeSHA256'],
             'buildId':item['buildId'],'roles':item['roles']}
        if not args.verify_only:
            task='T729-'+variant.lower()+'-'+item['unsignedSHA256'][:12]
            subprocess.run([sys.executable,'test/offline_handoff/package.py',task,str(project),
                            '--unsigned-sha',item['unsignedSHA256']],cwd=ROOT,check=True)
            receipt=ROOT/'test/reports'/task.lower()/'handoff.json'
            signed=json.loads(receipt.read_bytes())
            require((signed['entries']['libs/arm64-v8a/libgrs_g13.so']['sha256'],
                     signed['entries']['libs/arm64-v8a/libgrs_g13.so']['buildId'])==
                    (arm['nativeSHA256'],arm['buildId']),'SIGNED_PAYLOAD_IDENTITY')
            arm.update(signedSHA256=signed['signed']['sha256'],signedPath=signed['signed']['path'],
                       handoff=str(receipt))
        arms[variant]=arm
    raw={'schema':'T729_DEPLOYMENT_V1','parentSHA256':json.loads(args.monolith.read_bytes())['parentSHA256'],
         'signed':not args.verify_only,'device':'NOT_RUN','arms':arms}
    with args.output.open('x',encoding='utf8') as stream:
        json.dump(raw,stream,indent=2)
        stream.write('\n')

if __name__=='__main__':
    main()
