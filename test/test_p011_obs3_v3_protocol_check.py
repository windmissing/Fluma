#!/usr/bin/env python3
"""Synthetic unit tests for the Observation-3 v3 protocol checker.

Covers the task-mandated cases:
  - platform window verdict positives/negatives
  - eight-leg boundary ownership
  - bracket width gate (0.5 deg boundary values)
  - 3-point window monotone / non-monotone
  - first-transition selection
  - at least one case per output code
  - pilot three-dimension independent combinations
    (clean=PASS + platformReachability=PASS + pilotCoverage=INCOMPLETE)

Run: python test/test_p011_obs3_v3_protocol_check.py
"""
import json
import math
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import p011_obs3_v3_protocol_check as v3

H = math.tan(math.radians(30.0))
PASS, FAIL = 0, 0
FAILURES = []


def check(name, condition, detail=""):
    global PASS, FAIL
    if condition:
        PASS += 1
    else:
        FAIL += 1
        FAILURES.append(f"{name}: {detail}")


def make_manifest(protocol_id=v3.PROTOCOL_ID, start=28.0, stop=22.911, bindings=None):
    return {
        "calibrationProtocol": {
            "protocolId": protocol_id,
            "protocolRevision": v3.PROTOCOL_REVISION,
            "scheduleSha256": "a" * 64,
            "configuredThresholds": {"thetaStartDeg": start, "thetaStopDeg": stop},
            "commandBindings": bindings or {},
        }
    }


def write_ledger(records, directory):
    path = Path(directory) / "ledger.ndjson"
    path.write_text("\n".join(json.dumps(r) for r in records) + "\n", encoding="utf-8")
    return path


def write_manifest(manifest, directory):
    path = Path(directory) / "manifest.json"
    path.write_text(json.dumps(manifest), encoding="utf-8")
    return path


def base_context(idx, target=15.0, effective=None, rate=0.0, leg="L2",
                 phase_kind="ramp", theta_req=None, theta_acc=None,
                 executed=None, run="run-a", seq=None):
    i = idx
    effective = effective if effective is not None else target
    if theta_req is None:
        theta_req = effective
    if theta_acc is None:
        theta_acc = effective
    ctx = {
        "runId": run, "substepId": i + 1, "sequence": seq if seq is not None else 100 + i,
        "commandSequence": seq if seq is not None else 100 + i, "segmentId": leg, "legId": leg,
        "phaseKind": phase_kind, "phaseIndex": 2,
        "physicsTime": 10200.0 + i * 16.6667, "physicsDt": 1 / 60,
        "targetTilt": target, "effectiveTilt": effective,
        "effectiveTiltRate": rate,
        "thetaRequested": theta_req, "thetaAccepted": theta_acc,
        "runRole": "calibration",
        "observationRouteId": "natural-relax-hysteresis-v1",
    }
    if executed is not None:
        ctx["executedThresholds"] = executed
    return ctx


def base_edge(i, before, after, r, requested=0.05, granted=0.05, c=3.0):
    return {
        "interfaceIndex": c, "donor": c, "receiver": c + 1,
        "activeBefore": before, "activeAfter": after,
        "requested": requested, "granted": granted, "r": r,
        "transitionType": ("start" if (not before and after) else
                           "stop" if (before and not after) else None),
    }


EXEC = {"thetaStartDeg": 28.0, "thetaStopDeg": 22.911,
        "slopeStart": math.tan(math.radians(28.0)),
        "slopeStop": math.tan(math.radians(22.911)),
        "stateMachineVersion": "v3-test", "sourceCommit": "d78335b",
        "sourceFileSha256": "b" * 64, "buildSha256": "c" * 64}


def run_analyze(records, manifest):
    with tempfile.TemporaryDirectory() as tmp:
        ledger = write_ledger(records, tmp)
        mpath = write_manifest(manifest, tmp)
        out = Path(tmp) / "out.json"
        result = v3.analyze(ledger, mpath)
        out.write_text(json.dumps(result), encoding="utf-8")
        return result


