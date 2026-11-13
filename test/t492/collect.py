from pathlib import Path
import hashlib
import json
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '.codex/local-test'
REPORT = ROOT / 'test/reports/t492'
REPORT.mkdir(parents=True, exist_ok=True)


def sha(data):
    return hashlib.sha256(data).hexdigest()


summary = {'revision': 'T492_DEVELOPER_DELIVERY_V1', 'device': 'NOT_RUN',
           'snapshot': {'payloadBytes': 753664, 'ttlNs': '60000000000', 'copies': 1,
                        'pageRows': 4, 'pageBytes': 16384, 'traceAndDiagnosticBytes': 4194304},
           'rootDeadlineNs': '250000000', 'gapNs': '33333333',
           'routes': {}, 'overlayFiles': {}}
for path in sorted((ROOT / 'test/t492').glob('*')):
    if path.is_file():
        summary['overlayFiles'][path.relative_to(ROOT).as_posix()] = sha(path.read_bytes())

for route in ['m1', 'm2']:
    project = LOCAL / ('t492-' + route + '-delivery')
    base = project / 'entry/src/main/cpp/gpu_resident'
    manifest = json.loads((project / 't492-manifest.json').read_text())
    for name, digest in manifest['files'].items():
        assert sha((base / name).read_bytes()) == digest, name
    for name, digest in manifest['integrationFiles'].items():
        assert sha((project / 'entry/src/main/ets' / name).read_bytes()) == digest, name
    old = LOCAL / ('t485-' + route + '-final/entry/src/main/cpp/gpu_resident')
    physics = 'm1' if route == 'm1' else 'm2route'
    unchanged = {}
    for path in (old / physics).glob('*'):
        if path.is_file() and path.name not in ['scene.cpp', 'scene.h']:
            assert path.read_bytes() == (base / physics / path.name).read_bytes(), path
            unchanged[(Path(physics) / path.name).as_posix()] = sha(path.read_bytes())
    for name in ['program_cache/program_factory.h', 'program_cache/artifact_store.h']:
        assert (old / name).read_bytes() == (base / name).read_bytes(), name
    header = 'entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/m1_scene_sources.h'
    assert (project / header).read_text() == (LOCAL / ('t485-' + route + '-final') / header).read_text()
    assert 'w[1]=' + ('2;' if route == 'm1' else '3;') in (project / 'entry/src/main/ets/physics/GpuM1DemoController.ets').read_text()
    control_dir = LOCAL / ('t492-delivery-controls-' + route)
    native_log = LOCAL / ('t492-delivery-controls-' + route + '.txt')
    native_rows = [json.loads(line) for line in native_log.read_text(encoding='utf-8-sig').splitlines() if line.startswith('{')]
    assert native_rows[-2]['passed'] == 23 and native_rows[-1]['passed'] == 20
    assert all(row['failed'] == 0 for row in native_rows)
    page = json.loads((REPORT / ('page-' + route + '.json')).read_text())
    assert page['passed'] == 20 and page['failed'] == 0
    app = LOCAL / ('t492-delivery-app-' + route)
    app_log = LOCAL / (app.name + '.txt')
    app_text = app_log.read_text(encoding='utf-8-sig')
    assert app_text.count('first real host pulse baseline; zero cold quota PASS') == 2
    preparations = {}
    for phase in ['cold', 'warm']:
        d = json.loads((app / ('diagnostic-' + phase + '.json')).read_text())
        stages = {s['first']['stage']: int(s['count']) for s in d['stages']}
        assert stages['SCENE_BUILD_ENTER'] == stages['SCENE_BUILD_EXIT'] == 3
        assert stages['SCENE_VAO_ENTER'] == stages['SCENE_VAO_EXIT'] == 1
        preparations[phase] = stages
        shutil.copyfile(app / ('diagnostic-' + phase + '.json'), REPORT / ('prepare-' + route + '-' + phase + '.json'))
    assert preparations['cold']['GRAPHICS_COMPILE_ENTER'] == preparations['cold']['GRAPHICS_COMPILE_EXIT'] == 6
    assert preparations['warm']['GRAPHICS_LOAD_ENTER'] == preparations['warm']['GRAPHICS_LOAD_EXIT'] == 3
    cache_rows = []
    cache_evidence = REPORT / ('cache-' + route)
    cache_evidence.mkdir(exist_ok=True)
    for receipt_path in (app / 'cache/gpu-program-cache/v1').glob('*.jsonl'):
        rows = [json.loads(line) for line in receipt_path.read_text().splitlines()]
        cache_rows.extend(rows)
        shutil.copyfile(receipt_path, cache_evidence / receipt_path.name)
    for role in ['SCENE_FULL','SCENE_PARTICLES','SCENE_WATER']:
        assert any(r.get('role') == role and r.get('route') == 'BINARY_LOAD' and r.get('stage') == 'REFLECT_PREPARE_OK' and r.get('compileCalls') == 0 for r in cache_rows)
    source = LOCAL / ('t492-delivery-source-' + route)
    source_log = LOCAL / (source.name + '.txt')
    source_result = json.loads(source_log.read_text(encoding='utf-8-sig').splitlines()[-1])
    assert source_result['failed'] == 0 and source_result['admittedJQ'] == source_result['settledAfterUpQ'] > 0
    d = json.loads((source / 'execution.json').read_text())
    stages = {s['first']['stage']: int(s['count']) for s in d['stages']}
    assert stages['SOURCE_ACCEPT'] >= 1 and stages['EGL_SWAP_RETURN'] >= 1 and stages['SCENE_LAYER_EXIT'] >= 8
    assert stages['ROOT_SUBMIT'] > 0 and stages['CONSUME'] > 0
    assert d['unknownStageCount'] == d['scopeDropped'] == 0
    assert any(p['reply'] == 'ENQUEUED_NOT_ACK' and len(p['commandSHA256']) == 64 for p in d['packets'])
    assert any(g['firstQuantum']['nonce'] != '0' for g in d['groups'])
    flushes = [p for g in d['groups'] for p in g['events'] + g['firstQuantum']['events'] + g['tail'] if p['stage'] in ['BATCH_FLUSH_ENTER','BATCH_FLUSH_EXIT']]
    assert flushes and all(int(p['b']) >= int(p['a']) and int(p['callStart']) > 0 for p in flushes)
    build_log = LOCAL / ('t492-' + route + '-delivery-build.txt')
    assert 'BUILD SUCCESSFUL' in build_log.read_text(encoding='utf-8-sig')
    receipt = json.loads((ROOT / ('test/reports/t257/T492-' + route + '-delivery.json')).read_text())
    assert receipt['verifyApp'] == 'PASS'
    signed = Path(receipt['signed']['path'])
    assert sha(signed.read_bytes()) == receipt['signed']['sha256']
    with zipfile.ZipFile(signed) as package:
        elf = package.read('libs/arm64-v8a/libgrsm1.so')
    assert elf == (project / 'entry/build/default/intermediates/stripped_native_libs/default/arm64-v8a/libgrsm1.so').read_bytes()
    for marker in [b'T492_IMMUTABLE_TIMELINE_V1', b'T492_EXECUTION_DIAGNOSTICS_V1', b'timelineSnapshotBegin', b'newRunAction', b'T480_GRAPHICS_KEY_V1']:
        assert marker in elf, marker
    archive_path = REPORT / ('native-reader-' + route + '.zip')
    with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as archive:
        for p in sorted(control_dir.rglob('*')):
            if p.is_file() and (p.suffix in ['.json','.bin','.ndjson']):
                archive.write(p, p.relative_to(control_dir).as_posix())
    summary['routes'][route] = {'manifest': manifest, 'nativeControls': native_rows,
        'pageControls': page['passed'], 'preparations': preparations, 'sourceGL': source_result,
        'unchangedPhysics': unchanged, 'signed': receipt['signed'], 'elfSHA256': sha(elf),
        'nativeReaderArchive': {'file': archive_path.relative_to(ROOT).as_posix(), 'sha256': sha(archive_path.read_bytes())}}
    for origin, name in [(native_log,'controls'),(app_log,'app'),(source_log,'source'),(build_log,'build')]:
        shutil.copyfile(origin, REPORT / (name + '-' + route + '.txt'))
    shutil.copyfile(source / 'execution.json', REPORT / ('source-' + route + '-execution.json'))
    shutil.copyfile(project / 't492-manifest.json', REPORT / ('generation-' + route + '.json'))

