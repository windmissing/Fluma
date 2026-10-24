#!/usr/bin/env python3
"""Synthetic self-test for test/p011_dual_field_gate_check.py.

Covers every verdict branch of the DUALFIELD-* RED-baseline gate:
 1. route-OFF legacy ledger            -> LIFE-04a PASS, others NOT_IMPLEMENTED,
                                          leakage CLEAR, overall RED_BASELINE
 2. route-OFF + stray particle fields  -> LEAKAGE, LIFE-04a FAIL, overall FAIL
 3. route-ON consistent ledger         -> all ledger-level cases PASS,
                                          GES BLOCKED (device dependency)
 4. route-ON conservation drift        -> CONS-01 FAIL
 5. route-ON double-counted removal    -> CONS-02 FAIL
 6. route-ON dropped LAND remainder    -> CONS-03 FAIL
 7. route-ON MERGE mass loss           -> BUD-03 FAIL
 8. route-ON bySourceKind mismatch     -> SRC-01 FAIL
 9. route-ON SPAWN without admission   -> SRC-02 FAIL
10. route-ON zero-charge CROSS_COLUMN  -> BUD-02 FAIL
11. route-ON missing bySourceKind      -> SRC-01 NOT_IMPLEMENTED (exact fields)
12. route-ON LIFT touching IH          -> SRC-01 FAIL (entries mixed)
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CHECKER = ROOT / "test" / "p011_dual_field_gate_check.py"
PY = sys.executable

COLS = 4


def base_state(bh=10.0, mh=0.0, suspH=0.0):
    return {
        "bh": [bh] * COLS, "bl": [8.0] * COLS,
        "mh": [mh] * COLS, "ml": [2.0] * COLS,
        "suspH": [suspH] * COLS, "suspL": [1.0] * COLS,
        "S": [12.0] * COLS, "waterTop": [20.0] * COLS,
    }


def base_record(substep, *, route=False, particles=None, events=None,
                source_injected_h=0.0, source_injected_l=0.0, epoch=0,
                suspH=0.0, audit_extra=None):
    st = base_state(suspH=suspH)
    rec = {
        "context": {
            "substepId": substep, "physicsTime": 1000 + substep * 10,
            "sourceInjectedH": source_injected_h,
            "sourceInjectedL": source_injected_l,
            "sourceEpoch": epoch,
        },
        "stateBefore": base_state(),
        "stateAfter": st,
        "budgets": {"localUsed": [0.01] * COLS, "totalUsed": 0.04},
        "audit": {
            "heavyTotalBefore": 40.0, "heavyTotalAfter": 40.0,
            "lightTotalBefore": 44.0, "lightTotalAfter": 44.0,
            "waterTotalBefore": 32.0, "waterTotalAfter": 32.0,
        },
    }
    if route:
        rec["context"]["particleRouteActive"] = True
        rec["context"]["particleRouteId"] = "p011-dual-field-particle-v1"
    if particles is not None:
        rec["particles"] = particles
    if events is not None:
        rec["particleEvents"] = events
    if audit_extra:
        rec["audit"].update(audit_extra)
    return rec


def pool_digest(count, massH, massL, *, kinds=True, cap=1, degraded=(0.0, 0.0)):
    d = {"count": count, "massH": massH, "massL": massL,
         "capacityUsed": cap, "capacityLimit": 512,
         "degraded": {"count": 0, "massH": degraded[0], "massL": degraded[1]}}
    if kinds:
        d["bySourceKind"] = {
            "GESTURE_EXTERNAL": {"count": count, "massH": massH,
                                 "massL": massL},
            "SHAKE_LIFT": {"count": 0, "massH": 0.0, "massL": 0.0},
        }
    return d


def land_event(pid, mass, granted, remainder, disp="REMAINDER_HELD"):
    return {"kind": "LAND", "particleId": pid, "phase": "H", "mass": mass,
            "column": 1, "requested": granted + remainder,
            "granted": granted, "remainder": remainder,
            "remainderDisposition": disp, "deltaS": granted,
            "guardPreview": "PASS",
            "budget": {"local": 0.005, "bottom": 0.0, "total": 0.005}}


def run_checker(records, name):
    tmp = Path(tempfile.mkdtemp(prefix="dualfield-selftest-"))
    led = tmp / f"{name}.ndjson"
    out = tmp / f"{name}.json"
    with open(led, "w", encoding="utf-8") as f:
        for r in records:
            f.write(json.dumps(r) + "\n")
    r = subprocess.run([PY, str(CHECKER), "--ledger", str(led),
                        "--output", str(out)], capture_output=True, text=True)
    if r.returncode not in (0, 1):
        raise RuntimeError(f"checker crashed on {name}: {r.stderr[-400:]}")
    return json.loads(out.read_text(encoding="utf-8"))


PASS = FAIL = 0
FAILURES = []


def expect(cond, msg):
    global PASS, FAIL
    if cond:
        PASS += 1
    else:
        FAIL += 1
        FAILURES.append(msg)
        print("  FAIL:", msg)


def case(name, records, checks):
    print(f"[{name}]")
    res = run_checker(records, name)
    for desc, fn in checks:
        expect(fn(res), f"{name}: {desc}")
    return res


# 1. route-OFF legacy ledger
legacy = [base_record(i, source_injected_h=0.0) for i in range(3)]
case("legacy_route_off", legacy, [
    ("overall RED_BASELINE", lambda r: r["overall"] == "RED_BASELINE"),
    ("leakage CLEAR", lambda r: r["leakage"]["status"] == "CLEAR"),
    ("LIFE-04a PASS", lambda r:
        r["cases"]["DUALFIELD-LIFE-04a"]["status"] == "PASS"),
    ("CONS-01 NOT_IMPLEMENTED", lambda r:
        r["cases"]["DUALFIELD-CONS-01"]["status"] == "NOT_IMPLEMENTED"),
    ("CONS-01 missing lists particles", lambda r:
        "particles.count" in r["cases"]["DUALFIELD-CONS-01"]["missingFields"]),
    ("LIFE-04b NOT_IMPLEMENTED", lambda r:
        r["cases"]["DUALFIELD-LIFE-04b"]["status"] == "NOT_IMPLEMENTED"),
])

# 2. route-OFF + stray particle fields (leakage guard)
stray_rec = base_record(0)
stray_rec["particles"] = {"count": 1, "massH": 1.0, "massL": 0.0}
case("legacy_leakage", [stray_rec], [
    ("leakage LEAKAGE", lambda r: r["leakage"]["status"] == "LEAKAGE"),
    ("LIFE-04a FAIL", lambda r:
        r["cases"]["DUALFIELD-LIFE-04a"]["status"] == "FAIL"),
    ("overall FAIL", lambda r: r["overall"] == "FAIL"),
])

# 3. route-ON consistent ledger (spawn -> land, all closures hold)
spawn_ev = {"kind": "SPAWN", "particleId": "p1", "phase": "H", "mass": 5.0,
            "sourceKind": "GESTURE_EXTERNAL", "x": 1.5, "y": 3.0,
            "sourceId": 7, "requested": 5.0, "admitted": 5.0}
r1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0), events=[spawn_ev])
r2 = base_record(2, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(0, 0.0, 0.0, cap=0),
                 events=[land_event("p1", 5.0, 5.0, 0.0)])
r2["stateAfter"] = base_state(bh=10.0, mh=1.25)
r2["audit"]["heavyTotalAfter"] = 45.0
r2["snapshot"] = {"particles": {"count": 0, "massH": 0.0, "massL": 0.0}}
case("route_on_consistent", [r1, r2], [
    ("overall not FAIL", lambda r: r["overall"] != "FAIL"),
    ("CONS-01 PASS", lambda r:
        r["cases"]["DUALFIELD-CONS-01"]["status"] == "PASS"),
    ("CONS-02 PASS", lambda r:
        r["cases"]["DUALFIELD-CONS-02"]["status"] == "PASS"),
    ("CONS-03 PASS", lambda r:
        r["cases"]["DUALFIELD-CONS-03"]["status"] == "PASS"),
    ("BUD-01 PASS", lambda r:
        r["cases"]["DUALFIELD-BUD-01"]["status"] == "PASS"),
    ("BUD-03 PASS", lambda r:
        r["cases"]["DUALFIELD-BUD-03"]["status"] == "PASS"),
    ("LIFE-01 PASS (contract)", lambda r:
        r["cases"]["DUALFIELD-LIFE-01"]["status"] == "PASS"),
    ("LIFE-03 PASS (snapshot)", lambda r:
        r["cases"]["DUALFIELD-LIFE-03"]["status"] == "PASS"),
    ("SRC-01 PASS", lambda r:
        r["cases"]["DUALFIELD-SRC-01"]["status"] == "PASS"),
    ("SRC-02 PASS", lambda r:
        r["cases"]["DUALFIELD-SRC-02"]["status"] == "PASS"),
    ("GES-01 BLOCKED", lambda r:
        r["cases"]["DUALFIELD-GES-01"]["status"] == "BLOCKED"),
    ("GES-02 BLOCKED", lambda r:
        r["cases"]["DUALFIELD-GES-02"]["status"] == "BLOCKED"),
])

# 4. conservation drift (pool digest inconsistent with state+IH)
d1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0), events=[spawn_ev])
d2 = base_record(2, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 3.0, 0.0),
                 events=[land_event("p1", 5.0, 5.0, 0.0)])
case("route_on_drift", [d1, d2], [
    ("CONS-01 FAIL", lambda r:
        r["cases"]["DUALFIELD-CONS-01"]["status"] == "FAIL"),
    ("drift violation recorded", lambda r:
        r["cases"]["DUALFIELD-CONS-01"]["violationCount"] >= 1),
])

# 5. double-counted removal (LAND of unknown particle)
dc1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                  particles=pool_digest(0, 0.0, 0.0, cap=0),
                  events=[land_event("ghost", 5.0, 5.0, 0.0)])
case("route_on_double_count", [dc1], [
    ("CONS-02 FAIL", lambda r:
        r["cases"]["DUALFIELD-CONS-02"]["status"] == "FAIL"),
])

# 6. dropped LAND remainder (requested 5, granted 2, remainder 0 -> mass lost)
dr1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 3.0, 0.0),
                 events=[{"kind": "SPAWN", "particleId": "p1", "phase": "H",
                          "mass": 5.0, "sourceKind": "GESTURE_EXTERNAL",
                          "x": 1.0, "y": 2.0, "sourceId": 1,
                          "requested": 5.0, "admitted": 5.0}])
dr2 = base_record(2, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 3.0, 0.0),
                 events=[{"kind": "LAND", "particleId": "p1", "phase": "H",
                          "mass": 5.0, "column": 1, "requested": 5.0,
                          "granted": 2.0, "remainder": 0.0,
                          "remainderDisposition": "REMAINDER_HELD",
                          "deltaS": 2.0, "guardPreview": "PASS",
                          "budget": {"local": 0.005, "bottom": 0.0,
                                     "total": 0.005}}])
case("route_on_remainder_dropped", [dr1, dr2], [
    ("CONS-03 FAIL", lambda r:
        r["cases"]["DUALFIELD-CONS-03"]["status"] == "FAIL"),
    ("closure violation recorded", lambda r:
        any("granted+remainder" in str(v.get("error", "")) for v in
            r["cases"]["DUALFIELD-CONS-03"]["violations"])),
])

# 7. MERGE mass loss
m1 = base_record(1, route=True, source_injected_h=6.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0),
                 events=[{"kind": "MERGE", "particleIds": ["p1", "p2"],
                          "newParticleId": "p3", "phase": "H",
                          "massBefore": 6.0, "massAfter": 5.0,
                          "positionMomentPreserved": False}])
case("route_on_merge_loss", [m1], [
    ("BUD-03 FAIL", lambda r:
        r["cases"]["DUALFIELD-BUD-03"]["status"] == "FAIL"),
])

# 8. bySourceKind mismatch (kind ledger does not close to the pool digest)
k1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0), events=[spawn_ev])
k1["particles"]["bySourceKind"]["GESTURE_EXTERNAL"]["massH"] = 6.0
case("route_on_kind_mismatch", [k1], [
    ("SRC-01 FAIL", lambda r:
        r["cases"]["DUALFIELD-SRC-01"]["status"] == "FAIL"),
])

# 9. SPAWN without admission (rejected injection must not create particles).
#    s1 carries the admitted 5 units in suspH (v4 path, no particles yet);
#    s2 spawns a particle while IH does not move -> rejected-class defect.
s1 = base_record(1, route=True, source_injected_h=5.0, epoch=1, suspH=1.25,
                 particles=pool_digest(0, 0.0, 0.0, cap=0), events=[])
s2 = base_record(2, route=True, source_injected_h=5.0, epoch=1, suspH=0.0,
                 particles=pool_digest(1, 5.0, 0.0), events=[spawn_ev])
case("route_on_spawn_no_admission", [s1, s2], [
    ("SRC-02 FAIL", lambda r:
        r["cases"]["DUALFIELD-SRC-02"]["status"] == "FAIL"),
    ("SRC-03 FAIL", lambda r:
        r["cases"]["DUALFIELD-SRC-03"]["status"] == "FAIL"),
    ("CONS-01 still PASS (identity holds)", lambda r:
        r["cases"]["DUALFIELD-CONS-01"]["status"] == "PASS"),
])

# 10. zero-charge CROSS_COLUMN (free transport channel)
c1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0), events=[spawn_ev])
c2 = base_record(2, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0),
                 events=[{"kind": "CROSS_COLUMN", "particleId": "p1",
                          "phase": "H", "mass": 5.0, "fromColumn": 1,
                          "toColumn": 2,
                          "budget": {"local": 0.0, "bottom": 0.0,
                                     "total": 0.0}}])
case("route_on_free_crossing", [c1, c2], [
    ("BUD-02 FAIL", lambda r:
        r["cases"]["DUALFIELD-BUD-02"]["status"] == "FAIL"),
])

# 11. missing bySourceKind -> NOT_IMPLEMENTED with exact field paths
n1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0, kinds=False),
                 events=[spawn_ev])
case("route_on_missing_kinds", [n1], [
    ("SRC-01 NOT_IMPLEMENTED", lambda r:
        r["cases"]["DUALFIELD-SRC-01"]["status"] == "NOT_IMPLEMENTED"),
    ("exact missing field listed", lambda r:
        "particles.bySourceKind.GESTURE_EXTERNAL.massH" in
        r["cases"]["DUALFIELD-SRC-01"]["missingFields"]),
])

# 12. SHAKE_LIFT touching IH (entries mixed)
l1 = base_record(1, route=True, source_injected_h=5.0, epoch=1,
                 particles=pool_digest(1, 5.0, 0.0), events=[spawn_ev])
l2 = base_record(2, route=True, source_injected_h=9.0, epoch=1,
                 particles=pool_digest(2, 9.0, 0.0),
                 events=[{"kind": "LIFT", "particleId": "p2", "phase": "H",
                          "mass": 4.0, "sourceKind": "SHAKE_LIFT",
                          "fromLayer": "mh", "column": 1, "requested": 4.0,
                          "granted": 4.0,
                          "budget": {"local": 0.004, "bottom": 0.0,
                                     "total": 0.004}}])
case("route_on_lift_touches_ih", [l1, l2], [
    ("SRC-01 FAIL", lambda r:
        r["cases"]["DUALFIELD-SRC-01"]["status"] == "FAIL"),
    ("entries-mixed violation", lambda r:
        any("liftEventsChangingIH" in v for v in
            r["cases"]["DUALFIELD-SRC-01"]["violations"])),
])

print(f"\nPASS={PASS} FAIL={FAIL}")
if FAIL:
    print("FAILURES:")
    for f_ in FAILURES:
        print(" -", f_)
    sys.exit(1)
print("ALL SYNTHETIC BRANCHES GREEN")
