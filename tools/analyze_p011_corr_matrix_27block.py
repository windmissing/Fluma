#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P-011 27-block formal analysis pipeline dry-run (tester, DRY_RUN).

Full pipeline rehearsal on partially collected data (~82 runs, G1 12 complete
blocks + G2 8 complete blocks) per
correction-matrix-conditioning-statistics-revision-P011-v1.md:

  - pooled 27-block caliber: single conditioned working state; G1/G2/G0 are
    batch labels (diagnostics only), NOT separate scenarios
  - per-block gain_b = 1 - J_E,D/J_E,A (complete 4-variant blocks only;
    incomplete blocks are SKIPPED and COUNTED, never synthesized)
  - paired-block percentile bootstrap N=10000, seed=20261022, median of
    gain_b, two-sided 95% CI = 2.5/97.5 percentiles
  - drift diagnostics: per-batch-label J_E,A / frame time / theta profiles
    (descriptive only, NEVER block deletion or re-stratification)
  - noninferiority trio: frame time, correction crowding, budget excess
  - money closure check (correctionRequested == granted + rejected per run)
  - tan30 boundary diagnostics: invariantAtBoundary = |maxErr - H| <= 1e-9
    (contract-boundary saturation flag, NOT a violation flag)

J_E precise caliber (interim-verified, commit 4402c5f):
  r_e = S_after[e+1] - S_after[e] - tan(thetaAccepted)   (dx=1.0)
  e_e = max(0, |r_e| - slopeStop), E = 0.5 * sum e_e^2 (no active mask)
  J_E = segmented-constant integral over the FORMAL window (H0..H8; CND_*
  prefix excluded), real monotonic physicsTime, divided by window duration.