# ---------------------------------------------------------------------------
# 1. platform window verdict positives/negatives
# ---------------------------------------------------------------------------
def test_platform_window():
    w = v3.PlatformWindow()
    verdicts = []
    # 15 substeps at 15.0 deg, rate 0, dt 16.67 ms -> stable from the 13th.
    for i in range(15):
        verdicts.append(w.feed(15.0, 0.0, 5100.0 + i * 16.6667, i + 1, True))
    check("platform stable after 200ms/3 substeps",
          all(v == "stable" for v in verdicts[12:]),
          f"verdicts={verdicts}")
    # non-zero rate breaks the platform
    w2 = v3.PlatformWindow()
    seq = [w2.feed(15.0, 0.0, 5100.0 + i * 16.6667, i + 1, True) for i in range(5)]
    seq.append(w2.feed(15.0, 0.5, 5100.0 + 5 * 16.6667, 6, True))
    check("platform reset on nonzero rate", seq[-1] == "off", f"seq={seq}")
    # slow ramp cannot masquerade as a platform
    w3 = v3.PlatformWindow()
    ramp = [w3.feed(15.0 + 0.001 * i, 0.0, 5100.0 + i * 16.6667, i + 1, True)
            for i in range(30)]
    check("slow ramp never stable (range > 1e-6)",
          all(v != "stable" for v in ramp), f"ramp={ramp}")
    # eligibility starts at window completion: 200 ms needs 12 substeps at
    # 16.67 ms; records 1..12 are candidates, 13 completes the window.
    w4 = v3.PlatformWindow()
    verdicts = [w4.feed(15.0, 0.0, 5100.0 + i * 16.6667, i + 1, True)
                for i in range(13)]
    check("no backfill: pre-window records are candidates",
          all(v == "candidate" for v in verdicts[:12]) and verdicts[12] == "stable",
          f"verdicts={verdicts}")


# ---------------------------------------------------------------------------
# 2. eight-leg definitions & boundary ownership
# ---------------------------------------------------------------------------
def test_legs():
    check("eight legs defined", len(v3.LEGS) == 8)
    expected = {
        "L1": ("H0", "H1"), "L2": ("H1", "H2"), "L3": ("H2", "H3"),
        "L4": ("H3", "H4"), "L5": ("H4", "H5"), "L6": ("H5", "H6"),
        "L7": ("H6", "H7"), "L8": ("H7", "H8"),
    }
    check("leg platform pairs match preregistration",
          {k: (v[0], v[1]) for k, v in v3.LEGS.items()} == expected)
    # half-open boundary: endpoint command belongs to the next platform,
    # implemented via schedule binding lookup (legId from binding table).
    bindings = {"100": {"legId": "L2", "phaseKind": "ramp"},
                "101": {"legId": "L3", "phaseKind": "ramp"}}
    leg_a, _ = v3.leg_from_schedule({"commandSequence": 100}, bindings)
    leg_b, _ = v3.leg_from_schedule({"commandSequence": 101}, bindings)
    check("leg identity from pre-registered binding", leg_a == "L2" and leg_b == "L3",
          f"a={leg_a} b={leg_b}")
    # peak grid divisibility: n = |T1-T0| / (0.1*v) integer for both rates
    for v_rate, step in v3.ANGULAR_STEP.items():
        for a in (15.1, 20.0, 29.9):
            n1 = abs(a - 15.0) / step
            n2 = abs(a - 0.0) / step
            check(f"grid divisible rate={v_rate} A={a}",
                  abs(n1 - round(n1)) < 1e-9 and abs(n2 - round(n2)) < 1e-9,
                  f"n1={n1} n2={n2}")
    check("peak grid 149 points 15.1..29.9",
          len(v3.PEAK_GRID) == 149 and v3.PEAK_GRID[0] == 15.1
          and v3.PEAK_GRID[-1] == 29.9)


# ---------------------------------------------------------------------------
# 3. bracket qualification: width gate at 0.5 boundary
# ---------------------------------------------------------------------------
def make_stop_sequence(width_deg, threshold=22.911):
    """A stable +15 ramp segment where a real stop occurs with the given
    alpha width across thetaStop: alphaPrev=28.81 deg (r=0.55) then
    alphaNow = alphaPrev - width, which must land below thetaStop."""
    a_prev = 23.2  # just above thetaStop 22.911
    a_after = a_prev - width_deg  # crosses below the threshold
    records = []
    for i in range(25):
        if i < 19:
            before, after = False, False
        elif i == 19:
            before, after = True, True    # active holds
        elif i == 20:
            before, after = True, False   # real stop: crossing step
        else:
            before, after = False, False
        r = math.tan(math.radians(a_prev)) if i <= 19 else math.tan(math.radians(a_after))
        records.append({
            "context": base_context(i, theta_req=15.0, theta_acc=15.0,
                                    executed=EXEC),
            "relaxHysteresis": {"edges": [
                base_edge(i, before, after, r)]},
            "audit": {"errorCode": None},
        })
    return records


