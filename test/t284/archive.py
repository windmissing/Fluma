from pathlib import Path
import json,hashlib,gzip,sys
r=Path(__file__).resolve().parents[2];project=Path(sys.argv[1]).resolve();assert project.is_relative_to(r/'.codex/local-test');report=r/'test/reports/t284';sha=lambda b:hashlib.sha256(b).hexdigest()
manifest=json.loads((project/'t284-manifest.json').read_text());log=(project.parent/(project.name+'-package.log')).read_bytes();assert b'BUILD SUCCESSFUL' in log
for name,count in [('selftest.txt',15),('reassembly.txt',4),('clock-physics.txt',24),('land-parts.txt',16)]:
 s=(report/name).read_text();assert '"failed": 0' in s or '"failed":0' in s;assert ('"passed": '+str(count)) in s or ('"passed":'+str(count)) in s
(report/'package.log.gz').write_bytes(gzip.compress(log,mtime=0));(report/'generated-manifest.json').write_text(json.dumps(manifest,indent=2))
files=['test/t276/generate.cjs','test/t284/apply.cjs','test/t284/T284Cold.ets','test/t284/selftest.cjs','test/t284/reassembly_test.cjs','test/t284/reassemble.cjs']
result={'revision':'T284_BOUNDED_COLD_DELIVERY_V1','date':'2026-10-30','project':str(project),'build':'BUILD_SUCCESSFUL','device':'NOT_RUN','productionPhysics':'UNCHANGED','dtClock':'UNCHANGED','render':'XCOMPONENT_SHADER_ONLY','testsPassed':59,'testsFailed':0,'manifest':manifest,'signature':json.loads((r/'test/reports/t257/T284.json').read_text()),'files':[{'path':f,'sha256':sha((r/f).read_bytes())} for f in files],'limits':'time checked between bounded operations; one non-preemptible emit/capture may exceed measured limit then INCOMPLETE/BUDGET, never success','longTrace':'bounded prefix up to64 rows, omitted count/incomplete explicit; no full-run completeness claim'}
(report/'delivery.json').write_text(json.dumps(result,indent=2));print(json.dumps({'build':'BUILD_SUCCESSFUL','passed':59,'failed':0,'signedSha256':result['signature']['signed']['sha256']}))
