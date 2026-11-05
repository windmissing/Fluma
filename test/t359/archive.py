from pathlib import Path
import json,hashlib,shutil
r=Path(__file__).resolve().parents[2];p=r/'.codex/local-test/t359-handoff';out=r/'test/reports/t359';out.mkdir(parents=True,exist_ok=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
sign=json.loads((r/'test/reports/t257/T359-handoff.json').read_text());assert sign['verifyApp']=='PASS'
assert sha(Path(sign['signed']['path']).read_bytes())==sign['signed']['sha256']
log=(r/'.codex/local-test/t359-handoff.log').read_bytes();assert b'BUILD SUCCESSFUL' in log
units=json.loads((r/'.codex/local-test/t359-units.json').read_text());assert units['failed']==0 and units['passed']==7
shutil.copyfile(r/'.codex/local-test/t359-units.json',out/'units.json');shutil.copyfile(r/'test/reports/t257/T359-handoff.json',out/'sign.json');(out/'build.txt').write_bytes(log)
manifest=json.loads((p/'t359-manifest.json').read_text());(out/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
sources={str(f.relative_to(p)):sha(f.read_bytes()) for f in (p/'entry/src/main').rglob('*') if f.is_file() and f.suffix in ['.ets','.cpp','.h','.glsl','.vert','.frag']}
delivery={'revision':'T359_DELIVERY_V1','status':'DEVELOPMENT_COMPLETE','device':'NOT_RUN_T360','project':str(p),'compile':'PASS_ARKTS_NATIVE','package':'PASS','signVerify':'PASS','signed':sign['signed'],'unsigned':sign['input'],'sourceSHA':manifest['sourceSHA'],'sourceHashes':sources,'tests':units,'ringRows':256,'rowWords':48,'anchorIds':3,'hotJsonRows':0,'typedReceiptBytes':256*48*8+3*4+8,'sourceTouched':'ISOLATED_COPY_ONLY_T351_CURRENT_INPUTS_RETAINED'}
(out/'delivery.json').write_text(json.dumps(delivery,indent=2)+'\n');print(json.dumps({k:delivery[k] for k in ['status','compile','signVerify','sourceSHA','signed']}))
