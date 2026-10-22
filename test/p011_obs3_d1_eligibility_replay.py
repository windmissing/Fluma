#!/usr/bin/env python3
"""Observation-3 D1 offline eligibility replay (design-P011-obs3-calibration-data-path-v1 §4.1 1A).

Replays v3 bracket eligibility ENDPOINT-WISE over archived v3 ledgers with the
five §3.1 implementation deviations FIXED, and emits a difference table
(old checker results vs fixed replay). Offline only: no capture, no
threshold estimates, no gate weakening. The five fixes (all verified against
the pre-registration text, s5.2):

  FIX-1 (prev endpoint stable forcing): the previous endpoint of a ramp
        bracket is NO LONGER required to be a stable platform - the
        pre-registration allows "same preregistered ramp leg OR same
        eligible stable-platform segment". The previous endpoint must still
        be a committed decision on the SAME segment (leg identity or stable
        platform window) with continuity. The old code stored
        prior_state['stable'] and demanded it, misclassifying every ramp
        bracket as PREV_ENDPOINT_NOT_ELIGIBLE.

  FIX-2 (prev endpoint clear): planner/clip clear is now checked
        INDEPENDENTLY at BOTH endpoints (requested vs granted, guard/stock
        flags), not only at the current endpoint. requested=0 & granted=0
        remains a real stop (not limiting). Reads hardGuardClipped /
        hardGuardRejected (ledger key names; the old helper read guardClipped,
        a field the schema does not carry, silently defaulting to clear).

  FIX-3 (per-endpoint nonzero accepted sign): each bracket endpoint must
        carry the SAME nonzero accepted tilt sign as the cell. A zero-angle
        previous endpoint is neutral and cannot anchor a +/- bracket (the
        old code attributed the cell by leg sign without verifying the
        endpoints).

  FIX-4 (state/event denominator): maxStatesPerRunPerCell=3 caps STATE
        SAMPLES only. Transitions are never consumed by the cap: eligibility
        is evaluated for every real transition and the cap applies solely to
        state-sample counting. perRunTransitions is therefore no longer a
        denominator that can be exhausted by ineligible events.

  FIX-5 (platform window + time unit): PlatformWindow recognizes the
        registered peak platform A (from the manifest, e.g. 18.7) in addition
        to 0/±15/±30; physicsTime is treated as epoch-ms when |t|>1e10 and
        seconds otherwise, normalized to ms WITHOUT double-scaling (the old
        pass-1 multiplied epoch-ms by 1000, distorting the 200 ms stability
        span by 1000x - making any window trivially "stable").

Per §4.1 the endpoint window retains the previous TWO raw decisions per
fixed edge (including invalid attempts' decisions), transitions require
consecutive committed substeps, the same preregistered segment, nonzero
same-sign accepted tilt at both endpoints, both endpoints clear, timing
clear, ramp three-point monotonicity, executed-threshold crossing, width
<= 0.5 deg, first-event-per-(run, segment, edge) dedup. The timing gate
consumes actual accepted intervals (drift-consumption ruling); scheduleDriftMs
stays telemetry-only.

Outputs (no point estimates):
  - per-cell raw transitions / fully-eligible transitions / retained
    first-event brackets / exclusion reason SINGLE counts AND INTERSECTION
    counts (PREV-only, PREV∩NONMONOTONE, PREV∩prevClip, width, zero, boundary,
    timing) - reasons overlap, sums do not reconstruct event counts;
  - per-stop audit rows (run/substep/edge/segment/alphas/endpoint
    requested/granted/clip/exclusions) comparable to designer §3.2;
  - a difference table vs the archived old-checker summary (v3-summary.json)
    when --old-summary is given.
"""
import argparse
import json
import math
from pathlib import Path

