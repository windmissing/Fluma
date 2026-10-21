#!/usr/bin/env python3
"""Observation-3 v3 protocol gate (preregistration-v1, METHOD_FROZEN).

Implements docs/observation-protocol-P011-v3-preregistration-v1.md
sections 2-8 as a static analysis channel. Independent of the v1/v2
checkers; nothing here weakens safety gates, moves the search window,
authorizes capture, or produces threshold estimates.

Section mapping:
  s2  data identity & decision-time contract - per-attempt/per-edge
      fields, decisionG/R/Alpha at the pre-branch sampling point,
      executed-vs-configured threshold identity.
  s3  nine platforms & eight legs - leg identity from the pre-registered
      schedule, half-open boundary ownership, command step 100 ms /
      angular step 0.05 / 0.10 deg, peak-grid divisibility.
  s4  shadow audit support - peak grid constant, SHADOW_SAFE /
      SHADOW_COVERED predicates, EXECUTION_ENVELOPE_UNRESOLVED,
      SHADOW_PREDICTED_CENSOR.
  s5  sample eligibility, bracket formula, 3-point monotone window,
      first-transition-per-(runId, segmentId, fixedEdge), 0.5 deg width
      gate (wide events recorded BRACKET_TOO_WIDE, never split).
  s6  frozen statistical gates as constants (34 runs, 30 states / 10
      runs per main cell, 3-per-run cap, 20 events / 5 runs, 24 sub
      cells, bootstrap seed 20261019, CI gates, threshold relation).
  s7  coverage matrix (13+ column definitions, source=shadow|pilot
      isomorphic tables).
  s8  honest output & censoring discipline - v3 right-censoring new
      definition (platform arrival no longer required), STOP
      nine-way subcodes, pilot three-dimension verdict.
"""
import argparse
import json
import math
from pathlib import Path

PROTOCOL_ID = "p011-observation3-v3"
PROTOCOL_REVISION = "preregistration-v1"

# --- s3 frozen激励序列 constants -------------------------------------------
PLATFORM_SEQUENCE = [0.0, 15.0, None, 15.0, 0.0, -15.0, None, -15.0, 0.0]
# H2/H6 are the A-peak platforms; A is bound per-candidate from the manifest
# peak grid, never defaulted to 30 / 29.9.
PLATFORM_HOLD_MS = 5100.0
COMMAND_STEP_MS = 100.0
RATE_PRIMARY_DEG_PER_S = 0.5
RATE_SENSITIVITY_DEG_PER_S = 1.0
ANGULAR_STEP = {RATE_PRIMARY_DEG_PER_S: 0.05, RATE_SENSITIVITY_DEG_PER_S: 0.10}
PEAK_GRID = [round(15.1 + 0.1 * i, 1) for i in range(149)]  # 15.1 .. 29.9

LEGS = {
    "L1": ("H0", "H1", "increase"),
    "L2": ("H1", "H2", "increase"),
    "L3": ("H2", "H3", "decrease"),
    "L4": ("H3", "H4", "decrease"),
    "L5": ("H4", "H5", "decrease"),
    "L6": ("H5", "H6", "decrease"),
    "L7": ("H6", "H7", "increase"),
    "L8": ("H7", "H8", "increase"),
}

# --- s4/s5 tolerance constants ---------------------------------------------
PLATFORM_IDENTITY_TOLERANCE_DEG = 0.20
ANGLE_NUMERIC_TOLERANCE_DEG = 1e-9
RATE_TOLERANCE_DEG_PER_S = 1e-9
PLATFORM_RANGE_TOLERANCE_DEG = 1e-6
STABILITY_WINDOW_MS = 200.0
STABILITY_MIN_SUBSTEPS = 3
BRACKET_MAX_WIDTH_DEG = 0.5

# --- s3.2 input-timing gate (drift consumption ruling, 2026-10-21) ----------
# Ruling: docs/drift-consumption-ruling-P011-obs3-v1.md. The gate judges
# ACTUAL ACCEPTED INTERVALS between consecutively digested schedule commands
# (per protocol s3.2), NOT the absolute scheduleDriftMs which carries a
# session-start constant offset and frame quantization. The >50ms marker
# (SCHEDULE_DRIFT_EXCEEDED) is telemetry only - NEVER a disqualification.
# Command-level breaches invalidate the brackets of the substeps they drove.
SCHEDULE_TIMING = {
    # ramp -> ramp (adjacent commands inside one leg; ideal COMMAND_STEP_MS)
    "rampRamp": {"minMs": 50.0, "maxMs": 200.0},
    # platform -> ramp (hold end entering a leg; ideal PLATFORM_HOLD_MS)
    "platformRamp": {"minMs": 4000.0, "maxMs": 6200.0},
    # slow-frame coalescing: adjacent digested commands may span at most 2
    # angular steps (one skipped command); more = digestion stall.
    "maxAngularStepMultiplier": 2,
    # intervals below this are treated as catch-up fabrication
    "catchUpFloorMs": 50.0,
}
SCHEDULE_INTERVAL_CODE = "SCHEDULE_INTERVAL_OUT_OF_BAND"
SCHEDULE_COMMAND_GAP_CODE = "SCHEDULE_COMMAND_GAP_EXCEEDED"

# --- pre-expansion ruling (2026-10-21, docs/pre-expansion-ruling-P011-obs3-v1.md) ---
# Ruling 1: ramp legs ARE eligible segments (protocol s5.2 "same
# preregistered ramp leg OR same eligible stable-platform segment"). Ramp
# transitions are assigned to a +/- cell by the PREREGISTERED leg sign
# (s3.1 table), never by the observed accepted sign (no cross-zero split
# inside one leg). Zero-accepted records stay neutral. Ramp records with
# no leg identity take SEGMENT_IDENTITY_MISSING instead of silent skip.
RAMP_LEG_SIGN = {"L1": "+", "L2": "+", "L3": "+", "L4": "+",
                 "L5": "-", "L6": "-", "L7": "-", "L8": "-"}
CODE_RAMP_SIGN_MISMATCH = "RAMP_SIGN_MISMATCH"
# Ruling 2: post-schedule tail truncation - records after the player's
# schedule end are excluded from samples/brackets/platform windows and
# counted as postScheduleRecords (diagnostics kept, data not deleted).
POST_SCHEDULE_TOLERANCE_MS = 200.0
CODE_POST_SCHEDULE = "POST_SCHEDULE"

