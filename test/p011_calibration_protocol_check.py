#!/usr/bin/env python3
"""Observation-3 calibration audit; threshold identification is never PASS/FAIL here."""
import argparse
import json
import math
from pathlib import Path


EXPECTED_TARGETS = [0, 15, 0, 30, 0, -15, 0, -30, 0]
TAN_30 = math.tan(math.radians(30.0))
EPS = 1e-9


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def read_records(path):
    with path.open("r", encoding="utf-8") as stream:
        for line in stream:
            if line.strip():
                yield json.loads(line)


def is_platform(context, tolerance):
    target = context.get("targetTilt")
    accepted = context.get("effectiveTilt")
    rate = context.get("effectiveTiltRate")
    return bool(
        tolerance is not None
        and isinstance(target, (int, float)) and not isinstance(target, bool)
        and min(abs(abs(float(target)) - 15.0), abs(abs(float(target)) - 30.0)) <= EPS
        and isinstance(accepted, (int, float)) and not isinstance(accepted, bool)
        and isinstance(rate, (int, float)) and not isinstance(rate, bool)
        and abs(float(rate)) <= EPS
        and abs(float(accepted) - float(target)) <= float(tolerance)
    )


def audit(ledger_path, manifest_path):
    manifest = read_json(manifest_path)
    records = list(read_records(ledger_path))
    errors = []
    if not records:
        errors.append("ledgerEmpty")
    protocol = manifest.get("calibrationProtocol")
    if not isinstance(protocol, dict):
        protocol = {}
        errors.append("calibrationProtocolMissing")
    if protocol.get("scheduleTargets") != EXPECTED_TARGETS:
        errors.append("scheduleTargetsMissingOrMismatch")
    schedule_hash = protocol.get("scheduleSha256")
    if not isinstance(schedule_hash, str) or len(schedule_hash) != 64:
        errors.append("scheduleSha256Missing")
    tolerance = protocol.get("platformToleranceDeg")
    if not isinstance(tolerance, (int, float)) or isinstance(tolerance, bool) or not math.isfinite(tolerance) or tolerance <= 0:
        errors.append("platformToleranceDegMissingOrInvalid")
        tolerance = None
    hold_ms = protocol.get("platformHoldMs")
    if not isinstance(hold_ms, (int, float)) or isinstance(hold_ms, bool) or not math.isfinite(hold_ms) or hold_ms <= 0:
        errors.append("platformHoldMsMissingOrInvalid")

    first_context = records[0].get("context", {}) if records else {}
    calibration_run = first_context.get("runRole") == "calibration" and first_context.get("observationRouteId") == "natural-relax-hysteresis-v1"
    if not calibration_run:
        errors.append("notObservation3CalibrationRun")

    actual_targets = []
    for record in records:
        context = record.get("context", {})
        target = context.get("targetTilt")
        if isinstance(target, (int, float)) and not isinstance(target, bool):
            target = int(target) if float(target).is_integer() else target
            if target != (actual_targets[-1] if actual_targets else None):
                actual_targets.append(target)
    if actual_targets and actual_targets != EXPECTED_TARGETS:
        errors.append("observedScheduleTargetsMismatch")

    max_alpha = 0.0
    target_accepted_mismatch_records = 0
    rejected_requests = 0
    budget_limited_edges = 0
    platform_records = {15: 0, 30: 0, -15: 0, -30: 0}
    transitions = {"start/+": 0, "start/-": 0, "stop/+": 0, "stop/-": 0}
    qualified_platform_transitions = {key: 0 for key in transitions}
    event_candidates_missing_planner_telemetry = 0
    prior = {}
    for record in records:
        context = record.get("context", {})
        target = context.get("targetTilt")
        accepted = context.get("effectiveTilt")
        rate = context.get("effectiveTiltRate")
        phase_sign = 1 if isinstance(target, (int, float)) and target > 0 else -1 if isinstance(target, (int, float)) and target < 0 else 0
        plateau = is_platform(context, tolerance)
        if plateau:
            platform_records[int(target)] += 1
        elif isinstance(target, (int, float)) and isinstance(accepted, (int, float)) and abs(float(target)-float(accepted)) > EPS:
            target_accepted_mismatch_records += 1

        hysteresis = record.get("relaxHysteresis", {})
        edges = hysteresis.get("edges", []) if isinstance(hysteresis, dict) else []
        for edge in edges if isinstance(edges, list) else []:
            r = edge.get("r") if isinstance(edge, dict) else None
            if isinstance(r, (int, float)) and not isinstance(r, bool) and math.isfinite(r):
                max_alpha = max(max_alpha, math.degrees(math.atan(abs(float(r)))))
            requested, granted = edge.get("requested"), edge.get("granted")
            if isinstance(requested, (int, float)) and isinstance(granted, (int, float)) and requested > granted + EPS:
                budget_limited_edges += 1
            transition = edge.get("transitionType")
            if transition in {"start", "stop"}:
                key = f"{transition}/{'+' if phase_sign > 0 else '-'}" if phase_sign else None
                if key in transitions:
                    transitions[key] += 1
                    if not all(name in context for name in ("thetaAccepted", "requestRejected", "plannerHold")):
                        event_candidates_missing_planner_telemetry += 1
                    # A bracket candidate requires same edge, contiguous substep, real transition,
                    # stable accepted platform, and no budget-limited grant.
                    identity = (context.get("runId"), edge.get("interfaceIndex"))
                    previous = prior.get(identity)
                    current_limited = isinstance(requested, (int, float)) and isinstance(granted, (int, float)) and granted + EPS < requested
                    if plateau and not current_limited and previous:
                        old_context, old_edge, old_plateau = previous
                        contiguous = context.get("substepId") == old_context.get("substepId", -2) + 1
                        old_r, new_r = old_edge.get("r"), edge.get("r")
                        old_requested, old_granted = old_edge.get("requested"), old_edge.get("granted")
                        old_limited = isinstance(old_requested, (int, float)) and isinstance(old_granted, (int, float)) and old_granted + EPS < old_requested
                        if old_plateau and not old_limited and contiguous and isinstance(old_r, (int, float)) and isinstance(new_r, (int, float)):
                            qualified_platform_transitions[key] += 1
            if isinstance(edge, dict):
                identity = (context.get("runId"), edge.get("interfaceIndex"))
                prior[identity] = (context, edge, plateau)

    statuses = {}
    for key, count in qualified_platform_transitions.items():
        if count:
            statuses[key] = "BRACKET_CANDIDATES_PRESENT_REQUIRES_REVIEW"
        elif (key.startswith("start/") and transitions[key] == 0
              and platform_records[30 if key.endswith("+") else -30] > 0
              and 0.0 < max_alpha < 30.0):
            statuses[key] = "RIGHT_CENSORED_AT_TAN30"
        elif key.startswith("stop/"):
            statuses[key] = "STOP_TRANSITION_UNOBSERVED"
        else:
            statuses[key] = "INCOMPLETE / DESIGN_PENDING_OBSERVATION"

    # This tool only reports observability. Safety gates remain owned by the formal safety checker.
    return {
        "schemaVersion": 1,
        "testCase": "P011-OBS3-CALIBRATION-PROTOCOL",
        "overall": "INCOMPLETE" if errors or any(value != "BRACKET_CANDIDATES_PRESENT_REQUIRES_REVIEW" for value in statuses.values()) else "INCOMPLETE",
        "thresholdVerdict": "NOT_EVALUATED",
        "safetyMustPass": "UNCHANGED_AND_CHECKED_BY_EXISTING_SAFETY_CHECKER",
        "runRole": first_context.get("runRole"),
        "protocolMetadataErrors": errors,
        "observedScheduleTargets": actual_targets,
        "platformRecordCounts": platform_records,
        "maxObservedAlphaDeg": max_alpha,
        "tan30CensorApplies": max_alpha >= 29.0 and max_alpha <= 30.0 + 1e-6,
        "cellStatus": statuses,
        "rawRealTransitions": transitions,
        "platformBracketCandidateTransitions": qualified_platform_transitions,
        "targetAcceptedMismatchRecords": target_accepted_mismatch_records,
        "budgetLimitedEdges": budget_limited_edges,
        "rejectedRequests": None,
        "plannerTelemetryLimitation": "target/effective mismatch also occurs during ramps and cannot be labeled planner hold; ledger schema has no thetaAccepted/requestRejected/plannerHold fields, so rejection count is unavailable and no event involving such a state can be certified as a platform bracket",
        "sampleAndFitPolicy": "unchanged; this run-level audit never satisfies fit/holdout sample minima, CI, or holdout requirements",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ledger", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.ledger, args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0 if result["overall"] == "INCOMPLETE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
