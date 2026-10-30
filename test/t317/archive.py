from pathlib import Path
import gzip,hashlib,json,re
r=Path(__file__).resolve().parents[2];out=r/'test/reports/t317'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
sign=json.loads((r/'test/reports/t257/T317-final.json').read_text())
assert sign['verifyApp']=='PASS' and sha(Path(sign['signed']['path']))==sign['signed']['sha256']
checks={}
for name in ['bridge-regression','delays','m1-run','ui-pause-drain','app','m0-app']:
 s=(out/(name+'.txt')).read_text(encoding='utf-8',errors='replace');rows=[]
 for line in s.splitlines():
  if line.startswith('{'):
   try:rows.append(json.loads(line))
   except ValueError:pass
 summary=rows[-1];assert summary['failed']==0;checks[name]=summary
cold=json.loads((out/'cold.json').read_text());assert cold['failed']==0;checks['cold']=cold
assert sum(x['passed'] for x in checks.values())==79
log=(r/'.codex/local-test/t317-final-package.log').read_bytes();assert b'BUILD SUCCESSFUL' in log
(out/'package.log.gz').write_bytes(gzip.compress(log));(out/'package-tail.txt').write_bytes(b'\n'.join(log.splitlines()[-8:]))
(out/'sign.json').write_text(json.dumps(sign,indent=2)+'\n')
files=list((r/'entry/src/main/cpp/gpu_resident').rglob('*'))
files=[p for p in files if p.is_file() and p.suffix in ['.h','.cpp']]+[r/'entry/src/main/ets/physics/GpuBridgeTimelineCold.ets']
result={'revision':'T317_FIRST_ROOT_PREPARATION_V1','taskStatus':'LOOK_AT_COMMENTS','scope':'PARTIAL_REAL_TIME_SUBMISSION_BOUND_NOT_CLOSED','offlinePassed':79,'offlineFailed':0,'checks':checks,'build':'BUILD_SUCCESSFUL','sign':sign['signed'],'verifyApp':'PASS','stageRevision':'T317_STAGE39_V2','capacity':1024,'device':'NOT_RUN','productPerformance':'NOT_PROVEN','remaining':'partial-submit/continuation/cancel-before-fence contract required from architect; delayed Submit still raw CLOCK_GAP','sourceHashes':{str(p.relative_to(r)).replace('\\','/'):sha(p) for p in files}}
(out/'delivery.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'passed':79,'failed':0,'signedSHA':sign['signed']['sha256'],'status':result['taskStatus']}))
