"""T870 offline single-parcel experiment. Requires only the Python standard library."""

import argparse
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import platform


MASS_KG = 1.0
MU = 0.50
SOURCE_ID = 1
PULSE_END_MS = 100
END_MS = 400
STEPS_MS = (10, 5)
LOADS = {
    "A": ((0.0, -10.0), (0.0, -10.0)),
    "B": ((6.0, -10.0), (0.0, -10.0)),
    "C": ((6.0, 1.0), (0.0, -1.0)),
}


@dataclass
class Parcel:
    state: str = "S"
    x: float = 0.0
    vx: float = 0.0
    y: float = 0.0
    vy: float = 0.0
    outward_impulse: float = 0.0
    captured: bool = False

    def advance(self, t, dt, tangential, normal):
        events = []

        def transition(target, when, **observations):
            events.append(dict(fromState=self.state, toState=target, time_s=when, **observations))
            self.state = target

        if self.captured:
            return events
        if self.state == "P" or normal > 0:
            acceleration = normal / MASS_KG
            next_y = self.y + self.vy * dt + 0.5 * acceleration * dt**2
            next_vy = self.vy + acceleration * dt
            self.outward_impulse += max(normal, 0.0) * dt
            if self.state == "P" and next_y <= 0 and next_vy < 0:
                # First descending zero inside this constant-acceleration step.
                incoming = -math.sqrt(self.vy**2 - 2 * acceleration * self.y)
                crossing = (incoming - self.vy) / acceleration
                if not (0 <= crossing <= dt + 1e-12 and incoming < 0):
                    raise ValueError(f"capture outside its step at {t:g} s")
                transition("F", t + min(crossing, dt), yBefore_m=self.y,
                           yUnclipped_m=next_y, incomingVy_mps=incoming)
                self.y = self.vy = 0.0
                self.captured = True
            else:
                self.y, self.vy = next_y, next_vy
                if self.state != "P" and self.y > 0 and self.outward_impulse > 0:
                    transition("P", t + dt, firstPositiveY_m=self.y,
                               outwardImpulse_Ns=self.outward_impulse)
            return events

        friction = MU * -normal
        if self.state == "S":
            if abs(tangential) <= friction:
                return events
            direction = math.copysign(1.0, tangential)
            transition("F", t)
        else:
            direction = math.copysign(1.0, self.vx)
        acceleration = (tangential - direction * friction) / MASS_KG
        next_vx = self.vx + acceleration * dt
        if acceleration * direction < 0 and next_vx * direction <= 1e-14:
            stop = min(-self.vx / acceleration, dt)
            self.x += self.vx * stop + 0.5 * acceleration * stop**2
            self.vx = 0.0
            transition("S", t + stop, x_m=self.x)
        else:
            self.x += self.vx * dt + 0.5 * acceleration * dt**2
            self.vx = next_vx
        return events


def load(case, at_ms):
    return LOADS[case][int(at_ms >= PULSE_END_MS)]


def simulate(case, step_ms):
    parcel = Parcel()
    rows = []
    for at_ms in range(0, END_MS + 1, step_ms):
        current = load(case, at_ms)
        applied = load(case, at_ms - step_ms) if at_ms else None
        events = parcel.advance((at_ms - step_ms) / 1000, step_ms / 1000, *applied) if applied else []
        rows.append(dict(
            case=case, dt_s=step_ms / 1000, t_s=at_ms / 1000, sourceId=SOURCE_ID,
            T_N=current[0], N_N=current[1], appliedPreviousInterval=applied,
            baselineLift=abs(current[0]) > 5.0, state=parcel.state,
            S_kg=MASS_KG if parcel.state == "S" else 0.0,
            F_kg=MASS_KG if parcel.state == "F" else 0.0,
            P_kg=MASS_KG if parcel.state == "P" else 0.0,
            x_m=None if case == "C" else parcel.x, vx_mps=None if case == "C" else parcel.vx,
            y_m=parcel.y, vy_mps=parcel.vy, outwardImpulse_Ns=parcel.outward_impulse, events=events,
        ))
    return rows


