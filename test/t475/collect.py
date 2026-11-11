"""Collect T475 offline evidence. All GL receipts come from the actual factory."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '.codex/local-test'
REPORT = ROOT / 'test/reports/t475'
REPORT.mkdir(parents=True, exist_ok=True)
sha = lambda data: hashlib.sha256(data).hexdigest()
roles = {'SOURCE', 'INTERMEDIATE_QUANTUM', 'FINAL_QUANTUM'}
summary = {'schema': 'T475_DEVELOPER_OFFLINE_V1', 'device': 'NOT_RUN', 'routes': {}}
for route, directory in [('M2', 't475-app-final'), ('M1', 't475-m1-check')]:
    target = REPORT / route.lower()
    target.mkdir(exist_ok=True)
    cache = LOCAL / directory / 'cache/gpu-program-cache/v1'
    programs = []
    for receipt in cache.glob('*.jsonl'):
        events = [json.loads(line) for line in receipt.read_text(encoding='utf-8').splitlines()]
        role = events[-1]['role']
        assert role in roles
        cold = [e for e in events if e['packageSource'] == 'HOST_APP_cold']
        warm = [e for e in events if e['packageSource'] == 'HOST_APP_warm']
        assert any(e['stage'] == 'COMPILE_ENTER' for e in cold)
        assert any(e['stage'] == 'EXPORT_WRITE_SUBMITTED' and e['persistence'] == 'PERSISTED' for e in cold)
        assert any(e['route'] == 'BINARY_LOAD' and e['stage'] == 'REFLECT_PREPARE_OK' for e in warm)
        assert not any(e['stage'] == 'COMPILE_ENTER' for e in warm)
        assert all(e['compileBeginNs'] == 0 for e in warm)
        wire = receipt.with_suffix('.bin').read_bytes()
        format_id, length = struct.unpack('<II', wire[8:16])
        assert wire[:8] == b'T475B001' and len(wire) == length + 144
        assert wire[16:80].decode() == receipt.stem
        digest = sha(wire[144:])
        assert digest == wire[80:144].decode()
        final = warm[-1]
        assert final['binarySHA'] == digest and final['binaryBytes'] == length
        assert final['reflectEndNs'] >= final['reflectBeginNs'] > 0
        if role != 'INTERMEDIATE_QUANTUM':
            assert final['prepareEndNs'] >= final['prepareBeginNs'] > 0
        if role == 'FINAL_QUANTUM':
            assert any(e['stage'] == 'WARMUP_OK' for e in warm)
        shutil.copyfile(receipt, target / receipt.name)
        programs.append({'role': role, 'key': receipt.stem, 'binarySHA256': digest,
                         'binaryBytes': length, 'format': format_id,
                         'shaderSHA256': final['shaderSHA'], 'layoutSHA256': final['layoutSHA'],
                         'environmentSHA256': final['environmentSHA'],
                         'cold': 'SOURCE_COMPILE/PERSISTED', 'warm': 'BINARY_LOAD/REFLECT_PREPARE_OK',
                         'warmSourceCompileCalls': 0, 'receipt': str((target / receipt.name).relative_to(ROOT))})
    assert {p['role'] for p in programs} == roles and len(programs) == 3
    for phase in ['cold', 'warm']:
        native = LOCAL / directory / ('native-' + phase + '.json')
        value = json.loads(native.read_text())
        assert value['code'] == 0 and value['sealed']
        assert all(p['sourceBytes'] > 0 and len(p['sha256']) == 64 for p in value['programs'])
        shutil.copyfile(native, target / native.name)
    summary['routes'][route] = {'passed': True, 'programs': programs,
                               'fixture': 'generated M2 App' if route == 'M2' else 'same App fixture adapted to M1 namespace; actual generated M1 transport sources'}

logs = {
    'store.log': 't475-store-faults.log',
    'gl.log': 't475-gl-complete.log',
    'app-m2.log': 't475-app-final.log',
    'app-m1.log': 't475-m1-check.log',
    'build.log': 't475-build-handoff.log',
    'generate.log': 't475-generate-handoff.log',
}
for name, source in logs.items():
    data = (LOCAL / source).read_bytes()
    assert b'error C' not in data and b'Assertion failed' not in data
    if name == 'build.log':
        assert b'BUILD SUCCESSFUL' in data and b'BUILD FAILED' not in data
    # Keep the complete output while normalizing trailing spaces for review.
    text = data.decode('utf-8', errors='replace')
    (REPORT / name).write_text('\n'.join(line.rstrip() for line in text.splitlines()) + '\n', encoding='utf-8')
project = LOCAL / 't475-handoff-final'
manifest = json.loads((project / 't475-manifest.json').read_text())
for entry in manifest['cacheFiles']:
    source = ROOT / 'entry/src/main/cpp/gpu_resident/program_cache' / entry['name']
    generated = project / 'entry/src/main/cpp/gpu_resident/program_cache' / entry['name']
    assert sha(source.read_bytes()) == sha(generated.read_bytes()) == entry['sha256']
shutil.copyfile(project / 't475-manifest.json', REPORT / 'generation.json')
signing = json.loads((ROOT / 'test/reports/t257/T475-handoff.json').read_text())
assert signing['verifyApp'] == 'PASS'
signed = Path(signing['signed']['path'])
assert sha(signed.read_bytes()) == signing['signed']['sha256']
with zipfile.ZipFile(signed) as package:
    elf = next(name for name in package.namelist() if name.endswith('/arm64-v8a/libgrsm1.so'))
    summary['grsm1ElfSHA256'] = sha(package.read(elf))
summary['signedHap'] = signing['signed']
summary['verifyApp'] = 'PASS'
summary['injectedFaults'] = ['no binary formats', 'empty/oversized export length', 'ENOSPC', 'partial temporary write', 'sync failure', 'replacement failure']
summary['realNegativeCases'] = ['bad disk hash source fallback', 'truncated disk file source fallback', 'unknown binary format', 'driver rejects corrupt executable', 'invalid GLSL', 'preexisting product GL error', 'different live EGL context']
summary['excluded'] = ['scene vertex/fragment program cache', 'device run and device startup benefit']
shutil.copyfile(ROOT / 'test/reports/t257/T475-handoff.json', REPORT / 'signing.json')
(REPORT / 'handoff.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps({'offline': 'PASS', 'programs': 6, 'warmCompileCalls': 0,
                  'signedHapSHA256': summary['signedHap']['sha256'], 'elfSHA256': summary['grsm1ElfSHA256']}, ensure_ascii=False))
