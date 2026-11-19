"""Update normative mathematical semantics only; preserve historical T607 evidence."""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[1];D=R/'docs'
def get(n):return json.loads((D/n).read_text(encoding='utf-8-sig'))
def put(n,x):(D/n).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(x):return json.dumps(x,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()
def receipt(n):
 b=(D/n).read_bytes();return dict(path='docs/'+n,bytes=len(b),sha256=sha(b))
m=get('architecture-T607-g13-math-v1.json');m.pop('mathSHA',None)
m['requiredExpRelError']='2^-20 total including approximation and actual F32 conversion; representability alone insufficient; public17/math bit4 otherwise'
m['domainContract']=get('architecture-T628-math-domain-v1.json')
m['mathSHA']=sha(canonical(m));put('architecture-T607-g13-math-v1.json',m)
e=get('architecture-T607-g13-engineering-v1.json');e['hashes']['mathSHA']=m['mathSHA']
e['hashes']['algorithmSHA']=sha(canonical(dict(designMD=receipt('algorithm-G13-pure-fluid-gpu-v1.md')['sha256'],
 designManifest=receipt('design-G13-pure-fluid-manifest-v1.json')['sha256'],mathSHA=m['mathSHA'])))
e['T628Supplement']=receipt('architecture-T628-math-domain-v1.json')
put('architecture-T607-g13-engineering-v1.json',e)
print('T607 math semantics synchronized; no layout/gate/dispatch or product change')
