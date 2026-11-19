"""T659: reproduce T592's T598 product baseline, overlay real owner/glue sources.
Build preparation only; never runs tests, signs, installs, or uses a device.
"""
from pathlib import Path
import subprocess,tarfile,io,shutil,json,hashlib,sys
ROOT=Path(__file__).resolve().parents[2]
BASE='9ec800f4'
source=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else ROOT/'.codex/local-test/t659-source'
if not source.is_relative_to((ROOT/'.codex/local-test').resolve()) or source.exists():raise RuntimeError('FRESH_ISOLATED_SOURCE_REQUIRED')
paths=['AppScope','entry','hvigor','docs','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5']
paths += ['test/'+n for n in ['t351','t372','t385','t394','t406','t413','t426','t455','t469','t516','t540','t574','t598','t566_generate_composite.cjs','t576_generate_pipeline.cjs']]
raw=subprocess.check_output(['git','archive',BASE,*paths],cwd=ROOT)
source.mkdir(parents=True)
with tarfile.open(fileobj=io.BytesIO(raw)) as archive:archive.extractall(source,filter='data')
shutil.copyfile(ROOT/'local.properties',source/'local.properties')
subprocess.run(['cmd','/c','mklink','/J',str(source/'oh_modules'),str(ROOT/'oh_modules')],check=True,capture_output=True)
overlays=['test/t426/PrepareReadinessV4.ets','test/t426/PreparePairingEvidence.ets','test/t574/t574_prepare_v4_overlay.cjs','test/t574/t659_owner_glue.cjs']
for rel in overlays:
 target=source/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/rel,target)
product=source/'.codex/local-test/product'
result=subprocess.run(['node','test/t598/generate.cjs',str(product)],cwd=source,capture_output=True)
(source/'generation.log').write_bytes(result.stdout+result.stderr)
if result.returncode:print((result.stdout+result.stderr).decode('utf8',errors='replace')[-5000:]);raise RuntimeError('PRODUCT_GENERATION_FAILED')
manifest={'schema':'T681_PRODUCT_SOURCE_V1','baseline':subprocess.check_output(['git','rev-parse',BASE],cwd=ROOT).decode().strip(),'reason':'Same real product baseline as T592; current DiagnosticColdFiles has a different writer API and is outside owner fix scope. No baseline source overwritten in workspace.','overlays':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in overlays},'generated':{str(p.relative_to(product)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (product/'entry/src/main').rglob('*') if p.is_file()}}
(source/'t659-generated-sources.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(product)
