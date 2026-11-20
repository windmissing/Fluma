"""Check unchanged native inputs in both unsigned and signed HAPs."""
import hashlib
import json
from pathlib import Path
import sys
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'offline_handoff'))
from package import build_id

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def inspect(file):
    with zipfile.ZipFile(file) as z:
        names=[n for n in z.namelist() if n.startswith('libs/') and n.endswith('.so')]
        assert len(names)==len(set(names))
        libs={n:{'sha256':sha(z.read(n)),'bytes':len(z.read(n)),
                 'BuildId':build_id(z.read(n))} for n in names}
        module=json.loads(z.read('module.json'))['module']
        entry={k:module[k] for k in ('name','mainElement','pages','abilities')}
        return libs,entry,sha(z.read('ets/modules.abc'))

def verify(project, tag):
    staged=json.loads((project/'t694-staging.json').read_bytes())
    signing=json.loads(Path(f'test/reports/t257/{tag}.json').read_bytes())
    assert signing['verifyApp']=='PASS'
    baseline=Path(staged['baselinePackage']['path'])
    unsigned,signed=(Path(signing[k]['path']) for k in ('input','signed'))
    for file,digest in [(baseline,staged['baselinePackage']['sha256']),
                        (unsigned,signing['input']['sha256']),(signed,signing['signed']['sha256'])]:
        assert sha(file.read_bytes())==digest, file
    orig,entry,oldabc=inspect(baseline)
    un,unentry,unabc=inspect(unsigned)
    new,newentry,newabc=inspect(signed)
    assert {k:v['sha256'] for k,v in orig.items()}==staged['native']
    assert orig==un==new and all(v['BuildId'] for v in orig.values())
    assert entry==unentry==newentry
    assert oldabc!=newabc==unabc
    page=project/staged['page']
    raw=page.read_bytes()
    assert sha(raw)==staged['pageSHA'] and b'XComponent(' in raw and b"libraryname: 'grsm1'" in raw.replace(b"libraryname:'grsm1'",b"libraryname: 'grsm1'")
    assert b'./src/main/cpp/CMakeLists.txt' not in (project/'entry/build-profile.json5').read_bytes()
    return {'native':new,'baseline':staged['baselinePackage'],'oldAbcSHA':oldabc,
            'newAbcSHA':newabc,'pageSHA':staged['pageSHA'],'entry':newentry,
            'signed':signing['signed'],'device':'NOT_RUN','runtimeLoadedModule':'REQUIRES_DEVICE'}

if __name__=='__main__':
    print(json.dumps(verify(Path(sys.argv[1]),sys.argv[2]),indent=2))
