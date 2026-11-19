"""T681 static compile/artifact receipt. Does not execute report(), tests or product."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
def sha(data):
    return hashlib.sha256(data).hexdigest()

def build_id(data):
    h = struct.unpack_from('<16sHHIQQQIHHHHHH', data)
    assert h[0][:6] == b'\x7fELF\x02\x01' and h[2] == 183
    for i in range(h[10]):
        p = struct.unpack_from('<IIQQQQQQ', data, h[5]+i*h[9])
        if p[0] != 4:
            continue
        at, end = p[2], p[2]+p[5]
        while at < end:
            n, size, kind = struct.unpack_from('<III', data, at)
            at += 12
            name = data[at:at+n]
            at += (n+3)&~3
            note = data[at:at+size]
            at += (size+3)&~3
            if name == b'GNU\0' and kind == 3:
                return note.hex()
    return None

def main():
    source, log, output = [Path(arg).resolve() for arg in sys.argv[1:4]]
    product = source/'.codex/local-test/product'
    manifest_path = source/'t659-generated-sources.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf8'))
    assert manifest['schema'] == 'T681_PRODUCT_SOURCE_V1'
    for relative, digest in manifest['overlays'].items():
        assert sha((ROOT/relative).read_bytes()) == digest, relative
        assert (source/relative).read_bytes() == (ROOT/relative).read_bytes(), relative
    for relative, digest in manifest['generated'].items():
        assert sha((product/relative).read_bytes()) == digest, relative
    consumers = ['render/PrepareReadinessV4.ets', 'render/PreparePairingEvidence.ets',
                 'pages/P051DerivedV2Demo.ets', 'render/PrepareReadiness.ets',
                 'render/P013SingleSandController.ets', 'render/RasterGeometry.ets']
    for name in ['PrepareReadinessV4.ets', 'PreparePairingEvidence.ets']:
        assert (product/'entry/src/main/ets/render'/name).read_bytes() == (ROOT/'test/t426'/name).read_bytes()
    output.mkdir(parents=True, exist_ok=True)
    log_bytes = log.read_bytes()
    assert b'BUILD SUCCESSFUL' in log_bytes
    hap = product/'entry/build/default/outputs/default/entry-default-unsigned.hap'
    binaries = {}
    with zipfile.ZipFile(hap) as package:
        for name in package.namelist():
            if name.endswith('.so'):
                raw = package.read(name)
                binaries[name] = {'bytes': len(raw), 'sha256': sha(raw), 'BuildId': build_id(raw)}
        abc = package.read('ets/modules.abc')
        for token in [b'T680_PREPARE_PAIRING_EVIDENCE_V1', b'ARKTS_PACKET', b'NATIVE_GET_STATE', b'T659_PREPARE_OWNER_VALUE_V1']:
            assert token in abc, token
    record = {'schema': 'T681_COMPILE_DELIVERY_V1', 'status': 'DEVELOPMENT_COMPILE_COMPLETE',
              'deviceUsed': False, 'testsRun': False, 'productExecuted': False,
              'shaderExecuted': False, 'signed': False, 'deployed': False,
              'productPASS': False, 'baseline': manifest['baseline'], 'baselineReason': manifest['reason'],
              'canonicalOverlays': manifest['overlays'], 'sourceManifestSHA256': sha(manifest_path.read_bytes()),
              'verifiedGeneratedFiles': len(manifest['generated']), 'buildDirectory': str(product),
              'build': {'command': 'hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon',
                        'result': 'BUILD SUCCESSFUL', 'elapsed': re.sub(r'\x1b\[[0-9;]*m','',re.search(rb'BUILD SUCCESSFUL in ([^\r\n]+)', log_bytes).group(1).decode()).strip(),
                        'errors': 0, 'logSHA256': sha(log_bytes)},
              'unsignedHap': {'path': str(hap), 'bytes': hap.stat().st_size, 'sha256': sha(hap.read_bytes())},
              'abc': {'bytes': len(abc), 'sha256': sha(abc)}, 'packagedNative': binaries,
              'actualConsumers': {}, 'formalInputs': {}}
    for relative in consumers:
        path = product/'entry/src/main/ets'/relative
        target = output/'actual-consumers'/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)
        record['actualConsumers'][relative] = sha(path.read_bytes())
    for relative in ['docs/architecture-T680-v1.md', 'docs/architecture-T680-contract-v1.json',
                     'tools/singlesand/prepare_owner_build.py', 'tools/singlesand/build_owner_receipt.py']:
        record['formalInputs'][relative] = sha((ROOT/relative).read_bytes())
    shutil.copyfile(manifest_path, output/'generated-sources.json')
    shutil.copyfile(source/'generation.log', output/'generation.log')
    shutil.copyfile(log, output/'release-build.log')
    (output/'compile-receipt.json').write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf8', newline='\n')
    print(json.dumps({k: record[k] for k in ['status', 'verifiedGeneratedFiles', 'build', 'unsignedHap', 'abc', 'packagedNative']}, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
