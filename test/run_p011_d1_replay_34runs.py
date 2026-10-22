#!/usr/bin/env python3
"""D1 step 2: replay eligibility over all 34 expansion runs + build
difference table (old checker vs fixed replay) + intersection reason table.
Offline, no device. Keeps old results; produces QA counts by original
randomization groups (fit/holdout) WITHOUT unsealing holdout residuals."""
import json, subprocess, sys, time
from pathlib import Path

PY = r'C:/Users/admin/.workbuddy/binaries/python/envs/default/Scripts/python.exe'
ROOT = Path(r'D:/hmprojects/HelloWorld')
ART = ROOT / 'test/artifacts/p011-v3expansion-20261021'
MANIFEST = ROOT / 'calibration/randomization-manifest-obs3v3-expansion-v1.json'
OUTDIR = ROOT / 'test/reports'
OUTDIR.mkdir(exist_ok=True)

def main():
    manifest = json.loads(MANIFEST.read_text(encoding='utf-8-sig'))
    # fit/holdout split from the frozen permutation
    rand = manifest.get('randomization', {})
    perm = rand.get('permutation') or []
    if not perm:
        raise SystemExit('manifest permutation missing')
    fit_ids = perm[:24]
    hold_ids = perm[24:]
    role_of = {rid: ('fit' if rid in fit_ids else 'holdout') for rid in perm}

    runs = sorted(p for p in ART.iterdir() if p.is_dir() and p.name.startswith('p011-v3expansion-run-'))
    consolidated = []
    t_start = time.time()
    for i, run_dir in enumerate(runs, 1):
        ledger = run_dir / 'ledger-trimmed.ndjson'
        old_sum = run_dir / 'v3-summary.json'
        out = OUTDIR / f'd1_replay_{run_dir.name}.json'
        if not out.exists():
            cmd = [PY, str(ROOT / 'test/p011_obs3_d1_eligibility_replay.py'),
                   '--ledger', str(ledger),
                   '--manifest', str(MANIFEST),
                   '--old-summary', str(old_sum),
                   '--output', str(out), '--no-details']
            t0 = time.time()
            r = subprocess.run(cmd, capture_output=True, text=True, cwd=str(ROOT))
            if r.returncode != 0:
                print(f'FAIL {run_dir.name}: {r.stderr[-300:]}', flush=True)
                continue
            print(f'[{i}/{len(runs)}] {run_dir.name} {time.time()-t0:.0f}s', flush=True)
        d = json.loads(out.read_text(encoding='utf-8'))
        d['_runDir'] = run_dir.name
        d['_role'] = role_of.get(run_dir.name, 'unknown')
        consolidated.append(d)
    (OUTDIR / 'd1_replay_all_runs_consolidated.json').write_text(
        json.dumps(consolidated, indent=1, ensure_ascii=False), encoding='utf-8')

    # difference + intersection tables
    cells = ('start/+', 'start/-', 'stop/+', 'stop/-')
    agg_old = {c: {'real': 0, 'elig': 0, 'excl': {}} for c in cells}
    agg_new = {c: {'raw': 0, 'elig': 0, 'excl': {}} for c in cells}
    inter_new = {c: {} for c in cells}   # reason-set -> count (frozenset key)
    by_role = {'fit': {c: {'raw': 0, 'elig': 0, 'runs_with_elig': 0} for c in cells},
               'holdout': {c: {'raw': 0, 'elig': 0, 'runs_with_elig': 0} for c in cells}}
    runs_with_elig = {c: 0 for c in cells}
    for d in consolidated:
        old = d.get('diff') or {}
        for c in cells:
            if c in old:
                o = old[c]
                agg_old[c]['real'] += (o['realTransitions']['old'] or 0)
                agg_old[c]['elig'] += (o['eligibleBrackets']['old'] or 0)
                for k, v in (o['exclusions']['old'] or {}).items():
                    agg_old[c]['excl'][k] = agg_old[c]['excl'].get(k, 0) + v
            nc = d['cells'][c]
            agg_new[c]['raw'] += nc['rawTransitions']
            agg_new[c]['elig'] += nc['eligibleTransitions']
            for k, v in nc['exclusions'].items():
                agg_new[c]['excl'][k] = agg_new[c]['excl'].get(k, 0) + v
        # intersections need reason rows; with --no-details we only kept counts.
        # Re-derive intersections from per-run detail files? None. Keep counts only;
        # intersection table built in a second pass with details for transition rows only.
        role = d.get('_role', 'unknown')
        if role in by_role:
            for c in cells:
                by_role[role][c]['raw'] += d['cells'][c]['rawTransitions']
                by_role[role][c]['elig'] += d['cells'][c]['eligibleTransitions']
        for c in cells:
            if d['cells'][c]['eligibleTransitions'] > 0:
                runs_with_elig[c] += 1
                if d.get('_role') in by_role:
                    by_role[d['_role']][c]['runs_with_elig'] += 1

    summary = {
        'runsReplayed': len(consolidated),
        'elapsedSec': round(time.time() - t_start, 1),
        'oldCheckerAgg': agg_old,
        'fixedReplayAgg': agg_new,
        'runsRunsByCellWithEligible': runs_with_elig,
        'byRole': by_role,
        'roleNote': 'fit/holdout split by frozen randomization permutation '
                    '(first 24 = fit, last 10 = holdout); no residual or '
                    'threshold information computed for either group',
    }
    (OUTDIR / 'd1_replay_diff_summary.json').write_text(
        json.dumps(summary, indent=1, ensure_ascii=False), encoding='utf-8')
    print(json.dumps({c: {'old_real': agg_old[c]['real'], 'old_elig': agg_old[c]['elig'],
                          'new_raw': agg_new[c]['raw'], 'new_elig': agg_new[c]['elig']} for c in cells},
                     ensure_ascii=False))

if __name__ == '__main__':
    main()