def test_bracket_width():
    # width exactly 0.5 -> eligible (<= 0.5)
    result = run_analyze(make_stop_sequence(0.5), make_manifest())
    cell = result["cells"]["stop/+"]
    check("bracket width 0.5 eligible", cell["eligibleBrackets"] == 1,
          f"cell={json.dumps(cell)[:200]}")
    # width 0.6 -> BRACKET_TOO_WIDE, not split
    result = run_analyze(make_stop_sequence(0.6), make_manifest())
    cell = result["cells"]["stop/+"]
    check("bracket width 0.6 rejected BRACKET_TOO_WIDE",
          cell["eligibleBrackets"] == 0
          and cell["exclusionCounts"].get("BRACKET_TOO_WIDE", 0) >= 1
          and cell["stopUnobservedSubcode"] == "BRACKET_TOO_WIDE",
          f"cell={json.dumps(cell)[:220]}")
    check("wide events counted, not split",
          result["cells"]["stop/+"]["realTransitions"] == 1)


# ---------------------------------------------------------------------------
# 4. 3-point monotone window
# ---------------------------------------------------------------------------
def make_monotone_sequence(monotone=True, kind="stop"):
    """Stop geometry: alphas decrease 24.2 -> 23.2 -> 22.8 across the event
    (decreasing = monotone for a stop), crossing thetaStop 22.911 between
    alphaPrev (23.2) and alphaNow (22.8) with width 0.4 <= 0.5."""
    alphas = {18: 24.2, 19: 23.2, 20: 22.8, 21: 22.0}
    if not monotone:
        alphas[19] = 21.0  # breaks non-increasing direction
    records = []
    for i in range(30):
        if i < 19:
            before, after = False, False
        elif i == 19:
            before, after = True, True
        elif i == 20:
            before, after = True, False
        else:
            before, after = False, False
        r = math.tan(math.radians(alphas.get(i, 22.5)))
        records.append({
            "context": base_context(i, theta_req=15.0, theta_acc=15.0,
                                    executed=EXEC),
            "relaxHysteresis": {"edges": [base_edge(i, before, after, r)]},
            "audit": {"errorCode": None},
        })
    return records


def test_three_point_window():
    result = run_analyze(make_monotone_sequence(True), make_manifest())
    cell = result["cells"]["stop/+"]
    check("monotone window eligible", cell["eligibleBrackets"] == 1,
          f"cell={json.dumps(cell)[:220]}")
    # non-monotone window rejected with code (multiple reasons may fire; the
    # assertion is on NONMONOTONE_LOCAL_WINDOW being present + no bracket)
    result = run_analyze(make_monotone_sequence(False), make_manifest())
    cell = result["cells"]["stop/+"]
    check("non-monotone window rejected with code",
          cell["eligibleBrackets"] == 0
          and cell["exclusionCounts"].get("NONMONOTONE_LOCAL_WINDOW", 0) >= 1
          and cell["stopUnobservedSubcode"] is not None,
          f"cell={json.dumps(cell)[:220]}")


# ---------------------------------------------------------------------------
# 5. first-transition selection (start/stop whichever first)
# ---------------------------------------------------------------------------
def test_first_transition():
    """Two real stops on the same (run, leg, edge): only the first counts."""
    records = []
    # Each stop: alphaPrev=23.2 -> alphaNow=22.7 (width 0.5, crosses 22.911).
    # Between stops the edge re-activates then stops again; only the FIRST
    # qualifying stop enters estimation.
    stops = [20, 40]
    for i in range(60):
        if i in stops:
            before, after = True, False
            r = math.tan(math.radians(22.7))
        elif (i - 1) in stops:
            before, after = False, False
            r = math.tan(math.radians(22.7))
        elif (i + 1) in stops:
            before, after = True, True
            r = math.tan(math.radians(23.2))
        else:
            before, after = False, False
            r = math.tan(math.radians(23.2))
        records.append({
            "context": base_context(i, theta_req=15.0, theta_acc=15.0,
                                    executed=EXEC),
            "relaxHysteresis": {"edges": [base_edge(i, before, after, r)]},
            "audit": {"errorCode": None},
        })
    result = run_analyze(records, make_manifest())
    cell = result["cells"]["stop/+"]
    check("first transition only enters estimation",
          cell["realTransitions"] == 2 and cell["eligibleBrackets"] == 2
          and len(cell["firstBrackets"]) == 1
          and cell["firstBrackets"][0]["substepId"] == 21,
          f"cell={json.dumps(cell)[:300]}")


