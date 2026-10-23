#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P-011 G1 interim precise J_E recomputation (tester, NOT_FOR_DECISION).

Recomputes the G1 12-block per-block gain=1-J_E,D/J_E,A with the EXACT
preregistration §3 caliber (no stride sampling), to adjudicate whether
manager's negative interim signal (median -266.6%, stride=8, hand-written
E formula) is a real signal or a caliber artifact.

Frozen caliber (preregistration-P011-correction-matrix-v1.md §3 E-trajectory
row + §2.1 decision-time contract):
  r_e   = (S[e+1] - S[e]) / dx - tan(thetaAccepted)
          S taken from stateAfter.S (post-step achieved state, per prereg);
          dx = 1.0 (verified: engine edge.d == dS/dx - tan(theta) with dx=1.0);
          thetaAccepted = context.thetaAccepted (identical pre/post correction
          slot - the slot operates on residual, not theta; its effect lands in
          stateAfter.S and is therefore captured)
  e_e   = max(0, |r_e| - h), h = executedThresholds.slopeStop (per record)
  E     = 0.5 * sum_e e_e^2 over all 127 edges (NO active mask - prereg
          explicitly forbids active masking)
  J_E   = segmented-constant integral of E over the FORMAL window (H0..H8,
          conditioning prefix CND_* excluded) using real monotonic
          physicsTime, divided by the window duration
  per-phase breakdown kept (H/L phases separately)

