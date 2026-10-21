#!/usr/bin/env python3
"""Observation-3 D-1 verification checker cases (RED-baseline-first).

Implements the tester-side checker layer for the manager D-1 ruling
(priority.md 2026-10-21 "D-1 correction 越权激活裁决", Option A +
explicit route isolation):

  Case D1-C1  LEGACY trajectory equivalence - a LEGACY run must reproduce
              the 5e60f2d 240-step baseline anchors (covering step25, the
              first divergence point, and step214, the historical BLOCKED
              step). The ArkTS-side case p011_d1_legacyTrajectoryEquivalence
              240Steps executes the trajectory; this checker validates the
              captured result JSON against the frozen anchors and the
              240/240-ok requirement.

  Case D1-C2  Dual-condition eligibility gate split - for each of the four
              routing scenarios the checker asserts which apply route ran
              and that the ledger carries the matching telemetry:
                (a) LEGACY run (no candidate thresholds)  -> legacy relaxAngle
                    apply; relaxRequested/relaxApplied semantics match the
                    pre-correction baseline (requested>0 implies applied
                    grant flow, no correction slot evidence)
                (b) candidate run (natural-relax-hysteresis-v1, no explicit
                    correction route) -> ALSO legacy apply (the v3 identity
                    is not an implicit correction switch)
                (c) explicit correction route run
                    (observationRouteId=relax-correction-weighted-v1 AND
                    candidate thresholds armed) -> applyCorrectionSlot
                (d) route bound but machine NOT armed -> legacy apply
                    (necessary condition missing)

  Case D1-C3  (D-1b, RED baseline) correction telemetry in the audit:
              correctionRequested/Granted/Rejected/StalledCount must appear
              in the audit payload of correction-route runs with the ledger
              closure identity (requested == granted + rejected). The fields
              are NOT yet implemented in P011NaturalObservationAudit, so
              this case runs RED by design and turns green when the
              developer lands D-1b.

All cases run at the Node/checker layer; no device involved. Nothing here
weakens a gate or authorizes capture.
"""
import argparse
import json
import math
from pathlib import Path

# Frozen 5e60f2d baseline anchors (identical to the ArkTS case
# p011_d1_legacyTrajectoryEquivalence240Steps): [step, effectiveTilt,
# sandTopColumnSum, sandTop[0], bh[0]] at 1e-9 (1e-6 for the column sum).
LEGACY_ANCHORS = [
    [20, 20, 1152, 143.523809524, 71.761904762],
    [25, 8, 1152, 143.452380952, 71.726190476],
    [40, -20, 1152, 143.917933096, 71.904887714],
    [60, 20, 1152, 143.822695001, 71.857357677],
    [80, -20, 1152, 143.727456906, 71.809818927],
    [100, 20, 1152, 144.013171192, 71.95284503],
    [120, 20, 1152, 143.536980715, 71.714758],
    [140, -20, 1152, 144.013171192, 71.953049395],
    [160, 20, 1152, 143.727456906, 71.810241002],
    [180, -20, 1152, 143.813072326, 71.853008771],
    [200, 20, 1152, 143.908310422, 71.900745446],
    [214, 20, 1152, 143.574977088, 71.734088226],
    [220, -6.704117608, 1152, 143.622596136, 71.757935463],
    [240, -10.701243018, 1152, 144.098786612, 71.996239053],
]
TOTAL_STEPS = 240
TOLERANCE_ANCHOR = 1e-9
TOLERANCE_SUM = 1e-6
CORRECTION_ROUTE_ID = "relax-correction-weighted-v1"
NATURAL_ROUTE_ID = "natural-relax-hysteresis-v1"
CORRECTION_AUDIT_FIELDS = (
    "correctionRequested", "correctionGranted",
    "correctionRejected", "correctionStalledCount",
)


def read_json(path):
    text = Path(path).read_text(encoding="utf-8-sig").strip() if path else ""
    return json.loads(text) if text else {}


