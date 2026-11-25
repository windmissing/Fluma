"""T987 G9 adjacent ABBA/BAAB paired metering runner.

Runs T976 PE (873f9c98) on UHD 630 with fixed fixture/scope/detail, taking
two complete blocks: ABBA (A,B,B,A) and BAAB (B,A,A,B). Each trial is an
independent process invocation. Per T985: fixed warmup/process policy/
source order/attempts; each trial keeps SOURCE stop point and logs; any
position losing QUANTUM marks the block invalid.

A = baseline (--inventory-single-scan per T985 "单扫固定")
B = candidate (--face-exact-divide)
Both arms keep --quantum-detail face --quantum-timestamps.
"""
import json
import os
import subprocess
import sys
import time
import hashlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
ROOT = 'D:/hmprojects/HelloWorld'
PE = f'{ROOT}/.codex/local-test/t976-dm2-release/pc_sand_dm2_host.exe'
MODULES = f'{ROOT}/.codex/local-test/t976-dm2-release/sand_assets'
SOURCE = f'{ROOT}/test/reports/t923/runs/t924-runs/raw-dm2-t913-split-v2-at-t921-steps1-r2/accepted/1663211005500200-1/FIXTURE128.bin'
OUT = f'{ROOT}/test/reports/t987/runs/g9-paired'

def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()

def run_trial(label, candidate):
    prefix = f'{OUT}/{label}'
    rawdir = f'{OUT}/raw-{label}'
    argv = [PE, '--modules', MODULES, '--source', SOURCE, '--steps', '1',
            '--source-prepare-retired', '--quantum-timestamps', '--quantum-detail', 'face',
            '--inventory-single-scan',
            '--raw-envelope', rawdir]
    if candidate:
        argv.insert(-2, '--face-exact-divide')
    os.makedirs(OUT, exist_ok=True)
    t0 = time.time()
    with open(f'{prefix}.jsonl', 'w', encoding='utf8') as log:
        r = subprocess.run(argv, stdout=log, stderr=subprocess.STDOUT, cwd=ROOT)
    dt = time.time() - t0
    return {'label': label, 'candidate': candidate, 'exit': r.returncode, 'wall_s': round(dt, 1),
            'raw': rawdir, 'jsonl': f'{prefix}.jsonl'}

if __name__ == '__main__':
    blocks = {'ABBA': ['A1', 'B1', 'B2', 'A2'], 'BAAB': ['B3', 'A3', 'A4', 'B4']}
    results = []
    for block, trials in blocks.items():
        for name in trials:
            cand = name.startswith('B')
            res = run_trial(name, cand)
            results.append(res)
            print(json.dumps(res, ensure_ascii=False), flush=True)
    with open(f'{OUT}/runner-manifest.json', 'w', encoding='utf8') as f:
        json.dump({'pe': sha(PE), 'modules': sha(f'{MODULES}/roles.json'),
                   'source': sha(SOURCE), 'trials': results}, f, indent=1)
    print('DONE')
