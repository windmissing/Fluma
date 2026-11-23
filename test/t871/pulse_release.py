"""T871 short-pulse release experiment using one shared continuous trial trajectory."""

import argparse
from bisect import bisect_left
from dataclasses import asdict, dataclass
import hashlib
from itertools import combinations
import json
import math
from pathlib import Path
import platform


MASS_KG = 1.0
SOURCE_ID = 1
HEIGHT_M = 0.002
END_MS = 400
PULSE_MS = (20, 50, 100)
PHASE_MS = (0, 3)
STEP_MS = (10, 5)
TOL = 1e-6


@dataclass(frozen=True)
class Segment:
    start_s: float
    end_s: float
    normal_N: float
    y_m: float
    vy_mps: float
    impulse_Ns: float
    in_flight: bool = True

    @property
    def acceleration(self):
        return self.normal_N / MASS_KG if self.in_flight else 0.0

    def evaluate(self, time):
        dt = time - self.start_s
        return (self.y_m + dt * (self.vy_mps + 0.5 * self.acceleration * dt),
                self.vy_mps + self.acceleration * dt,
                self.impulse_Ns + max(0.0, self.normal_N) * dt)


def trial(phase_ms, duration_ms):
    start, end = phase_ms / 1000, (phase_ms + duration_ms) / 1000
    segments = []
    if start:
        segments.append(Segment(0.0, start, -1.0, 0.0, 0.0, 0.0, in_flight=False))
    push = Segment(start, end, 1.0, 0.0, 0.0, 0.0)
    y, v, impulse = push.evaluate(end)
    a = -1.0 / MASS_KG
    top = end - v / a
    climb = Segment(end, top, -1.0, y, v, impulse)
    y_top, _, _ = climb.evaluate(top)
    # Solve the descending zero with the actual integrated apex as the initial state.
    incoming = -math.sqrt(-2 * a * y_top)
    hit = top + incoming / a
    fall = Segment(top, hit, -1.0, y_top, 0.0, impulse)
    segments.extend((push, climb, fall, Segment(hit, END_MS / 1000, -1.0, 0.0, 0.0, impulse, in_flight=False)))
    return segments, (top, y_top), hit


def outward_crossing(segments, height):
    for segment in segments:
        if not segment.in_flight or segment.y_m >= height:
            continue
        end_y, end_v, _ = segment.evaluate(segment.end_s)
        # A contact with the threshold at the apex has no outward crossing.
        if end_y < height or (abs(end_v) <= 1e-14 and abs(end_y - height) <= 1e-15):
            continue
        delta_y = height - segment.y_m
        speed2 = segment.vy_mps**2 + 2 * segment.acceleration * delta_y
        if speed2 <= 0:
            continue
        # This form avoids subtracting nearly equal velocities near the threshold.
        time = segment.start_s + 2 * delta_y / (segment.vy_mps + math.sqrt(speed2))
        if not segment.start_s < time <= segment.end_s + 1e-12:
            continue
        time = min(time, segment.end_s)
        y, v, impulse = segment.evaluate(time)
        if v > 0 and impulse > 0:
            return dict(kind="F_TO_P", time_s=time, y_m=y, vy_mps=v, outwardImpulse_Ns=impulse,
                        segmentStart_s=segment.start_s, segmentEnd_s=segment.end_s,
                        fromMass_kg={"F": MASS_KG, "P": 0}, toMass_kg={"F": 0, "P": MASS_KG})
    return None


