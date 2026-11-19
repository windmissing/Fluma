from pathlib import Path
import hashlib,json,sys
root=Path(sys.argv[1]).resolve();receipt=json.loads((root/'m5-init-source-manifest.json').read_text(encoding='utf8'))
for name,expected in receipt['files'].items():
 path=root/name
 if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=expected:raise SystemExit('M5_INIT_SOURCE_STALE '+name)
raw=json.dumps(receipt['files'],separators=(',',':'),ensure_ascii=False).encode()
if hashlib.sha256(raw).hexdigest()!=receipt['sourceManifestSHA']:raise SystemExit('M5_INIT_MANIFEST_SHA')
header=(root/'entry/src/main/cpp/gpu_resident/m3b/m5_source_manifest.h').read_text(encoding='utf8')
if receipt['sourceManifestSHA'] not in header:raise SystemExit('M5_INIT_COMPILED_IDENTITY_STALE')
