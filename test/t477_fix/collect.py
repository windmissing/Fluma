import hashlib
import json
import re
import shutil
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '.codex/local-test'
REPORT = ROOT / 'test/reports/t477_fix'
PROJECT = LOCAL / 't477-fix-release'
REPORT.mkdir(parents=True, exist_ok=True)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def last_summary(path):
    for line in reversed(path.read_text(encoding='utf-8-sig').splitlines()):
        if line.startswith('{'):
            return json.loads(line)
    raise AssertionError('MISSING_SUMMARY ' + str(path))


units = last_summary(LOCAL / 't477-fix-units.txt')
frames = last_summary(LOCAL / 't477-fix-frames.txt')
assert units['passed'] == 9 and units['failed'] == 0
assert frames['passed'] == 12 and frames['failed'] == 0
regressions = {}
for group, count in [('t306', 20), ('t404', 22)]:
    row = json.loads((LOCAL / f't477-fix-{group}.json').read_text(encoding='utf-8'))
    assert row['passed'] == count and row['failed'] == 0
    regressions[group] = row
recovery = json.loads((LOCAL / 't477-fix-recovery/recovery-diff.json').read_text())
assert recovery['passed'] == recovery['total'] == 31
app_log = (LOCAL / 't477-fix-app-present.txt').read_text(encoding='utf-8-sig')
present_marker = LOCAL / 't477-fix-native-present.txt'
if present_marker.exists():
    app_log += present_marker.read_text(encoding='utf-8')
assert 'T477 native Draw/Swap current frames=' in app_log
native_frames = int(re.search(r'T477 native Draw/Swap current frames=(\d+)', app_log)[1])
assert native_frames > 0
app = last_summary(LOCAL / 't477-fix-app-present.txt')
assert app['passed'] == 1 and app['failed'] == 0
assert app['admittedJQ'] == app['settledAfterUpQ'] == 131072
sign = json.loads((ROOT / 'test/reports/t257/T477-fix.json').read_text())
assert sign['verifyApp'] == 'PASS'
assert sha(Path(sign['signed']['path']).read_bytes()) == sign['signed']['sha256']
with zipfile.ZipFile(sign['signed']['path']) as hap:
    elf = hap.read('libs/arm64-v8a/libgrsm1.so')
assert b'T477_STAGE67_V1' in elf
old = LOCAL / 't482-final'
for name in ['source-v3.glsl', 'quantum-v3.glsl']:
    assert (old / name).read_bytes() == (PROJECT / name).read_bytes()
    assert (PROJECT / name).read_bytes() + b'\0' in elf
bridge = (PROJECT / 'entry/src/main/cpp/gpu_resident/app_bridge.cpp').read_text(encoding='utf-8')
assert 'const bool runnable=AllowedLocked()&&run&&!failed' in bridge
assert 'TailStage(141)' not in bridge and 'TailStage(142)' not in bridge
build = (LOCAL / 't477-fix-build.txt').read_text(encoding='utf-8-sig')
assert 'BUILD SUCCESSFUL' in build and 'BUILD FAILED' not in build
original_path = ROOT / 'test/reports/t477/device-rerun-v2/timeline/manifest.json'
original = json.loads(original_path.read_text(encoding='utf-8'))
status = json.loads(original['statusBefore'])
assert status['newPhysicalFrames'] == '2446'
assert status['clockDeltaNs'] == 34329218 and status['pageFault'] == 'CLOCK_GAP'
summary = {
    'schema': 'T477_RERUN_V2_DEVELOPER_FIX_V1', 'status': 'DEVELOPER_COMPLETE',
    'device': 'NOT_RUN', 'units': units, 'frames': frames, 'regressions': regressions,
    'recovery': recovery, 'generatedM2App': app, 'nativeDrawSwapCurrentAsserted': True,
    'nativePresentedPhysicalFrames': native_frames,
    'build': 'PASS', 'signatureVerification': 'PASS', 'signed': sign['signed'],
    'elfSHA256': sha(elf), 'shaderBytesUnchangedFromT482': True,
    'originalEvidence': {
        'path': original_path.relative_to(ROOT).as_posix(),
        'sha256': sha(original_path.read_bytes()), 'exportRows': original['rows'],
        'nativeNewPhysicalFrames': status['newPhysicalFrames'],
        'gapNs': status['clockDeltaNs'], 'limitNs': 33333333,
        'interpretation': 'GAP revokes an established session; native counter alone does not verify system pixels'},
    'deviceFollowup': [
        'Export new timeline dictionary past row120 and keep all original rejected page bytes',
        'Explicit new run/session after a real GAP; correlate boot/recovery/Draw/Swap in one scope',
        'Confirm actual device presentation independently'],
    'realGapStillRevokes': True
}
for name in ['t477-fix-units.txt', 't477-fix-frames.txt', 't477-fix-recovery.txt',
             't477-fix-app-final.txt', 't477-fix-app-present.txt', 't477-fix-build.txt']:
    shutil.copyfile(LOCAL / name, REPORT / name)
if present_marker.exists():
    shutil.copyfile(present_marker, REPORT / present_marker.name)
shutil.copyfile(LOCAL / 't477-fix-timeline/pages.ndjson', REPORT / 'native-pages.ndjson')
(REPORT / 'delivery.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
print(json.dumps({'status': 'PASS', 'unitChecks': 9, 'frameChecks': 12,
                  'coldChecks': 42, 'recoveryChecks': 31, 'generatedM2App': 'PASS',
                  'signedSHA': sign['signed']['sha256']}))