# ---------------------------------------------------------------------------
# 6. output codes coverage
# ---------------------------------------------------------------------------
def test_output_codes():
    # RUN_INVALID_HARD_GATE
    records = [{"context": base_context(0, executed=EXEC),
                "relaxHysteresis": {"edges": []},
                "audit": {"errorCode": 99}}]
    result = run_analyze(records, make_manifest())
    check("RUN_INVALID_HARD_GATE assigned",
          result["runInvalidHardGate"]
          and any("RUN_INVALID_HARD_GATE" in c["censorCodes"]
                  for c in result["cells"].values()))
    # EXECUTION_THRESHOLD_IDENTITY_MISMATCH (missing executedThresholds)
    records = [{"context": base_context(0), "relaxHysteresis": {"edges": []},
                "audit": {"errorCode": None}}]
    result = run_analyze(records, make_manifest())
    check("EXECUTION_THRESHOLD_IDENTITY_MISMATCH + UNOBSERVABLE",
          not result["executionThresholdIdentity"]["ok"]
          and result["executionThresholdIdentity"]["mismatchCode"]
          == "EXECUTION_THRESHOLD_IDENTITY_MISMATCH"
          and any("EXECUTION_THRESHOLD_IDENTITY_MISMATCH" in c["censorCodes"]
                  for c in result["cells"].values()))
    # TELEMETRY_INCOMPLETE (thetaAccepted absent)
    ctx = base_context(0, executed=EXEC)
    ctx.pop("thetaAccepted")
    records = [{"context": ctx, "relaxHysteresis": {"edges": []},
                "audit": {"errorCode": None}}]
    result = run_analyze(records, make_manifest())
    check("TELEMETRY_INCOMPLETE when thetaAccepted missing",
          result["telemetryIncompleteRecords"] == 1
          and any("TELEMETRY_INCOMPLETE" in c["censorCodes"]
                  for c in result["cells"].values()))
    # PLATFORM_UNREACHED (no platform records at all)
    records = [{"context": base_context(0, executed=EXEC),
                "relaxHysteresis": {"edges": []},
                "audit": {"errorCode": None}}]
    result = run_analyze(records, make_manifest())
    check("PLATFORM_UNREACHED diagnostic",
          result["platformReachability"]["verdict"] == "FAIL"
          and all(p["stableRecords"] == 0
                  for p in result["platformReachability"]["perPhase"].values()))
    # STOP_TRANSITION_UNOBSERVED subcode NEVER_ACTIVE
    records = [{"context": base_context(i, executed=EXEC),
                "relaxHysteresis": {"edges": [
                    base_edge(i, False, False, 0.1)]},
                "audit": {"errorCode": None}} for i in range(15)]
    result = run_analyze(records, make_manifest())
    check("STOP_TRANSITION_UNOBSERVED NEVER_ACTIVE",
          any("STOP_TRANSITION_UNOBSERVED" in c["censorCodes"]
              for c in result["cells"].values()))
    # RIGHT_CENSORED_AT_TAN30: exposure below start + tan30 bound evidence
    records = []
    for i in range(20):
        ctx = base_context(i, target=15.0, effective=15.0, rate=0.0,
                           theta_req=15.0, theta_acc=15.0, executed=EXEC,
                           leg="L2", phase_kind="ramp")
        edge = base_edge(i, False, False, 0.50)  # alpha ~26.5 < 28 start
        records.append({"context": ctx,
                        "relaxHysteresis": {"edges": [edge]},
                        "audit": {"errorCode": None,
                                  "limitingReason": "tan30",
                                  "plannerBound": "tan30"}})
    result = run_analyze(records, make_manifest())
    sc = result["cells"]["start/+"]
    check("RIGHT_CENSORED_AT_TAN30 (v3: ramp exposure, tan30 bound evidence)",
          "RIGHT_CENSORED_AT_TAN30" in sc["censorCodes"],
          f"codes={sc['censorCodes']} alphaMax={sc['atRiskAlphaMaxDeg']}")
    # ...but NOT assigned without tan30 bound evidence
    records_no_evidence = [{**r, "audit": {"errorCode": None}} for r in records]
    result = run_analyze(records_no_evidence, make_manifest())
    check("RIGHT_CENSORED not assigned without tan30 bound evidence",
          "RIGHT_CENSORED_AT_TAN30" not in result["cells"]["start/+"]["censorCodes"],
          f"codes={result['cells']['start/+']['censorCodes']}")
    # START_OUTSIDE_ELIGIBLE_WINDOW: real start but ineligible (continuity)
    records = []
    for i in range(25):
        if i == 19:
            before, after = False, True  # real start
        else:
            before, after = False, False
        records.append({
            "context": base_context(i, theta_req=15.0, theta_acc=15.0,
                                    executed=EXEC),
            "relaxHysteresis": {"edges": [base_edge(i, before, after, 0.6)]},
            "audit": {"errorCode": None}})
    result = run_analyze(records, make_manifest())
    sc = result["cells"]["start/+"]
    check("START_OUTSIDE_ELIGIBLE_WINDOW (no prior state -> continuity)",
          sc["realTransitions"] == 1 and sc["eligibleBrackets"] == 0
          and "START_OUTSIDE_ELIGIBLE_WINDOW" in sc["censorCodes"],
          f"cell={json.dumps(sc)[:220]}")