def run(phase_ms, duration_ms, step_ms):
    segments, (top, y_top), hit = trial(phase_ms, duration_ms)
    pulse_start, pulse_end = phase_ms / 1000, (phase_ms + duration_ms) / 1000
    output_times = [ms / 1000 for ms in range(0, END_MS + 1, step_ms)]
    release = outward_crossing(segments, HEIGHT_M)
    events = [dict(kind="APEX", time_s=top, y_m=y_top, vy_mps=0.0)]
    if release:
        events.append(release)
    fall = segments[-2]
    residual, incoming, impulse = fall.evaluate(hit)
    next_sample = output_times[bisect_left(output_times, hit)]
    events.append(dict(kind="P_TO_F" if release else "TRIAL_RETURN", time_s=hit,
                       y_m=0.0, vy_mps=0.0, incomingVy_mps=incoming, outwardImpulse_Ns=impulse,
                       yBefore_m=fall.y_m, yAtRootUnclipped_m=residual,
                       yNextSampleUnclipped_m=fall.evaluate(next_sample)[0],
                       fromMass_kg={"F": 0, "P": MASS_KG} if release else {"F": MASS_KG, "P": 0},
                       toMass_kg={"F": MASS_KG, "P": 0}))
    events.sort(key=lambda event: event["time_s"])
    events_at = {}
    for event in events:
        next_index = bisect_left(output_times, event["time_s"])
        event["nextSample_s"] = output_times[next_index]
        events_at.setdefault(next_index, []).append(event)
    rows, baseline = [], None
    index = 0
    identity = dict(phase_ms=phase_ms, pulse_ms=duration_ms, dt_ms=step_ms)
    for sample_index, time in enumerate(output_times):
        while index + 1 < len(segments) and time >= segments[index].end_s:
            index += 1
        segment = segments[index]
        y, v, impulse = segment.evaluate(time)
        if baseline is None and y > 0 and impulse > 0:
            baseline = dict(kind="FIRST_POSITIVE_SAMPLE", time_s=time, y_m=y, vy_mps=v, outwardImpulse_Ns=impulse)
            events_at.setdefault(sample_index, []).append(baseline)
        is_p = release is not None and release["time_s"] <= time < hit
        load_interval = ((0.0, pulse_start) if time < pulse_start else
                         (pulse_start, pulse_end) if time < pulse_end else (pulse_end, END_MS / 1000))
        rows.append(dict(**identity, t_s=time, sourceId=SOURCE_ID, normal_N=segment.normal_N,
                         loadInterval_s=load_interval,
                         trialActive=segment.in_flight, y_m=y, vy_mps=v, outwardImpulse_Ns=impulse,
                         S_kg=0, F_kg=0 if is_p else MASS_KG, P_kg=MASS_KG if is_p else 0,
                         events=events_at.get(sample_index, [])))
    return dict(**identity, segments=[asdict(s) for s in segments], baseline=baseline,
                events=events, samples=rows)


def reference(duration_ms):
    tau = duration_ms / 1000
    publish = None
    if tau**2 > HEIGHT_M:
        if tau**2 / 2 >= HEIGHT_M:
            publish = math.sqrt(2 * HEIGHT_M)
        else:
            publish = 2 * tau - math.sqrt(2 * (tau**2 - HEIGHT_M))
    return dict(tau=tau, publish_s=publish, apex_s=2*tau, apex_m=tau**2,
                return_s=(2 + math.sqrt(2)) * tau, incomingVy_mps=-math.sqrt(2)*tau)


def reference_state(s, expected):
    tau = expected["tau"]
    if s <= 0 or s >= expected["return_s"]:
        return 0.0, 0.0
    if s < tau:
        return s**2 / 2, s
    u = s - tau
    return tau**2 / 2 + tau * u - u**2 / 2, tau - u


