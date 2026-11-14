from pathlib import Path
import hashlib,json,shutil,zipfile
ROOT=Path(__file__).resolve().parents[2];LOCAL=ROOT/'.codex/local-test';REPORT=ROOT/'test/reports/t502';REPORT.mkdir(parents=True,exist_ok=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
store_log=(LOCAL/'t502-store-evidence.txt').read_text(encoding='utf-8-sig')
rows=[json.loads(line) for line in store_log.splitlines() if line.startswith('{')]
assert rows[0]['passed']==12 and rows[0]['failed']==0 and len(rows)==3 and all(row['passed'] for row in rows[1:])
metrics=[json.loads(line) for line in (LOCAL/'t502-store-evidence/data/metrics.ndjson').read_text().splitlines()]
assert len(metrics)==13 and any(m['tempBytes']>0 and m['cleanupFailures']>0 for m in metrics)
assert any(m['tempBytes']>134217728 and m['diskRoomRejected'] for m in metrics)
assert any(m['diskRoomRejected'] for m in metrics) and any(m['queueRejected'] for m in metrics) and any(m['pinRejected'] for m in metrics)
regression=(LOCAL/'t502-regression3.txt').read_text(encoding='utf-8-sig');assert 'T476 store differential: 26/26 PASS' in regression and 'PASS persistence/restart' in regression
graphics=(LOCAL/'t502-graphics-run.txt').read_text(encoding='utf-8-sig');graphic_result=json.loads(graphics.splitlines()[-1]);assert graphic_result['passed']==16 and graphic_result['failed']==0
legacy=(LOCAL/'t502-legacy-compute/run.txt').read_text();assert 'route=BINARY_LOAD key=7234cbb48b952df7966f31a83f07bde24db8f7d210cc2ecbb64f4e0b4c86d153' in legacy
delivery={'revision':'T502_DEVELOPER_DELIVERY_V1','device':'NOT_RUN','store':rows,'graphics':graphic_result,'legacyCompute':'BINARY_LOAD_OLD_KEY','routes':{}}
for route,payload in [('m1',37728720),('m2',38388176)]:
 project=LOCAL/('t502-'+route+'-final');base=project/'entry/src/main/cpp/gpu_resident';manifest=json.loads((project/'t502-manifest.json').read_text())
 for name,digest in manifest['files'].items():assert sha((base/name).read_bytes())==digest,name
 expected=(ROOT/'entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h').read_text()
 actual=(base/'program_cache/artifact_store.h').read_text().replace('#include <sstream>\n','',1).replace('  std::string Stats501(){return Diagnostics502();}\n','')
 assert expected==actual
 old=LOCAL/('t501-'+route+'-final/entry/src/main/cpp/gpu_resident')
 assert (base/'program_cache/program_factory.h').read_bytes()==(old/'program_cache/program_factory.h').read_bytes()
 assert (base/'resource_ledger.h').read_bytes()==(old/'resource_ledger.h').read_bytes()
 app=LOCAL/('t502-app-'+route);app_log=LOCAL/(app.name+'.txt');assert app_log.read_text(encoding='utf-8-sig').count('first real host pulse baseline; zero cold quota PASS')==2
 for phase in ['cold','warm']:
  ledger=json.loads((app/('ledger-'+phase+'.json')).read_text());assert ledger['incomplete']==0 and ledger['liveBytes']==payload;shutil.copyfile(app/('ledger-'+phase+'.json'),REPORT/('prepared-'+route+'-'+phase+'.json'))
 build=LOCAL/('t502-'+route+'-final-build.txt');assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
 receipt=json.loads((ROOT/('test/reports/t257/T502-'+route+'-delivery.json')).read_text());assert receipt['verifyApp']=='PASS';signed=Path(receipt['signed']['path']);assert sha(signed.read_bytes())==receipt['signed']['sha256']
 with zipfile.ZipFile(signed) as z:elf=z.read('libs/arm64-v8a/libgrsm1.so')
 assert elf==(project/'entry/build/default/intermediates/stripped_native_libs/default/arm64-v8a/libgrsm1.so').read_bytes()
 assert b'T502_CACHE_STORAGE_V1' in elf and b'cacheStorageState' in elf
 delivery['routes'][route]={'manifest':manifest,'signed':receipt['signed'],'elfSHA256':sha(elf),'build':'PASS','coldWarmApp':'PASS'}
 shutil.copyfile(project/'t502-manifest.json',REPORT/('generation-'+route+'.json'));shutil.copyfile(app_log,REPORT/('app-'+route+'.txt'));shutil.copyfile(build,REPORT/('build-'+route+'.txt'))
for src,dst in [('t502-store-evidence.txt','store.txt'),('t502-regression3.txt','regression.txt'),('t502-graphics-run.txt','graphics.txt'),('t502-legacy-compute/run.txt','legacy-compute.txt'),('t502-store-evidence/data/metrics.ndjson','store-metrics.ndjson'),('t502-store-evidence/seed-metrics.ndjson','seed-metrics.ndjson'),('t502-store-evidence/restart-metrics.ndjson','restart-metrics.ndjson')]:shutil.copyfile(LOCAL/src,REPORT/dst)
delivery['status']='DEVELOPER_COMPLETE';(REPORT/'delivery.json').write_text(json.dumps(delivery,indent=2)+'\n');print(json.dumps({'status':'PASS','storeCases':12,'processRestart':True,'legacyStore':'PASS/26','graphicsCases':16,'buildSignVerify':True}))