DRY_RUN / NOT_FOR_DECISION: no gate is decided from this run; collection is
never influenced; frozen anchors untouched.
"""
import csv
import hashlib
import json
import math
import os
import random
import statistics
from pathlib import Path

ROOT = Path(r"D:/hmprojects/HelloWorld/test/artifacts/p011-corr-matrix-formal-108-20261022")
OUT = Path(r"D:/hmprojects/HelloWorld/test/reports/dryrun_27block_pipeline_v1.json")
VARIANTS = ("A", "B", "C", "D")
BOOTSTRAP_N = 10000
BOOTSTRAP_SEED = 20261022
TAN30 = 0.5773502691896257
ENGINE_EPS = 1e-9
RESOLUTION_NOTE = "J_E,A resolution: if A denominator ~0 -> INCOMPLETE_EXCITATION per block"


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def discover_blocks():
    blocks = {}
    for d_ in sorted(os.listdir(ROOT)):
        if "-b" in d_ and "partial" not in d_ and (ROOT / d_).is_dir():
            p = d_.split("-")
            if len(p) < 5:
                continue
            strat, blk, var = p[2], p[3], p[4]
            blocks.setdefault(f"{strat}-{blk}", {"stratum": strat, "runs": {}})["runs"][var] = d_
    return blocks


def je_precise(run_dir):
    seg_E = {}
    prev_t = prev_E = prev_seg = None
    theta_mismatch = 0
    boundary_substeps = 0
    max_err_seen = 0.0
    h_used = set()
    corr = {"requested": 0, "granted": 0, "rejected": 0}
    money_bad = 0
    budget_excess_substeps = 0
    last_theta = []
    with (run_dir / "ledger-trimmed.ndjson").open("r", encoding="utf-8") as stream:
        for line in stream:
            if not line.strip():
                continue
            record = json.loads(line)
            c = record["context"]
            seg = c.get("segmentId") or ""
            t = c.get("physicsTime")
            et = record.get("executedThresholds") or c.get("executedThresholds") or {}
            h = et.get("slopeStop", 0.36397023426620234)
            h_used.add(round(h, 12))
            theta = c["thetaAccepted"]
            if len(last_theta) < 3:
                last_theta.append(theta)
            if abs(theta - c.get("effectiveTilt", theta)) > 1e-9:
                theta_mismatch += 1
            S = record["stateAfter"]["S"]
            E = 0.0
            for e in range(len(S) - 1):
                r_e = (S[e + 1] - S[e]) - math.tan(math.radians(theta))
                excess = abs(r_e) - h
                if excess > 0:
                    E += 0.5 * excess * excess
            audit = record.get("audit", {})
            me = audit.get("maxSandTopInvariantError")
            if me is not None:
                max_err_seen = max(max_err_seen, me)
                if abs(me - TAN30) <= ENGINE_EPS:
                    boundary_substeps += 1
            if (audit.get("totalBudgetExcess") or 0) > 0:
                budget_excess_substeps += 1
            a = audit
            if "correctionRequested" in a:
                corr["requested"] += a.get("correctionRequested") or 0
                corr["granted"] += a.get("correctionGranted") or 0
                corr["rejected"] += a.get("correctionRejected") or 0
                if abs((a.get("correctionRequested") or 0)
                       - ((a.get("correctionGranted") or 0)
                          + (a.get("correctionRejected") or 0))) > 1e-9:
                    money_bad += 1
            if prev_t is not None and prev_seg is not None:
                dt = t - prev_t
                if dt > 0:
                    slot = seg_E.setdefault(prev_seg, [0.0, 0.0])
                    slot[0] += prev_E * dt
                    slot[1] += dt
            prev_t, prev_E, prev_seg = t, E, seg
    formal_segs = [s for s in seg_E if not s.startswith("CND")]
    num = sum(seg_E[s][0] for s in formal_segs)
    den = sum(seg_E[s][1] for s in formal_segs)
    return {
        "J_E_formal": round(num / den, 6) if den > 0 else None,
        "windowMs": round(den, 1),
        "thetaMismatches": theta_mismatch,
        "slopeStopValues": sorted(h_used),
        "boundarySubsteps": boundary_substeps,
        "maxErr": round(max_err_seen, 6),
        "correction": corr,
        "moneyClosureBadSubsteps": money_bad,
        "budgetExcessSubsteps": budget_excess_substeps,
        "firstThetas": last_theta,
    }


def wall_time(run_dir):
    return None  # filled from batch-log


def main():
    blocks = discover_blocks()
    complete, incomplete = {}, []
    for b, info in sorted(blocks.items()):
        vs = sorted(set(info["runs"].keys()))
        if tuple(vs) == VARIANTS:
            complete[b] = info
        else:
            incomplete.append({"block": b, "stratum": info["stratum"],
                               "variantsPresent": vs, "missing": [v for v in VARIANTS if v not in vs]})
    print(f"complete blocks: {len(complete)}  incomplete: {len(incomplete)}")
    results = {}
    for b, info in sorted(complete.items()):
        results[b] = {"stratum": info["stratum"]}
        for v in VARIANTS:
            print(f"  {b}-{v}: computing...", flush=True)
            results[b][v] = je_precise(ROOT / info["runs"][v])
    gains = {}
    for b in sorted(complete):
        ja = results[b]["A"]["J_E_formal"]
        jd = results[b]["D"]["J_E_formal"]
        gains[b] = {"gain": round(1.0 - jd / ja, 6) if ja and ja > 0 else None,
                    "stratum": results[b]["stratum"],
                    "J_E_A": ja, "J_E_D": jd,
                    "incompleteExcitation": (ja is None or ja <= 0)}
    vals = [g["gain"] for g in gains.values() if g["gain"] is not None]
    med = statistics.median(vals) if vals else None
    rng = random.Random(BOOTSTRAP_SEED)
    boots = []
    for _ in range(BOOTSTRAP_N):
        sample = [vals[rng.randrange(len(vals))] for _ in vals]
        boots.append(statistics.median(sample))
    boots.sort()
    ci = [boots[int(0.025 * (BOOTSTRAP_N - 1))], boots[int(0.975 * (BOOTSTRAP_N - 1))]]
    # drift diagnostics (descriptive only)
    drift = {}
    for strat in ("G1", "G2", "G0"):
        keys = [b for b in complete if b.startswith(strat + "-")]
        if not keys:
            continue
        je_a = [results[b]["A"]["J_E_formal"] for b in keys]
        drift[strat] = {
            "completeBlocks": len(keys),
            "J_E_A_median": round(statistics.median(je_a), 6),
            "J_E_A_range": [min(je_a), max(je_a)],
            "note": "batch label diagnostics only; no re-stratification",
        }
    # noninferiority trio from batch-log
    wall = {}
    log = ROOT / "batch-log.csv"
    if log.exists():
        with log.open(encoding="utf-8-sig", newline="") as handle:
            for row in csv.reader(handle):
                if not row or row[0] == "runIndex":
                    continue
                name = row[1]
                if "-b" in name and "partial" not in name:
                    p = name.split("-")
                    wall.setdefault(f"{p[2]}-{p[4]}", []).append(int(row[-1]))
    trio = {"wallSecMean": {v: round(sum(w) / len(w), 1) for v, w in sorted(wall.items())}}
    # money closure + crowding aggregates per variant
    crowd = {}
    for v in VARIANTS:
        req = sum(results[b][v]["correction"]["requested"] for b in complete)
        gr = sum(results[b][v]["correction"]["granted"] for b in complete)
        rj = sum(results[b][v]["correction"]["rejected"] for b in complete)
        crowd[v] = {"requested": req, "granted": gr, "rejected": rj,
                    "rejectRate": round(rj / req, 4) if req else None}
    money_bad_total = sum(results[b][v]["moneyClosureBadSubsteps"]
                          for b in complete for v in VARIANTS)
    budget_total = sum(results[b][v]["budgetExcessSubsteps"]
                       for b in complete for v in VARIANTS)
    # boundary diagnostics
    boundary = {
        "runsAtContractBoundary": sum(
            1 for b in complete for v in VARIANTS
            if results[b][v]["boundarySubsteps"] > 0),
        "substepsAtBoundary": sum(results[b][v]["boundarySubsteps"]
                                  for b in complete for v in VARIANTS),
        "maxErrMax": round(max(results[b][v]["maxErr"] for b in complete
                               for v in VARIANTS), 6),
        "note": "contract-boundary saturation flag |maxErr-H|<=1e-9; NOT a "
                "violation flag; no H/EPS/guard relaxation",
    }
    out = {
        "run": "27-block formal pipeline DRY_RUN",
        "status": "DRY_RUN / NOT_FOR_DECISION",
        "caliber": "correction-matrix-conditioning-statistics-revision-P011-v1: "
                   "pooled single working state, paired-block bootstrap "
                   "N=10000 seed=20261022, median gain>=10% & CI lower>0 gate "
                   "unchanged",
        "blockCompleteness": {
            "complete": sorted(complete.keys()),
            "incompleteSkipped": incomplete,
            "skippedCount": len(incomplete),
            "rule": "incomplete blocks are skipped and counted; never "
                    "synthesized; formal batch must fill them",
        },
        "blocks": {b: {"gain": gains[b],
                       **{v: results[b][v] for v in VARIANTS}}
                   for b in sorted(complete)},
        "primary": {
            "nCompleteBlocks": len(complete),
            "gainMedian": med,
            "gainRange": [min(vals), max(vals)] if vals else None,
            "gainStdev": round(statistics.stdev(vals), 6) if len(vals) > 1 else None,
            "bootstrapMedianCI95": ci,
            "bootstrapN": BOOTSTRAP_N, "bootstrapSeed": BOOTSTRAP_SEED,
            "gate": "median>=10% AND CI-lower>0 (frozen values, unchanged)",
        },
        "driftDiagnostics": drift,
        "noninferiority": {"wallTime": trio["wallSecMean"],
                           "correctionCrowding": crowd,
                           "budgetExcessSubstepsTotal": budget_total,
                           "moneyClosureBadSubsteps": money_bad_total},
        "tan30Boundary": boundary,
        "resolutionNote": RESOLUTION_NOTE,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n",
                   encoding="utf-8")
    print(json.dumps(out["primary"], ensure_ascii=False))
    print("skipped incomplete:", len(incomplete))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
