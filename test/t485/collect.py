from pathlib import Path
import hashlib
import json
import re
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '.codex/local-test'
REPORT = ROOT / 'test/reports/t485'
REPORT.mkdir(parents=True, exist_ok=True)


def sha(data):
    return hashlib.sha256(data).hexdigest()


pixels = json.loads((REPORT / 'pixels.json').read_text())
assert pixels['byteExactPerLayerAndFinal'] and pixels['sameBusinessTokenPerRoute']
assert len(pixels['runs']) == 6 and len(pixels['sharedBinaryKeys']) == 3
assert len(pixels['diskFallbackCases']) == 2
faults = json.loads((LOCAL / 't485-factory-all.txt').read_text(encoding='utf-8-sig').splitlines()[-1])
assert faults['passed'] == 13 and faults['failed'] == 0
legacy = (LOCAL / 't485-legacy/run.txt').read_text()
assert 'route=BINARY_LOAD key=7234cbb48b952df7966f31a83f07bde24db8f7d210cc2ecbb64f4e0b4c86d153' in legacy
summary = {'revision': 'T485_SCENE_CACHE_DEVELOPER_V1', 'status': 'DEVELOPER_COMPLETE',
           'device': 'NOT_RUN', 'graphicsFactoryFaults': faults,
           'pixelEvidence': 'test/reports/t485/pixels.json',
           'rawPixelArchive': {'path': 'test/reports/t485/pixel-originals.zip',
                               'sha256': sha((REPORT / 'pixel-originals.zip').read_bytes())},
           'routes': {}, 'computeCompatibility': {'oldKeyUnchanged': True,
               'actualLegacyBinaryLoaded': True,
               'key': '7234cbb48b952df7966f31a83f07bde24db8f7d210cc2ecbb64f4e0b4c86d153'}}
