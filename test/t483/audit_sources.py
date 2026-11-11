"""Audit frozen signed artifacts and emit installation inputs without a device."""
import hashlib
import json
import re
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '.codex/local-test'
REPORT = ROOT / 'test/reports/t483'
REPORT.mkdir(parents=True, exist_ok=True)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def source_hashes(project):
    main = project / 'entry/src/main'
    return {p.relative_to(main).as_posix(): sha(p.read_text(encoding='utf-8').encode())
            for p in sorted(main.rglob('*')) if p.suffix in {'.ets', '.cpp', '.h', '.inc'}}


def artifact(receipt_path):
    receipt = json.loads(receipt_path.read_text(encoding='utf-8'))
    project = Path(receipt['input']['path']).parents[5]
    hap = Path(receipt['signed']['path'])
    assert receipt['verifyApp'] == 'PASS'
    assert sha(hap.read_bytes()) == receipt['signed']['sha256']
    assert sha(Path(receipt['input']['path']).read_bytes()) == receipt['input']['sha256']
    with zipfile.ZipFile(hap) as package:
        elf = package.read('libs/arm64-v8a/libgrsm1.so')
    assert elf == (project / 'entry/build/default/intermediates/stripped_native_libs/default/arm64-v8a/libgrsm1.so').read_bytes()
    bridge = project / 'entry/src/main/cpp/gpu_resident/app_bridge.cpp'
    source = bridge.read_text(encoding='utf-8')
    assert 'CaptureRevokeLocked(replace?"WINDOW_OR_RUN_CHANGED":"RUN_RESET")' in source
    return project, {'receipt': receipt_path.relative_to(ROOT).as_posix(),
                     'signed': receipt['signed'], 'elfSHA': sha(elf),
                     'bridgeRawSHA': sha(bridge.read_bytes()),
                     'bridgeNormalizedSHA': sha(source.encode()),
                     'captureResetConditional': True}, elf


summary = {'schema': 'T483_SOURCE_RECOVERY_AUDIT_V1', 'status': 'DEVELOPER_COMPLETE',
           'device': 'NOT_RUN', 'sources': {}}
for target in ['regular', 'experience', 't410', 't437']:
    project, row, elf = artifact(ROOT / f'test/reports/t257/T467-{target}-final.json')
    fresh = LOCAL / ('t483-' + target)
    frozen_hashes = source_hashes(project)
    fresh_hashes = source_hashes(fresh)
    assert all(fresh_hashes.get(name) == digest for name, digest in frozen_hashes.items())
    added = sorted(fresh_hashes.keys() - frozen_hashes.keys())
    assert added == ['cpp/gpu_resident/m2/create_diagnostics.h',
                     'cpp/gpu_resident/program_cache/artifact_store.h',
                     'cpp/gpu_resident/program_cache/program_factory.h',
                     'cpp/gpu_resident/program_cache/sha256.h']
    for name in added:
        # New shared headers are copied by the base generator; M1 never includes them.
        for old_name in frozen_hashes:
            if old_name.startswith('cpp/'):
                old_text = (project / 'entry/src/main' / old_name).read_text(encoding='utf-8')
                assert not re.search(r'#include\s+["<][^">]*' + re.escape(Path(name).name) + r'[">]', old_text)
    assert '"m1/run.h"' in (project / 'entry/src/main/cpp/gpu_resident/app_bridge.cpp').read_text()
    row.update({'route': 'M1', 'project': str(project), 'freshProject': str(fresh),
                'normalizedSourceFilesMatched': len(frozen_hashes),
                'addedUnreferencedHeaders': added,
                'sourceTreeNormalizedSHA': sha(json.dumps(frozen_hashes, sort_keys=True).encode()),
                'freshBridgeRawSHA': sha((fresh / 'entry/src/main/cpp/gpu_resident/app_bridge.cpp').read_bytes())})
    summary['sources'][target] = row
    if target in ['t410', 't437']:
        result = subprocess.check_output([
            'python', '-X', 'utf8', 'test/t450/install_input.py', target, row['signed']['path'],
            '--receipt', row['receipt'], '--declared-sha', row['signed']['sha256']
        ], text=True)
        (REPORT / ('install-' + target + '.json')).write_text(result, encoding='utf-8')

old = LOCAL / 't467-dev/entry/src/main/cpp/gpu_resident/app_bridge.cpp'
old_source = old.read_text(encoding='utf-8')
assert 'CaptureRevokeLocked("WINDOW_OR_RUN_CHANGED")' in old_source
assert sha(old_source.encode()) != summary['sources']['experience']['bridgeNormalizedSHA']
summary['intermediate'] = {'path': str(old), 'rawSHA': sha(old.read_bytes()),
                           'classification': 'INTERMEDIATE_SOURCE_NOT_FINAL_SIGNING_INPUT',
                           'preserved': True}