# --- s6 frozen statistical gates (inherited unchanged) ----------------------
STATS = {
    "runsPerCandidatePair": 34,
    "fitRuns": 24,
    "holdoutRuns": 10,
    "minRunsPerDirectionPerGroup": 5,
    "mainCellMinStateSamples": 30,
    "mainCellMinRuns": 10,
    "maxStatesPerRunPerCell": 3,
    "minQualifiedEventsPerDirectionPerType": 20,
    "minRunsPerDirectionPerType": 5,
    "subCellCount": 24,
    "subCellMinStateSamples": 30,
    "subCellMinRuns": 10,
    "bootstrapIterations": 10000,
    "bootstrapSeed": 20261019,
    "ciPercentiles": [2.5, 97.5],
    "ciWidthMaxDeg": 2.0,
    "fitHoldoutEstimateDiffMaxDeg": 1.0,
    "holdoutOutsideFitCIRateMax": 0.05,
    "subCellMisjudgmentRateMax": 0.05,
    "startLowerAboveStopUpper": True,
    "pointSeparationMinNoiseUnits": 2.0,
    "startUpperCIBelowDeg": 30.0,
    "sigmaNoiseFactor": 1.4826,
    "primaryRateDegPerS": RATE_PRIMARY_DEG_PER_S,
    "sensitivityRateDegPerS": RATE_SENSITIVITY_DEG_PER_S,
    "sensitivityIndependentRunsIfConfirmed": 68,
}

# --- s8 output codes (none produce point estimates) -------------------------
CODE_RIGHT_CENSORED_AT_TAN30 = "RIGHT_CENSORED_AT_TAN30"
CODE_STOP_TRANSITION_UNOBSERVED = "STOP_TRANSITION_UNOBSERVED"
STOP_SUBCODES = (
    "NEVER_ACTIVE",
    "STOP_BEFORE_ELIGIBLE_SEGMENT",
    "STILL_ACTIVE_AT_END",
    "ONLY_NEUTRAL_ZERO_ENDPOINT",
    "ONLY_PLANNER_HOLD_OR_CLAMP",
    "ONLY_BUDGET_GUARD_OR_STOCK_LIMITED",
    "BRACKET_TOO_WIDE",
    "NONMONOTONE_LOCAL_WINDOW",
    "CONTINUITY_MISSING",
)
CODE_START_OUTSIDE_ELIGIBLE_WINDOW = "START_OUTSIDE_ELIGIBLE_WINDOW"
CODE_PLATFORM_UNREACHED = "PLATFORM_UNREACHED"
CODE_PLATFORM_UNSTABLE = "PLATFORM_UNSTABLE"
CODE_TELEMETRY_INCOMPLETE = "TELEMETRY_INCOMPLETE"
CODE_RUN_INVALID_HARD_GATE = "RUN_INVALID_HARD_GATE"
CODE_EXECUTION_IDENTITY_MISMATCH = "EXECUTION_THRESHOLD_IDENTITY_MISMATCH"
CODE_UNOBSERVABLE = "UNOBSERVABLE"
CODE_EXECUTION_ENVELOPE_UNRESOLVED = "EXECUTION_ENVELOPE_UNRESOLVED"
CODE_SHADOW_PREDICTED_CENSOR = "SHADOW_PREDICTED_CENSOR"
CODE_BRACKET_TOO_WIDE = "BRACKET_TOO_WIDE"
CODE_NONMONOTONE_LOCAL_WINDOW = "NONMONOTONE_LOCAL_WINDOW"

MAIN_CELLS = ("start/+", "start/-", "stop/+", "stop/-")
COVERAGE_STATUSES = (
    "OBSERVED", "NOT_OBSERVED", "UNDEFINED",
    "PROVEN_UNREACHABLE_IN_REGISTERED_CLASS", "UNKNOWN",
)


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


def decision_alpha(decision_r):
    """s2: decisionAlpha = atan(abs(decisionR)) in degrees (r normalized) or
    atan(abs(d)/dx) when r is a length; the ledger's `r` field is treated as
    the normalized residual per the existing checker convention."""
    if not finite(decision_r):
        return None
    return math.degrees(math.atan(abs(float(decision_r))))


def executed_identity_ok(context, manifest):
    """s2: executed thresholds must exist, match configured values, and carry
    implementation identity. Merely copying the manifest into logs is not
    enough: stateMachineVersion/sourceCommit/sourceFileSha256/buildSha256
    must be present. Absence -> EXECUTION_THRESHOLD_IDENTITY_MISMATCH."""
    executed = context.get("executedThresholds")
    configured = (manifest.get("calibrationProtocol", {}) or {}).get(
        "configuredThresholds") or manifest.get("configuredThresholds")
    if not isinstance(executed, dict):
        return False, "executedThresholdsMissing"
    for field in ("thetaStartDeg", "thetaStopDeg", "slopeStart", "slopeStop"):
        if not finite(executed.get(field)):
            return False, f"executedFieldMissing:{field}"
    for field in ("stateMachineVersion", "sourceCommit", "sourceFileSha256", "buildSha256"):
        if not executed.get(field):
            return False, f"executionIdentityMissing:{field}"
    if isinstance(configured, dict):
        for key in ("thetaStartDeg", "thetaStopDeg"):
            if finite(configured.get(key)) and finite(executed.get(key)):
                if abs(float(configured[key]) - float(executed[key])) > ANGLE_NUMERIC_TOLERANCE_DEG:
                    return False, f"configuredExecutedMismatch:{key}"
    return True, None


def leg_from_schedule(context, schedule_bindings):
    """s3: leg identity comes from the pre-registered schedule binding
    (commandSequence -> segmentId/legId), never re-segmented from observed
    transitions. Returns (legId, phaseKind) or (None, None)."""
    seq = context.get("commandSequence")
    if seq is None:
        seq = context.get("sequence")
    binding = schedule_bindings.get(str(seq)) if schedule_bindings else None
    if isinstance(binding, dict):
        return binding.get("legId"), binding.get("phaseKind")
    # Fall back to ledger-carried fields if the manifest did not bind;
    # absence is surfaced as telemetry-incomplete by the caller.
    # phaseKindV3 is the ledger key (GAP-2 schema); phaseKind accepted for
    # synthetic ledgers.
    return (context.get("legId"),
            context.get("phaseKindV3") or context.get("phaseKind"))


