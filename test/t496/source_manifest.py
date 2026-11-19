"""Freeze tester/source/header closure without device access."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[2]
def collect():
 paths=[]
 for name in ['test/t496','test/t488','test/t485','test/t324','.codex/local-test/t55-gpu/sdk','.codex/local-test/t492-m1-delivery/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident','.codex/local-test/t492-m1-delivery/entry/src/main/cpp','test/t295','.codex/local-test/t281','.codex/local-test/t236-source-host','.codex/local-test/t280','.codex/local-test/t279','.codex/local-test/t240-source']:
  root=ROOT/name
  paths.extend(p for p in root.rglob('*') if p.is_file() and p.suffix.lower() in ('.h','.inc','.cpp','.c','.py','.cmd','.lib'))
 rows=[{'path':str(p.relative_to(ROOT)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(set(paths))]
 cache=[]
 for directory in ['.codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1','.codex/local-test/t496/t657r3-files/gpu-program-cache/v1']:
  for p in sorted((ROOT/directory).glob('*')):
   if p.is_file():cache.append({'path':str(p.relative_to(ROOT)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
 return {'cacheInputs':cache,'schema':'T496_SOURCE_CLOSURE_V1','files':rows,'device':'NOT_RUN','frozenProduct':'t492-m1-delivery; current root not substituted'}
if __name__=='__main__':print(json.dumps(collect(),indent=2))
