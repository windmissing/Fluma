import hashlib
import json
import re
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '.codex/local-test'
REPORT = ROOT / 'test/reports/t482'
PROJECT = LOCAL / 't482-final'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def packaged_elf(receipt):
    value = json.loads(receipt.read_text())
    signed = Path(value['signed']['path'])
    assert sha(signed.read_bytes()) == value['signed']['sha256']
    assert value['verifyApp'] == 'PASS'
    with zipfile.ZipFile(signed) as hap:
        elf = hap.read('libs/arm64-v8a/libgrsm1.so')
    return value, elf


REPORT.mkdir(parents=True, exist_ok=True)
boundary = json.loads(subprocess.check_output([
    'python', '-X', 'utf8', str(ROOT / 'test/t482/check_source.py'),
    str(LOCAL / 't475-handoff-final'), str(PROJECT)
], text=True))
old_receipt, old_elf = packaged_elf(ROOT / 'test/reports/t257/T475-handoff.json')
new_receipt, new_elf = packaged_elf(ROOT / 'test/reports/t257/T482-final.json')
old_source = (LOCAL / 't475-handoff-final/source-v3.glsl').read_bytes()
source = (PROJECT / 'source-v3.glsl').read_bytes()
assert old_source + b'\0' in old_elf
assert source + b'\0' in new_elf
assert old_source + b'\0' not in new_elf
assert (PROJECT / 'entry/build/default/intermediates/stripped_native_libs/default/arm64-v8a/libgrsm1.so').read_bytes() == new_elf

programs = []
for receipt in sorted((LOCAL / 't482-final-gl/cache').rglob('*.jsonl')):
    rows = [json.loads(line) for line in receipt.read_text().splitlines()]
    assert any(row['stage'] == 'EXPORT_WRITE_SUBMITTED' and row['persistence'] == 'PERSISTED' for row in rows)
    warm = [row for row in rows if row['packageSource'] == 'HOST_APP_warm']
    assert warm and any(row['route'] == 'BINARY_LOAD' for row in warm)
    assert not any(row['stage'] == 'COMPILE_ENTER' for row in warm)
    assert any(row['stage'] == 'REFLECT_PREPARE_OK' for row in warm)
    last = warm[-1]
    programs.append({'role': last['role'], 'key': last['key'],
                     'shaderSHA': last['shaderSHA'], 'cold': 'PERSISTED',
                     'warm': 'BINARY_LOAD', 'warmCompileCalls': 0})
    shutil.copyfile(receipt, REPORT / receipt.name)
assert {p['role'] for p in programs} == {'SOURCE', 'INTERMEDIATE_QUANTUM', 'FINAL_QUANTUM'}
layout_source = (PROJECT / 'entry/src/main/cpp/gpu_resident/m2route/source_transport.cpp').read_text()
layout = re.search(r'programcache::Build\(0,shader,kSourceActualSha,"([a-f0-9]{64})"', layout_source)[1]
source_row = next(p for p in programs if p['role'] == 'SOURCE')
rows = [json.loads(line) for line in next(
    p for p in (LOCAL / 't482-final-gl/cache').rglob('*.jsonl')
    if p.stem == source_row['key']).read_text().splitlines()]
env = rows[-1]['environmentSHA']
link = 'COMPUTE|SEPARABLE=0|EXPLICIT_GLSL_BINDINGS|NO_PRELINK_ATTRIBUTE_BINDINGS'


def key(shader):
    data = b'T475_PROGRAM_KEY_V1'
    for value in [shader, layout.encode(), env.encode(), link.encode()]:
        data += len(value).to_bytes(8, 'little') + value
    return sha(data)


assert key(source) == source_row['key']
assert key(old_source) != key(source)
for phase in ['cold', 'warm']:
    path = LOCAL / f't482-final-gl/native-{phase}.json'
    data = json.loads(path.read_text())
    assert data['code'] == 0 and data['sealed']
    assert len(data['programs']) == 3
    assert data['programs'][0]['sha256'] == sha(source)
    shutil.copyfile(path, REPORT / path.name)
roots = (LOCAL / 't482-roots.txt').read_text(encoding='utf-8-sig')
profiles = ['192/64', '192/128', '512/64', '512/128']
for profile in profiles:
    assert f'T452 SOURCE {profile} twenty GPU roots verified' in roots
build = (LOCAL / 't482-final-build.txt').read_text(encoding='utf-8-sig')
assert 'BUILD SUCCESSFUL' in build and 'BUILD FAILED' not in build
for name in ['t482-roots.txt', 't482-final-build.txt', 't482-final-gl.txt']:
    shutil.copyfile(LOCAL / name, REPORT / name)
summary = {'schema': 'T482_DEVELOPER_DELIVERY_V1', 'status': 'DEVELOPER_COMPLETE',
           'device': 'NOT_RUN', 'sourceBoundary': boundary, 'programs': programs,
           'gpuRoots': {'profiles': profiles,
                        'scope': 'T452 SOURCE/replay/QUANTUM and rejection assertions'},
           'cacheKey': {'old': key(old_source), 'new': key(source),
                        'sameEnvironmentAndLayout': True},
           'build': 'PASS', 'signVerify': 'PASS', 'signed': new_receipt['signed'],
           'elfSHA': sha(new_elf), 'oldElfSHA': sha(old_elf),
           'actualSourceBytesMatchedSignedElf': True,
           'deviceFollowup': ['T477 cold/warm main chain', 'T461 M2 whole root'],
           'maleoonCompileResult': 'NOT_RUN'}
(REPORT / 'delivery.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps({'status': 'PASS', 'programs': 3, 'boundaryChecks': 8,
                  'signedSHA': new_receipt['signed']['sha256']}))
