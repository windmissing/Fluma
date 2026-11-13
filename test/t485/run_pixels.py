from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[2]
out = root / (sys.argv[3] if len(sys.argv) > 3 else '.codex/local-test/t485-pixel-matrix')
out.mkdir()
report = root / 'test/reports/t485'
report.mkdir(parents=True, exist_ok=True)
env = dict(os.environ)
env['PATH'] = r'C:\Users\admin\AppData\Local\Programs\CokePLUS;' + env['PATH']
rows = []
hashes = {}
for route, exe in [('m1', sys.argv[1]), ('m2', sys.argv[2])]:
    cache = out / ('cache-' + route)
    for mode in ['cold', 'warm', 'off']:
        pixels = out / (route + '-' + mode)
        pixels.mkdir()
        run = subprocess.run([exe, str(cache), mode, str(pixels)], env=env, capture_output=True)
        (pixels / 'run.log').write_bytes(run.stdout + run.stderr)
        assert run.returncode == 0, (route, mode, run.stderr.decode(errors='replace'))
        rows.append(json.loads(run.stdout.decode().strip().splitlines()[-1]) | {'route': route})
        for scenario in ['empty', 'mixed', 'mirror', 'viewport']:
            token = (pixels / (scenario + '.token')).read_bytes()
            if mode == 'cold':
                hashes[route + '/' + scenario + '/token'] = hashlib.sha256(token).hexdigest()
            else:
                assert token == (out / (route + '-cold') / (scenario + '.token')).read_bytes()
            for layer in range(8):
                name = scenario + '-' + str(layer) + '.rgba'
                data = (pixels / name).read_bytes()
                digest = hashlib.sha256(data).hexdigest()
                hashes[route + '/' + mode + '/' + name] = digest
                if mode != 'cold':
                    assert data == (out / (route + '-cold') / name).read_bytes()
                if route == 'm2':
                    assert data == (out / ('m1-' + mode) / name).read_bytes()
    events = []
    for receipt in (cache / 'gpu-program-cache/v1').glob('*.jsonl'):
        events.extend(json.loads(line) for line in receipt.read_text().splitlines())
        shutil.copyfile(receipt, report / (route + '-' + receipt.name))
    for role in ['SCENE_FULL', 'SCENE_PARTICLES', 'SCENE_WATER']:
        scoped = [r for r in events if r.get('contextGeneration') == '1' and r['role'] == role]
        cold = [r for r in scoped if r['packageSource'] == 'HOST_T485_cold']
        warm = [r for r in scoped if r['packageSource'] == 'HOST_T485_warm']
        assert any(r['stage'] == 'EXPORT_WRITE_SUBMITTED' and r['persistence'] == 'PERSISTED' for r in cold)
        assert any(r['stage'] == 'REFLECT_PREPARE_OK' and r['route'] == 'BINARY_LOAD' and r['compileCalls'] == 0 for r in warm)
        reflected = next(r for r in warm if r['stage'] == 'REFLECT_OK')
        assert reflected['colorLocation'] == 0 and reflected['colorType'] == 35666 and reflected['colorSize'] == 1
        if role == 'SCENE_FULL':
            assert reflected['publicationBinding'] == 0
        if role == 'SCENE_PARTICLES':
            assert reflected['aStateInactive'] or any(a['name'] == 'aState' and a['location'] == 3 and a['type'] == 5125 for a in reflected['attributes'])
        for r in scoped:
            assert sum(s['compileLog']['copied'] for s in r['shaderStages']) + r['linkLog']['copied'] <= 4096
    assert len(list((cache / 'gpu-program-cache/v1').glob('*.bin'))) == 3
# Corrupt only a persisted graphics artifact, then start a fresh process.
disk_cases = []
cache = out / 'cache-m2'
directory = cache / 'gpu-program-cache/v1'
full_key = next(json.loads(line)['key'] for p in directory.glob('*.jsonl')
                for line in p.read_text().splitlines()
                if json.loads(line)['role'] == 'SCENE_FULL')
for case in ['bad_hash', 'truncated']:
    path = directory / (full_key + '.bin')
    data = bytearray(path.read_bytes())
    if case == 'bad_hash':
        data[144] ^= 1
    else:
        data = data[:50]
    path.write_bytes(data)
    pixels = out / case
    pixels.mkdir()
    run = subprocess.run([sys.argv[2], str(cache), case, str(pixels)], env=env, capture_output=True)
    (pixels / 'run.log').write_bytes(run.stdout + run.stderr)
    assert run.returncode == 0, run.stderr.decode(errors='replace')
    events = [json.loads(line) for line in (directory / (full_key + '.jsonl')).read_text().splitlines()]
    actual = [r for r in events if r['packageSource'] == 'HOST_T485_' + case and r['contextGeneration'] == '1']
    assert any(r['compileCalls'] == 2 and r['route'] == 'REJECT_THEN_SOURCE_ONCE' and r['stage'] == 'REFLECT_OK' for r in actual)
    assert any(r['cacheReason'].startswith('DISK_') for r in actual)
    for raw in pixels.glob('*.rgba'):
        assert raw.read_bytes() == (out / 'm2-cold' / raw.name).read_bytes()
    disk_cases.append({'case': case, 'sourceCompileCalls': 2, 'fallbacks': 1, 'pixelsUnchanged': True})
shutil.copyfile(directory / (full_key + '.jsonl'), report / ('m2-' + full_key + '.jsonl'))
# Same stage bytes and ABI permit M1/M2 to share three binaries.
keys = lambda route: {p.stem for p in (out / ('cache-' + route) / 'gpu-program-cache/v1').glob('*.bin')}
assert keys('m1') == keys('m2')
with zipfile.ZipFile(report / 'pixel-originals.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
    for folder in out.iterdir():
        if folder.is_dir() and not folder.name.startswith('cache-'):
            for path in sorted(folder.iterdir()):
                archive.write(path, folder.name + '/' + path.name)
summary = {'schema': 'T485_SCENE_PIXEL_COMPARISON_V1', 'device': 'NOT_RUN',
           'runs': rows, 'rawHashes': hashes, 'sharedBinaryKeys': sorted(keys('m1')),
           'byteExactPerLayerAndFinal': True, 'sameBusinessTokenPerRoute': True,
           'publicationScope': 'FROZEN_HOST_FIXTURE_REAL_LEASE_AND_GL_BUFFER',
           'execution': 'ACTUAL_GENERATED_SCENE_ANGLE_GLES',
           'archive': 'test/reports/t485/pixel-originals.zip', 'diskFallbackCases': disk_cases}
(report / 'pixels.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps({'status': 'PASS', 'routes': 2, 'modes': 3, 'scenarios': 4, 'layersIncludingFinal': 8, 'sharedBinaryKeys': 3}))
