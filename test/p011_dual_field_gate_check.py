#!/usr/bin/env python3
"""P-011 route C (dual-field particle) DUALFIELD-* gate checker (RED baseline).

Pre-registration: docs/preregistration-P011-dual-field-particle-v1.md
(protocolId=p011-dual-field-particle-v1, revision=preregistration-v1).

Ledger-level gate for the route-C particle layer (active particles + 128-col
height field). Today route C is NOT_IMPLEMENTED, so every case that needs the
schema-v7 `particles` block reports NOT_IMPLEMENTED with the exact missing
field paths; the route-OFF leakage guard and the legacy equivalence guard
(DUALFIELD-LIFE-04a) are executable NOW. When the particle fields land, the
same checker runs the conservation / atomicity / budget / source-accounting
assertions without any method change.

Usage:
  python test/p011_dual_field_gate_check.py --ledger <ledger.ndjson> \
      --output <out.json> [--max-records N]

Streaming: safe for 2GB+ ledgers (per-record aggregates only; the particle
pool tracker is bounded by the pool capacity).
"""
import argparse
import json
import math
import sys
from pathlib import Path

PROTOCOL_ID = "p011-dual-field-particle-v1"
PROTOCOL_REVISION = "preregistration-v1"

DRIFT_TOL = 1e-9          # conservation drift tolerance (gesture-units channel)
MASS_TOL = 1e-9           # per-event mass closure tolerance
WATER_REL_TOL = 1e-3      # free-water relative tolerance (algorithm 5: <0.1%)
TOTAL_MOTION_BUDGET = 300.0  # units/s shared motion budget (frozen constant)
LOCAL_BUDGET_PER_DT = 28.0   # vSandTopMax*dt per column (frozen constant)
BOTTOM_BUDGET_PER_DT = 10.0  # bottom channel per dt (frozen constant)

# Case registry: mirrors preregistration section 8. `requires` lists the
# ledger field paths each case consumes (dot paths from the record root).
CASES = [
    ("DUALFIELD-CONS-01", "MUST-PASS",
     ["particles.count", "particles.massH", "particles.massL",
      "stateAfter.bh", "stateAfter.mh", "stateAfter.suspH",
      "stateAfter.bl", "stateAfter.ml", "stateAfter.suspL",
      "audit.heavyTotalBefore"]),
    ("DUALFIELD-CONS-02", "MUST-PASS",
     ["particleEvents", "particles.massH", "particles.massL"]),
    ("DUALFIELD-CONS-03", "MUST-PASS",
     ["particleEvents"]),
    ("DUALFIELD-CONS-04", "MUST-PASS",
     ["particles.count", "context.sourceEpoch",
      "context.depositedHeavyFraction", "context.depositedLightFraction",
      "context.t90Heavy"]),
    ("DUALFIELD-BUD-01", "MUST-PASS",
     ["particleEvents", "budgets.localUsed", "budgets.totalUsed"]),
    ("DUALFIELD-BUD-02", "MUST-PASS",
     ["particleEvents"]),
    ("DUALFIELD-BUD-03", "MUST-PASS",
     ["particleEvents", "particles.capacityUsed", "particles.capacityLimit",
      "particles.degraded.massH", "particles.degraded.massL"]),
    ("DUALFIELD-LIFE-01", "MUST-PASS",
     ["particles.count", "particleEvents"]),
    ("DUALFIELD-LIFE-02", "MUST-PASS",
     ["particles.count", "context.sourceEpoch"]),
    ("DUALFIELD-LIFE-03", "MUST-PASS",
     ["particles.count", "particles.massH", "particles.massL"]),
    ("DUALFIELD-LIFE-04b", "MUST-PASS",
     ["particleEvents"]),
    ("DUALFIELD-SRC-01", "MUST-PASS",
     ["particles.bySourceKind.GESTURE_EXTERNAL.massH",
      "particles.bySourceKind.GESTURE_EXTERNAL.massL",
      "particles.bySourceKind.SHAKE_LIFT.massH",
      "particles.bySourceKind.SHAKE_LIFT.massL",
      "particles.massH", "particles.massL"]),
    ("DUALFIELD-SRC-02", "MUST-PASS",
     ["particleEvents", "context.sourceInjectedH", "context.sourceInjectedL"]),
    ("DUALFIELD-SRC-03", "MUST-PASS",
     ["particleEvents"]),
    ("DUALFIELD-GES-01", "TIERED",
     ["particleEvents", "context.physicsTime"]),
    ("DUALFIELD-GES-02", "MUST-PASS",
     ["particleEvents"]),
]

