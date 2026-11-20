"""Verify the exact delivered G11 and RX artifacts before handoff."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/g11'))
from build_receipt import elf

def sha(data):
    return hashlib.sha256(data).hexdigest()

def read(path):
    return json.loads(path.read_text(encoding='utf8'))

def package(project, receipt, module):
    hap = project / 'entry/build/default/outputs/default/entry-default-unsigned.hap'
    assert sha(hap.read_bytes()) == (receipt.get('hap') or receipt['unsignedHap'])['sha256']
    with zipfile.ZipFile(hap) as archive:
        binary = archive.read('libs/arm64-v8a/' + module)
        abc = archive.read('ets/modules.abc')
    assert sha(binary) == (receipt.get('packagedNative') or receipt['packagedELF'])['sha256']
    if 'abc' in receipt:
        assert sha(abc) == receipt['abc']['sha256']
    return binary

def g11(project, validator):
    directory = ROOT / 'test/reports/t639'
    receipt = read(directory / 'compile-receipt-v2.json')
    binary = package(project, receipt, 'libgrs_g11.so')
    identity = elf(binary)
    assert identity['BuildId'] == receipt['packagedNative']['BuildId']
    for key, record in receipt['compiledSources'].items():
        assert sha((project / key).read_bytes()) == record['rawCompiledSHA256'], key
    for key, digest in receipt['generatorInputs'].items():
        assert sha((project / key).read_bytes()) == digest, key
    checked = {}
    for name, digest in receipt['staticShaders'].items():
        stage = name.rsplit('.', 1)[1]
        matches = list(project.glob('entry/.cxx/**/g11_assets/' + name))
        source = matches[0] if stage == 'comp' else directory / name
        raw = source.read_bytes()
        assert sha(raw) == digest, name
        assert raw in binary, name
        argv = [str(validator), '-S', stage, str(source)]
        result = subprocess.run(argv, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        checked[name] = {'argv': argv, 'stdout': result.stdout, 'stderr': result.stderr, 'sha256': digest}
    assert len(checked) == 8
    return {'BuildId':identity['BuildId'],'compiledSources':len(receipt['compiledSources']),
            'generatedInputs':len(receipt['generatorInputs']),'shaders':checked}

def rx(project):
    directory = ROOT / 'test/reports/t674'
    identity = read(directory / 'pipeline_identity.json')
    assets = next(project.glob('entry/.cxx/**/rx_assets/pipeline_identity.json')).parent
    assert read(assets / 'pipeline_identity.json') == identity
    raw = (assets / 'pipeline.comp').read_bytes()
    literal = (assets / 'pipeline_source.h').read_bytes().split(b'R"RXGPU(', 1)[1].split(b')RXGPU"', 1)[0]
    assert len(raw) == 145458 and raw == literal and sha(raw) == identity['shaderSHA']
    assert not any(mark in raw for mark in (b'\r', b'\0', b'#line', b'\xef\xbb\xbf'))
    assert identity['scopeTransform']['transforms'] == 1 and identity['variant'] == 'PRODUCTION'
    assert identity['shaderSHA'] in (assets / 'contract_generated.h').read_text(encoding='utf8')
    reader = read(assets / 'reader_schema.json')['sourceIdentity']
    assert all(identity[key] == value for key, value in reader.items())
    assert (assets / 'reader_schema.json').read_bytes() == (directory / 'reader_schema.json').read_bytes()
    receipt = read(directory / 'compile-receipt.json')
    binary = package(project, receipt, 'libgrs_g9rx.so')
    assert raw + b'\0' in binary
    old = project / 'entry/src/main/cpp/gpu_resident/g9rx/frozen/pipeline.comp'
    assert sha(old.read_bytes()) != identity['shaderSHA'], 'OLD_SHADER_IDENTITY_ACCEPTED'
    return {'shaderSHA':sha(raw),'bytes':len(raw),'transforms':1,'oldSourceMatches':False,
            'readerIdentity':reader}

if __name__ == '__main__':
    mode, project, *extra = sys.argv[1:]
    path = Path(project).resolve()
    result = g11(path, Path(extra[0]).resolve()) if mode == 'g11' else rx(path)
    print(json.dumps({'checks':result,'device':'NOT_RUN','productPASS':False}, indent=2))
