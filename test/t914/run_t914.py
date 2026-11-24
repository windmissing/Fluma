"""T914 interleaved runner: DM2/R1 x baseline/t913-v1/split-v2 with flag matrix.

Runs each PE via subprocess with stdout direct to file (no pipe stall),
freezing the t894 source input for all arms.
"""
import hashlib
import json
import os
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', '..'))
INPUT = os.path.join(ROOT, '.codex/local-test/t894-model-source.u32le')

BUILDS = [
    ('dm2', 'baseline', '.codex/local-test/t913-dm2-baseline', 'pc_sand_dm2_host.exe'),
    ('dm2', 'v1', '.codex/local-test/t913-dm2-t913-v1', 'pc_sand_dm2_host.exe'),
    ('dm2', 'split-v2', '.codex/local-test/t913-dm2-t913-split-v2', 'pc_sand_dm2_host.exe'),
    ('r1', 'baseline', '.codex/local-test/t913-r1-baseline', 'pc_sand_r1_host.exe'),
    ('r1', 'v1', '.codex/local-test/t913-r1-t913-v1', 'pc_sand_r1_host.exe'),
    ('r1', 'split-v2', '.codex/local-test/t913-r1-t913-split-v2', 'pc_sand_r1_host.exe'),
]

FLAG_SETS = {
    'plain': [],
    'detail': ['--source-timestamps-detail'],
    'prewarm': ['--source-prewarm'],
    'detail-prewarm': ['--source-timestamps-detail', '--source-prewarm'],
}


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def main():
    out_root = os.path.join(ROOT, 'test/reports/t914/runs')
    os.makedirs(out_root, exist_ok=True)
    manifest = []
    for family, variant, build, exe in BUILDS:
        host = os.path.join(ROOT, build, exe)
        modules = os.path.join(ROOT, build, 'sand_assets')
        for flag_name, flags in FLAG_SETS.items():
            tag = f'{family}-{variant}-{flag_name}'
            out = os.path.join(out_root, tag + '.jsonl')
            cmd = [host, '--modules', modules, '--source', INPUT, *flags]
            with open(out, 'wb') as log:
                proc = subprocess.run(cmd, stdout=log, stderr=subprocess.PIPE, timeout=600)
            manifest.append(dict(tag=tag, family=family, variant=variant, flags=flag_name,
                                 exit=proc.returncode, out=os.path.relpath(out, ROOT),
                                 hostSHA=sha256(host),
                                 stderr=proc.stderr.decode('utf-8', 'replace')[:200]))
            print(tag, 'exit=', proc.returncode, 'stderr=', manifest[-1]['stderr'][:80])
    with open(os.path.join(out_root, 'manifest.json'), 'w', encoding='utf-8') as f:
        json.dump(dict(inputSHA=sha256(INPUT), runs=manifest), f, indent=1)


if __name__ == '__main__':
    main()
