"""Compare actual CPU-mapped completion256 bytes, never generated GPU output."""
import hashlib,json,re,struct,sys
from pathlib import Path
from check_actions import check
def load(directory):
 root=Path(directory);launch=json.loads((root/'launch.json').read_text());attempt=launch['attempt']
 rows=[json.loads(l) for l in (root/f't496_log_{attempt}.ndjson').read_text().splitlines()]
 check(rows)
 observed=[r for r in rows if r.get('phase')=='ACCEPTED_OBSERVED']
 assert observed,'NO_ACCEPTED_ROOT_OBSERVATIONS'
 maps=[]
 for line in (root/f'gltrace-{attempt}.ndjson').read_text().splitlines():
  r=json.loads(line)
  if r.get('event')!='CPU_MAP_RETURN_BYTES':continue
  m=re.search(r'sha256=([0-9a-f]{64}) hex=([0-9a-f]{2048})$',r['detail']);assert m,'MAP_SHAPE'
  raw=bytes.fromhex(m[2]);assert hashlib.sha256(raw).hexdigest()==m[1],'MAP_SHA'
  w=struct.unpack('<256I',raw)
  if w[0:4]!=(1381192263,2,256,3235823838):continue
  maps.append({'callId':r['callId'],'sha256':m[1],'words':w})
 assert maps and len(maps)==len(observed),'COMPLETION_ACCEPTED_COUNT'
 assert any(m['words'][27]==1 for m in maps),'SOURCE_EMPTY'
 assert any(m['words'][27]==2 for m in maps),'QUANTUM_EMPTY'
 return launch,rows,observed,maps
def compare(off,on):
 a,ar,ao,am=load(off);b,br,bo,bm=load(on)
 assert a['PE']['sha256']==b['PE']['sha256'],'PE_MISMATCH'
 def actions(rows):
  result=[]
  for r in rows:
   if not str(r.get('actionId','')).isdigit():continue
   phase=r['phase'];entry=[r['actionId'],r['declaredScriptNs']]
   if phase=='PULSE':entry+=['PULSE',r['detail']]
   elif phase.startswith('SOURCE_'):
    m=re.search(r'payloadSHA=([0-9a-f]{64}) words=([0-9,]+)',r['detail']);entry+=[phase,m[1],m[2]]
   else:entry+=['CAPTURE_CONTROL'] # only declared experimental difference
   result.append(entry)
  return result
 assert actions(ar)==actions(br),'DRIVER_INPUT_MISMATCH'
 assert len(am)==len(bm),'ROOT_COUNT_MISMATCH'
 for i,(x,y) in enumerate(zip(am,bm)):
  assert x['words']==y['words'],f'COMPLETION256_MISMATCH_ROOT_{i+1}'
 for i,(x,y) in enumerate(zip(ao,bo)):
  assert x['detail']==y['detail'],f'ACCEPTED_LEDGER_TOKEN_MISMATCH_ROOT_{i+1}'
 return {'result':'PASS','schema':'T496_ACTUAL_ROOT_DIFF_V1','roots':len(am),'sourceRoots':sum(m['words'][27]==1 for m in am),'quantumRoots':sum(m['words'][27]==2 for m in am),'completion256':'byte-identical actual mapped output','acceptedLedgersAndTokens':'exact','actions':52,'PE':a['PE']['sha256'],'device':'NOT_RUN','productPASS':False}
if __name__=='__main__':print(json.dumps(compare(*sys.argv[1:3]),indent=2))
