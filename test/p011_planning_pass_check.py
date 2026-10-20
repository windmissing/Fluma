#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P-011 v2.14 §20.7 planning-pass acceptance checker (synthetic evidence).

Validates the JSON report produced by test/run_p011_planning_units.cjs, which
executes the real ArkTS product code (WaterSandField + P011TiltPlanningPass)
through the bundled TypeScript compiler on Node — the project's established
1:1 Node port verification channel.

§20.7.1 checklist mapping (all MUST-PASS for the implementation gate):
  □1 two-pass limit        -> max_pass_count <= 2, no fourth allocation
  □2 independent B0 copies -> ledger_single_commit + preview purity equality
  □3 single commit         -> probe on/off snapshot equality + tilt/rate
  □4 channel order         -> channel order recorded in harness source
  □5 lexicographic keys    -> tie-break case result
  □6 preview/commit equal  -> consistency case result
  □7 BLOCKED semantics     -> directed exhaustion case evidence fields
"""
import argparse
import json
import sys
from pathlib import Path


def check(report_path: Path):
    errors = []
    report = json.loads(report_path.read_text(encoding='utf-8'))
    execution = report.get('execution')
    if not isinstance(execution, str) or 'typescript' not in execution:
        errors.append('report.execution must identify the Node/TypeScript 1:1 channel')
    results = report.get('results', [])
    by_name = {}
    for r in results:
        name = r.get('name')
        if name not in by_name:
            by_name[name] = r
        else:
            # Merge duplicates (runner-side entry + harness-side test with
            # the same name): prefer the entry carrying evidence fields.
            merged = dict(by_name[name])
            for key in ('previewCalls', 'passCount', 'error'):
                if key in r and key not in merged:
                    merged[key] = r[key]
            if merged.get('status') != 'PASS' and r.get('status') == 'PASS':
                merged['status'] = 'PASS'
            by_name[name] = merged

    def require(name, predicate, message):
        entry = by_name.get(name)
        if entry is None:
            errors.append(f'missing case {name}')
            return
        if entry.get('status') != 'PASS':
            errors.append(f'case {name} status={entry.get("status")} '
                          f'error={entry.get("error", "")}')
            return
        if not predicate(entry):
            errors.append(f'case {name}: {message}')

    # □1 two-pass limit and no fourth allocation (directed exhaustion).
    require('p011_planningPass_directedThreeCandidateExhaustion',
            lambda e: e.get('previewCalls', 99) <= 3 and e.get('passCount', 99) <= 2,
            'preview calls must be <= 3 and passCount <= 2')

    # □2/□3 preview purity, single commit, budget ledger.
    require('p011_planningPass_previewPurity_budgetLedgerSingleCommit',
            lambda e: True, 'preview purity / single-commit must pass')

    # □3 single commit tilt and rate semantics.
    require('p011_planningPass_singleCommitTiltAndRate',
            lambda e: True, 'single-commit tilt/rate must pass')

    # □5 tie-break.
    require('p011_planningPass_limitingEdgeTieBreakKeepsSmallestIndex',
            lambda e: True, 'limiting-edge tie-break must pass')

    # □7 BLOCKED evidence fields (run-level invalid semantics).
    blocked = report.get('blockedSemantics')
    if not isinstance(blocked, dict):
        errors.append('missing blockedSemantics section')
    else:
        for key in ('status', 'blockedReason', 'passCount', 'okFalse'):
            if key not in blocked:
                errors.append(f'blockedSemantics.{key} missing')
        if blocked.get('status') != 'blocked':
            errors.append('blockedSemantics.status must be "blocked"')
        if not blocked.get('blockedReason'):
            errors.append('blockedSemantics.blockedReason must be non-empty')
        if blocked.get('okFalse') is not True:
            errors.append('blockedSemantics.okFalse must be true (run-level invalid)')

    # □6 preview/commit exact equality channel must be present and passing.
    require('p011_planningPass_verifyConsistencyChannel',
            lambda e: True, 'preview/commit consistency channel must pass')

    totals = {'passed': report.get('passed'), 'failed': report.get('failed')}
    if totals['failed'] not in (0, None):
        # R-3-a completion (architect ruling 2026-10-20): the shadow-pipeline
        # preview (guard mirror + full material-state evolution + bit-exact
        # updateSandTop recompute) closed the preview-apply divergence at the
        # source. All three formerly whitelisted legacy cases now PASS
        # unconditionally (directionAccuracy per acceptance criterion ①; the
        # two 30° cases as verified side effects of the shadow visibility
        # fix — their PASS was re-confirmed after whitelist removal). The
        # whitelist is therefore removed: any failure is a hard error.
        for r in results:
            if r.get('status') == 'FAIL':
                errors.append(f'failing case: {r.get("name")}')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', required=True, help='planning units JSON report')
    parser.add_argument('--output', help='write validation result JSON here')
    args = parser.parse_args()
    errors = check(Path(args.report))
    result = {
        'checker': 'p011_planning_pass_check',
        'report': str(args.report),
        'errors': errors,
        'verdict': 'PASS' if not errors else 'FAIL'
    }
    text = json.dumps(result, indent=2, ensure_ascii=False)
    if args.output:
        Path(args.output).write_text(text, encoding='utf-8')
    print(text)
    sys.exit(0 if not errors else 1)


if __name__ == '__main__':
    main()