# ---------------------------------------------------------------------------
# 7. pilot three-dimension independence
# ---------------------------------------------------------------------------
def test_pilot_dimensions():
    # clean=PASS + platformReachability=PASS + pilotCoverage=INCOMPLETE
    records = []
    # stable +15 platform (200ms+) then stable 0 platform
    for i in range(20):
        records.append({
            "context": base_context(i, target=15.0, effective=15.0, rate=0.0,
                                    theta_req=15.0, theta_acc=15.0,
                                    executed=EXEC),
            "relaxHysteresis": {"edges": [base_edge(i, False, False, 0.1)]},
            "audit": {"errorCode": None}})
    for i in range(20):
        records.append({
            "context": base_context(20 + i, target=0.0, effective=0.0, rate=0.0,
                                    theta_req=0.0, theta_acc=0.0,
                                    executed=EXEC),
            "relaxHysteresis": {"edges": []},
            "audit": {"errorCode": None}})
    result = run_analyze(records, make_manifest())
    check("clean=PASS with valid identity and no gate failure",
          result["overall"] == "PILOT_REPORT"
          and result["executionThresholdIdentity"]["ok"],
          f"overall={result['overall']}")
    pr = result["platformReachability"]
    check("platformReachability=PASS (+15 stable, planner clear)",
          pr["perPhase"]["15"]["stableRecords"] > 0
          and pr["perPhase"]["15"]["plannerLimitedStableRecords"] == 0,
          f"pr={json.dumps(pr['perPhase'])[:200]}")
    check("pilotCoverage=INCOMPLETE (cells without brackets)",
          result["pilotCoverage"] == "INCOMPLETE")
    check("three dimensions independent: threshold still NOT_EVALUATED",
          result["thresholdVerdict"] == "NOT_EVALUATED"
          and result["thetaStartEstimate"] is None
          and result["thetaStopEstimate"] is None
          and result["observedStartTransition"] is None
          and result["observedStopTransition"] is None)
    # clean=INVALID makes overall INVALID even if coverage complete
    records_bad = [{**r, "audit": {"errorCode": 99}} for r in records]
    result = run_analyze(records_bad, make_manifest())
    check("clean failure dominates: overall INVALID",
          result["overall"] == "INVALID")