def assess(runs):
    checks = []
    metrics = {}

    def check(case, step, name, passed, **details):
        checks.append(dict(case=case, dt_ms=step, check=name, passed=bool(passed), **details))

    # Independent closed-form reference. B brakes under 5 m/s² from 0.10 m/s.
    # C reaches its apex after another 0.10 s, then falls from 0.010 m at 1 m/s².
    capture_reference = 0.2 + math.sqrt(2 * 0.010)
    baseline = {}
    for step in STEPS_MS:
        baseline[step] = {case: any(r["baselineLift"] for r in runs[case, step]) for case in LOADS}
        check("A/B/C", step, "baseline_has_B_false_lift", baseline[step] == {"A": False, "B": True, "C": True},
              observed=baseline[step])
        check("B/C", step, "same_tangential_input",
              [r["T_N"] for r in runs["B", step]] == [r["T_N"] for r in runs["C", step]])
        for case in LOADS:
            rows = runs[case, step]
            events = [e for row in rows for e in row["events"]]
            order = [(e["fromState"], e["toState"]) for e in events]
            check(case, step, "event_order", order == {"A": [], "B": [("S", "F"), ("F", "S")],
                                                       "C": [("S", "P"), ("P", "F")]}[case], observed=order)
            invalid = next((r for r in rows if r["sourceId"] != SOURCE_ID
                            or any(not math.isfinite(r[key]) for key in ("y_m", "vy_mps", "outwardImpulse_Ns"))
                            or r["y_m"] < -1e-9
                            or abs(sum(r[k] for k in ("S_kg", "F_kg", "P_kg")) - MASS_KG) > 1e-12
                            or any(r[f"{s}_kg"] != (MASS_KG if s == r["state"] else 0)
                                   for s in "SFP")), None)
            check(case, step, "unique_mass_source_and_no_penetration", invalid is None, firstInvalidSample=invalid)
            pulse_end = rows[PULSE_END_MS // step]
            if case in ("A", "B"):
                invalid = next((r for r in rows if r["P_kg"] != 0 or abs(r["y_m"]) > 1e-9
                                or abs(r["vy_mps"]) > 1e-9 or not math.isfinite(r["x_m"])
                                or not math.isfinite(r["vx_mps"])
                                or (case == "A" and (r["state"] != "S" or abs(r["x_m"]) > 1e-9
                                                     or abs(r["vx_mps"]) > 1e-9))), None)
                check(case, step, "supported_at_all_samples", invalid is None, firstInvalidSample=invalid)
            if case == "B":
                stop = next((e for e in events if e["toState"] == "S"), {})
                metrics[case, step] = [
                    ("pulse_x_m", pulse_end["x_m"], 0.005, 0.001),
                    ("pulse_vx_mps", pulse_end["vx_mps"], 0.10, 0.01),
                    ("stop_s", stop.get("time_s"), 0.12, step / 1000),
                    ("stop_x_m", stop.get("x_m"), 0.006, 0.001),
                ]
                settled = [r for r in rows if r["t_s"] >= stop.get("time_s", math.inf) - 1e-12]
                check(case, step, "remains_stopped", bool(settled) and
                      all(r["state"] == "S" and abs(r["vx_mps"]) <= 1e-9
                          and abs(r["x_m"] - stop["x_m"]) <= 1e-9 for r in settled))
            if case == "C":
                takeoff = next((e for e in events if e["toState"] == "P"), {})
                capture = next((e for e in events if e["fromState"] == "P"), {})
                peak = max(rows, key=lambda r: r["y_m"])
                metrics[case, step] = [
                    ("pulse_y_m", pulse_end["y_m"], 0.005, 0.001),
                    ("pulse_vy_mps", pulse_end["vy_mps"], 0.10, 0.01),
                    ("peak_y_m", peak["y_m"], 0.010, 0.001),
                    ("peak_s", peak["t_s"], 0.20, step / 1000),
                    ("capture_s", capture.get("time_s"), capture_reference, step / 1000),
                ]
                check(case, step, "positive_takeoff_evidence", takeoff.get("firstPositiveY_m", 0) > 0
                      and takeoff.get("outwardImpulse_Ns", 0) > 0, event=takeoff)
                check(case, step, "descending_zero_and_continuous_P",
                      bool(capture and takeoff) and capture["yBefore_m"] > 0 >= capture["yUnclipped_m"]
                      and capture["incomingVy_mps"] < 0
                      and all(r["state"] == "P" for r in rows
                              if takeoff["time_s"] - 1e-12 <= r["t_s"] < capture["time_s"]), event=capture)
                after = [r for r in rows if r["t_s"] >= capture.get("time_s", math.inf)]
                check(case, step, "captured_mass_stays_in_F", bool(after) and
                      all(r["state"] == "F" and r["P_kg"] == 0 and abs(r["y_m"]) <= 1e-9
                          and abs(r["vy_mps"]) <= 1e-9 for r in after), firstCaptured=after[0] if after else None)
    for (case, step), values in metrics.items():
        for name, actual, reference, tolerance in values:
            error = abs(actual - reference) if actual is not None and math.isfinite(actual) else None
            check(case, step, name, error is not None and error <= tolerance,
                  observed=actual, reference=reference, tolerance=tolerance, absoluteError=error)
    for case in ("B", "C"):
        for coarse, fine in zip(metrics[case, STEPS_MS[0]], metrics[case, STEPS_MS[1]], strict=True):
            name, actual_coarse, reference, _ = coarse
            _, actual_fine, _, _ = fine
            a = abs(actual_coarse - reference) if actual_coarse is not None else None
            b = abs(actual_fine - reference) if actual_fine is not None else None
            check(case, STEPS_MS[1], "half_step:" + name, a is not None and b is not None and b <= a + 1e-9,
                  coarseAbsoluteError=a, fineAbsoluteError=b)
    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    runs = {(case, step): simulate(case, step) for step in STEPS_MS for case in LOADS}
    checks = assess(runs)
    rows = [r for run in runs.values() for r in run]
    samples = "".join(json.dumps(row, separators=(",", ":"), allow_nan=False) + "\n" for row in rows).encode()
    (args.out / "samples.jsonl").write_bytes(samples)
    first = next((c for c in checks if not c["passed"]), None)
    report = dict(
        experiment="T870_CONTACT_LIFT_V1", status="FAIL" if first else "PASS", seed=0,
        environment=dict(os=platform.platform(), python=platform.python_version(), machine=platform.machine()),
        implementationSHA256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inputs=dict(mass_kg=MASS_KG, sourceId=SOURCE_ID, staticFriction=MU, dynamicFriction=MU,
                    dt_ms=STEPS_MS, pulseEnd_ms=PULSE_END_MS, end_ms=END_MS, loads_T_N_N_N=LOADS),
        sampleCount=len(rows), samplesSHA256=hashlib.sha256(samples).hexdigest(), checks=checks, firstFailure=first,
    )
    (args.out / "summary.json").write_bytes((json.dumps(report, indent=2, allow_nan=False) + "\n").encode())
    print(f"{report['status']} · {len(rows)} samples · {sum(c['passed'] for c in checks)}/{len(checks)} checks")
    return int(first is not None)


if __name__ == "__main__":
    raise SystemExit(main())