def assess(runs):
    checks = []

    def check(identity, name, passed, **details):
        checks.append(dict(**identity, check=name, passed=bool(passed), **details))

    def near(identity, name, observed, expected):
        check(identity, name, abs(observed - expected) <= TOL, observed=observed, reference=expected)

    for result in runs:
        identity = {k: result[k] for k in ("phase_ms", "pulse_ms", "dt_ms")}
        expected = reference(result["pulse_ms"])
        offset = result["phase_ms"] / 1000
        rows = result["samples"]
        events = {event["kind"]: event for event in result["events"]}
        release = events.get("F_TO_P")
        expected_order = (["APEX", "TRIAL_RETURN"] if expected["publish_s"] is None
                          else ["F_TO_P", "APEX", "P_TO_F"])
        check(identity, "candidate_order", [e["kind"] for e in result["events"]] == expected_order,
              observed=[e["kind"] for e in result["events"]], reference=expected_order)
        check(identity, "baseline_positive_sample", result["baseline"] is not None,
              observed=result["baseline"], falsePositive=expected["publish_s"] is None)
        if release and expected["publish_s"] is not None:
            publish = expected["publish_s"]
            near(identity, "release_time_s", release["time_s"] - offset, publish)
            near(identity, "release_height_m", release["y_m"], HEIGHT_M)
            near(identity, "release_velocity_mps", release["vy_mps"], reference_state(publish, expected)[1])
            near(identity, "release_impulse_Ns", release["outwardImpulse_Ns"], min(publish, expected["tau"]))
            check(identity, "release_pulse_side", (release["time_s"] - offset > expected["tau"]) ==
                  (expected["publish_s"] > expected["tau"]))
            check(identity, "release_inherits_trial_state", release["vy_mps"] > 0 and
                  release["outwardImpulse_Ns"] > 0 and release["nextSample_s"] >= release["time_s"])
        apex = events["APEX"]
        near(identity, "apex_time_s", apex["time_s"] - offset, expected["apex_s"])
        near(identity, "apex_height_m", apex["y_m"], expected["apex_m"])
        impact = result["events"][-1]
        near(identity, "return_time_s", impact["time_s"] - offset, expected["return_s"])
        near(identity, "return_incoming_velocity_mps", impact["incomingVy_mps"], expected["incomingVy_mps"])
        near(identity, "return_height_m", impact["yAtRootUnclipped_m"], 0.0)
        check(identity, "descending_crossing", impact["incomingVy_mps"] < 0 and impact["yBefore_m"] > 0
              and impact["yNextSampleUnclipped_m"] <= 1e-12)
        first_invalid = None
        for row in rows:
            exact_y, exact_v = reference_state(row["t_s"] - offset, expected)
            p = MASS_KG if release and release["time_s"] <= row["t_s"] < impact["time_s"] else 0
            if (not math.isfinite(row["y_m"]) or not math.isfinite(row["vy_mps"])
                    or row["sourceId"] != SOURCE_ID or row["S_kg"] != 0
                    or row["normal_N"] != (1 if offset <= row["t_s"] <
                                           (result["phase_ms"] + result["pulse_ms"]) / 1000 else -1)
                    or row["F_kg"] != MASS_KG - p or row["P_kg"] != p
                    or row["y_m"] < -1e-9 or abs(row["y_m"] - exact_y) > TOL
                    or abs(row["vy_mps"] - exact_v) > TOL
                    or (row["t_s"] >= impact["time_s"] and max(abs(row["y_m"]), abs(row["vy_mps"])) > 1e-9)):
                first_invalid = dict(sample=row, referenceY_m=exact_y, referenceVy_mps=exact_v)
                break
        check(identity, "all_samples_trajectory_mass_and_absorption", first_invalid is None, firstInvalid=first_invalid)
    for duration_ms in PULSE_MS:
        group = [r for r in runs if r["pulse_ms"] == duration_ms]
        identity = dict(pulse_ms=duration_ms)
        orders = [[e["kind"] for e in r["events"]] for r in group]
        check(identity, "all_steps_and_phases_event_order", all(order == orders[0] for order in orders))
        times = [[event["time_s"] - r["phase_ms"] / 1000 for event in r["events"]] for r in group]
        drift = max((abs(a - b) for p, q in combinations(times, 2) for a, b in zip(p, q)), default=0.0)
        check(identity, "all_steps_and_phases_event_times", drift <= 2e-6, maxDifference_s=drift)
    segments, (_, apex), _ = trial(0, 50)
    check({}, "tangent_apex_does_not_publish", outward_crossing(segments, apex) is None, threshold_m=apex)
    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    runs = [run(phase, duration, step) for duration in PULSE_MS for phase in PHASE_MS for step in STEP_MS]
    checks = assess(runs)
    rows = [row for result in runs for row in result.pop("samples")]
    raw = "".join(json.dumps(row, allow_nan=False, separators=(",", ":")) + "\n" for row in rows).encode()
    (args.out / "samples.jsonl").write_bytes(raw)
    first = next((c for c in checks if not c["passed"]), None)
    report = dict(experiment="T871_PULSE_RELEASE_V1", status="FAIL" if first else "PASS",
                  environment=dict(os=platform.platform(), python=platform.python_version(), machine=platform.machine()),
                  seed=0, mass_kg=MASS_KG, sourceId=SOURCE_ID, threshold_m=HEIGHT_M, window_ms=END_MS,
                  tangential_N=0, beforePulse_N=-1, duringPulse_N=1, afterPulse_N=-1,
                  implementationSHA256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  samplesSHA256=hashlib.sha256(raw).hexdigest(), sampleCount=len(rows),
                  runs=runs, checks=checks, firstFailure=first)
    (args.out / "summary.json").write_bytes((json.dumps(report, indent=2, allow_nan=False) + "\n").encode())
    print(f"{report['status']} - {len(runs)} runs, {len(rows)} samples, {sum(c['passed'] for c in checks)}/{len(checks)} checks")
    return int(first is not None)


if __name__ == "__main__":
    raise SystemExit(main())