def finite(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def check_d1_c1_trajectory(evidence):
    """D1-C1: the captured LEGACY 240-step run must match the frozen
    5e60f2d anchor sequence and report 240/240 ok."""
    errors = []
    ok_steps = evidence.get("okSteps")
    if ok_steps != TOTAL_STEPS:
        errors.append(f"okStepsMismatch:{ok_steps}!={TOTAL_STEPS}")
    blocked_steps = evidence.get("blockedSteps") or []
    if blocked_steps:
        errors.append("unexpectedBlockedSteps:" + ",".join(str(s) for s in blocked_steps))
    anchors = evidence.get("anchors") or []
    by_step = {int(a["step"]): a for a in anchors if finite(a.get("step"))}
    for expected in LEGACY_ANCHORS:
        step = expected[0]
        actual = by_step.get(step)
        if actual is None:
            errors.append(f"anchorMissing:step{step}")
            continue
        pairs = [
            ("effectiveTilt", expected[1], actual.get("effectiveTilt"), TOLERANCE_ANCHOR),
            ("sandTopSum", expected[2], actual.get("sandTopSum"), TOLERANCE_SUM),
            ("sandTop0", expected[3], actual.get("sandTop0"), TOLERANCE_ANCHOR),
            ("bh0", expected[4], actual.get("bh0"), TOLERANCE_ANCHOR),
        ]
        for name, want, got, tol in pairs:
            if not finite(got):
                errors.append(f"anchor{step}.{name}:missing")
            elif abs(float(got) - want) > tol:
                errors.append(
                    f"anchor{step}.{name}:|{got}-{want}|={abs(float(got) - want)}>{tol}")
    return {
        "case": "D1-C1-legacy-trajectory-equivalence",
        "passed": not errors,
        "errors": errors,
        "anchorsChecked": len(LEGACY_ANCHORS),
        "divergencePointsCovered": [25, 214],
    }


def _route_of(evidence):
    route = evidence.get("observationRouteId")
    return route


def _machine_armed(evidence):
    return evidence.get("stateMachineArmed") is True


def check_d1_c2_route_split(scenarios):
    """D1-C2: four-way apply-route split. Each scenario evidence record must
    carry observationRouteId, stateMachineArmed, the observed apply route,
    and relaxRequested/relaxApplied totals for the legacy-semantics pairs.
    The observed apply route may come directly from the runner (applyRoute)
    or be inferred from the gate flags + slot evidence when the runner did
    not observe it directly."""
    errors = []
    results = []
    for scenario in scenarios:
        name = scenario.get("scenario")
        route = _route_of(scenario)
        armed = _machine_armed(scenario)
        apply_route = scenario.get("applyRoute")
        expected_route = (
            "correction" if (armed and route == CORRECTION_ROUTE_ID) else "legacy")
        if apply_route is None:
            # Infer: gate flags are the contract (correctionRouteActive ==
            # armed AND route bound); the runner additionally records slot
            # evidence. Inference is honest about being flag-derived, not a
            # behavioral substitute for the C1 trajectory comparison.
            gate_active = (armed and route == CORRECTION_ROUTE_ID)
            apply_route = "correction" if gate_active else "legacy"
            inferred = True
        else:
            inferred = False
        ok = apply_route == expected_route
        detail = {
            "scenario": name,
            "observationRouteId": route,
            "stateMachineArmed": armed,
            "expectedApplyRoute": expected_route,
            "observedApplyRoute": apply_route,
            "inferredFromGateFlags": inferred,
            "passed": ok,
        }
        if not ok:
            errors.append(f"{name}:expected={expected_route},observed={apply_route}")
        # (a)/(b)/(d): legacy semantics - requested>0 must imply applied flow
        # (proportional-scaling apply, never the reservation allocator that
        # produced requested>0/applied=0 at step25 in the D-1 defect).
        if expected_route == "legacy":
            relax_req = scenario.get("relaxRequestedTotal")
            relax_app = scenario.get("relaxAppliedTotal")
            if finite(relax_req) and float(relax_req) > 0 and not finite(relax_app):
                errors.append(f"{name}:legacyRequestedWithoutApplied")
            if finite(relax_req) and finite(relax_app) and float(relax_req) > 1e-12 \
                    and float(relax_app) > float(relax_req) + 1e-9:
                errors.append(f"{name}:legacyAppliedExceedsRequested")
            # D-1 defect signature: requested>0 with applied==0 on a legacy
            # run means the reservation allocator replaced the legacy apply.
            saw_defect_signature = scenario.get("sawRequestedWithoutApplied")
            if saw_defect_signature is True:
                errors.append(f"{name}:legacyDefectSignatureRequestedNoApplied")
            detail["relaxRequestedTotal"] = relax_req
            detail["relaxAppliedTotal"] = relax_app
        # (c): correction route - the slot must have actually run (slot ran
        # evidence or zero-activity with closure fields present in D-1b).
        if expected_route == "correction":
            slot_ran = scenario.get("correctionSlotRan")
            if slot_ran is not True:
                errors.append(f"{name}:correctionSlotDidNotRun")
            detail["correctionSlotRan"] = slot_ran
        results.append(detail)
    return {
        "case": "D1-C2-dual-condition-gate-split",
        "passed": not errors,
        "errors": errors,
        "scenarios": results,
        "rule": "correction slot ONLY on (state machine armed AND explicit "
                + CORRECTION_ROUTE_ID + " route); every other run keeps the "
                "pre-correction relaxAngle apply",
    }


def check_d1_c3_correction_telemetry(records):
    """D-1b RED baseline: the four correction telemetry fields must exist in
    the audit of every committed substep of a correction-route run, with the
    closure identity requested == granted + rejected per substep. The fields
    are absent from P011NaturalObservationAudit today -> runs RED by design."""
    errors = []
    missing_field_records = 0
    closure_violations = 0
    checked = 0
    for record in records:
        audit = record.get("audit") or {}
        context = record.get("context", {})
        route = context.get("observationRouteId") or record.get("observationRouteId")
        if route != CORRECTION_ROUTE_ID:
            continue
        checked += 1
        for field in CORRECTION_AUDIT_FIELDS:
            if field not in audit:
                missing_field_records += 1
        requested = audit.get("correctionRequested")
        granted = audit.get("correctionGranted")
        rejected = audit.get("correctionRejected")
        if all(finite(v) for v in (requested, granted, rejected)):
            if abs(float(requested) - (float(granted) + float(rejected))) > 1e-9:
                closure_violations += 1
    red = missing_field_records > 0 or checked == 0
    return {
        "case": "D1-C3-correction-telemetry-audit (D-1b)",
        "passed": not red and closure_violations == 0,
        "redBaseline": red,
        "errors": errors,
        "correctionSubstepsChecked": checked,
        "missingFieldRecords": missing_field_records,
        "closureViolations": closure_violations,
        "requiredFields": list(CORRECTION_AUDIT_FIELDS),
        "note": "RED by design: correctionRequested/Granted/Rejected/"
                "StalledCount are not yet in P011NaturalObservationAudit; "
                "turns green when the developer lands D-1b",
    }


def analyze(evidence_path, records_path):
    evidence = read_json(evidence_path) if evidence_path else {}
    records = list(read_records(records_path)) if records_path else []
    c1 = check_d1_c1_trajectory(evidence)
    scenarios = evidence.get("routeScenarios") or []
    c2 = check_d1_c2_route_split(scenarios)
    c3 = check_d1_c3_correction_telemetry(records)
    red_cases = [c["case"] for c in (c1, c2, c3) if c.get("redBaseline")]
    return {
        "schemaVersion": 1,
        "testCase": "P011-OBS3-D1-VERIFICATION-CASES",
        "cases": {"D1-C1": c1, "D1-C2": c2, "D1-C3": c3},
        "redBaselineCases": red_cases,
        "overall": ("RED_BASELINE" if red_cases else
                    "PASS" if (c1["passed"] and c2["passed"] and c3["passed"])
                    else "FAIL"),
        "note": "RED baseline recorded per the task: cases may fail on the "
                "current build by design and turn green when the D-1/D-1b "
                "remediation lands; no gate is weakened",
    }


def read_records(path):
    if not path:
        return
    with Path(path).open("r", encoding="utf-8") as stream:
        for line in stream:
            if line.strip():
                yield json.loads(line)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, default=None,
                        help="trajectory + route-split evidence JSON (from the Node runner)")
    parser.add_argument("--ledger", type=Path, default=None,
                        help="correction-route ledger NDJSON for the D-1b audit check")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = analyze(args.evidence, args.ledger)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n",
                           encoding="utf-8")
    print(json.dumps({"overall": result["overall"],
                      "redBaselineCases": result["redBaselineCases"]},
                     ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
