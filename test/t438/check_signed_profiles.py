from pathlib import Path
import hashlib
import json
import subprocess
import sys
import zipfile


def digest(data):
    return hashlib.sha256(data).hexdigest()


rows = []
for profile in ('regular', 'experiment'):
    receipt = json.loads(Path(f'test/reports/t257/T438-{profile}-release.json').read_text(encoding='utf-8'))
    hap = Path(receipt['signed']['path'])
    assert digest(hap.read_bytes()) == receipt['signed']['sha256']
    with zipfile.ZipFile(hap) as z:
        module = 'libs/arm64-v8a/libgrsm1.so'
        lib = z.read(module)
    enabled = profile == 'experiment'
    assert b'GRS_M1_EXPERIENCE_V1' in lib and b'GRS_GPU_SCENE_Q20_M1_V2' in lib
    assert (b'T438_EXPERIMENT_V2' in lib) == enabled
    assert (b'T438_REGULAR' in lib) != enabled
    assert (b'PROGRESS_V2_UNSUPPORTED' in lib) != enabled
    assert (b'PROGRESS_V2_NEXT_RUN' in lib) == enabled
    project = Path(sys.argv[1 if not enabled else 2]) if len(sys.argv) == 3 else Path(f'.codex/local-test/t438-{profile}')
    for filename in ('full_transport.cpp', 'full_transport.h', 'quantum_candidate.cpp', 'quantum_candidate.h', 'scene.cpp'):
        relative = f'entry/src/main/cpp/gpu_resident/m1/{filename}'
        original = subprocess.check_output(['git', 'show', f'0228a7d:{relative}'])
        assert (project / relative).read_bytes() == original, relative
    rows.append(dict(profile=profile, hapSha256=receipt['signed']['sha256'], libSha256=digest(lib),
                     progressV2Macro=enabled, nativeBranch='PROGRESS_V2_NEXT_RUN' if enabled else 'PROGRESS_V2_UNSUPPORTED'))

out = dict(schema='T438_SIGNED_PROFILE_DIFFERENTIAL_V1', frozenNative='0228a7d', passed=len(rows), failed=0, rows=rows)
Path('test/reports/t438/signed-profiles.json').write_text(json.dumps(out, indent=2) + '\n', encoding='utf-8')
print(json.dumps(out))
