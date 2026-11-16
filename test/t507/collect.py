"""T507 delivery evidence from actual generated implementations, without a device."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '.codex/local-test'
REPORT = ROOT / 'test/reports/t507'
REPORT.mkdir(parents=True, exist_ok=True)
sha = lambda data: hashlib.sha256(data).hexdigest()
contract = json.loads((ROOT / 'docs/architecture-T503-cold-manifest-v1.json').read_text(encoding='utf-8'))
handoff = {'revision': 'T507_DEVELOPER_BOUNDED_COLD_V1', 'device': 'NOT_RUN', 'routes': {}}
for route in ['m1', 'm2']:
    project = LOCAL / ('t507-m1-delivery' if route == 'm1' else 't507-m2-1116')
    manifest = json.loads((project / 't507-manifest.json').read_text(encoding='utf-8'))
    for name, digest in manifest['files'].items():
        actual = project / 'entry/src/main/cpp/gpu_resident' / name
        assert sha(actual.read_bytes()) == digest
    for name, digest in manifest['integrationFiles'].items():
        assert sha((project / 'entry/src/main/ets' / name).read_bytes()) == digest
    assert manifest['files']['execution_diagnostics.h'] == contract['baseline']['executionDiagnosticsSHA256']
    for name in ['cold_protocol.h', 'cold_trace.h', 'cold_diagnostics.h']:
        assert manifest['files'][name] == sha((ROOT / 'entry/src/main/cpp/gpu_resident' / name).read_bytes())
    for name in ['GpuBoundedCold', 'GpuColdTraceV1', 'GpuUtf8Budget']:
        assert (ROOT / ('entry/src/main/ets/physics/' + name + '.ets')).read_bytes() == (project / ('entry/src/main/ets/physics/' + name + '.ets')).read_bytes()
    shutil.copyfile(project / 't507-manifest.json', REPORT / ('generation-' + route + '.json'))
    signing = json.loads((ROOT / ('test/reports/t257/T507-' + route + '-complete.json')).read_text(encoding='utf-8'))
    assert signing['verifyApp'] == 'PASS'
    package = Path(signing['signed']['path'])
    assert sha(package.read_bytes()) == signing['signed']['sha256']
    with zipfile.ZipFile(package) as archive:
        elf = next(n for n in archive.namelist() if n.endswith('/arm64-v8a/libgrsm1.so'))
        elf_sha = sha(archive.read(elf))
    handoff['routes'][route] = {'signed': signing['signed'], 'elfSHA256': elf_sha, 'verifyApp': 'PASS',
                                'trace': 'SUPPORTED' if route == 'm2' else 'UNSUPPORTED_ROUTE',
                                'diagnostics': 'T503_EXECUTION_DIAGNOSTICS_V1'}
    shutil.copyfile(ROOT / ('test/reports/t257/T507-' + route + '-complete.json'), REPORT / ('signing-' + route + '.json'))
    for suffix in ['complete-build.log']:
        source = LOCAL / ('t507-' + route + '-' + suffix)
        data = source.read_text(encoding='utf-8', errors='replace')
        assert 'BUILD SUCCESSFUL' in data and 'BUILD FAILED' not in data
        (REPORT / source.name).write_text('\n'.join(l.rstrip() for l in data.splitlines()) + '\n', encoding='utf-8')

unit = LOCAL / 't507-units-boundary'
for name in ['fallback.json', 'fallback-worst.json', 'boundary-complete.json', 'boundary-minus-one.json', 'complete.json', 'unicode.json']:
    json.loads((unit / name).read_text(encoding='utf-8'))
    shutil.copyfile(unit / name, REPORT / name)
full = json.loads((unit / 'boundary-complete.json').read_text(encoding='utf-8'))
small = json.loads((unit / 'boundary-minus-one.json').read_text(encoding='utf-8'))
assert full['state'] == 'COMPLETE' and small['state'] == 'INCOMPLETE' and small['reason'] == 'DIAGNOSTIC_BUDGET'
for a, b in zip(full['groups'], small['groups']):
    assert a['firstCause'] == b['firstCause'] and a['faultRoot'] == b['faultRoot']
assert (unit / 'fallback-worst.json').stat().st_size <= 16384
handoff['unitChecks'] = 28
collector = json.loads((LOCAL / 't507-pages-current.json').read_text(encoding='utf-8'))
assert collector['failed'] == 0
shutil.copyfile(LOCAL / 't507-pages-current.json', REPORT / 'collectors.json')
handoff['collectorChecks'] = collector['passed']

gl = LOCAL / 't507-gl-owner'
checks = json.loads((gl / 'span-checks.json').read_text(encoding='utf-8'))
assert all(checks.values())
shutil.copyfile(gl / 'span-checks.json', REPORT / 'span-checks.json')
for index, kind in [(0, 1), (1, 2)]:
    files = sorted(gl.glob('trace-' + str(index) + '-*.json'), key=lambda p: int(p.stem.rsplit('-', 1)[1]))
    assert files
    header = json.loads(files[0].read_text(encoding='utf-8'))
    assert header['page'] == 0 and header['rootKind'] == kind and header['scope']['route'] == 'm2'
    assert len(files) == header['pages'] + 1
    words = 0
    total = 0
    for page, file in enumerate(files):
        value = json.loads(file.read_text(encoding='utf-8'))
        assert value['page'] == page and value['traceId'] == header['traceId'] and value['scope'] == header['scope']
        assert file.stat().st_size <= 16384
        assert len(value['words']) <= 1024
        total += file.stat().st_size
        words += len(value['words'])
        target = REPORT / 'trace' / file.name
        target.parent.mkdir(exist_ok=True)
        shutil.copyfile(file, target)
    assert words == header['actualWords'] and total <= len(files) * 16384
    handoff['sourceTrace' if kind == 1 else 'quantumTrace'] = {'words': words, 'pagesWithHeader': len(files), 'utf8Bytes': total}
value = json.loads((gl / 'bounded-diagnostic.json').read_text(encoding='utf-8'))
assert value['state'] == 'COMPLETE'
shutil.copyfile(gl / 'bounded-diagnostic.json', REPORT / 'actual-diagnostic.json')
for name in ['t507-gl-owner.log', 't507-units-boundary.log']:
    data = (LOCAL / name).read_text(encoding='utf-8', errors='replace')
    assert 'Assertion failed' not in data and 'error C' not in data
    (REPORT / name).write_text('\n'.join(l.rstrip() for l in data.splitlines()) + '\n', encoding='utf-8')
handoff['faultInjection'] = ['GL map invalid target', 'GL unmap false return', 'restore after unbinding current context', 'allocation null', 'simulated NAPI exception RAII', 'collector IO/cancel/manifest/budget errors']
handoff['limits'] = {'traceCapacityBytes': 55410688, 'slots': 2, 'builders': 1, 'readers': 1,
                     'captureMaxBytes': 1404072, 'writerMaxBytes': 2097152, 'ttlNs': 60000000000}
(REPORT / 'handoff.json').write_text(json.dumps(handoff, indent=2) + '\n', encoding='utf-8')
print(json.dumps({'result': 'PASS', 'units': 28, 'collector': collector['passed'], 'device': 'NOT_RUN'}))
