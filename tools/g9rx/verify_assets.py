"""Fatal read-only build guard; no source repair, GL calls or product execution."""
from pathlib import Path
import hashlib
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT/'entry/src/main/cpp/gpu_resident/g9rx'
def sha(b): return hashlib.sha256(b).hexdigest()
def verify(out):
    identity = json.loads((out/'pipeline_identity.json').read_text(encoding='utf8'))
    dependencies = json.loads((out/'inputs.json').read_text(encoding='utf8'))
    for name, digest in dependencies.items():
        path = ROOT/name if name.startswith(('entry/', 'docs/', 'tools/')) else MOD/name
        if sha(path.read_bytes()) != digest:
            raise ValueError('RX_STALE_INPUT '+name)
    for name, digest in identity['generatedFiles'].items():
        if sha((out/name).read_bytes()) != digest:
            raise ValueError('RX_STALE_GENERATED '+name)
    raw = (out/'pipeline.comp').read_bytes()
    if b'\r' in raw or b'\0' in raw or raw.startswith(b'\xef\xbb\xbf') or b'#line' in raw:
        raise ValueError('RX_CANONICAL_SOURCE_ENCODING')
    literal = (out/'pipeline_source.h').read_bytes().split(b'R"RXGPU(', 1)[1].split(b')RXGPU"', 1)[0]
    if literal != raw or sha(raw) != identity['shaderSHA'] or len(raw) != identity['canonicalBytes']:
        raise ValueError('RX_SOURCE_IDENTITY_MISMATCH')
    header = (out/'contract_generated.h').read_text(encoding='utf8')
    if re.search(r'kPipelineSHA="([a-f0-9]+)"', header).group(1) != identity['shaderSHA']:
        raise ValueError('RX_PROGRAM_CONTRACT_MISMATCH')
    bundle = dict(identity); expected = bundle.pop('bundleSHA')
    if sha(json.dumps(bundle, sort_keys=True, separators=(',', ':')).encode()) != expected:
        raise ValueError('RX_BUNDLE_IDENTITY_MISMATCH')
    return identity
if __name__ == '__main__':
    value = verify(Path(sys.argv[1]).resolve())
    print('RX_CANONICAL_SOURCE_VERIFIED', value['shaderSHA'], value['canonicalBytes'])
