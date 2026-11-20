"""Check manifest and archive joining with explicit unsigned test fixtures."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import zipfile
from sign_pair import ROOT, pair
from units import reject

def sha(data):
    return hashlib.sha256(data).hexdigest()

def make(base,name,tag,sources,parent):
    project=base/name
    project.mkdir()
    header=bytearray(20)
    header[:4]=b'\x7fELF'
    struct.pack_into('<H',header,18,183)
    native=bytes(header)+struct.pack('<III',4,len(tag),3)+b'GNU\0'+tag
    roles={}
    for role,kind,opcodes,raw in sources:
        path=project/(role+'.glsl')
        path.write_bytes(raw)
        roles[role]=dict(path=path.name,kind=kind,opcodes=opcodes,bytes=len(raw),
                         sha256=sha(raw),reworked=name=='SPLIT' and kind=='compute')
        native+=raw+b'\0'
    package=project/'fixture-unsigned.hap'
    with zipfile.ZipFile(package,'w') as out:
        out.writestr('libs/arm64-v8a/libgrs_g13.so',native)
    item=dict(variant=name,project=str(project),parentReceipt=str(parent),
              parentSHA256=sha(parent.read_bytes()),unsigned=package.name,
              unsignedSHA256=sha(package.read_bytes()),nativeSHA256=sha(native),buildId=tag.hex(),roles=roles)
    path=project/'arm.json'
    path.write_text(json.dumps(item))
    return path,item

if __name__=='__main__':
    with TemporaryDirectory(prefix='t729-sign-unit-',dir=ROOT/'.codex/local-test') as work:
        base=Path(work)
        parent=base/'parent.json'
        parent.write_text('{"fixture":true}')
        graph=('SCENE','graphics',[],b'test scene')
        a,orig=make(base,'MONOLITH',bytes.fromhex('12345678'),
                    [('WHOLE','compute',[0,64,65,66],b'test entire'),graph],parent)
        b,item=make(base,'SPLIT',bytes.fromhex('45678901'),
                    [('BOOT','compute',[64,65,66],b'test boot'),('STEP','compute',[0],b'test step'),graph],parent)
        assert pair(a,b)[0][2]=={0,64,65,66}
        other=base/'other.json'
        other.write_text('{"fixture":false}')
        cases=[
          ('PARENT_SHA_MISMATCH',lambda d:d.update(parentReceipt=str(other),parentSHA256=sha(other.read_bytes()))),
          ('OPCODE_OVERLAP',lambda d:d['roles']['STEP'].update(opcodes=[64])),
          ('OPCODE_COVERAGE_MISMATCH',lambda d:d['roles']['STEP'].update(opcodes=[1])),
          ('G13_ELF_IDENTITY',lambda d:d.update(buildId=orig['buildId'])),
          ('ROLE_BYTES',lambda d:d['roles']['BOOT'].update(bytes=1)),
        ]
        for code,change in cases:
            bad=deepcopy(item)
            change(bad)
            b.write_text(json.dumps(bad))
            reject(lambda:pair(a,b),code)
        print(json.dumps({'syntheticPair':'VERIFIED_ONLY','negativeCases':[c for c,_ in cases],
                          'signed':False,'device':'NOT_RUN'},indent=2))