legacy = json.loads((LOCAL / 't492-legacy-units.txt').read_text().splitlines()[-1])
assert legacy['passed'] == 9 and legacy['failed'] == 0
summary['legacyTimeline'] = legacy
shutil.copyfile(LOCAL / 't492-legacy-units.txt', REPORT / 'legacy-timeline.txt')
summary['readerRegressions'] = {}
for group in ['t306','t404']:
    source = LOCAL / ('t492-regression-' + group + '.json')
    result = json.loads(source.read_text())
    assert result['failed'] == 0
    summary['readerRegressions'][group] = result['passed']
    shutil.copyfile(source, REPORT / ('regression-' + group + '.json'))
for route in ['m1','m2']:
    source = LOCAL / ('t492-recovery-page-' + route + '.txt')
    result = json.loads(source.read_text().splitlines()[-1])
    assert result['passed'] == 12 and result['failed'] == 0
    shutil.copyfile(source, REPORT / ('recovery-page-' + route + '.txt'))
summary['status'] = 'DEVELOPER_COMPLETE'
(REPORT / 'delivery.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps({'status': 'PASS', 'routes': 2, 'native': '23+20 each', 'page': '20 each',
                  'coldWarmPrepare': True, 'sourceGLDrawSwap': True, 'buildSignVerify': True}))