# ---------------------------------------------------------------------------
# 8. per-run cap (max 3 transitions per run per cell)
# ---------------------------------------------------------------------------
def test_per_run_cap():
    records = []
    # Four real stops, each a narrow crossing bracket
    # (alphaPrev=23.2 -> alphaNow=22.7, width 0.5): first 3 enter estimation,
    # 4th capped.
    stops = [21, 41, 61, 81]
    for i in range(100):
        if i in stops:
            before, after = True, False
            r = math.tan(math.radians(22.7))
        elif (i - 1) in stops:
            before, after = False, False
            r = math.tan(math.radians(22.7))
        elif (i + 1) in stops:
            before, after = True, True
            r = math.tan(math.radians(23.2))
        else:
            before, after = False, False
            r = math.tan(math.radians(23.2))
        records.append({
            "context": base_context(i, theta_req=15.0, theta_acc=15.0,
                                    executed=EXEC),
            "relaxHysteresis": {"edges": [base_edge(i, before, after, r)]},
            "audit": {"errorCode": None}})
    result = run_analyze(records, make_manifest())
    cell = result["cells"]["stop/+"]
    check("per-run cap 3: 4th transition capped",
          cell["realTransitions"] == 4 and cell["cappedTransitions"] == 1
          and cell["eligibleBrackets"] == 3,
          f"real={cell['realTransitions']} capped={cell['cappedTransitions']} "
          f"elig={cell['eligibleBrackets']}")


# ---------------------------------------------------------------------------
# 9. coverage matrix structure
# ---------------------------------------------------------------------------
def test_coverage_matrix():
    records = [{"context": base_context(0, executed=EXEC),
                "relaxHysteresis": {"edges": []},
                "audit": {"errorCode": None}}]
    result = run_analyze(records, make_manifest())
    rows = result["coverageMatrix"]
    main_rows = [r for r in rows if r["parentCell"] is None]
    sub_rows = [r for r in rows if r["parentCell"] is not None]
    check("matrix: 4 main cells + 24 sub cells", len(main_rows) == 4 and len(sub_rows) == 24,
          f"main={len(main_rows)} sub={len(sub_rows)}")
    check("sub cells zero-record rows retained with UNDEFINED",
          all(r["coverageStatus"] == "UNDEFINED"
              and r["wNoiseSource"] == "UNDEFINED_NO_FIT_NOISE"
              for r in sub_rows))
    check("matrix source tagged", all(r["source"] == "pilot" for r in rows))
    check("frozen stats constants inherited",
          result["statsGates"]["runsPerCandidatePair"] == 34
          and result["statsGates"]["bootstrapSeed"] == 20261019
          and result["statsGates"]["subCellCount"] == 24)


