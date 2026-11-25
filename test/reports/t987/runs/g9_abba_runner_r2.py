"""T987 G9 adjacent ABBA/BAAB paired metering - redo with warmup.

Warmup trial (A-mode, not metered) first to absorb cold pipeline compile and
driver cold start, then two complete blocks: ABBA (A,B,B,A) and BAAB (B,A,A,B).
Each position keeps SOURCE stop point; any position losing QUANTUM marks the
block invalid and the block is redone in full.
"""
import json
import os
import subprocess
import sys
import time
import hashlib

ROOT = 'D:/hmprojects/HelloWorld'
PE = f'{ROOT}/.codex/local-test/t976-dm2-release/pc_sand_dm2_host.exe'
MODULES = f'{ROOT}/.codex/local-test/t976-dm2-release/sand_assets'
SOURCE = f'{ROOT}/test/reports/t923/runs/t924-runs/raw-dm2-t913-split-v2-at-t921-steps1-r2/accepted/1663211005500200-1/FIXTURE128.bin'
OUT = f'{ROOT}/test/reports/t987/runs/g9-paired-r2'

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

def has_quantum_ts(name):
    out = []
    for l in open(f'{OUT}/{name}.jsonl', encoding='utf8', errors='replace'):
        l = l.strip()
        if not l.startswith('{'):
            continue
        try:
            d = json.loads(l)
        except Exception:
            continue
        if d.get('stage') == 'ROOT_GPU_TIMESTAMPS' and d.get('kind') == 'QUANTUM':
            if all(p.get('available') for p in d.get('points', [])):
                return True
    return False

def run_block(block_name, labels, results):
    for attempt in range(1, 4):
        block_ok = True
        trial_out = []
        for name in labels:
            cand = name.startswith('B')
            res = run_trial(name, cand)
            trial_out.append(res)
            print(json.dumps(res, ensure_ascii=False), flush=True)
            if not has_quantum_ts(name):
                print(f'BLOCK {block_name} attempt {attempt}: {name} lost QUANTUM -> redo', flush=True)
                block_ok = False
                break
        if block_ok:
            results.extend(trial_out)
            print(f'BLOCK {block_name} attempt {attempt}: COMPLETE', flush=True)
            return True
    return False

if __name__ == '__main__':
    results = []
    # warmup (not metered)
    wu = run_trial('warmup', False)
    print('WARMUP ' + json.dumps(wu, ensure_ascii=False), flush=True)
    ok1 = run_block('ABBA', ['A1', 'B1', 'B2', 'A2'], results)
    ok2 = run_block('BAAB', ['B3', 'A3', 'A4', 'B4'], results)
    with open(f'{OUT}/runner-manifest.json', 'w', encoding='utf8') as f:
        json.dump({'pe': sha(PE), 'modules': sha(f'{MODULES}/roles.json'),
                   'source': sha(SOURCE), 'warmup': wu, 'trials': results,
                   'abbaComplete': ok1, 'baabComplete': ok2}, f, indent=1)
    print('DONE abba=%s baab=%s' % (ok1, ok2))
