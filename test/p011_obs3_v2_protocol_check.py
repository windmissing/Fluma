#!/usr/bin/env python3
"""Observation-3 v2 protocol gate (preregistration-v1).

Implements docs/observation-protocol-P011-v2-preregistration-v1.md
sections 4-7 as a static analysis channel. This tool never produces
threshold estimates: analysisRole=pilot always outputs
thresholdVerdict=NOT_EVALUATED with null estimates, and every
censoring / not-observed outcome is reported as a fixed output code
(section 6) instead of a point value.

Safety gates (tan30, budgets, conservation, inventory, capacity,
finite values, telemetry/ledger alignment, edge identity, stop
semantics) remain owned by the existing safety checkers and are not
weakened or duplicated here.

Section mapping:
  s4  platform identification  - actual effectiveTilt stable platforms
       (NOT targetTilt), 0.20 deg identity tolerance only, 200 ms /
       >=3 substep stability window, planner acceptance separation.
  s5  four main cells          - start/+ start/- stop/+ stop/- assigned
       by accepted-tilt sign, first qualifying transition per
       (runId, phaseIndex, c), max 3 per run per main cell (time order).
  s6  censoring / not-observed - seven fixed output codes, none of
       which produces a point estimate.
  s8  pilot three-dimension verdict - clean / platformReachability /
       pilotCoverage reported independently; never rewritten into a
       calibration PASS.
"""
import argparse
import json
import math
from pathlib import Path

PROTOCOL_ID = "p011-observation3-v2"
PROTOCOL_REVISION = "preregistration-v1"
EXPECTED_SCHEDULE = [0, 15, 0, 30, 0, -15, 0, -30, 0]
PLATFORM_IDENTITY_TOLERANCE_DEG = 0.20
ANGLE_NUMERIC_TOLERANCE_DEG = 1e-9
RATE_TOLERANCE_DEG_PER_S = 1e-9
PLATFORM_RANGE_TOLERANCE_DEG = 1e-6
STABILITY_WINDOW_MS = 200.0
STABILITY_MIN_SUBSTEPS = 3
MAX_TRANSITIONS_PER_RUN_PER_CELL = 3

# Section 6 fixed output codes (never produce point estimates).
CODE_RIGHT_CENSORED_AT_TAN30 = "RIGHT_CENSORED_AT_TAN30"
CODE_STOP_TRANSITION_UNOBSERVED = "STOP_TRANSITION_UNOBSERVED"
STOP_UNOBSERVED_SUBCODES = (
    "neverActive",
    "alreadyStoppedBeforePlateau",
    "stillActiveAtEnd",
    "onlyPlannerHold",
    "onlyBudgetLimited",
    "noEligibleBracket",
)
CODE_START_OUTSIDE_ELIGIBLE_WINDOW = "START_OUTSIDE_ELIGIBLE_WINDOW"
CODE_PLATFORM_UNREACHED = "PLATFORM_UNREACHED"
CODE_PLATFORM_UNSTABLE = "PLATFORM_UNSTABLE"
CODE_TELEMETRY_INCOMPLETE = "TELEMETRY_INCOMPLETE"
CODE_RUN_INVALID_HARD_GATE = "RUN_INVALID_HARD_GATE"

MAIN_CELLS = ("start/+", "start/-", "stop/+", "stop/-")


def read_json(path):
    text = Path(path).read_text(encoding="utf-8-sig").strip() if path else ""
    return json.loads(text) if text else {}


def read_records(path):
    with Path(path).open("r", encoding="utf-8") as stream:
        for line in stream:
            if line.strip():
                yield json.loads(line)