def test_schedule_timing_gate():
    """s3.2 input-timing gate (drift consumption ruling): actual accepted
    intervals judged per domain; drift marker is telemetry only."""
    # Baseline: commands every 100ms with drift 60ms (marker would fire) -
    # the marker must NOT disqualify anything.
    def timing_records(gaps, kinds=None, targets=None, drift=60.0):
        records = []
        t = 0.0
        seq = 0
        for gi, gap in enumerate(gaps):
            seq += 1
            kind = kinds[gi] if kinds else "ramp"
            target = targets[gi] if targets else 15.0 + 0.05 * gi
            t += gap
            rec_ctx = base_context(seq, target=target, leg="L2",
                                   phase_kind=kind, seq=seq)
            rec_ctx["physicsTime"] = t / 1000.0  # seconds, gate expects s
            records.append({
                "context": rec_ctx,
                "relaxHysteresis": {"edges": []},
                "audit": {"errorCode": None},
            })
        # inject drift after building (base_context has no drift field)
        for rec in records:
            rec["context"]["scheduleDriftMs"] = drift
            rec["context"]["scheduleDriftExceeded"] = drift > 50
            rec["context"]["phaseKindV3"] = rec["context"].get("phaseKind")
        return records

    # A: marker fires (drift 60 > 50) but intervals in-band -> no breach.
    s, breach = v3.schedule_timing_gate(
        timing_records([100.0] * 5), {})
    check("marker is telemetry only: in-band intervals not disqualified",
          s["breachedCommands"] == 0 and s["driftMarkerRecords"] == 5
          and s["gate"] == "actualAcceptedInterval",
          f"summary={json.dumps(s)[:200]} breach={breach}")

    # B: catch-up fabrication - interval 40ms (< 50 floor) -> breach.
    s, breach = v3.schedule_timing_gate(
        timing_records([100.0, 40.0, 100.0, 100.0]), {})
    check("catch-up interval (<50ms) disqualified",
          any(any(c.startswith(v3.SCHEDULE_INTERVAL_CODE + ":catchUp")
                  for c in codes) for codes in breach.values()),
          f"breach={breach}")

    # C: stalled digestion - interval 220ms (> 200 band) -> breach.
    s, breach = v3.schedule_timing_gate(
        timing_records([100.0, 220.0, 100.0, 100.0]), {})
    check("stalled interval (>200ms) disqualified",
          any(v3.SCHEDULE_INTERVAL_CODE in codes for codes in breach.values()),
          f"breach={breach}")

    # D: hold domain - platform->ramp gap 5090ms in [4000,6200] passes;
    # 6500ms breaches.
    s, breach = v3.schedule_timing_gate(
        timing_records([0.0, 5100.0, 100.0, 100.0],
                       kinds=["platform", "ramp", "ramp", "ramp"]), {})
    check("hold domain in-band passes",
          s["breachedCommands"] == 0 and s["acceptedIntervalHoldMs"]["n"] == 1,
          f"summary={json.dumps(s['acceptedIntervalHoldMs'])} breach={breach}")
    s, breach = v3.schedule_timing_gate(
        timing_records([0.0, 6500.0, 100.0, 100.0],
                       kinds=["platform", "ramp", "ramp", "ramp"]), {})
    check("hold domain overstayed (>6200ms) disqualified",
          any(":hold" in c for codes in breach.values() for c in codes),
          f"breach={breach}")

    # E: coalescing span - one skipped command (2 angular steps, 0.10 deg)
    # allowed; three steps (0.15 deg) breach.
    ok_targets = [15.0, 15.1, 15.15, 15.2]  # deltas 0.10/0.05/0.05
    s, breach = v3.schedule_timing_gate(
        timing_records([100.0] * 4, targets=ok_targets), {})
    check("coalescing k=2 (one skipped command) allowed",
          s["breachedCommands"] == 0, f"breach={breach}")
    bad_targets = [15.0, 15.15, 15.2, 15.25]  # first delta 0.15 = 3 steps
    s, breach = v3.schedule_timing_gate(
        timing_records([100.0] * 4, targets=bad_targets), {})
    check("coalescing k=3 breached",
          any(v3.SCHEDULE_COMMAND_GAP_CODE in codes
              for codes in breach.values()), f"breach={breach}")

    # F: end-to-end - a stop bracket whose endpoint substep is driven by a
    # breached command (catch-up 40ms interval) loses eligibility with the
    # timing code in exclusionCounts. Geometry per make_stop_sequence:
    # alphaPrev=23.2 -> alphaNow=22.7 crosses 22.911 (width 0.5).
    records = []
    t_ms = 0.0
    for i in range(25):
        t_ms += 40.0 if i == 3 else (500.0 if i == 0 else 100.0)
        eff = 15.0
        seq = i + 1
        ctx = base_context(i, target=15.0, leg="L2", phase_kind="ramp", seq=seq)
        ctx["physicsTime"] = t_ms / 1000.0
        ctx["effectiveTilt"] = eff
        ctx["thetaRequested"] = eff
        ctx["thetaAccepted"] = eff
        ctx["phaseKindV3"] = "ramp"
        ctx["scheduleDriftMs"] = 60.0
        ctx["scheduleDriftExceeded"] = True
        if i == 2:
            before, after = True, True
            r_val = math.tan(math.radians(23.2))
        elif i == 3:
            before, after = True, False
            r_val = math.tan(math.radians(22.7))
        else:
            before, after = False, False
            r_val = math.tan(math.radians(23.2))
        records.append({
            "context": ctx,
            "relaxHysteresis": {"edges": [base_edge(i, before, after, r_val)]},
            "audit": {"errorCode": None},
        })
    result = run_analyze(records, make_manifest())
    cell = result["cells"]["stop/+"]
    check("breached command propagates to bracket exclusion",
          cell["realTransitions"] >= 1
          and any(v3.SCHEDULE_INTERVAL_CODE in k
                  for k in cell["exclusionCounts"]),
          f"exclusions={cell['exclusionCounts']}")


