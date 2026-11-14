from pathlib import Path
import hashlib,json,shutil,zipfile
ROOT=Path(__file__).resolve().parents[2];LOCAL=ROOT/'.codex/local-test';REPORT=ROOT/'test/reports/t501';REPORT.mkdir(parents=True,exist_ok=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
delivery={'revision':'T501_DEVELOPER_DELIVERY_V1','device':'NOT_RUN','driver':'DRIVER_NOT_MEASURED','routes':{}}
for route,buffers,payload in [('m1',17,37728720),('m2',20,38388176)]:
 project=LOCAL/('t501-'+route+'-final');base=project/'entry/src/main/cpp/gpu_resident'
 manifest=json.loads((project/'t501-manifest.json').read_text())
 for name,digest in manifest['files'].items():assert sha((base/name).read_bytes())==digest,name
 control=LOCAL/('t501-controls-'+route+'.txt');rows=[json.loads(s) for s in control.read_text(encoding='utf-8-sig').splitlines() if s.startswith('{')]
 assert [r['passed'] for r in rows]==[23,20,7] and all(r['failed']==0 for r in rows)
 app=LOCAL/('t501-app-'+route);app_log=LOCAL/(app.name+'.txt');assert app_log.read_text(encoding='utf-8-sig').count('first real host pulse baseline; zero cold quota PASS')==2
 for phase in ['cold','warm']:
  ledger=json.loads((app/('ledger-'+phase+'.json')).read_text());assert ledger['liveBytes']==payload and ledger['incomplete']==0
  assert sum(o['objectKind']==1 for o in ledger['objects'])==buffers
  assert sum(o['objectKind']==3 for o in ledger['objects'])==5
  assert ledger['driverAccounting']=='DRIVER_NOT_MEASURED'
  shutil.copyfile(app/('ledger-'+phase+'.json'),REPORT/('prepared-'+route+'-'+phase+'.json'))
 exit_dir=LOCAL/('t501-exit-'+route);exits=[]
 for mode in range(9):
  result=json.loads((exit_dir/('exit-'+str(mode)+'.json')).read_text());e=result['exit'];assert e['attempted'] and e['shutdown']
  if 1<=mode<=4:
   assert not e['complete'] and e['firstRawEgl'] and e['heldWindow']!='0'
   if mode<4:assert result['retiringBytes']>0
  else:assert e['complete'] and result['liveBytes']==result['retiringBytes']==0
  if mode>=5:assert any(h['firstRawEgl'] for h in result['contextExits'])
  shutil.copyfile(exit_dir/('exit-'+str(mode)+'.json'),REPORT/('exit-'+route+'-'+str(mode)+'.json'));exits.append(e)
 source=LOCAL/('t501-source-'+route+'.txt');r=json.loads(source.read_text(encoding='utf-8-sig').splitlines()[-1]);assert r['failed']==0 and r['admittedJQ']==r['settledAfterUpQ']>0
 build=LOCAL/('t501-'+route+'-final-build.txt');assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
 receipt=json.loads((ROOT/('test/reports/t257/T501-'+route+'-delivery.json')).read_text());assert receipt['verifyApp']=='PASS';signed=Path(receipt['signed']['path']);assert sha(signed.read_bytes())==receipt['signed']['sha256']
 with zipfile.ZipFile(signed) as z:elf=z.read('libs/arm64-v8a/libgrsm1.so')
 assert elf==(project/'entry/build/default/intermediates/stripped_native_libs/default/arm64-v8a/libgrsm1.so').read_bytes()
 assert b'T501_OWNER_RESOURCE_LEDGER_V1' in elf and b'resourceState' in elf
 for p,n in [(control,'controls'),(app_log,'app'),(source,'source'),(build,'build'),(LOCAL/('t501-exit-'+route+'.txt'),'exit')]:shutil.copyfile(p,REPORT/(n+'-'+route+'.txt'))
 shutil.copyfile(project/'t501-manifest.json',REPORT/('generation-'+route+'.json'))
 delivery['routes'][route]={'buffers':buffers,'logicalPayload':payload,'programs':5,'controls':rows,'exitCases':9,'sourceGL':r,'signed':receipt['signed'],'elfSHA256':sha(elf),'manifest':manifest}
scene=LOCAL/'t501-scene-pixels/resource-end.json';d=json.loads(scene.read_text());assert d['liveBytes']==d['retiringBytes']==d['incomplete']==0 and d['allocFailures']==1
shutil.copyfile(scene,REPORT/'scene-resource-end.json');shutil.copyfile(LOCAL/'t501-scene-run.txt',REPORT/'scene-run.txt')
delivery['status']='DEVELOPER_COMPLETE';(REPORT/'delivery.json').write_text(json.dumps(delivery,indent=2)+'\n');print(json.dumps({'status':'PASS','routes':2,'exitCases':18,'native':'23+20+7 each','buildSignVerify':True}))