def finite(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def is_command_angle(target):
    """True when targetTilt is one of the nine-phase command angles."""
    return finite(target) and any(abs(float(target) - t) <= ANGLE_NUMERIC_TOLERANCE_DEG for t in EXPECTED_SCHEDULE)


class PlatformWindow:
    """Section 4 stable-accepted-platform window tracker.

    A record reaches the candidate platform when
    abs(effectiveTilt - T) <= 0.20 deg with effectiveTilt consistent with
    thetaAccepted (numeric check tolerance 1e-9 deg). The 0.20 deg value
    distinguishes platform identity only; the tan30 residual gate is
    unchanged. Stability additionally requires abs(rate) <= 1e-9 deg/s,
    within-window effectiveTilt range <= 1e-6 deg, >= 3 contiguous
    submitted substeps spanning >= 200 ms, and no identity break.
    Platform eligibility starts at the record that COMPLETES the window;
    earlier window records are never backfilled as stable samples.
    """

    def __init__(self):
        self.window = []  # list of (physicsTimeMs, effectiveTilt, substepId)

    def reset(self):
        self.window = []

    def feed(self, record, accepted_consistent):
        """Feed one record; return eligibility verdict for this record.

        Returns one of:
          "stable"     - this record completes/extends a valid window and
                         is platform-eligible from this record onward.
          "candidate"  - on the candidate platform but window not yet met.
          "off"        - not on the candidate platform.
        """
        context = record.get("context", {})
        effective = context.get("effectiveTilt")
        rate = context.get("effectiveTiltRate")
        time_ms = float(context.get("physicsTime", 0.0)) * 1000.0
        substep = context.get("substepId")
        if not (finite(effective) and finite(rate) and finite(time_ms)):
            self.reset()
            return "off"
        effective = float(effective)
        rate = float(rate)
        # Candidate platform: identity tolerance vs nearest command angle T
        # (0 included for zero-phase diagnostics).
        near = None
        for t in EXPECTED_SCHEDULE:
            if abs(effective - float(t)) <= PLATFORM_IDENTITY_TOLERANCE_DEG:
                near = float(t)
                break
        if near is None or not accepted_consistent or abs(rate) > RATE_TOLERANCE_DEG_PER_S:
            self.reset()
            return "off"
        # Contiguity: same platform, monotone substeps, no gap.
        if self.window:
            last_substep = self.window[-1][2]
            if not (isinstance(substep, int) and substep == last_substep + 1
                    and abs(effective - self.window[-1][1]) <= PLATFORM_IDENTITY_TOLERANCE_DEG):
                self.reset()
        self.window.append((time_ms, effective, substep))
        # Trim records that no longer matter (keep the recent tail).
        if len(self.window) > 4096:
            self.window = self.window[-2048:]
        # Stability: >= 3 contiguous substeps spanning >= 200 ms, range <= 1e-6.
        if len(self.window) >= STABILITY_MIN_SUBSTEPS:
            span_ms = self.window[-1][0] - self.window[0][0]
            tilts = [item[1] for item in self.window]
            if span_ms >= STABILITY_WINDOW_MS and (max(tilts) - min(tilts)) <= PLATFORM_RANGE_TOLERANCE_DEG:
                return "stable"
        return "candidate"


def accepted_consistent(context):
    """effectiveTilt vs thetaAccepted numeric consistency (1e-9 deg).

    thetaAccepted may be absent from the current ledger schema; absence
    is reported as telemetry-incomplete, never treated as consistent.
    """
    theta_accepted = context.get("thetaAccepted")
    effective = context.get("effectiveTilt")
    if theta_accepted is None:
        return None  # unknown -> telemetry incomplete, not assumed consistent
    if not (finite(theta_accepted) and finite(effective)):
        return False
    return abs(float(theta_accepted) - float(effective)) <= ANGLE_NUMERIC_TOLERANCE_DEG


def sign_cell(accepted):
    """Section 4.2/5.2: cell sign comes from the stable platform's
    effectiveTilt (accepted) sign - never donor, residual, or command sign."""
    if not finite(accepted) or float(accepted) == 0.0:
        return None
    return "+" if float(accepted) > 0 else "-"


def analyze(ledger_path, manifest_path):
    manifest = read_json(manifest_path) if manifest_path else {}
    records = list(read_records(ledger_path))
    protocol_errors = []

    protocol = manifest.get("calibrationProtocol", {}) if isinstance(manifest, dict) else {}
    if protocol.get("protocolId") != PROTOCOL_ID:
        protocol_errors.append("protocolIdMissingOrMismatch")
    if protocol.get("protocolRevision") != PROTOCOL_REVISION:
        protocol_errors.append("protocolRevisionMissingOrMismatch")
    if protocol.get("scheduleTargets") != EXPECTED_SCHEDULE:
        protocol_errors.append("scheduleTargetsMissingOrMismatch")
    schedule_hash = protocol.get("scheduleSha256")
    if not isinstance(schedule_hash, str) or len(schedule_hash) != 64:
        protocol_errors.append("scheduleSha256Missing")

    first_context = records[0].get("context", {}) if records else {}
    if not (first_context.get("runRole") == "calibration"
            and first_context.get("observationRouteId") == "natural-relax-hysteresis-v1"):
        protocol_errors.append("notObservation3CalibrationRun")

    # ---- Pass 1: per-substep platform windows and reachability (s4) ----
    window = PlatformWindow()
    platform_reach = {
        "15": _new_reach(),
        "30": _new_reach(),
        "-15": _new_reach(),
        "-30": _new_reach(),
    }
    telemetry_incomplete_records = 0
    substep_platform_state = []  # per record: (stable: bool, accepted: float|None, sign)

    for record in records:
        context = record.get("context", {})
        consistent = accepted_consistent(context)
        if consistent is None:
            telemetry_incomplete_records += 1
        verdict = window.feed(record, consistent is True)
        accepted = context.get("effectiveTilt")
        target = context.get("targetTilt")
        if is_command_angle(target) and finite(accepted):
            key = str(int(float(target))) if abs(float(target)) >= 1 else None
            state = {"stable": verdict == "stable", "accepted": float(accepted),
                     "sign": sign_cell(accepted)}
        else:
            key = None
            state = {"stable": False, "accepted": None, "sign": None}
        substep_platform_state.append(state)
        if key and verdict in ("stable", "candidate"):
            reach = platform_reach.get(key)
            if reach is not None:
                if reach["firstArrivalMs"] is None:
                    reach["firstArrivalMs"] = float(context.get("physicsTime", 0.0)) * 1000.0
                if verdict == "stable":
                    if reach["firstStableMs"] is None:
                        reach["firstStableMs"] = float(context.get("physicsTime", 0.0)) * 1000.0
                    reach["stableRecords"] += 1
                    reach["acceptedMin"] = accepted if reach["acceptedMin"] is None else min(reach["acceptedMin"], accepted)
                    reach["acceptedMax"] = accepted if reach["acceptedMax"] is None else max(reach["acceptedMax"], accepted)
                    # Planner limitation on a stable platform: thetaRequested
                    # must equal thetaAccepted (1e-9); otherwise the endpoint
                    # loses calibration eligibility (s4.2), recorded not hidden.
                    theta_req = context.get("thetaRequested")
                    theta_acc = context.get("thetaAccepted")
                    if theta_req is None or theta_acc is None or not (
                            finite(theta_req) and finite(theta_acc)
                            and abs(float(theta_req) - float(theta_acc)) <= ANGLE_NUMERIC_TOLERANCE_DEG):
                        reach["plannerLimitedStableRecords"] += 1

    # ---- Pass 2: state samples, four cells, transitions (s5) ----
    cells = {
        key: {
            "stateSamples": 0,
            "stateRuns": set(),
            "realTransitions": 0,
            "eligibleTransitions": 0,
            "firstBrackets": 0,  # first qualifying transition per (run, phaseIndex, c)
            "perRunTransitions": {},  # runId -> count (cap 3 for main-cell accounting)
            "censorCodes": [],
            "atRiskAlphaMax": None,
            "stopUnobservedSubcode": None,
        }
        for key in MAIN_CELLS
    }
    neutral_states = 0
    first_bracket_keys = set()
    prior_state = {}  # (runId, c) -> (substepId, activeBefore, activeAfter, alpha, meta)

    for record, plat_state in zip(records, substep_platform_state):
        context = record.get("context", {})
        run_id = context.get("runId")
        substep_id = context.get("substepId")
        phase_index = context.get("phaseIndex")
        hysteresis = record.get("relaxHysteresis", {})
        edges = hysteresis.get("edges", []) if isinstance(hysteresis, dict) else []
        if not isinstance(edges, list):
            edges = []
        for edge in edges:
            if not isinstance(edge, dict):
                continue
            donor = edge.get("donor")
            receiver = edge.get("receiver")
            interface = edge.get("interfaceIndex")
            # s5.1 fixed physical edge identity: c = min(donor, receiver),
            # transport records require |donor - receiver| == 1.
            if not (finite(donor) and finite(receiver) and finite(interface)):
                continue
            if abs(float(donor) - float(receiver)) == 1:
                c = min(float(donor), float(receiver))
                if float(interface) != c:
                    continue  # directional donor index misused as edge identity
            active_before = edge.get("activeBefore")
            active_after = edge.get("activeAfter")
            if not isinstance(active_before, bool):
                continue
            accepted = plat_state["accepted"]
            cell_sign = plat_state["sign"] if plat_state["stable"] else None
            alpha = None
            r = edge.get("r")
            if finite(r):
                alpha = math.degrees(math.atan(abs(float(r))))
            # ---- state sample accounting (denominator separated from transitions) ----
            if cell_sign:
                kind = "start" if not active_before else "stop"
                cell = cells[f"{kind}/{cell_sign}"]
                cell["stateSamples"] += 1
                cell["stateRuns"].add(run_id)
                if alpha is not None and kind == "stop":
                    cell["atRiskAlphaMax"] = alpha if cell["atRiskAlphaMax"] is None else max(cell["atRiskAlphaMax"], alpha)
            else:
                neutral_states += 1
            # ---- real transition detection (s5.1) ----
            if not isinstance(active_after, bool):
                continue
            # Per-edge previous-state tracking: EVERY edge record updates the
            # prior state (not only transitions), so bracket continuity can
            # span arbitrarily many stable non-transition substeps.
            identity = (run_id, float(interface))
            previous = prior_state.get(identity)
            prior_state[identity] = (substep_id, active_before, active_after, alpha, plat_state["stable"])
            is_real_start = (not active_before) and active_after
            is_real_stop = active_before and (not active_after)
            if not (is_real_start or is_real_stop):
                continue
            kind = "start" if is_real_start else "stop"
            cell_sign_now = cell_sign
            if cell_sign_now is None:
                continue
            cell_key = f"{kind}/{cell_sign_now}"
            cell = cells[cell_key]
            cell["realTransitions"] += 1
            run_count = cell["perRunTransitions"].setdefault(run_id, 0)
            # s5.2 cap: max 3 per run per main cell, time/substepId/c order
            # (ledger order is substep order; no residual-based selection).
            if run_count >= MAX_TRANSITIONS_PER_RUN_PER_CELL:
                continue
            cell["perRunTransitions"][run_id] = run_count + 1
            # bracket eligibility: contiguous same-edge substeps, both
            # endpoints platform-eligible, previous activeAfter == this
            # activeBefore, no budget-limited ambiguity on either endpoint.
            prev_substep, prev_before, prev_after, prev_alpha, prev_stable = previous
            contiguous = isinstance(substep_id, int) and prev_substep == substep_id - 1
            continuity = prev_after == active_before
            requested = edge.get("requested")
            granted = edge.get("granted")
            budget_limited = finite(requested) and finite(granted) and float(granted) + 1e-9 < float(requested)
            theta_req = context.get("thetaRequested")
            theta_acc = context.get("thetaAccepted")
            planner_clear = (finite(theta_req) and finite(theta_acc)
                             and abs(float(theta_req) - float(theta_acc)) <= ANGLE_NUMERIC_TOLERANCE_DEG)
            if not (contiguous and continuity and prev_stable and plat_state["stable"]
                    and not budget_limited and planner_clear
                    and prev_alpha is not None and alpha is not None):
                continue
            cell["eligibleTransitions"] += 1
            # s5.1 first qualifying transition per (runId, phaseIndex, c):
            # start OR stop, whichever appears first; later round-trips keep
            # raw records but never enter estimation.
            bracket_key = (run_id, phase_index if isinstance(phase_index, int) else str(phase_index), float(interface))
            if bracket_key not in first_bracket_keys:
                first_bracket_keys.add(bracket_key)
                cell["firstBrackets"] += 1

    # ---- s6 censoring / not-observed codes per cell ----
    hard_gate_failed = bool(protocol_errors) is False and any(
        record.get("audit", {}).get("errorCode") not in (None, 0, "0")
        for record in records
    )
    for key, cell in cells.items():
        codes = []
        sign = key.split("/")[1]
        kind = key.split("/")[0]
        big_platform = platform_reach["30" if sign == "+" else "-30"]
        small_platform = platform_reach["15" if sign == "+" else "-15"]
        if hard_gate_failed:
            codes.append(CODE_RUN_INVALID_HARD_GATE)
        if telemetry_incomplete_records > 0:
            codes.append(CODE_TELEMETRY_INCOMPLETE)
        if big_platform["stableRecords"] == 0 and small_platform["stableRecords"] == 0:
            codes.append(CODE_PLATFORM_UNREACHED if big_platform["firstArrivalMs"] is None
                         else CODE_PLATFORM_UNSTABLE)
        elif cell["eligibleTransitions"] == 0:
            if kind == "stop":
                codes.append(CODE_STOP_TRANSITION_UNOBSERVED)
                cell["stopUnobservedSubcode"] = _stop_subcode(cell, big_platform, records)
            elif cell["realTransitions"] > 0:
                codes.append(CODE_START_OUTSIDE_ELIGIBLE_WINDOW)
            elif cell["atRiskAlphaMax"] is not None and cell["atRiskAlphaMax"] < 25.0:
                codes.append(CODE_RIGHT_CENSORED_AT_TAN30)
        cell["censorCodes"] = codes

    # ---- s8 pilot three-dimension verdict (independent, never a PASS for calibration) ----
    clean_pass = (not protocol_errors) and not hard_gate_failed and telemetry_incomplete_records == 0
    reach_pass = all(
        platform_reach[k]["stableRecords"] > 0
        and platform_reach[k]["plannerLimitedStableRecords"] == 0
        for k in ("15", "30", "-15", "-30")
    )
    coverage = ("FOUR_CELL_OBSERVED"
                if all(cells[k]["firstBrackets"] >= 1 for k in MAIN_CELLS)
                else "INCOMPLETE")

    result = {
        "schemaVersion": 2,
        "testCase": "P011-OBS3-V2-PROTOCOL-GATE",
        "protocolId": PROTOCOL_ID,
        "protocolRevision": PROTOCOL_REVISION,
        "analysisRole": "pilot",
        "overall": "INVALID" if (hard_gate_failed or protocol_errors) else "PILOT_REPORT",
        "thresholdVerdict": "NOT_EVALUATED",
        "thetaStartEstimate": None,
        "thetaStopEstimate": None,
        "thetaStartCI": None,
        "thetaStopCI": None,
        "safetyMustPass": "UNCHANGED_AND_CHECKED_BY_EXISTING_SAFETY_CHECKERS",
        "protocolMetadataErrors": protocol_errors,
        "telemetryIncompleteRecords": telemetry_incomplete_records,
        "runInvalidHardGate": hard_gate_failed,
        "platformReachability": {
            "verdict": "PASS" if reach_pass else "FAIL",
            "perPhase": platform_reach,
            "note": "verdicts independent of pilotCoverage; never folded into a calibration PASS",
        },
        "pilotCoverage": coverage,
        "cells": {
            key: {
                "stateSamples": cell["stateSamples"],
                "stateRunCount": len(cell["stateRuns"]),
                "realTransitions": cell["realTransitions"],
                "eligibleTransitions": cell["eligibleTransitions"],
                "firstBrackets": cell["firstBrackets"],
                "perRunCappedTransitions": dict(sorted(cell["perRunTransitions"].items())),
                "atRiskAlphaMaxDeg": cell["atRiskAlphaMax"],
                "censorCodes": cell["censorCodes"],
                "stopUnobservedSubcode": cell["stopUnobservedSubcode"],
            }
            for key, cell in cells.items()
        },
        "neutralStateSamples": neutral_states,
        "censorCodeCatalog": {
            "RIGHT_CENSORED_AT_TAN30": "fixed output; no endpoint events, no point fit, never calibration PASS",
            "STOP_TRANSITION_UNOBSERVED": "subcodes: " + ", ".join(STOP_UNOBSERVED_SUBCODES),
            "START_OUTSIDE_ELIGIBLE_WINDOW": "start only on ramp or already active at plateau entry",
            "PLATFORM_UNREACHED": "no stable window; never infer alpha from targetTilt",
            "PLATFORM_UNSTABLE": "arrived but no qualifying stable window",
            "TELEMETRY_INCOMPLETE": "missing planner/state fields; unknown is not no-transition",
            "RUN_INVALID_HARD_GATE": "whole run quarantined; no cherry-picked substeps",
        },
        "samplePolicy": "pilot never counts toward 34-run fit/holdout; thresholds NOT_EVALUATED",
    }
    return result


def _new_reach():
    return {"firstArrivalMs": None, "firstStableMs": None, "stableMs": 0.0,
            "acceptedMin": None, "acceptedMax": None, "stableRecords": 0,
            "plannerLimitedStableRecords": 0}


def _stop_subcode(cell, big_platform, records):
    """Section 6 stop-unobserved six-way subcode (single primary cause)."""
    if cell["stateSamples"] == 0:
        return "neverActive"
    if cell["realTransitions"] == 0 and big_platform["stableRecords"] > 0:
        return "noEligibleBracket"
    if cell["perRunTransitions"]:
        return "noEligibleBracket"
    return "stillActiveAtEnd"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, default=None)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = analyze(args.ledger, args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, ensure_ascii=False))
    # Pilot gate never PASSes calibration; exit 0 means the report was produced.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
