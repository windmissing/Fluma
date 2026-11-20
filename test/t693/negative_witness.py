"""Exercise semantic rejection against a copy of one sealed T703 arm."""
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys
from tempfile import TemporaryDirectory

from verify_t703 import validate

def check(directory):
    validate(directory)
    launch=json.loads((directory/'launch.json').read_bytes())
    stem=f"gltrace-{launch['attempt']}.ndjson.t693"
    cases={
        'access': (rb'(target=36662 length=1024 offset=0 access=)1 ',rb'\g<1>2 '),
        'offset': (rb'(target=36662 length=1024 offset=)0( access=1 )',rb'\g<1>1\g<2>'),
        'nullMap': (rb'nonNull=1( contextGeneration=)',rb'nonNull=0\g<1>'),
        'falseUnmap': (rb'success=1( matched=1 mapToken| matched=1 contextGeneration)',rb'success=0\g<1>'),
        'objectGeneration': (rb'(target=36662 length=1024 offset=0 access=1 contextGeneration=)1 ',rb'\g<1>9 '),
        'clockMode': (rb' T=AUTHORIZE,',rb' T=BAD_CLOCK,'),
        'acceptedProfile': (rb'(acceptedRoot .*? w54=)[0-9]',rb'\g<1>9'),
    }
    outcomes={}
    with TemporaryDirectory(prefix='T703-negative-') as work:
        copy=Path(work)
        for source in directory.iterdir():
            if source.is_file():
                shutil.copyfile(source,copy/source.name)
        dictionary=copy/(stem+'.dict')
        index=copy/(stem+'.index')
        data=dictionary.read_bytes()
        blocks=index.read_bytes().splitlines()
        for name,(before,after) in cases.items():
            rejected, count=re.subn(before,after,data,count=1)
            assert count and rejected!=data and len(rejected)==len(data),name
            dictionary.write_bytes(rejected)
            fixed=[]
            for line in blocks:
                kind,offset,size,digest=line.split()
                if kind==b'D':
                    start,length=int(offset),int(size)
                    digest=hashlib.sha256(rejected[start:start+length]).hexdigest().encode()
                fixed.append(b' '.join((kind,offset,size,digest)))
            index.write_bytes(b'\n'.join(fixed)+b'\n')
            try:
                validate(copy)
            except AssertionError as error:
                outcomes[name]=str(error) or 'WITNESS_ASSERTION'
            else:
                raise AssertionError(f'T703_NEGATIVE_ACCEPTED {name}')
        return outcomes

if __name__=='__main__':
    print(json.dumps(check(Path(sys.argv[1])),indent=2))