# Particle-ish field paths whose presence on a route-OFF ledger is leakage.
LEAKAGE_PATHS = [
    "particles", "particleEvents", "particleRouteActive", "particleRouteId",
]

STATUS_NOT_IMPL = "NOT_IMPLEMENTED"
STATUS_BLOCKED = "BLOCKED"
STATUS_PASS = "PASS"
STATUS_FAIL = "FAIL"
STATUS_NA = "N/A"


def dig(obj, path):
    """Dot-path getter; returns (found, value)."""
    cur = obj
    for part in path.split("."):
        if not isinstance(cur, dict) or part not in cur:
            return False, None
        cur = cur[part]
    return True, cur


def route_active(record):
    ctx = record.get("context", {})
    found, val = dig(ctx, "particleRouteActive")
    if not found:
        return False
    return bool(val)


def find_stray_particle_fields(record):
    """Any particle-ish key anywhere in the record while the route is OFF."""
    stray = []

    def walk(obj, prefix):
        if isinstance(obj, dict):
            for k, v in obj.items():
                p = f"{prefix}.{k}" if prefix else k
                if k in ("particles", "particleEvents") or "particle" in k.lower():
                    stray.append(p)
                walk(v, p)
        elif isinstance(obj, list) and obj:
            walk(obj[0], f"{prefix}[0]")

    walk(record, "")
    return sorted(set(stray))


def check_leakage(records_iter):
    """Route-OFF ledger must carry no particle fields at all."""
    stray = []
    n = 0
    for rec in records_iter:
        n += 1
        if route_active(rec):
            continue
        stray.extend((n, p) for p in find_stray_particle_fields(rec))
        if len(stray) > 20:
            break
    return stray