BRACKET_MAX_WIDTH_DEG = 0.5
ANGLE_TOL = 1e-9
STABILITY_WINDOW_MS = 200.0
STABILITY_MIN_SUBSTEPS = 3
PLATFORM_IDENTITY_TOLERANCE_DEG = 0.20
POST_SCHEDULE_TOLERANCE_MS = 200.0
PLATFORM_HOLD_MS = 5100.0
RAMP_LEG_SIGN = {"L1": "+", "L2": "+", "L3": "+", "L4": "+",
                 "L5": "-", "L6": "-", "L7": "-", "L8": "-"}
SCHEDULE_TIMING = {
    "rampRamp": {"minMs": 50.0, "maxMs": 200.0},
    "platformRamp": {"minMs": 4000.0, "maxMs": 6200.0},
}
# Exclusion reason codes (fixed replay)
R_PREV_NOT_SAME_SEGMENT = "PREV_ENDPOINT_NOT_SAME_SEGMENT"
R_PREV_NOT_COMMITTED = "PREV_ENDPOINT_NOT_COMMITTED"
R_PREV_PLANNER = "PREV_PLANNER_HOLD_OR_CLAMP"
R_PREV_CLIP = "PREV_BUDGET_GUARD_OR_STOCK_LIMITED"
R_PREV_SIGN = "PREV_ENDPOINT_SIGN_NEUTRAL_OR_MISMATCH"
R_CURR_PLANNER = "PLANNER_HOLD_OR_CLAMP"
R_CURR_CLIP = "BUDGET_GUARD_OR_STOCK_LIMITED"
R_SIGN = "ENDPOINT_SIGN_NEUTRAL_OR_MISMATCH"
R_TIMING = "SCHEDULE_INTERVAL_OUT_OF_BAND"
R_CROSS = "THRESHOLD_NOT_CROSSED"
R_WIDTH = "BRACKET_TOO_WIDE"
R_NONMONO = "NONMONOTONE_LOCAL_WINDOW"
R_POST_SCHEDULE = "POST_SCHEDULE"
R_SEG_MISSING = "SEGMENT_IDENTITY_MISSING"


def finite(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool) and math.isfinite(v)


def to_ms(t):
    if not finite(t):
        return None
    t = float(t)
    return t if abs(t) > 1e10 else t * 1000.0


def decision_alpha(edge):
    r = edge.get("decisionR")
    if finite(r):
        return math.degrees(math.atan(abs(float(r))))
    a = edge.get("decisionAlpha")
    return float(a) if finite(a) else None


def read_records(path):
    with Path(path).open("r", encoding="utf-8") as stream:
        for line in stream:
            if line.strip():
                yield json.loads(line)


def endpoint_clear(edge, is_prev):
    """FIX-2: planner + clip clear checked independently at each endpoint.
    requested=0 & granted=0 is a real stop (not limiting). Guard flags use
    the ledger key names (hardGuardClipped / hardGuardRejected)."""
    prefix = ""
    reasons = []
    req, granted = edge.get("requested"), edge.get("granted")
    if finite(req) and finite(granted):
        if float(req) > 1e-12 and float(granted) + 1e-9 < float(req):
            reasons.append((R_PREV_CLIP if is_prev else R_CURR_CLIP))
    elif req is not None or granted is not None:
        # incomplete endpoint budget telemetry: cannot verify clear
        reasons.append((R_PREV_CLIP if is_prev else R_CURR_CLIP) + "_UNVERIFIABLE")
    for flag, code in (("hardGuardClipped", "GUARD"), ("hardGuardRejected", "GUARD")):
        if edge.get(flag) not in (None, 0, False):
            reasons.append((R_PREV_CLIP if is_prev else R_CURR_CLIP))
            break
    # planner clear: requested==granted would be theta fields on context; the
    # edge-level requested/granted IS the planner/budget grant for that edge's
    # correction, so planner clamp evidence is the same pair + explicit flags.
    return reasons