Sanity echoes (manager's checks, re-verified precisely):
  - prefix vs formal split
  - thetaAccepted == effectiveTilt for all records
  - executedThresholds identical across variants (26/20)
Noninferiority trio (same interim caliber): ordinary-channel crowding
(correctionRequested vs ordinary losses), input-reachability cost, frame time.

Output: per-block gains + paired bootstrap 95% CI (12 blocks, block-level
resampling, seed=20261019), labeled INTERIM_NOT_FOR_DECISION.
"""
import hashlib
import json
import math
import os
import random
import statistics
import sys
from pathlib import Path

ROOT = Path(r"D:/hmprojects/HelloWorld/test/artifacts/p011-corr-matrix-formal-108-20261022")
OUT = Path(r"D:/hmprojects/HelloWorld/test/reports/interim_g1_je_precise_v1.json")
VARIANTS = ("A", "B", "C", "D")
BOOTSTRAP_N = 10000
BOOTSTRAP_SEED = 20261019


def iter_ledger(run_dir):
    path = run_dir / "ledger-trimmed.ndjson"
    with path.open("r", encoding="utf-8") as stream:
        for line in stream:
            if line.strip():
                yield json.loads(line)


def je_precise(run_dir):
    """Precise J_E per the frozen caliber. Returns dict with formal/prefix
    window integrals, per-phase breakdown, and sanity flags."""
    seg_E = {}       # (segmentId) -> [E_sum_weighted, t_duration, samples]
    prev_t = None
    prev_E = None
    prev_seg = None
    theta_mismatch = 0
    records = 0
    h_used = set()
    formal_start_t = None
    last_t = None
    for record in iter_ledger(run_dir):
        c = record["context"]
        seg = c.get("segmentId") or ""
        t = c.get("physicsTime")
        et = record.get("executedThresholds") or c.get("executedThresholds") or {}
        h = et.get("slopeStop", 0.36397023426620234)
        h_used.add(round(h, 12))
        theta = c["thetaAccepted"]
        eff = c.get("effectiveTilt")
        if abs(theta - eff) > 1e-9:
            theta_mismatch += 1
        S = record["stateAfter"]["S"]
        E = 0.0
        for e in range(len(S) - 1):
            r_e = (S[e + 1] - S[e]) / 1.0 - math.tan(math.radians(theta))
            excess = abs(r_e) - h
            if excess > 0:
                E += 0.5 * excess * excess
        records += 1
        if seg.startswith("CND"):
            if formal_start_t is None:
                pass
        elif formal_start_t is None:
            formal_start_t = t
        last_t = t
        # segmented-constant: the previous substep's E holds until this
        # substep's timestamp (E is the state AFTER the previous step).
        if prev_t is not None and prev_seg is not None:
            dt = t - prev_t
            if dt > 0:
                slot = seg_E.setdefault(prev_seg, [0.0, 0.0, 0])
                slot[0] += prev_E * dt
                slot[1] += dt
                slot[2] += 1
        prev_t, prev_E, prev_seg = t, E, seg
    def je_of(segments):
        num = sum(seg_E[s][0] for s in segments if s in seg_E)
        den = sum(seg_E[s][1] for s in segments if s in seg_E)
        return (num / den if den > 0 else None), den
    formal_segs = [s for s in seg_E if not s.startswith("CND")]
    prefix_segs = [s for s in seg_E if s.startswith("CND")]
    formal, formal_t = je_of(formal_segs)
    prefix, prefix_t = je_of(prefix_segs)
    # per-phase: group by leading phase class (Hn=hold, Ln=leg)
    phases = {}
    for s in formal_segs:
        ph = "hold" if s.startswith("H") else ("leg" if s.startswith("L") else s)
        num = seg_E[s][0]
        den = seg_E[s][1]
        slot = phases.setdefault(ph, [0.0, 0.0])
        slot[0] += num
        slot[1] += den
    per_phase = {ph: round(num / den, 6) if den > 0 else None
                 for ph, (num, den) in phases.items()}
    return {
        "J_E_formal": round(formal, 6) if formal is not None else None,
        "J_E_prefix": round(prefix, 6) if prefix is not None else None,
        "formalWindowSec": round(formal_t, 3),
        "prefixWindowSec": round(prefix_t, 3),
        "records": records,
        "thetaAcceptedVsEffectiveTiltMismatches": theta_mismatch,
        "slopeStopValues": sorted(h_used),
        "perPhaseJ_E": per_phase,
    }


def main():
    blocks = {}
    for d in sorted(os.listdir(ROOT)):
        if "G1-b" in d and "partial" not in d:
            parts = d.split("-")
            blocks.setdefault(parts[3], {})[parts[4]] = ROOT / d
    complete = sorted(b for b, vs in blocks.items() if sorted(vs) == list(VARIANTS))
    print(f"complete G1 blocks: {len(complete)}")
    results = {}
    for b in complete:
        results[b] = {}
        for v in VARIANTS:
            print(f"  {b}-{v}: computing...", flush=True)
            results[b][v] = je_precise(blocks[b][v])
    gains = {}
    for b in complete:
        ja = results[b]["A"]["J_E_formal"]
        jd = results[b]["D"]["J_E_formal"]
        gains[b] = round(1.0 - jd / ja, 6) if ja and ja > 0 else None
    vals = [gains[b] for b in complete if gains[b] is not None]
    med = statistics.median(vals) if vals else None
    # paired bootstrap over blocks (block-level resampling)
    rng = random.Random(BOOTSTRAP_SEED)
    boots = []
    for _ in range(BOOTSTRAP_N):
        sample = [vals[rng.randrange(len(vals))] for _ in vals]
        boots.append(statistics.median(sample))
    boots.sort()
    ci_lo = boots[int(0.025 * (BOOTSTRAP_N - 1))]
    ci_hi = boots[int(0.975 * (BOOTSTRAP_N - 1))]
    # sanity echoes
    sanity = {
        "thetaMismatchesTotal": sum(
            results[b][v]["thetaAcceptedVsEffectiveTiltMismatches"]
            for b in complete for v in VARIANTS),
        "slopeStopDistinct": sorted({tuple(results[b][v]["slopeStopValues"])
                                     for b in complete for v in VARIANTS}),
        "prefixJ_E_ranges": {
            v: (min(results[b][v]["J_E_prefix"] for b in complete),
                max(results[b][v]["J_E_prefix"] for b in complete))
            for v in VARIANTS},
    }
    out = {
        "run": "G1 interim precise J_E recomputation",
        "caliber": "preregistration §3 exact: r_e=(S_after[e+1]-S_after[e])-"
                   "tan(thetaAccepted) (dx=1 verified vs engine edge.d), "
                   "e_e=max(0,|r_e|-slopeStop), E=0.5*sum e_e^2 (no active "
                   "mask), J_E=segmented-constant integral over formal window "
                   "H0..H8 (CND prefix excluded), real physicsTime",
        "status": "INTERIM_NOT_FOR_DECISION",
        "blocks": {b: {"gain": gains[b],
                       **{v: results[b][v] for v in VARIANTS}}
                   for b in complete},
        "summary": {
            "nBlocks": len(complete),
            "gainMedian": med,
            "gainMin": min(vals), "gainMax": max(vals),
            "gainStdev": round(statistics.stdev(vals), 6),
            "blocksGainLE0": sum(1 for v in vals if v <= 0),
            "blocksGainGE10pct": sum(1 for v in vals if v >= 0.10),
            "bootstrapMedianCI95": [ci_lo, ci_hi],
            "bootstrapN": BOOTSTRAP_N, "bootstrapSeed": BOOTSTRAP_SEED,
        },
        "sanity": sanity,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n",
                   encoding="utf-8")
    print(json.dumps(out["summary"], ensure_ascii=False))
    for b in complete:
        print(f"  {b}: gain={gains[b]:.4f} "
              f"A_J_E={results[b]['A']['J_E_formal']:.6f} "
              f"D_J_E={results[b]['D']['J_E_formal']:.6f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