def evaluate_route_on(ledger_path, max_records):
    """Run the route-ON assertions. Returns per-case results + aggregates."""
    results = {cid: {"status": STATUS_PASS, "violations": [], "checked": 0}
               for cid, _, _ in CASES}
    missing = {cid: [] for cid, _, _ in CASES}

    pool = {}            # particleId -> mass (tracker for atomicity)
    vh0 = vl0 = None
    prev_injected_h = prev_injected_l = None
    prev_epoch = None
    degraded_mass_h = degraded_mass_l = 0.0
    cross_charge_zero = 0
    cross_total = 0.0
    spawn_without_admission = 0
    lift_changes_ih = 0
    water_rel_max = 0.0
    n = 0

    with open(ledger_path, "r", encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                results["DUALFIELD-CONS-01"]["violations"].append(
                    {"record": n + 1, "error": "unparsable ledger line"})
                continue
            n += 1
            ctx = rec.get("context", {})
            state = rec.get("stateAfter", {})
            audit = rec.get("audit", {})
            events = rec.get("particleEvents") or []

            # ---- record presence of required fields (missing-field audit) --
            for cid, _, requires in CASES:
                for path in requires:
                    found, _ = dig(rec, path)
                    if not found and path not in missing[cid]:
                        missing[cid].append(path)

            # ---- run initial baselines ------------------------------------
            if vh0 is None:
                fnd, v = dig(audit, "heavyTotalBefore")
                vh0 = float(v) if fnd else 0.0
                fnd, v = dig(audit, "lightTotalBefore")
                vl0 = float(v) if fnd else 0.0

            # ---- CONS-01: conservation identity ---------------------------
            has_state = all(k in state for k in
                            ("bh", "bl", "mh", "ml", "suspH", "suspL"))
            has_pool = ("particles" in rec and "massH" in rec["particles"]
                        and "massL" in rec["particles"])
            if has_state and has_pool and not missing["DUALFIELD-CONS-01"]:
                results["DUALFIELD-CONS-01"]["checked"] += 1
                vh_actual = (sum(state["bh"]) + sum(state["mh"]) +
                             sum(state["suspH"]) +
                             float(rec["particles"]["massH"]))
                vl_actual = (sum(state["bl"]) + sum(state["ml"]) +
                             sum(state["suspL"]) +
                             float(rec["particles"]["massL"]))
                fnd, inj_h = dig(ctx, "sourceInjectedH")
                inj_h = float(inj_h) if fnd else 0.0
                fnd, inj_l = dig(ctx, "sourceInjectedL")
                inj_l = float(inj_l) if fnd else 0.0
                drift_h = vh_actual - (vh0 + inj_h)
                drift_l = vl_actual - (vl0 + inj_l)
                if abs(drift_h) > DRIFT_TOL or abs(drift_l) > DRIFT_TOL:
                    results["DUALFIELD-CONS-01"]["violations"].append({
                        "record": n, "driftH": drift_h, "driftL": drift_l})
                # free water (eta construction keeps it constant)
                fnd, wb = dig(audit, "waterTotalBefore")
                fnd2, wa = dig(audit, "waterTotalAfter")
                if fnd and fnd2 and abs(float(wb)) > 1e-12:
                    rel = abs(float(wa) - float(wb)) / abs(float(wb))
                    water_rel_max = max(water_rel_max, rel)
                    if rel > WATER_REL_TOL:
                        results["DUALFIELD-CONS-01"]["violations"].append({
                            "record": n, "waterRel": rel})

            # ---- event-ledger driven cases --------------------------------
            for ev in events:
                kind = ev.get("kind")
                pid = ev.get("particleId")
                mass = float(ev.get("mass", 0.0) or 0.0)
                phase = ev.get("phase")
                src = ev.get("sourceKind")

                # CONS-02: pool tracker atomicity (no double count / no
                # vanishing mass). Every removal must hit a live particle.
                if kind == "SPAWN":
                    if pid in pool:
                        results["DUALFIELD-CONS-02"]["violations"].append({
                            "record": n, "error": "SPAWN id reuse",
                            "particleId": pid})
                    pool[pid] = mass
                    results["DUALFIELD-CONS-02"]["checked"] += 1
                    # SRC-02/03: spawn only against real admission
                    if src == "GESTURE_EXTERNAL":
                        results["DUALFIELD-SRC-02"]["checked"] += 1
                        results["DUALFIELD-SRC-03"]["checked"] += 1
                        cur_h = ctx.get("sourceInjectedH")
                        if cur_h is not None and prev_injected_h is not None:
                            if float(cur_h) - float(prev_injected_h) <= 0:
                                spawn_without_admission += 1
                if kind in ("LAND", "TO_SUSP", "DEGRADE"):
                    if pid not in pool:
                        results["DUALFIELD-CONS-02"]["violations"].append({
                            "record": n, "error": f"{kind} of unknown/"
                            "already-removed particle (double count)",
                            "particleId": pid})
                    else:
                        del pool[pid]
                    results["DUALFIELD-CONS-02"]["checked"] += 1
                if kind == "LAND":
                    results["DUALFIELD-BUD-01"]["checked"] += 1
                    b = ev.get("budget") or {}
                    if not b:
                        results["DUALFIELD-BUD-01"]["violations"].append({
                            "record": n, "error":
                            "LAND without four-layer budget record"})
                    if ev.get("guardPreview") is None:
                        results["DUALFIELD-BUD-01"]["violations"].append({
                            "record": n, "error":
                            "LAND without 19.5 guard preview record"})
                    # CONS-03: remainder never dropped
                    req = float(ev.get("requested", 0.0) or 0.0)
                    gr = float(ev.get("granted", 0.0) or 0.0)
                    rem = float(ev.get("remainder", 0.0) or 0.0)
                    results["DUALFIELD-CONS-03"]["checked"] += 1
                    if abs((gr + rem) - req) > MASS_TOL:
                        results["DUALFIELD-CONS-03"]["violations"].append({
                            "record": n, "requested": req, "granted": gr,
                            "remainder": rem,
                            "error": "granted+remainder != requested"})
                    if gr < req - MASS_TOL:
                        disp = ev.get("remainderDisposition")
                        if disp not in ("REMAINDER_HELD", "TRANSFERRED_SUSP"):
                            results["DUALFIELD-CONS-03"]["violations"].append({
                                "record": n, "disposition": disp,
                                "error": "partial grant without declared "
                                "remainder disposition"})
                if kind == "CROSS_COLUMN":
                    cross_total += 1
                    results["DUALFIELD-BUD-01"]["checked"] += 1
                    b = ev.get("budget") or {}
                    charged = (float(b.get("local", 0.0) or 0.0) +
                               float(b.get("bottom", 0.0) or 0.0) +
                               float(b.get("total", 0.0) or 0.0))
                    if charged <= 0:
                        cross_charge_zero += 1
                    if not b:
                        results["DUALFIELD-BUD-01"]["violations"].append({
                            "record": n, "error":
                            "CROSS_COLUMN without budget charge record"})
                if kind == "LIFT":
                    results["DUALFIELD-BUD-01"]["checked"] += 1
                    b = ev.get("budget") or {}
                    if not b:
                        results["DUALFIELD-BUD-01"]["violations"].append({
                            "record": n, "error":
                            "LIFT without four-layer budget record"})
                    if src == "SHAKE_LIFT":
                        cur_h = ctx.get("sourceInjectedH")
                        if cur_h is not None and prev_injected_h is not None:
                            if abs(float(cur_h) - float(prev_injected_h)) > 0:
                                lift_changes_ih += 1
                if kind == "MERGE":
                    mb = float(ev.get("massBefore", 0.0) or 0.0)
                    ma = float(ev.get("massAfter", 0.0) or 0.0)
                    results["DUALFIELD-BUD-03"]["checked"] += 1
                    if abs(ma - mb) > MASS_TOL or not ev.get(
                            "positionMomentPreserved", False):
                        results["DUALFIELD-BUD-03"]["violations"].append({
                            "record": n, "massBefore": mb, "massAfter": ma,
                            "moment": ev.get("positionMomentPreserved")})
                    for old in ev.get("particleIds", []):
                        if old not in pool:
                            results["DUALFIELD-CONS-02"]["violations"].append({
                                "record": n, "error": "MERGE of unknown id",
                                "particleId": old})
                        else:
                            del pool[old]
                    new_id = ev.get("newParticleId", pid)
                    if new_id is not None:
                        pool[new_id] = ma
                if kind == "DEGRADE":
                    if ev.get("flag") != "PARTICLE_DEGRADED_TO_COLUMN":
                        results["DUALFIELD-BUD-03"]["violations"].append({
                            "record": n, "flag": ev.get("flag"),
                            "error": "degrade without "
                            "PARTICLE_DEGRADED_TO_COLUMN flag"})
                    results["DUALFIELD-BUD-03"]["checked"] += 1
                    degraded_mass_h += mass if phase == "H" else 0.0
                    degraded_mass_l += mass if phase == "L" else 0.0

            # BUD-03: capacity + degraded cumulative closure
            if "particles" in rec and "capacityUsed" in rec["particles"]:
                cap = float(rec["particles"].get("capacityUsed", 0.0))
                lim_found, lim = dig(rec, "particles.capacityLimit")
                if lim_found and cap > float(lim) + 1e-9:
                    results["DUALFIELD-BUD-03"]["violations"].append({
                        "record": n, "capacityUsed": cap, "capacityLimit": lim})
                results["DUALFIELD-BUD-03"]["checked"] += 1
                dh_found, dh = dig(rec, "particles.degraded.massH")
                dl_found, dl = dig(rec, "particles.degraded.massL")
                if dh_found and dl_found:
                    if (abs(float(dh) - degraded_mass_h) > MASS_TOL or
                            abs(float(dl) - degraded_mass_l) > MASS_TOL):
                        results["DUALFIELD-BUD-03"]["violations"].append({
                            "record": n,
                            "digestDegradedH": float(dh),
                            "eventDegradedH": degraded_mass_h,
                            "digestDegradedL": float(dl),
                            "eventDegradedL": degraded_mass_l,
                            "error": "degraded digest != event ledger sum"})

            # SRC-01: bySourceKind closure against pool digest
            if "particles" in rec and "bySourceKind" in rec["particles"]:
                bk = rec["particles"]["bySourceKind"]
                ok = True
                for entry in ("GESTURE_EXTERNAL", "SHAKE_LIFT"):
                    if entry not in bk or "massH" not in bk[entry] \
                            or "massL" not in bk[entry]:
                        ok = False
                if ok and "massH" in rec["particles"]:
                    results["DUALFIELD-SRC-01"]["checked"] += 1
                    sh = (float(bk["GESTURE_EXTERNAL"]["massH"]) +
                          float(bk["SHAKE_LIFT"]["massH"]))
                    sl = (float(bk["GESTURE_EXTERNAL"]["massL"]) +
                          float(bk["SHAKE_LIFT"]["massL"]))
                    if abs(sh - float(rec["particles"]["massH"])) > MASS_TOL \
                            or abs(sl - float(rec["particles"]["massL"])) > MASS_TOL:
                        results["DUALFIELD-SRC-01"]["violations"].append({
                            "record": n, "kindSumH": sh, "poolH":
                            float(rec["particles"]["massH"]),
                            "kindSumL": sl, "poolL":
                            float(rec["particles"]["massL"]),
                            "error": "bySourceKind does not close to pool"})

            # LIFE-03: snapshot block (when present) must carry a particle
            # digest consistent with the contemporaneous record.
            snap = rec.get("snapshot")
            if isinstance(snap, dict) and "particles" in snap:
                sp = snap["particles"]
                if "particles" in rec and "massH" in rec["particles"]:
                    results["DUALFIELD-LIFE-03"]["checked"] += 1
                    if (abs(float(sp.get("massH", 0)) -
                            float(rec["particles"]["massH"])) > MASS_TOL or
                            abs(float(sp.get("massL", 0)) -
                                float(rec["particles"]["massL"])) > MASS_TOL):
                        results["DUALFIELD-LIFE-03"]["violations"].append({
                            "record": n, "error":
                            "snapshot particle digest != live digest"})

            # CONS-04 (conditional): depositedFraction denominator must be the
            # cumulative expectation, and t90 must not survive an epoch bump.
            dep_h_found, dep_h = dig(ctx, "depositedHeavyFraction")
            if dep_h_found:
                exp_h_found, exp_h = dig(ctx, "expectedHeavyTotal")
                if exp_h_found:
                    results["DUALFIELD-CONS-04"]["checked"] += 1
                    if abs(float(exp_h) - (vh0 + float(
                            ctx.get("sourceInjectedH", 0.0) or 0.0))) > DRIFT_TOL:
                        results["DUALFIELD-CONS-04"]["violations"].append({
                            "record": n, "expectedHeavyTotal": float(exp_h),
                            "VH0+IH": vh0 + float(
                                ctx.get("sourceInjectedH", 0.0) or 0.0),
                            "error": "depositedFraction denominator is not "
                            "VH0+IH"})
            t90_found, t90 = dig(ctx, "t90Heavy")
            epoch = ctx.get("sourceEpoch")
            if t90_found and t90 is not None and prev_epoch is not None \
                    and epoch is not None and int(epoch) > int(prev_epoch):
                results["DUALFIELD-CONS-04"]["violations"].append({
                    "record": n, "t90Heavy": t90,
                    "error": "t90 survived a source-epoch bump "
                    "(must be invalid or epoch-restamped)"})
                results["DUALFIELD-CONS-04"]["checked"] += 1

            prev_injected_h = ctx.get("sourceInjectedH", prev_injected_h)
            prev_injected_l = ctx.get("sourceInjectedL", prev_injected_l)
            prev_epoch = ctx.get("sourceEpoch", prev_epoch)

            if max_records and n >= max_records:
                break

    # Rollups after the stream ends.
    if spawn_without_admission:
        for cid in ("DUALFIELD-SRC-02", "DUALFIELD-SRC-03"):
            results[cid]["violations"].append({
                "spawnsWithoutAdmission": spawn_without_admission,
                "error": "GESTURE_EXTERNAL spawn in a substep with no "
                "new admission (rejected/zero admission must yield "
                "zero particles)"})
    if lift_changes_ih:
        results["DUALFIELD-SRC-01"]["violations"].append({
            "liftEventsChangingIH": lift_changes_ih,
            "error": "SHAKE_LIFT touched external-source ledger "
            "(entries mixed)"})
    # BUD-02 rollup: zero-charge crossings = free transport channel
    if cross_charge_zero:
        results["DUALFIELD-BUD-02"]["violations"].append({
            "zeroChargeCrossings": cross_charge_zero,
            "error": "cross-column transport without budget charge "
            "(free channel)"})
    results["DUALFIELD-BUD-02"]["checked"] = cross_total

    # pool tracker must end empty only if all particles landed; otherwise the
    # live pool must equal the digest on the last record (consistency signal)
    return results, missing, {
        "records": n, "poolTrackerSize": len(pool),
        "waterRelMax": water_rel_max, "crossings": cross_total}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ledger", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--max-records", type=int, default=0)
    args = ap.parse_args()

    ledger = Path(args.ledger)
    out = {
        "protocolId": PROTOCOL_ID,
        "protocolRevision": PROTOCOL_REVISION,
        "ledger": str(ledger),
        "cases": {},
    }

    # Pass 1: route identity + leakage guard (streaming, bounded).
    route_on = False
    stray = []
    first_records = []
    with open(ledger, "r", encoding="utf-8") as f:
        for i, line in enumerate(f):
            if not line.strip():
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue
            if len(first_records) < 3:
                first_records.append(rec)
            if route_active(rec):
                route_on = True
                break
            s = find_stray_particle_fields(rec)
            if s:
                stray.append({"record": i + 1, "fields": s})
                if len(stray) >= 5:
                    break

    out["routeActive"] = route_on
    out["leakage"] = {
        "status": "LEAKAGE" if stray else "CLEAR",
        "strayFields": stray,
    }

    if not route_on:
        # Route OFF: LIFE-04a executable (guard must hold); everything else
        # NOT_IMPLEMENTED with exact missing fields.
        out["overall"] = "FAIL" if stray else "RED_BASELINE"
        for cid, gate, requires in CASES:
            if cid == "DUALFIELD-LIFE-04b":
                # paired equivalence needs a C run to pair against
                out["cases"][cid] = {
                    "status": STATUS_NOT_IMPL, "gate": gate,
                    "missingFields": ["particleRouteActive (route C run)"],
                    "detail": "route C not implemented; no C run to pair "
                    "against the legacy baseline"}
                continue
            miss = ["context.particleRouteActive"]
            for p in requires:
                if p not in miss:
                    miss.append(p)
            out["cases"][cid] = {
                "status": STATUS_NOT_IMPL, "gate": gate,
                "missingFields": miss,
                "detail": "route C (dual-field particle layer) not "
                "implemented in this ledger's build"}
        out["cases"]["DUALFIELD-LIFE-04a"] = {
            "status": STATUS_PASS if not stray else STATUS_FAIL,
            "gate": "MUST-PASS",
            "missingFields": [],
            "detail": "route-OFF guard: no particle fields on a "
            "route-closed ledger (legacy path bit-equivalence precondition)",
        }
        out["summary"] = {
            "note": "RED baseline: route C not implemented. Executable "
            "today: leakage guard + LIFE-04a. All particle cases "
            "NOT_IMPLEMENTED with exact missing field paths.",
        }
    else:
        results, missing, agg = evaluate_route_on(
            str(ledger), args.max_records)
        # Behavioral cases whose assertions live in the Node 1:1 channel
        # (reset/invert/mode-switch are operations, not ledger observations);
        # the ledger gate verifies the telemetry contract is present.
        BEHAVIORAL = {"DUALFIELD-LIFE-01", "DUALFIELD-LIFE-02"}
        overall = "RED_BASELINE"
        out_vacuous = False
        for cid, gate, _ in CASES:
            if missing[cid]:
                st = STATUS_NOT_IMPL
            elif results[cid]["violations"]:
                st = STATUS_FAIL
                overall = "FAIL"
            elif cid in BEHAVIORAL:
                st = STATUS_PASS
            elif cid.startswith("DUALFIELD-GES"):
                st = STATUS_BLOCKED
            elif results[cid]["checked"] == 0:
                # Contract present, no violating observation, but also no
                # event of this kind in the ledger: vacuously satisfied.
                st = STATUS_PASS
                out_vacuous = True
            else:
                st = STATUS_PASS
                out_vacuous = False
            out["cases"][cid] = {
                "status": st, "gate": gate,
                "missingFields": missing[cid],
                "checked": results[cid]["checked"],
                "violations": results[cid]["violations"][:20],
                "violationCount": len(results[cid]["violations"]),
            }
            if cid in BEHAVIORAL and st == STATUS_PASS:
                out["cases"][cid]["detail"] = (
                    "ledger gate: telemetry contract present; behavioral "
                    "assertions (reset/invert/mode-switch) run in the Node "
                    "1:1 channel per preregistration section 12")
            if out_vacuous:
                out["cases"][cid]["detail"] = (
                    "vacuously satisfied: contract fields present and no "
                    "violation, but no event of this kind occurred in this "
                    "ledger")
        out["overall"] = overall
        out["aggregates"] = agg
        out["summary"] = {
            "note": "route C active in this ledger; assertions executed "
            "per preregistration v1. GES cases need device capture "
            "(BLOCKED until s134 fix build 6adb3508/6adb7038 or with a "
            "data-pollution declaration).",
        }

    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    Path(args.output).write_text(
        json.dumps(out, indent=1, ensure_ascii=False), encoding="utf-8")

    n_ni = sum(1 for c in out["cases"].values()
               if c["status"] == STATUS_NOT_IMPL)
    n_pass = sum(1 for c in out["cases"].values()
                 if c["status"] == STATUS_PASS)
    n_fail = sum(1 for c in out["cases"].values()
                 if c["status"] == STATUS_FAIL)
    print(f"overall={out['overall']} routeActive={route_on} "
          f"leakage={out['leakage']['status']} "
          f"PASS={n_pass} FAIL={n_fail} NOT_IMPLEMENTED={n_ni}")
    return 0 if out["overall"] in ("RED_BASELINE", "PASS") else 1


if __name__ == "__main__":
    sys.exit(main())