def test_pre_expansion_rulings():
    """Pre-expansion rulings (2026-10-21): ramp cell attribution by
    preregistered leg sign; post-schedule tail truncation."""
    # Ramp start inside L2 (+): attributed to start/+ even though the
    # platform window is off (A-platform 18.7 is not a near-label).
    records = []
    t_ms = 0.0
    for i in range(30):
        t_ms += 100.0
        eff = min(18.7, 0.187 * (i + 1))
        rate = 0.0 if eff >= 18.7 else 0.5
        ctx = base_context(i, target=18.7, leg="L2", phase_kind="ramp", seq=i + 1)
        ctx["physicsTime"] = t_ms / 1000.0
        ctx["effectiveTilt"] = eff
        ctx["thetaRequested"] = eff
        ctx["thetaAccepted"] = eff
        ctx["phaseKindV3"] = "ramp"
        if i == 19:
            before, after = False, True   # real start inside ramp L2
            r_val = math.tan(math.radians(20.0))
        elif i == 18:
            before, after = False, False
            r_val = math.tan(math.radians(19.5))
        elif i == 20:
            before, after = True, True
            r_val = math.tan(math.radians(20.5))
        else:
            before, after = False, False
            r_val = math.tan(math.radians(19.0))
        # donor/receiver omitted (ledger schema v3 optionality, v4 pilot)
        edge = {"interfaceIndex": 3.0, "donor": None, "receiver": None,
                "activeBefore": before, "activeAfter": after,
                "requested": 0.05, "granted": 0.05, "r": r_val,
                "transitionType": "start" if i == 19 else None}
        records.append({"context": ctx,
                        "relaxHysteresis": {"edges": [edge]},
                        "audit": {"errorCode": None}})
    result = run_analyze(records, make_manifest())
    cell = result["cells"]["start/+"]
    check("ramp start attributed by preregistered leg sign (+L2)",
          cell["realTransitions"] >= 1,
          f"cell={json.dumps(cell)[:200]}")
    # Sign mismatch: L5 (preregistered -) with observed + -> excluded.
    records2 = []
    t_ms = 0.0
    for i in range(30):
        t_ms += 100.0
        eff = -min(18.7, 0.187 * (i + 1))
        ctx = base_context(i, target=-18.7, leg="L5", phase_kind="ramp", seq=i + 1)
        ctx["physicsTime"] = t_ms / 1000.0
        ctx["effectiveTilt"] = eff
        ctx["thetaRequested"] = eff
        ctx["thetaAccepted"] = eff
        ctx["phaseKindV3"] = "ramp"
        # simulate a bogus positive-transition record: eff positive on L5
        if i == 19:
            ctx["effectiveTilt"] = 5.0
            ctx["thetaRequested"] = 5.0
            ctx["thetaAccepted"] = 5.0
            before, after = False, True
        elif i == 18:
            before, after = False, False
        elif i == 20:
            before, after = True, True
        else:
            before, after = False, False
        r_val = math.tan(math.radians(10.0 if i == 19 else 8.0))
        edge = {"interfaceIndex": 3.0, "donor": None, "receiver": None,
                "activeBefore": before, "activeAfter": after,
                "requested": 0.05, "granted": 0.05, "r": r_val,
                "transitionType": "start" if i == 19 else None}
        records2.append({"context": ctx,
                         "relaxHysteresis": {"edges": [edge]},
                         "audit": {"errorCode": None}})
    result2 = run_analyze(records2, make_manifest())
    got_mismatch = any("RAMP_SIGN_MISMATCH" in k or k == "NEUTRAL_ZERO_CROSSING"
                       for cell2 in result2["cells"].values()
                       for k in cell2["exclusionCounts"])
    neg_clean = result2["cells"]["start/-"]["realTransitions"] == 0
    check("ramp sign mismatch excluded (not silently counted)",
          got_mismatch and neg_clean,
          f"exclusions={ {k: c['exclusionCounts'] for k, c in result2['cells'].items()} }")
    # Post-schedule: last records beyond schedule end never enter cells.
    # Use the stop geometry with the schedule end computed from peakA.
    result3 = run_analyze(records, make_manifest())
    ps = result3["postSchedule"]
    check("postSchedule block present with code",
          ps["code"] == "POST_SCHEDULE" and "scheduleEndMs" in ps,
          f"postSchedule={json.dumps(ps)[:160]}")


def main():
    test_platform_window()
    test_legs()
    test_bracket_width()
    test_three_point_window()
    test_first_transition()
    test_output_codes()
    test_pilot_dimensions()
    test_per_run_cap()
    test_coverage_matrix()
    test_schedule_timing_gate()
    test_pre_expansion_rulings()
    print(f"PASS={PASS} FAIL={FAIL}")
    for failure in FAILURES:
        print("  FAIL:", failure)
    return 0 if FAIL == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