class PlatformWindow:
    """s3/s5 stable-accepted-platform window (same semantics as v2,
    retained for platform diagnostics; v3 does not require platforms for
    start events)."""

    def __init__(self):
        self.window = []

    def reset(self):
        self.window = []

    def feed(self, effective, rate, time_ms, substep, consistent):
        if not (finite(effective) and finite(rate) and finite(time_ms)):
            self.reset()
            return "off"
        effective = float(effective)
        near = None
        for t in [0.0, 15.0, -15.0, 30.0, -30.0]:
            if abs(effective - t) <= PLATFORM_IDENTITY_TOLERANCE_DEG:
                near = t
                break
        if near is None or not consistent or abs(float(rate)) > RATE_TOLERANCE_DEG_PER_S:
            self.reset()
            return "off"
        if self.window:
            if not (isinstance(substep, int) and substep == self.window[-1][2] + 1
                    and abs(effective - self.window[-1][1]) <= PLATFORM_IDENTITY_TOLERANCE_DEG):
                self.reset()
        self.window.append((float(time_ms), effective, substep))
        if len(self.window) > 4096:
            self.window = self.window[-2048:]
        if len(self.window) >= STABILITY_MIN_SUBSTEPS:
            span = self.window[-1][0] - self.window[0][0]
            tilts = [w[1] for w in self.window]
            if span >= STABILITY_WINDOW_MS and (max(tilts) - min(tilts)) <= PLATFORM_RANGE_TOLERANCE_DEG:
                return "stable"
        return "candidate"


def endpoint_planner_clear(edge, context):
    """s5.2: no planner clamp/hold/reject/rollback at the endpoint.
    thetaRequested must equal thetaAccepted (1e-9 deg); explicit
    plannerHold/requestRejected/rollback flags must be falsy when present.
    Missing theta fields mean UNVERIFIABLE -> not clear."""
    theta_req = context.get("thetaRequested")
    theta_acc = context.get("thetaAccepted")
    if not (finite(theta_req) and finite(theta_acc)):
        return False
    if abs(float(theta_req) - float(theta_acc)) > ANGLE_NUMERIC_TOLERANCE_DEG:
        return False
    for flag in ("plannerHold", "requestRejected", "rollback"):
        value = context.get(flag)
        if value is True:
            return False
    return True


def endpoint_clip_clear(edge):
    """s5.2: no budget/guard/stock/capacity clipping ambiguity at the
    endpoint. requested>granted marks budget clipping; explicit guard or
    stock/capacity flags count too. requested=0 & granted=0 is a real stop
    with budgetScale possibly null - NOT treated as limiting."""
    requested = edge.get("requested")
    granted = edge.get("granted")
    if finite(requested) and finite(granted):
        if float(requested) > 1e-12 and float(granted) + 1e-9 < float(requested):
            return False
    for flag in ("guardClipped", "stockLimited", "capacityClipped"):
        if edge.get(flag) is True:
            return False
    return True


def schedule_timing_gate(records, schedule_bindings):
    """s3.2 input-timing gate over ACTUAL ACCEPTED INTERVALS (drift
    consumption ruling 2026-10-21). Returns (summary, breach) where breach
    maps commandSequence -> list of breach codes for commands whose substeps
    lose bracket eligibility. scheduleDriftMs is summarized but NEVER judged:
    the >50ms marker is telemetry, not a censoring condition."""
    # First/last digested substep physicsTime per command sequence.
    first_sub = {}
    last_sub = {}
    drift_of = {}
    target_of = {}
    kind_of = {}
    for record in records:
        context = record.get("context", {})
        seq = context.get("commandSequence")
        if not (isinstance(seq, int) and context.get("scheduleDriftMs") is not None):
            continue
        t = context.get("physicsTime")
        if not finite(t):
            continue
        # Unit normalization: device ledgers carry wall-clock physicsTime in
        # ms (epoch-scale, >1e10); synthetic/Node 1:1 ledgers use seconds.
        # Normalize everything to ms.
        t = float(t) * 1000.0 if abs(float(t)) < 1e10 else float(t)
        if seq not in first_sub or t < first_sub[seq]:
            first_sub[seq] = t
        if seq not in last_sub or t > last_sub[seq]:
            last_sub[seq] = t
        drift_of.setdefault(seq, context.get("scheduleDriftMs"))
        target_of.setdefault(seq, context.get("targetTilt"))
        kind_of.setdefault(seq, context.get("phaseKindV3"))
    seqs = sorted(first_sub)

    # Angular step for coalescing span checks: prefer the manifest binding,
    # fall back to the observed minimum in-leg target delta.
    rate = None
    proto = None
    bindings = schedule_bindings if isinstance(schedule_bindings, dict) else {}
    observed_rate = bindings.get("rateDegPerSec")
    if finite(observed_rate) and float(observed_rate) in ANGULAR_STEP:
        rate = float(observed_rate)
    else:
        deltas = []
        for a, b in zip(seqs, seqs[1:]):
            if kind_of.get(a) == "ramp" and kind_of.get(a) == kind_of.get(b):
                ta, tb = target_of.get(a), target_of.get(b)
                if finite(ta) and finite(tb):
                    d = abs(float(tb) - float(ta))
                    if 1e-9 < d < 0.5:
                        deltas.append(d)
        if deltas:
            rate = min(ANGULAR_STEP, key=lambda r: abs(ANGULAR_STEP[r] - min(deltas)))
    angular_step = ANGULAR_STEP.get(rate, ANGULAR_STEP[RATE_PRIMARY_DEG_PER_S]) if rate else None

    drifts_all = sorted(drift_of[s] for s in seqs if drift_of.get(s) is not None)
    marker_count = 0
    for record in records:
        context = record.get("context", {})
        if context.get("scheduleDriftExceeded") is True:
            marker_count += 1

    # dt distribution summary (for the s4.1 dt-trajectory registration).
    dts = []
    for a, b in zip(seqs, seqs[1:]):
        pass
    prev_t = None
    for record in records:
        context = record.get("context", {})
        t = context.get("physicsTime")
        if finite(t):
            t = float(t) * 1000.0 if abs(float(t)) < 1e10 else float(t)  # -> ms
            if prev_t is not None and t > prev_t:
                dts.append(round(t - prev_t, 1))
            prev_t = t

    breach = {}
    ramp_gaps = []
    hold_gaps = []
    for a, b in zip(seqs, seqs[1:]):
        gap = first_sub[b] - first_sub[a]
        kind_a = kind_of.get(a)
        kind_b = kind_of.get(b)
        codes = []
        if kind_a == "ramp" and kind_b == "ramp":
            ramp_gaps.append(gap)
            band = SCHEDULE_TIMING["rampRamp"]
            if gap < SCHEDULE_TIMING["catchUpFloorMs"]:
                codes.append(SCHEDULE_INTERVAL_CODE + ":catchUp")
            elif not (band["minMs"] <= gap <= band["maxMs"]):
                codes.append(SCHEDULE_INTERVAL_CODE)
            # coalescing span: target delta within one leg must be
            # <= multiplier * angular step (1 skipped command allowed).
            if angular_step:
                ta, tb = target_of.get(a), target_of.get(b)
                if finite(ta) and finite(tb):
                    steps = abs(float(tb) - float(ta)) / angular_step
                    if steps > SCHEDULE_TIMING["maxAngularStepMultiplier"] + 1e-9:
                        codes.append(SCHEDULE_COMMAND_GAP_CODE)
        elif kind_a == "platform" and kind_b == "ramp":
            hold_gaps.append(gap)
            band = SCHEDULE_TIMING["platformRamp"]
            if not (band["minMs"] <= gap <= band["maxMs"]):
                codes.append(SCHEDULE_INTERVAL_CODE + ":hold")
        if codes:
            breach[b] = codes

    def pct(vals, p):
        if not vals:
            return None
        v = sorted(vals)
        return round(v[min(int(len(v) * p), len(v) - 1)], 1)

    summary = {
        "digestedCommands": len(seqs),
        "driftMs": {"p50": pct(drifts_all, 0.5), "p95": pct(drifts_all, 0.95),
                    "max": drifts_all[-1] if drifts_all else None},
        "driftMarkerRecords": marker_count,
        "acceptedIntervalRampMs": {"p50": pct(ramp_gaps, 0.5), "p95": pct(ramp_gaps, 0.95),
                                   "min": min(ramp_gaps) if ramp_gaps else None,
                                   "max": max(ramp_gaps) if ramp_gaps else None,
                                   "n": len(ramp_gaps)},
        "acceptedIntervalHoldMs": {"p50": pct(hold_gaps, 0.5),
                                   "min": min(hold_gaps) if hold_gaps else None,
                                   "max": max(hold_gaps) if hold_gaps else None,
                                   "n": len(hold_gaps)},
        "dtIntervalMs": {"p50": pct(dts, 0.5), "p95": pct(dts, 0.95),
                         "over40msRatio": round(sum(1 for d in dts if d > 40.0) / len(dts), 4) if dts else None},
        "breachedCommands": sum(1 for codes in breach.values() if codes),
        "gate": "actualAcceptedInterval",
        "driftRole": "telemetryOnly",
    }
    return summary, breach


