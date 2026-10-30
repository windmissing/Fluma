from pathlib import Path
import json,hashlib,gzip,subprocess
r=Path(__file__).resolve().parents[2];p=r/'test/reports/t311';sha=lambda b:hashlib.sha256(b).hexdigest();project=r/'.codex/local-test/t311-candidate'
units=json.loads((p/'units.json').read_text());scales=json.loads((p/'scaling.json').read_text());assert units['failed']==scales['failed']==0
before=json.loads((p/'static-before.json').read_text());after=json.loads((p/'static-current.json').read_text());old={(v['owner'],v['text']) for v in before['allocations']};new={(v['owner'],v['text']) for v in after['allocations']};assert not new-old
(p/'static-comparison.json').write_text(json.dumps({'beforeStatus':before['status'],'afterStatus':after['status'],'beforeRemaining':before['remaining'],'afterRemaining':after['remaining'],'newExplicitAllocationSites':[],'gate':'NOT_PASSED','need':'architect first attribution; do not change analyzer verdict/whitelist in developer task'},indent=2))
for name in ['static-before','static-current']:
 src=p/(name+'.json');(p/(name+'.json.gz')).write_bytes(gzip.compress(src.read_bytes(),mtime=0))
manifest=json.loads((project/'t311-manifest.json').read_text());files=[v['file'] for v in manifest['files']];manifest['finalBuiltFiles']=[{'file':f,'sha256':sha((project/'entry/src/main/ets'/f).read_bytes())} for f in files];(p/'manifest.json').write_text(json.dumps(manifest,indent=2))
for source,name in [(r/'.codex/local-test/t311-package.log','production-package'),(r/'.codex/local-test/t311-candidate-package.log','candidate-package')]:
 text=source.read_text(errors='replace');assert 'BUILD SUCCESSFUL' in text;(p/(name+'-tail.txt')).write_text('\n'.join(text.splitlines()[-6:]));(p/(name+'.log.gz')).write_bytes(gzip.compress(source.read_bytes(),mtime=0))
sign=json.loads((p/'sign-final.json').read_text());prod=json.loads((p/'production-sign.json').read_text());assert sign['sign']==sign['verify']==prod['sign']==prod['verify']=='PASS'
for label,info,pack in [('T311-final',sign,project/'entry/build/default/outputs/default/entry-default-unsigned.hap'),('T311-production',prod,r/'entry/build/default/outputs/default/entry-default-unsigned.hap')]:
 assert sha(pack.read_bytes())==info['inputSha'];assert sha((r/'.codex/local-test/t257-sign'/label/(label+'-signed.hap')).read_bytes())==info['signedSha']
changed=['P011ParticleHot.ets','P011ParticleField.ets','P011ParticleM2Sink.ets','P011ParticleM2Field.ets','P011ParticleHotBudget.ets','WaterSandField.ets']
inputs=[{'file':f,'beforeSha':sha((r/'.codex/local-test/t311-baseline/ets/physics'/f).read_bytes()),'afterSha':sha((r/'entry/src/main/ets/physics'/f).read_bytes())} for f in changed]
(p/'delivery.json').write_text(json.dumps({'task':'T311','status':'LOOK_AT_COMMENTS_STATIC_GATE_INCOMPLETE','runtimePassed':97,'runtimeFailed':0,'checks':{'newUnits':9,'scalingRollback':7,'hotSink':17,'M2':13,'M2Transactions':14,'gesture':18,'cold':15,'coldReassembly':4},'staticGate':{'before':before['remaining'],'after':after['remaining'],'status':'NOT_ZERO_ALLOCATION','newExplicitAllocationSites':0},'productionBuild':'PASS','candidateBuild':'PASS','productionSign':prod,'candidateSign':sign,'candidateProject':str(project),'device':'NOT_RUN','watchdogFixed':'NOT_PROVEN','remainingComplexity':'O(R*(B+N log N+C+budget+derived)), whole-audit reads Omega(R*(N+C)) retained','source':inputs},indent=2))
# The legacy suite wrote its own shared report. Restore only if still exactly
# our captured output; preserve any intervening other agent change.
legacy=r/'test/reports/t168-rev2/effect-selftest.json';captured=p/'static-adversarial.json'
if legacy.read_bytes()==captured.read_bytes():legacy.write_bytes(subprocess.check_output(['git','show','HEAD:test/reports/t168-rev2/effect-selftest.json']))
print(json.dumps({'runtimePassed':97,'builds':'PASS','static':'INCOMPLETE','candidateSha':sign['signedSha']}))