project, row, elf = artifact(ROOT / 'test/reports/t257/T475-handoff.json')
assert '"m2route/run.h"' in (project / 'entry/src/main/cpp/gpu_resident/app_bridge.cpp').read_text()
old_shader = (project / 'source-v3.glsl').read_bytes()
assert old_shader + b'\0' in elf
assert sha(old_shader) == 'eb479a7b4fb3be0dc06245aef95b05e4ceabcaa9f9de7051994c5ac4504a1c1c'
assert (LOCAL / 't482-final/entry/src/main/cpp/gpu_resident/app_bridge.cpp').read_text(encoding='utf-8') == (project / 'entry/src/main/cpp/gpu_resident/app_bridge.cpp').read_text(encoding='utf-8')
row.update({'route': 'M2', 'shaderSHA': sha(old_shader),
            'sourceLine1138': old_shader.decode().splitlines()[1137],
            'recoveryOverlayMatchesT482': True})
summary['sources']['T475'] = row
probe = json.loads((ROOT / 'test/reports/t473/boot-after-ready.json').read_text(encoding='utf-8'))
attempt = next(a for a in probe['attempts'] if a['epoch'] == '4')
assert attempt['readyEpoch'] == attempt['epoch']
assert attempt['firstFailure']['stage'] == 'V2_PULSE_CLOCK_GAP'
gap = int(attempt['firstFailure']['c'])
assert gap > 33333333
summary['recoveryEvidence'] = {'file': 'test/reports/t473/boot-after-ready.json',
    'sha256': sha((ROOT / 'test/reports/t473/boot-after-ready.json').read_bytes()),
    'readyEpoch': attempt['readyEpoch'], 'firstFailure': attempt['firstFailure'],
    'latest': attempt['latest'], 'gapNs': gap, 'limitNs': 33333333,
    'ringIncomplete': probe['recovery']['incomplete'],
    'ringDropped': probe['recovery']['dropped'],
    'deviceInstalledHapIdentity': 'TESTER_REPORTED_T467_EXPERIENCE_FINAL',
    'driverCallCause': 'NOT_ESTABLISHED'}
layout = (ROOT / 'test/reports/t410-v4/t410_ld.json').read_text(encoding='utf-8')
declarations = sorted(set(re.findall(r'[a-f0-9]{64}', layout)))
old_sha = json.loads((ROOT / 'test/reports/t257/T450-t410.json').read_text())['signed']['sha256']
assert declarations == [old_sha]
assert old_sha != summary['sources']['t410']['signed']['sha256']
summary['t410Declaration'] = {'file': 'test/reports/t410-v4/t410_ld.json',
    'declaredSHA': old_sha, 'expectedT467SHA': summary['sources']['t410']['signed']['sha256'],
    'identity': 'OLD_T450_DECLARATION', 'installedHapIdentity': 'NOT_PROVEN_BY_DECLARATION'}
units = json.loads(subprocess.check_output(['python', '-X', 'utf8', 'test/t483/install_units.py'], text=True))
summary['installInputTests'] = units
recovery = json.loads((LOCAL / 't483-recovery-final/recovery-diff.json').read_text())
assert recovery['passed'] == recovery['total'] == 31
summary['recoveryChecks'] = recovery
(REPORT / 'recovery-diff.json').write_text(json.dumps(recovery, indent=2) + '\n')
build_path = LOCAL / 't483-build.txt'
build = build_path.read_text(encoding='utf-8-sig')
assert 'BUILD SUCCESSFUL' in build and 'BUILD FAILED' not in build
summary['build'] = {'status': 'PASS', 'project': str(LOCAL / 't483-experience'),
                    'log': 'test/reports/t483/build.txt'}
shutil.copyfile(build_path, REPORT / 'build.txt')
shutil.copyfile(LOCAL / 't483-recovery-final.txt', REPORT / 'recovery-build.txt')
summary['nextConditions'] = {
    'T461_T477': 'Use T482-final signing receipt for M2 whole root and cold/warm verification',
    'T410': 'Use install-t410.json exact HAP and Want SHA; explicit reset; capture fresh same-run READY and first GAP domains',
    'T437': 'Use install-t437.json exact dual-route HAP and Want SHA; explicit reset; compare full/prefix in valid READY window',
    'T473': 'READY then GAP is observed; current ring overflow prevents attributing a particular driver call'}
(REPORT / 'delivery.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
print(json.dumps({'status': 'PASS', 'signedSources': 5, 'recoveryChecks': 31,
                  'installInputChecks': 6, 'device': 'NOT_RUN'}))