def three_point_monotone(window_alphas, kind):
    """s5.2: endpoints + the immediately preceding same-leg decision must be
    monotone in the event direction (start non-decreasing, stop
    non-increasing; equality allowed). Window is exactly the three adjacent
    records - no forward search for favorable endpoints."""
    if len(window_alphas) < 3:
        return False
    a0, a1, a2 = window_alphas[-3:]
    if any(a is None for a in (a0, a1, a2)):
        return False
    if kind == "start":
        return a0 <= a1 <= a2
    return a0 >= a1 >= a2


def analyze(ledger_path, manifest_path, source="pilot"):
    manifest = read_json(manifest_path) if manifest_path else {}
    records = list(read_records(ledger_path))
    protocol_errors = []
    notes = []

    protocol = manifest.get("calibrationProtocol", {}) if isinstance(manifest, dict) else {}
    if protocol.get("protocolId") != PROTOCOL_ID:
        protocol_errors.append("protocolIdMissingOrMismatch")
    if protocol.get("protocolRevision") != PROTOCOL_REVISION:
        protocol_errors.append("protocolRevisionMissingOrMismatch")
    schedule_hash = protocol.get("scheduleSha256")
    if not isinstance(schedule_hash, str) or len(schedule_hash) != 64:
        protocol_errors.append("scheduleSha256Missing")
    configured = protocol.get("configuredThresholds")
    if not isinstance(configured, dict) or not (finite(configured.get("thetaStartDeg"))
                                                and finite(configured.get("thetaStopDeg"))):
        protocol_errors.append("configuredThresholdsMissing")
        configured = {}
    else:
        # s1 search window: start [25,30), stop [15,25], stop<start.
        start_c, stop_c = float(configured["thetaStartDeg"]), float(configured["thetaStopDeg"])
        if not (25.0 <= start_c < 30.0 and 15.0 <= stop_c <= 25.0 and stop_c < start_c):
            protocol_errors.append("configuredThresholdsOutsideSearchWindow")

    first_context = records[0].get("context", {}) if records else {}
    if not (first_context.get("runRole") == "calibration"
            and first_context.get("observationRouteId") == "natural-relax-hysteresis-v1"):
        protocol_errors.append("notObservation3CalibrationRun")

    # ---- s2 executed threshold identity (per run) ----
    identity_ok, identity_reason = executed_identity_ok(first_context, manifest)
    identity_mismatch = not identity_ok
    if identity_mismatch:
        notes.append(f"executionIdentity:{identity_reason}")

    # ---- schedule bindings for leg identity (s3) ----
    schedule_bindings = protocol.get("commandBindings") or manifest.get("commandBindings") or {}

    # ---- Pass 1: platform windows & telemetry completeness ----
    window = PlatformWindow()
    telemetry_incomplete_records = 0
    platform_reach = {k: {"stableRecords": 0, "firstStableMs": None,
                          "acceptedMin": None, "acceptedMax": None,
                          "plannerLimitedStableRecords": 0}
                      for k in ("15", "30", "-15", "-30")}
    substep_states = []
    hard_gate_failed = False
    for record in records:
        context = record.get("context", {})
        audit = record.get("audit", {})
        code = audit.get("errorCode")
        if code not in (None, 0, "0", 5, "5", "TILT_PLANNING_BLOCKED"):
            hard_gate_failed = True
        theta_req = context.get("thetaRequested")
        theta_acc = context.get("thetaAccepted")
        consistent = None
        if theta_acc is None:
            consistent = None
        elif finite(theta_acc) and finite(context.get("effectiveTilt")):
            consistent = abs(float(theta_acc) - float(context["effectiveTilt"])) <= ANGLE_NUMERIC_TOLERANCE_DEG
        else:
            consistent = False
        if consistent is None:
            telemetry_incomplete_records += 1
        effective = context.get("effectiveTilt")
        rate = context.get("effectiveTiltRate")
        time_ms = float(context.get("physicsTime", 0.0)) * 1000.0 if finite(context.get("physicsTime")) else None
        verdict = window.feed(effective, rate, time_ms or 0.0,
                              context.get("substepId"), consistent is True)
        accepted = float(effective) if finite(effective) else None
        target = context.get("targetTilt")
        key = None
        if finite(target) and abs(float(target)) > ANGLE_NUMERIC_TOLERANCE_DEG and abs(float(target) - round(float(target) / 15.0) * 15.0) <= PLATFORM_IDENTITY_TOLERANCE_DEG:
            key = str(int(round(float(target))))
        state = {"stable": verdict == "stable", "accepted": accepted,
                 "sign": ("+" if accepted > 0 else "-") if accepted is not None and accepted != 0 else None,
                 "legId": None, "phaseKind": None}
        leg_id, phase_kind = leg_from_schedule(context, schedule_bindings)
        state["legId"] = leg_id
        state["phaseKind"] = phase_kind
        substep_states.append(state)
        if key and verdict in ("stable", "candidate"):
            reach = platform_reach.get(key)
            if reach is not None:
                if verdict == "stable":
                    reach["stableRecords"] += 1
                    if reach["firstStableMs"] is None:
                        reach["firstStableMs"] = time_ms
                    reach["acceptedMin"] = accepted if reach["acceptedMin"] is None else min(reach["acceptedMin"], accepted)
                    reach["acceptedMax"] = accepted if reach["acceptedMax"] is None else max(reach["acceptedMax"], accepted)
                    if theta_req is None or theta_acc is None or not (
                            finite(theta_req) and finite(theta_acc)
                            and abs(float(theta_req) - float(theta_acc)) <= ANGLE_NUMERIC_TOLERANCE_DEG):
                        reach["plannerLimitedStableRecords"] += 1

    # ---- s3.2 input-timing gate over actual accepted intervals ----
    schedule_timing, timing_breach = schedule_timing_gate(records, schedule_bindings)
    breach_by_seq = timing_breach

    # ---- Ruling 2: post-schedule tail truncation ----
    # scheduleEndMs = first command dispatch + theoretical duration
    # T = 9*5100ms + 4*A/v*1000 (A/v from the manifest schedule binding,
    # fallback to the observed peak); tolerance covers last-command drift.
    first_dispatch_ms = None
    for record in records:
        context = record.get("context", {})
        if context.get("commandSequence") == 1 and finite(context.get("physicsTime")):
            t = float(context["physicsTime"])
            first_dispatch_ms = t if abs(t) > 1e10 else t * 1000.0
            break
    peak_a = bindings_rate = None
    proto_cfg = manifest.get("calibrationProtocol", {}) if isinstance(manifest, dict) else {}
    peak_a = proto_cfg.get("peakADeg")
    if not finite(peak_a):
        peak_a = manifest.get("peakADeg")
    if not finite(peak_a):
        # derive from observed |target| maximum inside ramp legs
        mx = 0.0
        for record in records:
            context = record.get("context", {})
            if context.get("phaseKindV3") == "ramp" and finite(context.get("targetTilt")):
                mx = max(mx, abs(float(context["targetTilt"])))
        peak_a = mx if mx > 0 else None
    bindings_rate = proto_cfg.get("rateDegPerSec") or manifest.get("rateDegPerSec")
    if not finite(bindings_rate):
        bindings_rate = RATE_PRIMARY_DEG_PER_S
    schedule_end_ms = None
    if first_dispatch_ms is not None and finite(peak_a) and float(peak_a) > 0:
        theory_s = 9 * (PLATFORM_HOLD_MS / 1000.0) + 4 * float(peak_a) / float(bindings_rate)
        schedule_end_ms = first_dispatch_ms + theory_s * 1000.0
    post_schedule_count = 0

    # ---- Pass 2: state samples, cells, brackets (s5) ----
    cells = {key: {"stateSamples": 0, "stateRuns": set(), "realTransitions": 0,
                   "eligibleBrackets": 0, "cappedTransitions": 0,
                   "perRunTransitions": {}, "censorCodes": [],
                   "stopSubcode": None, "atRiskAlphaMax": None,
                   "exclusions": {}, "brackets": []}
             for key in MAIN_CELLS}
    neutral_states = 0
    first_bracket_keys = set()
    prior_state = {}
    prior_decisions = {}  # (runId, c, legId) -> deque of last alphas (3-point window)
    wide_events = 0
    nonmonotone_events = 0
    continuity_missing = 0

    for record, state in zip(records, substep_states):
        context = record.get("context", {})
        run_id = context.get("runId")
        substep_id = context.get("substepId")
        segment_id = state.get("legId") or context.get("segmentId")
        # Ruling 2: post-schedule tail exclusion (diagnostics kept).
        is_post_schedule = False
        if schedule_end_ms is not None and finite(context.get("physicsTime")):
            t = float(context["physicsTime"])
            t_ms = t if abs(t) > 1e10 else t * 1000.0
            is_post_schedule = t_ms > schedule_end_ms + POST_SCHEDULE_TOLERANCE_MS
        if is_post_schedule:
            post_schedule_count += 1
        phase_kind = state.get("phaseKind") or context.get("phaseKindV3")
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
            if not finite(interface):
                continue
            # Edge identity: interfaceIndex is the primary key (protocol s5.1
            # "same edge uses interfaceIndex"). donor/receiver are optional
            # in ledger schema v3 (v4 pilot emits None); when present they
            # must be adjacent and consistent with the fixed-edge identity.
            if finite(donor) and finite(receiver):
                if abs(float(donor) - float(receiver)) != 1:
                    continue
                if float(interface) != min(float(donor), float(receiver)):
                    continue  # directional donor index misused as edge identity
            active_before = edge.get("activeBefore")
            active_after = edge.get("activeAfter")
            if not isinstance(active_before, bool):
                continue
            accepted = state["accepted"]
            # Ruling 1 cell attribution: stable-platform segments keep the
            # observed accepted sign; ramp legs take the PREREGISTERED leg
            # sign (s3.1 table) - no cross-zero split inside one leg.
            if state["stable"]:
                cell_sign = state["sign"]
            elif phase_kind == "ramp" and segment_id in RAMP_LEG_SIGN:
                cell_sign = RAMP_LEG_SIGN[segment_id]
                # a ramp record whose observed sign contradicts the
                # preregistered leg sign is excluded (reserved code)
                if accepted is not None and accepted != 0 \
                        and ("+" if accepted > 0 else "-") != cell_sign:
                    cell_sign = None  # falls to neutral; transition excluded below
            else:
                cell_sign = None
            alpha = decision_alpha(edge.get("r"))
            kind_state = "start" if not active_before else "stop"
            if cell_sign:
                cell = cells[f"{kind_state}/{cell_sign}"]
                cell["stateSamples"] += 1
                cell["stateRuns"].add(run_id)
                # at-risk alpha maximum: start risk state = inactive, stop
                # risk state = active (s7); both cells track their own state.
                if alpha is not None:
                    cell["atRiskAlphaMax"] = alpha if cell["atRiskAlphaMax"] is None else max(cell["atRiskAlphaMax"], alpha)
            else:
                neutral_states += 1
            if not isinstance(active_after, bool):
                continue
            # Per-edge tracking: EVERY edge record updates both the prior
            # state and the same-leg decision window (not only transitions),
            # so bracket continuity and the 3-point window can span
            # arbitrarily many committed non-transition substeps.
            identity = (run_id, float(interface))
            previous = prior_state.get(identity)
            prior_state[identity] = (substep_id, active_before, active_after, alpha, state["stable"])
            window_key = (run_id, float(interface), segment_id)
            window_alphas = prior_decisions.setdefault(window_key, [])
            window_alphas.append(alpha)
            if len(window_alphas) > 3:
                del window_alphas[0]
            is_real_start = (not active_before) and active_after
            is_real_stop = active_before and (not active_after)
            if not (is_real_start or is_real_stop):
                continue
            kind = "start" if is_real_start else "stop"
            # Ruling 1: transitions without an attributable cell (neutral
            # zero, sign mismatch, or missing leg identity) are recorded in
            # the nearest main cell's exclusion bookkeeping instead of a
            # silent skip, so the denominator is auditable.
            if cell_sign is None:
                if is_post_schedule:
                    target_cell = cells[f"{kind}/+"]
                    target_cell["exclusions"][CODE_POST_SCHEDULE] = \
                        target_cell["exclusions"].get(CODE_POST_SCHEDULE, 0) + 1
                elif phase_kind == "ramp":
                    # ramp record: distinguish missing identity vs sign issue
                    anchor = cells[f"{kind}/+"]
                    if not segment_id:
                        anchor["exclusions"]["SEGMENT_IDENTITY_MISSING"] = \
                            anchor["exclusions"].get("SEGMENT_IDENTITY_MISSING", 0) + 1
                    elif segment_id in RAMP_LEG_SIGN:
                        anchor["exclusions"][CODE_RAMP_SIGN_MISMATCH] = \
                            anchor["exclusions"].get(CODE_RAMP_SIGN_MISMATCH, 0) + 1
                    else:
                        anchor["exclusions"]["NEUTRAL_ZERO_CROSSING"] = \
                            anchor["exclusions"].get("NEUTRAL_ZERO_CROSSING", 0) + 1
                else:
                    neutral_states += 1
                continue
            cell_key = f"{kind}/{cell_sign}"
            cell = cells[cell_key]
            # Ruling 2: post-schedule transitions never enter the cells.
            if is_post_schedule:
                cell["exclusions"][CODE_POST_SCHEDULE] = \
                    cell["exclusions"].get(CODE_POST_SCHEDULE, 0) + 1
                continue
            cell["realTransitions"] += 1
            run_count = cell["perRunTransitions"].setdefault(run_id, 0)
            capped = run_count >= STATS["maxStatesPerRunPerCell"]
            if capped:
                cell["cappedTransitions"] += 1
            else:
                cell["perRunTransitions"][run_id] = run_count + 1
            # s5.2 fixed exclusion order: identity/transaction -> segment/sign
            # -> transition/width. All hit reasons retained.
            reasons = []
            # Input-timing gate (s3.2 ruling): a bracket endpoint substep
            # driven by a schedule command that breached the accepted-interval
            # band or the coalescing-span cap loses eligibility. The drift
            # marker itself is NOT a reason (telemetry only).
            seq_breaches = breach_by_seq.get(context.get("commandSequence"))
            if seq_breaches:
                for code in seq_breaches:
                    reasons.append(code)
            if previous is None:
                reasons.append("CONTINUITY_MISSING")
            else:
                prev_substep, prev_before, prev_after, prev_alpha, prev_stable = previous
                if not (isinstance(substep_id, int) and prev_substep == substep_id - 1):
                    reasons.append("CONTINUITY_MISSING")
                if prev_after != active_before:
                    reasons.append("CONTINUITY_MISSING")
                if not prev_stable:
                    reasons.append("PREV_ENDPOINT_NOT_ELIGIBLE")
            if not state["stable"]:
                # v3 allows ramp legs as eligible segments; "stable" here only
                # marks stable-platform segments. Ramp eligibility is granted
                # via leg identity + committed transaction, so absence of a
                # platform does not disqualify; only missing leg identity does.
                if not segment_id:
                    reasons.append("SEGMENT_IDENTITY_MISSING")
            if not segment_id:
                reasons.append("SEGMENT_IDENTITY_MISSING")
            if not endpoint_planner_clear(edge, context):
                reasons.append("PLANNER_HOLD_OR_CLAMP")
            if not endpoint_clip_clear(edge):
                reasons.append("BUDGET_GUARD_OR_STOCK_LIMITED")
            executed = context.get("executedThresholds") or {}
            if kind == "start":
                threshold = float(executed.get("thetaStartDeg", configured.get("thetaStartDeg", float("nan"))))
                if not (finite(previous[3] if previous else None) and False):
                    pass
                if previous and previous[3] is not None and alpha is not None:
                    if not (previous[3] < threshold <= alpha):
                        reasons.append("START_NOT_CROSSED")
                cross_ok = bool(previous and previous[3] is not None and alpha is not None
                                and previous[3] < threshold <= alpha)
            else:
                threshold = float(executed.get("thetaStopDeg", configured.get("thetaStopDeg", float("nan"))))
                if previous and previous[3] is not None and alpha is not None:
                    if not (previous[3] > threshold >= alpha):
                        reasons.append("STOP_NOT_CROSSED")
                cross_ok = bool(previous and previous[3] is not None and alpha is not None
                                and previous[3] > threshold >= alpha)
            width_ok = False
            if previous and previous[3] is not None and alpha is not None:
                width = abs(alpha - previous[3])
                if width > BRACKET_MAX_WIDTH_DEG:
                    reasons.append("BRACKET_TOO_WIDE")
                    wide_events += 1
                else:
                    width_ok = True
            if not three_point_monotone(window_alphas[:-1] + [alpha], kind):
                reasons.append("NONMONOTONE_LOCAL_WINDOW")
                nonmonotone_events += 1
            for reason in reasons:
                cell["exclusions"][reason] = cell["exclusions"].get(reason, 0) + 1
            if "CONTINUITY_MISSING" in reasons:
                continuity_missing += 1
            if reasons:
                continue
            if capped:
                # s5.2 cap: the 4th+ transition per run per main cell keeps
                # raw audit but never enters estimation/first-bracket bookkeeping.
                continue
            cell["eligibleBrackets"] += 1
            bracket_key = (run_id, segment_id, float(interface))
            if bracket_key not in first_bracket_keys:
                first_bracket_keys.add(bracket_key)
                cell["brackets"].append({
                    "runId": run_id, "segmentId": segment_id,
                    "fixedEdge": float(interface), "substepId": substep_id,
                    "alphaPrevDeg": previous[3], "alphaNowDeg": alpha,
                    "widthDeg": abs(alpha - previous[3]),
                    "thresholdUsedDeg": threshold,
                })

    # ---- s6/s7 coverage matrix (pilot source) ----
    matrix_rows = []
    for main_key in MAIN_CELLS:
        cell = cells[main_key]
        required_states = STATS["mainCellMinStateSamples"]
        state_count = cell["stateSamples"]
        rows = [{
            "cellId": main_key, "parentCell": None,
            "residualSign": None, "noiseBand": None,
            "atRiskPresent": state_count > 0,
            "atRiskRecords": state_count,
            "distinctRuns": len(cell["stateRuns"]),
            "alphaMaxDeg": cell["atRiskAlphaMax"],
            "trueEventCount": cell["realTransitions"],
            "eligibleBracketCount": cell["eligibleBrackets"],
            "stateSampleCount": state_count,
            "cappedCount": cell["cappedTransitions"],
            "runCount": len(cell["stateRuns"]),
            "required": required_states,
            "deficit": max(0, required_states - state_count),
            "noiseDefinable": False,
            "wNoiseSource": "UNDEFINED_NO_FIT_NOISE",
            "exclusionCounts": cell["exclusions"],
            "coverageStatus": ("OBSERVED" if cell["eligibleBrackets"] > 0
                               else ("NOT_OBSERVED" if state_count > 0 else "UNKNOWN")),
            "structuralAssessment": None,
            "source": source,
        }]
        # 24 sub cells: residualSign x noiseBand placeholders (noise undefined
        # without fit); rows retained even at zero records.
        for rsign in ("d<0", "d>0"):
            for band in ("near", "shoulder", "far"):
                rows.append({
                    "cellId": f"{main_key}|{rsign}|{band}", "parentCell": main_key,
                    "residualSign": rsign, "noiseBand": band,
                    "atRiskPresent": False, "atRiskRecords": 0,
                    "distinctRuns": 0, "alphaMaxDeg": None,
                    "trueEventCount": 0, "eligibleBracketCount": 0,
                    "stateSampleCount": 0, "cappedCount": 0, "runCount": 0,
                    "required": STATS["subCellMinStateSamples"], "deficit": STATS["subCellMinStateSamples"],
                    "noiseDefinable": False, "wNoiseSource": "UNDEFINED_NO_FIT_NOISE",
                    "exclusionCounts": {}, "coverageStatus": "UNDEFINED",
                    "structuralAssessment": None, "source": source,
                })
        matrix_rows.extend(rows)

    # ---- s8 censoring codes per cell ----
    for key, cell in cells.items():
        codes = []
        sign = key.split("/")[1]
        kind = key.split("/")[0]
        big = platform_reach["30" if sign == "+" else "-30"]
        if hard_gate_failed:
            codes.append(CODE_RUN_INVALID_HARD_GATE)
        if identity_mismatch:
            codes.append(CODE_EXECUTION_IDENTITY_MISMATCH)
        if telemetry_incomplete_records > 0:
            codes.append(CODE_TELEMETRY_INCOMPLETE)
        if kind == "stop":
            if cell["realTransitions"] > 0 and cell["eligibleBrackets"] == 0:
                codes.append(CODE_STOP_TRANSITION_UNOBSERVED)
                exclusions = cell["exclusions"]
                if exclusions.get("BRACKET_TOO_WIDE"):
                    cell["stopSubcode"] = "BRACKET_TOO_WIDE"
                elif exclusions.get("NONMONOTONE_LOCAL_WINDOW"):
                    cell["stopSubcode"] = "NONMONOTONE_LOCAL_WINDOW"
                elif exclusions.get("CONTINUITY_MISSING"):
                    cell["stopSubcode"] = "CONTINUITY_MISSING"
                elif exclusions.get("PLANNER_HOLD_OR_CLAMP"):
                    cell["stopSubcode"] = "ONLY_PLANNER_HOLD_OR_CLAMP"
                elif exclusions.get("BUDGET_GUARD_OR_STOCK_LIMITED"):
                    cell["stopSubcode"] = "ONLY_BUDGET_GUARD_OR_STOCK_LIMITED"
                elif cell["stateSamples"] == 0:
                    cell["stopSubcode"] = "NEVER_ACTIVE"
                else:
                    cell["stopSubcode"] = "STILL_ACTIVE_AT_END"
            elif cell["stateSamples"] == 0 and big["stableRecords"] == 0:
                codes.append(CODE_STOP_TRANSITION_UNOBSERVED)
                cell["stopSubcode"] = "NEVER_ACTIVE"
        else:
            if cell["realTransitions"] > 0 and cell["eligibleBrackets"] == 0:
                codes.append(CODE_START_OUTSIDE_ELIGIBLE_WINDOW)
        cell["censorCodes"] = codes

    # s8 v3 right-censoring: four conditions must ALL hold; platform arrival
    # is NOT required (ramp exposure qualifies). Conditions checked per start
    # cell: (1) identity true + inactive risk state + complete legal committed
    # segment + auditable alpha upper bound below candidate start + no
    # qualified crossing start; (2) next planner/guard constraint evidence is
    # explicitly the tan30 boundary (attempt/constraint/limitingEdge fields);
    # (3) rejected attempts are not observation endpoints; (4) leg identity
    # and sign complete. Condition (2) requires ledger evidence fields the
    # current schema may not carry - absence means the code is NOT assigned.
    for key in ("start/+", "start/-"):
        cell = cells[key]
        if key in ("start/+", "start/-") and cell["eligibleBrackets"] == 0:
            tan30_bound_evidence = any(
                (record.get("audit", {}) or {}).get("limitingReason") == "tan30"
                or (record.get("audit", {}) or {}).get("plannerBound") == "tan30"
                for record in records)
            if (not identity_mismatch and not hard_gate_failed
                    and cell["atRiskAlphaMax"] is not None
                    and cell["atRiskAlphaMax"] < float(configured.get("thetaStartDeg", 30.0))
                    and tan30_bound_evidence):
                cell["censorCodes"].append(CODE_RIGHT_CENSORED_AT_TAN30)

    # ---- s8 pilot three-dimension verdict ----
    clean_pass = (not protocol_errors) and not hard_gate_failed and not identity_mismatch
    # Ruling (pre-expansion): reachability is judged over platforms the
    # schedule ACTUALLY contains. A < 30 deg means the +/-30 platforms do
    # not exist by design (protocol s3.1 "A strictly < 30"), so their
    # absence is NOT_IN_SCHEDULE, not a FAIL.
    schedule_peak = peak_a if finite(peak_a) else None
    required_platforms = ["15", "-15"]
    not_in_schedule = []
    if schedule_peak is not None and float(schedule_peak) >= 30.0:
        required_platforms += ["30", "-30"]
    else:
        not_in_schedule = ["30", "-30"]
    reach_pass = all(platform_reach[k]["stableRecords"] > 0
                     and platform_reach[k]["plannerLimitedStableRecords"] == 0
                     for k in required_platforms)
    coverage = ("FOUR_CELL_OBSERVED"
                if all(cells[k]["eligibleBrackets"] >= 1 for k in MAIN_CELLS)
                else "INCOMPLETE")

    result = {
        "schemaVersion": 3,
        "testCase": "P011-OBS3-V3-PROTOCOL-GATE",
        "protocolId": PROTOCOL_ID,
        "protocolRevision": PROTOCOL_REVISION,
        "analysisRole": "pilot",
        "overall": "INVALID" if (hard_gate_failed or protocol_errors or identity_mismatch) else "PILOT_REPORT",
        "thresholdVerdict": "NOT_EVALUATED",
        "observedStartTransition": None,
        "observedStopTransition": None,
        "thetaStartEstimate": None,
        "thetaStopEstimate": None,
        "thetaStartCI": None,
        "thetaStopCI": None,
        "safetyMustPass": "UNCHANGED_AND_CHECKED_BY_EXISTING_SAFETY_CHECKERS",
        "protocolMetadataErrors": protocol_errors,
        "executionThresholdIdentity": {
            "ok": not identity_mismatch,
            "reason": identity_reason,
            "mismatchCode": CODE_EXECUTION_IDENTITY_MISMATCH if identity_mismatch else None,
        },
        "telemetryIncompleteRecords": telemetry_incomplete_records,
        "runInvalidHardGate": hard_gate_failed,
        "platformReachability": {
            "verdict": "PASS" if reach_pass else "FAIL",
            "perPhase": platform_reach,
            "requiredPlatforms": required_platforms,
            "notInSchedule": not_in_schedule,
            "note": "diagnostic only; v3 does not require platforms for start events; +/-30 required only when schedule peak A >= 30 deg (pre-expansion ruling)",
        },
        "postSchedule": {
            "scheduleEndMs": schedule_end_ms,
            "toleranceMs": POST_SCHEDULE_TOLERANCE_MS,
            "excludedRecords": post_schedule_count,
            "code": CODE_POST_SCHEDULE,
        },
        "pilotCoverage": coverage,
        "scheduleTiming": schedule_timing,
        "cells": {
            key: {
                "stateSamples": cell["stateSamples"],
                "stateRunCount": len(cell["stateRuns"]),
                "realTransitions": cell["realTransitions"],
                "eligibleBrackets": cell["eligibleBrackets"],
                "cappedTransitions": cell["cappedTransitions"],
                "perRunTransitions": dict(sorted(cell["perRunTransitions"].items())),
                "atRiskAlphaMaxDeg": cell["atRiskAlphaMax"],
                "censorCodes": cell["censorCodes"],
                "stopUnobservedSubcode": cell["stopSubcode"],
                "exclusionCounts": cell["exclusions"],
                "firstBrackets": cell["brackets"][:8],
            }
            for key, cell in cells.items()
        },
        "coverageMatrix": matrix_rows,
        "statsGates": STATS,
        "codeCatalog": {
            "RIGHT_CENSORED_AT_TAN30": "v3: four conditions all required; platform arrival not required; ramp exposure qualifies; never a calibration PASS",
            "STOP_TRANSITION_UNOBSERVED": "nine subcodes: " + ", ".join(STOP_SUBCODES),
            "START_OUTSIDE_ELIGIBLE_WINDOW": "start only outside eligible ramp/platform segments",
            "PLATFORM_UNREACHED": "diagnostic; platforms not mandatory for starts in v3",
            "PLATFORM_UNSTABLE": "arrived but window never qualified",
            "TELEMETRY_INCOMPLETE": "missing decision/planner/continuity evidence; unknown is not no-transition",
            "RUN_INVALID_HARD_GATE": "whole run quarantined",
            "EXECUTION_THRESHOLD_IDENTITY_MISMATCH": "configured/executed/state-machine identity failure + UNOBSERVABLE",
            "EXECUTION_ENVELOPE_UNRESOLVED": "no peak A on the frozen grid satisfies SHADOW_SAFE && SHADOW_COVERED",
            "SHADOW_PREDICTED_CENSOR": "shadow-visible censoring; never substitutes real-run exposure evidence",
            "BRACKET_TOO_WIDE": "width > 0.5 deg; wide events recorded, never split",
            "NONMONOTONE_LOCAL_WINDOW": "3-point window fails event-direction monotonicity",
        },
        "frozenConstants": {
            "peakGrid": {"min": PEAK_GRID[0], "max": PEAK_GRID[-1], "points": len(PEAK_GRID)},
            "legs": LEGS,
            "commandStepMs": COMMAND_STEP_MS,
            "angularStepByRate": ANGULAR_STEP,
            "bracketMaxWidthDeg": BRACKET_MAX_WIDTH_DEG,
        },
        "notes": notes,
    }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, default=None)
    parser.add_argument("--source", choices=["pilot", "shadow"], default="pilot")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = analyze(args.ledger, args.manifest, source=args.source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({k: result[k] for k in (
        "overall", "thresholdVerdict", "pilotCoverage",
        "platformReachability")}, ensure_ascii=False, default=str))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