roles = ['SCENE_FULL', 'SCENE_PARTICLES', 'SCENE_WATER']
for route, app_name in [('m1', 't485-app-m1-exact'), ('m2', 't485-app-m2-exact')]:
    project = LOCAL / ('t485-' + route + '-final')
    manifest = json.loads((project / 't485-manifest.json').read_text())
    base = project / 'entry/src/main/cpp/gpu_resident'
    for file, digest in manifest['files'].items():
        assert sha((base / file).read_bytes()) == digest, file
    assert (base / 'program_cache/program_factory.h').read_bytes() == (ROOT / 'entry/src/main/cpp/gpu_resident/program_cache/program_factory.h').read_bytes()
    assert (base / 'program_cache/artifact_store.h').read_bytes() == (ROOT / 'entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h').read_bytes()
    frozen = LOCAL / 't475-handoff-final/entry/src/main/cpp/gpu_resident/program_cache/program_factory.h'
    assert frozen.read_text(encoding='utf-8') == (base / 'program_cache/program_factory.h').read_text(encoding='utf-8')
    cmake_header = project / 'entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/m1_scene_sources.h'
    host_header = LOCAL / ('t485-pixel-host-' + route) / 'm1_scene_sources.h'
    # C++ translation phase 1 normalizes Windows CRLF before parsing raw strings.
    assert cmake_header.read_text() == host_header.read_text()
    sources = re.findall(r'const char\* (kM1\w+)=R"M1SC\((.*?)\)M1SC";', cmake_header.read_text(), re.S)
    assert len(sources) == 6
    inventory = {name: {'sha256': sha(source.encode()), 'bytes': len(source.encode())} for name, source in sources}
    app_log = (LOCAL / (app_name + '.txt')).read_text(encoding='utf-8-sig')
    assert app_log.count('T485 scene prepared before READY; first real host pulse baseline; zero cold quota PASS') == 2
    assert 'Assertion failed' not in app_log
    app_events = []
    for receipt in (LOCAL / app_name / 'cache/gpu-program-cache/v1').glob('*.jsonl'):
        app_events.extend(json.loads(line) for line in receipt.read_text().splitlines())
        shutil.copyfile(receipt, REPORT / ('app-' + route + '-' + receipt.name))
    graphics = [r for r in app_events if r.get('programKind') == 'GRAPHICS']
    inventory_roles = []
    for role in roles:
        cold = [r for r in graphics if r['role'] == role and r['packageSource'] == 'HOST_APP_cold']
        warm = [r for r in graphics if r['role'] == role and r['packageSource'] == 'HOST_APP_warm']
        assert any(r['stage'] == 'EXPORT_WRITE_SUBMITTED' and r['persistence'] == 'PERSISTED' for r in cold)
        assert any(r['stage'] == 'REFLECT_PREPARE_OK' and r['route'] == 'BINARY_LOAD' and r['compileCalls'] == 0 for r in warm)
        record = next(r for r in warm if r['stage'] == 'REFLECT_OK')
        inventory_roles.append({k: record[k] for k in ['role','key','bindingAbiSHA','linkConfigSHA','environmentSHA','shaderStages','attributes','uniforms','publicationBinding','aStateInactive','colorLocation','colorType']})
    signing_path = ROOT / ('test/reports/t257/T485-' + route + '-handoff.json')
    signing = json.loads(signing_path.read_text())
    assert signing['verifyApp'] == 'PASS'
    signed = Path(signing['signed']['path'])
    assert sha(signed.read_bytes()) == signing['signed']['sha256']
    with zipfile.ZipFile(signed) as package:
        elf = package.read('libs/arm64-v8a/libgrsm1.so')
    stripped = project / 'entry/build/default/intermediates/stripped_native_libs/default/arm64-v8a/libgrsm1.so'
    assert stripped.read_bytes() == elf
    for _, source in sources:
        assert source.encode() + b'\0' in elf
    assert b'T480_GRAPHICS_KEY_V1' in elf and b'T480_SCENE_PROGRAM_ARTIFACT_V1' in elf
    build_log = LOCAL / ('t485-' + route + '-handoff-build.txt')
    build = build_log.read_text(encoding='utf-8-sig')
    assert 'BUILD SUCCESSFUL' in build and 'BUILD FAILED' not in build
    row = {'sourceManifest': manifest, 'shaderInventory': inventory, 'graphicsRoles': inventory_roles,
           'coldPrepareBeforeReady': True, 'firstPulseColdQuota': 0,
           'applicationMode': 'M1 compute binaries seeded; scene cold then warm' if route == 'm1' else 'compute and scene cold then warm',
           'signed': signing['signed'], 'elfSHA256': sha(elf),
           'signingReceipt': signing_path.relative_to(ROOT).as_posix(), 'build': 'PASS'}
    summary['routes'][route] = row
    shutil.copyfile(build_log, REPORT / ('build-' + route + '.txt'))
    shutil.copyfile(LOCAL / (app_name + '.txt'), REPORT / ('app-' + route + '.txt'))
    for phase in ['cold', 'warm']:
        shutil.copyfile(LOCAL / app_name / ('native-' + phase + '.json'), REPORT / ('native-' + route + '-' + phase + '.json'))
    (REPORT / ('generation-' + route + '.json')).write_text(json.dumps(manifest, indent=2) + '\n')
assert summary['routes']['m1']['shaderInventory'] == summary['routes']['m2']['shaderInventory']
shutil.copyfile(LOCAL / 't485-factory-all.txt', REPORT / 'factory-faults.txt')
shutil.copyfile(LOCAL / 't485-legacy/run.txt', REPORT / 'legacy-compute.txt')
summary['limits'] = {'binaryFormat': 'T475B001', 'headerBytes': 144,
                     'entryBytes': 33554432, 'payloadBytes': 134217728, 'entries': 32,
                     'rawLogsPerProgramBytes': 4096, 'productFallback': 'GPU_GLSL_ONLY',
                     'pixelScope': 'FROZEN_PUBLICATION_REAL_SCENE_GLES; compute acceptance not re-estimated'}
(REPORT / 'delivery.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps({'status': 'PASS', 'graphicsRoutes': 6, 'faultCases': 13,
                  'pixelBuffers': 192, 'diskFallbackCases': 2, 'legacyCompute': 'BINARY_LOAD',
                  'm1BuildSignVerify': 'PASS', 'm2BuildSignVerify': 'PASS'}))
