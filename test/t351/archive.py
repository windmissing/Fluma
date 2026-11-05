from pathlib import Path
import json,hashlib,gzip
r=Path(__file__).resolve().parents[2];out=r/'test/reports/t351';project=r/'.codex/local-test/t351-project'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
unit=json.loads((out/'units.json').read_text());scene=json.loads((out/'scene.json').read_text());strict=json.loads((out/'regression/t216/selftest.json').read_text());budget=json.loads((out/'budget.json').read_text())
assert unit['failed']==scene['failed']==strict['failed']==0 and len(budget['profiles'])==32
recovery=json.loads((out/'report-recovery.json').read_text())
for row in recovery['files']:assert sha(r/row['file'])==row['recoveredSHA'],'Original deterministic report changed after isolated strict run'
sources=['pages/P051DerivedV2Demo.ets','physics/P011ParticleHot.ets','physics/WaterSandField.ets','physics/derived/P011DerivedCommitV2.ets','physics/derived/P011DerivedSnapshotV2.ets','physics/derived/P011DerivedTupleBarrierV2.ets','physics/derived/P011FrameBudgetV2.ets','physics/derived/P051DerivedDemoV2.ets','physics/derived/P051DerivedFacadeV2.ets','render/P013SceneController.ets']
for s in sources:assert (r/'entry/src/main/ets'/s).read_bytes()==(project/'entry/src/main/ets'/s).read_bytes(),s
signs={}
for kind in ['production','isolated']:
 sign=json.loads((r/('test/reports/t257/T351-'+kind+'-final.json')).read_text());assert sign['verifyApp']=='PASS' and sha(Path(sign['signed']['path']))==sign['signed']['sha256'];signs[kind]=sign
 (out/(kind+'-sign.json')).write_text(json.dumps(sign,indent=2)+'\n')
 log=(r/('.codex/local-test/t351-'+kind+'-signable-package.log')).read_bytes();assert b'BUILD SUCCESSFUL' in log;(out/(kind+'-package.log.gz')).write_bytes(gzip.compress(log));(out/(kind+'-package-tail.txt')).write_bytes(b'\n'.join(log.splitlines()[-8:]))
summary=[]
for p in budget['profiles']:
 if p['mode'] in ['before','after-on']:
  total=sum(s['total'] or 0 for s in p['stages']);summary.append({'scenario':p['scenario'],'mode':p['mode'],'L':p['startLive'],'p50':p['wallP50'],'p95':p['wallP95'],'stageExclusiveMs':total,'stages':p['stages'],'counterCalls':p['authorityCounterCalls']})
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
result={'revision':'T351_DELIVERY_V1','baselineCommit':'5970fbd','device':'NOT_RUN','scope':'CPU_V2_ENGINEERING_AND_OFFLINE_CHECKS','status':'DELIVERED','differentialCases':unit['passed'],'sceneCases':scene['passed'],'strictSuites':strict['passed'],'budgetProfiles':32,'measuredHostFrames':768,'referenceBudgetMs':33.333333,'devicePerformance':'T352_REQUIRED_NOT_PROVEN','nativeTiming':'NAPI_DOUBLE_NOT_DEVICE_TIME','strictVerification':'STRUCTURE_EXACT70_BYTE_COMPARE_ALL_RETAINED','normalComparisonNewViewsPerPublication':0,'coldComparisonViews':63,'frameTraceRows':128,'frameTraceTypedBytes':132736,'runTokenLimits':[256,512],'artifacts':{k:v['signed'] for k,v in signs.items()},'verifyApp':'PASS','sourceHashes':{s:sha(r/'entry/src/main/ets'/s) for s in sources}}
(out/'delivery.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))
