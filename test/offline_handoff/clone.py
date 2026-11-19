"""Shared fresh isolated output copier, preserves source but never caches or SDK."""
from pathlib import Path
import shutil,subprocess,json
ROOT=Path(__file__).resolve().parents[2]
def clone(source,out):
 source=Path(source).resolve();out=Path(out).resolve();assert out.is_relative_to(ROOT/'.codex/local-test') and not out.exists()
 shutil.copytree(source,out,ignore=shutil.ignore_patterns('build','.cxx','.hvigor','oh_modules','.git','node_modules','*.log'))
 subprocess.run(['cmd','/c','mklink','/J',str(out/'oh_modules'),str(ROOT/'oh_modules')],check=True,capture_output=True)
 p=out/'build-profile.json5';d=json.loads(p.read_text());d['app'].pop('signingConfigs',None)
 for x in d['app']['products']:x.pop('signingConfig',None)
 p.write_text(json.dumps(d,indent=2)+'\n');return out