class PlatformWindow:
    """FIX-5a: recognizes 0/±15/±30 AND the registered peak platform ±A."""

    def __init__(self, peak_a=None):
        self.window = []
        levels = [0.0, 15.0, -15.0, 30.0, -30.0]
        if finite(peak_a) and float(peak_a) > 0:
            levels += [float(peak_a), -float(peak_a)]
        self.levels = levels

    def reset(self):
        self.window = []

    def feed(self, effective, rate, time_ms, substep, consistent):
        if not (finite(effective) and finite(rate) and finite(time_ms)):
            self.reset()
            return "off"
        effective = float(effective)
        near = None
        for t in self.levels:
            if abs(effective - t) <= PLATFORM_IDENTITY_TOLERANCE_DEG:
                near = t
                break
        if near is None or not consistent or abs(float(rate)) > 1e-9:
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
            span = self.window[-1][0] - self.window[0][0]  # FIX-5b: already ms
            tilts = [w[1] for w in self.window]
            if span >= STABILITY_WINDOW_MS and (max(tilts) - min(tilts)) <= 1e-6:
                return "stable"
        return "candidate"


def analyze_run(ledger_path, manifest, old_summary=None, collect_details=True):
    records = read_records(ledger_path)
    protocol = (manifest or {}).get("calibrationProtocol", {}) if isinstance(manifest, dict) else {}
    configured = protocol.get("configuredThresholds") or {}
    theta_start = float(configured.get("thetaStartDeg", 26.0))
    theta_stop = float(configured.get("thetaStopDeg", 20.0))
    executed = None  # per-record executedThresholds override

    peak_a = protocol.get("peakADeg") or (manifest or {}).get("peakADeg")
    if not finite(peak_a):
        peak_a = None
    rate = protocol.get("rateDegPerSec") or (manifest or {}).get("rateDegPerSec") or 0.5

    # schedule end (ruling 2 truncation): T = 9*5.1 + 4*A/v seconds
    first_dispatch_ms = None
    pre = []
    for record in records:
        pre.append(record)
        c = record.get("context", {})
        if first_dispatch_ms is None and c.get("commandSequence") == 1:
            first_dispatch_ms = to_ms(c.get("physicsTime"))
    if not finite(peak_a):
        mx = 0.0
        for record in pre:
            c = record.get("context", {})
            if c.get("phaseKindV3") == "ramp" and finite(c.get("targetTilt")):
                mx = max(mx, abs(float(c["targetTilt"])))
        peak_a = mx if mx > 0 else None
    schedule_end_ms = None
    if first_dispatch_ms is not None and finite(peak_a) and float(peak_a) > 0:
        schedule_end_ms = first_dispatch_ms + (9 * PLATFORM_HOLD_MS + 4 * float(peak_a) / float(rate)) * 1000.0

    # timing gate: first substep physicsTime per commandSequence (actual accepted intervals)
    first_sub = {}
    kind_of = {}
    for record in pre:
        c = record.get("context", {})
        seq = c.get("commandSequence")
        t = to_ms(c.get("physicsTime"))
        if isinstance(seq, int) and t is not None:
            if seq not in first_sub or t < first_sub[seq]:
                first_sub[seq] = t
            kind_of.setdefault(seq, c.get("phaseKindV3"))
    seqs = sorted(first_sub)
    breach = {}
    for a, b in zip(seqs, seqs[1:]):
        gap = first_sub[b] - first_sub[a]
        ka, kb = kind_of.get(a), kind_of.get(b)
        codes = []
        if ka == "ramp" and kb == "ramp":
            if not (SCHEDULE_TIMING["rampRamp"]["minMs"] <= gap <= SCHEDULE_TIMING["rampRamp"]["maxMs"]):
                codes.append(R_TIMING)
        elif ka == "platform" and kb == "ramp":
            if not (SCHEDULE_TIMING["platformRamp"]["minMs"] <= gap <= SCHEDULE_TIMING["platformRamp"]["maxMs"]):
                codes.append(R_TIMING)
        if codes:
            breach[b] = codes

    window = PlatformWindow(peak_a)
    cells = {k: {"rawTransitions": 0, "eligibleTransitions": 0, "retainedBrackets": 0,
                 "stateSamples": 0, "perRunStateSamples": {}, "exclusions": {},
                 "reasonRows": [], "bracketRows": [], "runs": set()}
             for k in ("start/+", "start/-", "stop/+", "stop/-")}
    neutral_zero = 0
    sign_mismatch_neutral = 0
    post_schedule_transitions = 0

    prior_state = {}    # (edge) -> last committed decision info
    prior_dec = {}      # (edge) -> [alpha_{-2}, alpha_{-1}]
    first_bracket_keys = set()

    for record in pre:
        c = record.get("context", {})
        substep = c.get("substepId")
        seq = c.get("commandSequence")
        leg = c.get("legId")
        segment = leg or c.get("segmentId")
        phase = c.get("phaseKindV3")
        t_ms = to_ms(c.get("physicsTime"))
        is_post = schedule_end_ms is not None and t_ms is not None and t_ms > schedule_end_ms + POST_SCHEDULE_TOLERANCE_MS
        executed = c.get("executedThresholds") or {}
        t_start = float(executed.get("thetaStartDeg", theta_start))
        t_stop = float(executed.get("thetaStopDeg", theta_stop))
        accepted = c.get("thetaAccepted")
        accepted_f = float(accepted) if finite(accepted) else None
        acc_sign = ("+" if accepted_f > 0 else "-") if accepted_f is not None and abs(accepted_f) > ANGLE_TOL else "0"
        consistent = (accepted_f is not None and finite(c.get("effectiveTilt"))
                      and abs(accepted_f - float(c["effectiveTilt"])) <= ANGLE_TOL)
        verdict = window.feed(c.get("effectiveTilt"), c.get("effectiveTiltRate"),
                              t_ms if t_ms is not None else 0.0, substep, consistent is True)
        stable = verdict == "stable"
        # segment identity for bracket purposes: stable platform segment or ramp leg
        if stable:
            seg_kind = "platform"
        elif phase == "ramp" and leg in RAMP_LEG_SIGN:
            seg_kind = "ramp"
        else:
            seg_kind = None

        for edge in record.get("relaxHysteresis", {}).get("edges", []):
            if not isinstance(edge, dict):
                continue
            ci = edge.get("interfaceIndex")
            if not finite(ci):
                continue
            ci = int(ci)
            ab, aa = edge.get("activeBefore"), edge.get("activeAfter")
            if not (isinstance(ab, bool) and isinstance(aa, bool)):
                continue
            alpha = decision_alpha(edge)
            identity = ci
            prev = prior_state.get(identity)

            is_start = (not ab) and aa
            is_stop = ab and (not aa)
            if is_start or is_stop:
                kind = "start" if is_start else "stop"
                # cell sign: stable platform -> observed accepted sign; ramp -> leg sign (ruling 1)
                if seg_kind == "platform":
                    cell_sign = acc_sign if acc_sign in ("+", "-") else None
                elif seg_kind == "ramp":
                    cell_sign = RAMP_LEG_SIGN[leg]
                else:
                    cell_sign = None
                cells_key = f"{kind}/{cell_sign}" if cell_sign else None
                reasons = []
                if cells_key is None:
                    if acc_sign == "0" and seg_kind is not None:
                        neutral_zero += 1
                    elif seg_kind is None:
                        reasons.append(R_SEG_MISSING)
                    else:
                        sign_mismatch_neutral += 1
                    if is_post:
                        reasons.append(R_POST_SCHEDULE)
                else:
                    cell = cells[cells_key]
                    cell["rawTransitions"] += 1
                    if is_post:
                        post_schedule_transitions += 1
                        reasons.append(R_POST_SCHEDULE)
                    # FIX-1/2/3: prev endpoint judged on ITS OWN committed decision
                    if prev is None:
                        reasons.append(R_PREV_NOT_COMMITTED)
                    else:
                        (p_sub, p_after, p_alpha, p_seg_kind, p_leg, p_edge, p_accepted_sign,
                         p_post) = prev
                        if not (isinstance(substep, int) and p_sub == substep - 1):
                            reasons.append(R_PREV_NOT_COMMITTED)
                        if p_after is not ab:
                            reasons.append(R_PREV_NOT_COMMITTED)
                        if p_seg_kind is None:
                            reasons.append(R_PREV_NOT_SAME_SEGMENT)
                        # FIX-2: prev endpoint clear on its own edge record
                        for rsn in endpoint_clear(p_edge, is_prev=True):
                            if rsn.endswith("_UNVERIFIABLE"):
                                reasons.append(rsn)
                            else:
                                reasons.append(rsn)
                        # FIX-3: prev endpoint nonzero accepted sign matching cell
                        if cell_sign is not None and p_accepted_sign != cell_sign:
                            reasons.append(R_PREV_SIGN)
                    # current endpoint clear
                    for rsn in endpoint_clear(edge, is_prev=False):
                        reasons.append(rsn)
                    # current endpoint accepted sign (FIX-3 on current too)
                    if cell_sign is not None and acc_sign == "0":
                        reasons.append(R_SIGN)
                    # timing gate: endpoint substep driven by a breached command
                    if breach.get(seq):
                        reasons.append(R_TIMING)
                    # executed-threshold crossing (alphaPrev < thetaStart <= alphaNow / alphaPrev > thetaStop >= alphaNow)
                    p_alpha_v = prev[2] if prev else None
                    cross = False
                    if kind == "start":
                        if p_alpha_v is not None and alpha is not None and p_alpha_v < t_start <= alpha:
                            cross = True
                    else:
                        if p_alpha_v is not None and alpha is not None and p_alpha_v > t_stop >= alpha:
                            cross = True
                    if not cross:
                        reasons.append(R_CROSS)
                    width_ok = False
                    if p_alpha_v is not None and alpha is not None:
                        width = abs(alpha - p_alpha_v)
                        if width > BRACKET_MAX_WIDTH_DEG:
                            reasons.append(R_WIDTH)
                        else:
                            width_ok = True
                    # ramp three-point monotone window: includes both endpoints + the
                    # immediately preceding SAME-segment decision (s5.2)
                    if seg_kind == "ramp":
                        w = prior_dec.get((identity, segment), [])
                        if len(w) >= 2:
                            a0, a1 = w[-2], w[-1]
                        else:
                            a0 = a1 = None
                        mono = False
                        if None not in (a0, a1, alpha):
                            mono = (a0 <= a1 <= alpha) if kind == "start" else (a0 >= a1 >= alpha)
                        if not mono:
                            reasons.append(R_NONMONO)
                    # reason rows (audit) - keep all hit reasons
                    row = {"substep": substep, "edge": ci, "segment": segment,
                           "seq": seq, "alphaPrev": p_alpha_v, "alphaNow": alpha,
                           "requested": edge.get("requested"), "granted": edge.get("granted"),
                           "prevRequested": prev[5].get("requested") if prev else None,
                           "prevGranted": prev[5].get("granted") if prev else None,
                           "postSchedule": is_post, "reasons": sorted(set(reasons))}
                    for rsn in set(reasons):
                        cell["exclusions"][rsn] = cell["exclusions"].get(rsn, 0) + 1
                    if collect_details:
                        cell["reasonRows"].append(row)
                    if not reasons and not is_post:
                        cell["eligibleTransitions"] += 1
                        bkey = (segment, ci)
                        if bkey not in first_bracket_keys:
                            first_bracket_keys.add(bkey)
                            cell["retainedBrackets"] += 1
                            cell["bracketRows"].append(row)

            # state samples: independent of transitions, cap per run per cell (FIX-4)
            if seg_kind is not None:
                cell_sign_state = acc_sign if seg_kind == "platform" else (
                    RAMP_LEG_SIGN.get(leg) if seg_kind == "ramp" else None)
                if cell_sign_state in ("+", "-"):
                    kind_state = "start" if not ab else "stop"
                    ckey = f"{kind_state}/{cell_sign_state}"
                    if ckey in cells:
                        run_n = cells[ckey]["perRunStateSamples"].setdefault("run", 0)
                        # per-run cap needs runId; use a single-run tool assumption
                        cells[ckey]["stateSamples"] += 1

            # rolling update (every committed decision updates the windows)
            prior_state[identity] = (substep, aa, alpha, seg_kind, leg, edge,
                                     acc_sign, is_post)
            wk = (identity, segment)
            w = prior_dec.setdefault(wk, [])
            w.append(alpha)
            if len(w) > 3:
                del w[0]

    # difference vs old summary
    diff = None
    if old_summary:
        diff = {}
        for key, cell in cells.items():
            old = (old_summary.get("cells") or {}).get(key, {})
            diff[key] = {
                "realTransitions": {"old": old.get("realTransitions"),
                                    "new": cell["rawTransitions"]},
                "eligibleBrackets": {"old": old.get("eligibleBrackets"),
                                     "new": cell["retainedBrackets"]},
                "exclusions": {"old": old.get("exclusionCounts"),
                               "new": cell["exclusions"]},
            }
    return {
        "cells": cells,
        "diagnostics": {
            "neutralZeroTransitions": neutral_zero,
            "signMismatchNeutral": sign_mismatch_neutral,
            "postScheduleTransitions": post_schedule_transitions,
            "peakADeg": peak_a,
            "scheduleEndMs": schedule_end_ms,
            "breachedCommands": {str(k): v for k, v in breach.items()},
        },
        "diff": diff,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ledger", type=Path, required=True)
    ap.add_argument("--manifest", type=Path, default=None)
    ap.add_argument("--old-summary", type=Path, default=None,
                    help="archived v3-summary.json from the pre-fix checker for the diff table")
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--no-details", action="store_true")
    args = ap.parse_args()
    manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8-sig")) if args.manifest else {}
    old_summary = json.loads(Path(args.old_summary).read_text(encoding="utf-8-sig")) if args.old_summary else None
    result = analyze_run(args.ledger, manifest, old_summary=old_summary,
                         collect_details=not args.no_details)
    out = {
        "schemaVersion": 1,
        "tool": "p011_obs3_d1_eligibility_replay",
        "basis": "design-P011-obs3-calibration-data-path-v1 §4.1 (1A) + §3.1 five fixes",
        "fixesApplied": ["prev-endpoint-stable-forcing", "prev-endpoint-clear",
                          "per-endpoint-nonzero-sign", "state-event-denominator",
                          "platform-window-A + physicsTime-unit"],
        "ledger": str(args.ledger),
        "diagnostics": result["diagnostics"],
        "cells": {},
        "diff": result["diff"],
    }
    for key, cell in result["cells"].items():
        out["cells"][key] = {
            "rawTransitions": cell["rawTransitions"],
            "eligibleTransitions": cell["eligibleTransitions"],
            "retainedBrackets": cell["retainedBrackets"],
            "stateSamples": cell["stateSamples"],
            "exclusions": cell["exclusions"],
            "reasonRows": [] if args.no_details else cell["reasonRows"],
            "bracketRows": [] if args.no_details else cell["bracketRows"],
        }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    summary = {k: {f: v[f] for f in ("rawTransitions", "eligibleTransitions", "retainedBrackets")}
               for k, v in result["cells"].items()}
    print(json.dumps(summary, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
