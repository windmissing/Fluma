#!/usr/bin/env python3
"""Objective checker for P-003/P-005 WaterSand WS_* logs.

This is a tester-owned skeleton. It parses the agreed hook format and applies
the algorithm thresholds. Missing product hooks are reported as BLOCKED rather
than silently passing.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from statistics import quantiles
from typing import Any


HOOK_RE = re.compile(r"WS_(METRIC|FRAME|EVENT|SNAPSHOT)(?:\s+([A-Za-z0-9_]+))?\s+(\{.*\})")


@dataclass
class CheckResult:
    name: str
    status: str
    measured: dict[str, Any]
    threshold: dict[str, Any]
    evidence_count: int = 0
    note: str = ""


def parse_lines(lines: list[str]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for line_number, line in enumerate(lines, start=1):
        match = HOOK_RE.search(line)
        if not match:
            continue
        hook = match.group(1)
        subtype = match.group(2)
        try:
            payload = json.loads(match.group(3))
        except json.JSONDecodeError as error:
            records.append(
                {
                    "type": hook,
                    "subtype": subtype,
                    "line": line_number,
                    "data": {},
                    "parse_error": str(error),
                }
            )
            continue
        records.append({"type": hook, "subtype": subtype, "line": line_number, "data": payload})
    return records


def metrics(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [record["data"] for record in records if record["type"] == "METRIC"]


def frames(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [record["data"] for record in records if record["type"] == "FRAME"]


def events(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    parsed_events: list[dict[str, Any]] = []
    for record in records:
        if record["type"] != "EVENT":
            continue
        event = dict(record["data"])
        event.setdefault("type", record.get("subtype"))
        parsed_events.append(event)
    return parsed_events


def snapshot_records(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [record for record in records if record["type"] == "SNAPSHOT"]


def metric_records(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [record for record in records if record["type"] == "METRIC"]


def nearest_metric_record(
    snapshot_record: dict[str, Any], records: list[dict[str, Any]]
) -> dict[str, Any] | None:
    rows = metric_records(records)
    if not rows:
        return None
    snapshot_line = snapshot_record.get("line", 0)
    preceding = [row for row in rows if row.get("line", 0) <= snapshot_line]
    if preceding:
        return max(preceding, key=lambda row: row.get("line", 0))
    return min(rows, key=lambda row: abs(row.get("line", 0) - snapshot_line))


def build_segments(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    # TODO(product-hook): align event line numbers with hilog/record timestamps.
    # The final implementation will split init, tilt, shake, invert, rest, and
    # long_rest phases for per-segment conservation checks.
    boundaries = [
        index
        for index, record in enumerate(records)
        if record["type"] == "EVENT"
        and record.get("subtype") in {"init", "reset", "tilt", "shake", "invert180", "dormant", "wake"}
    ]
    if not boundaries:
        return []
    starts = boundaries + [len(records)]
    return [
        {"name": records[start].get("subtype", "segment"), "records": records[start:ends]}
        for start, ends in zip(starts, starts[1:])
    ]


def blocked(name: str, missing: list[str]) -> CheckResult:
    return CheckResult(
        name=name,
        status="BLOCKED",
        measured={},
        threshold={},
        note="BLOCKED(missing_hook): missing " + ", ".join(missing),
    )


SNAPSHOT_FIELDS = (
    "schemaVersion",
    "cols",
    "capacity",
    "waterTop",
    "bh",
    "bl",
    "mh",
    "ml",
    "suspH",
    "suspL",
    "hs",
    "hl",
    "sandTop",
)


def validate_snapshot(data: dict[str, Any]) -> tuple[list[str], list[str]]:
    missing = [field for field in SNAPSHOT_FIELDS if field not in data]
    if missing:
        return missing, []
    invalid: list[str] = []
    if data["schemaVersion"] != 2:
        invalid.append("schemaVersion")
    if not isinstance(data["capacity"], int) or data["capacity"] <= 0:
        invalid.append("capacity")
    cols = data["cols"]
    if not isinstance(cols, int) or cols <= 0:
        invalid.append("cols")
        return missing, invalid
    for field in SNAPSHOT_FIELDS[3:]:
        values = data[field]
        if not isinstance(values, list) or len(values) != cols:
            invalid.append(field)
            continue
        if any(not isinstance(value, (int, float)) or not math.isfinite(value) for value in values):
            invalid.append(field)
    if invalid:
        return missing, invalid
    for index in range(cols):
        if (
            abs(data["hs"][index] - data["bh"][index] - data["mh"][index]) > 1e-9
            or abs(data["hl"][index] - data["bl"][index] - data["ml"][index]) > 1e-9
            or abs(data["sandTop"][index] - data["hs"][index] - data["hl"][index]) > 1e-9
        ):
            invalid.append("derived hs/hl/sandTop")
            break
    return missing, invalid


def snapshot_totals(data: dict[str, Any]) -> dict[str, float]:
    water = 0.0
    heavy = 0.0
    light = 0.0
    for index in range(data["cols"]):
        water += max(
            0.0,
            data["waterTop"][index]
            - data["sandTop"][index]
            - data["suspH"][index]
            - data["suspL"][index],
        )
        heavy += data["bh"][index] + data["mh"][index] + data["suspH"][index]
        light += data["bl"][index] + data["ml"][index] + data["suspL"][index]
    return {"water": water, "heavy": heavy, "light": light}


def snapshot_drift(data: dict[str, Any], initial: dict[str, float]) -> dict[str, float]:
    totals = snapshot_totals(data)
    return {
        phase: abs(totals[phase] - initial[phase]) / max(initial[phase], 1e-9)
        for phase in ("water", "heavy", "light")
    }


def snapshot_separation_profile(data: dict[str, Any]) -> dict[str, float | None]:
    low_heavy = 0.0
    low_light = 0.0
    high_heavy = 0.0
    high_light = 0.0
    has_heavy = False
    has_light = False
    epsilon = 1e-9
    for index in range(data["cols"]):
        bh = data["bh"][index]
        bl = data["bl"][index]
        mh = data["mh"][index]
        ml = data["ml"][index]
        sand_top = data["sandTop"][index]
        has_heavy = has_heavy or bh + mh > epsilon
        has_light = has_light or bl + ml > epsilon
        lower_bound = sand_top / 3
        upper_bound = 2 * sand_top / 3
        mix_start = bh + bl
        mix_total = max(mh + ml, epsilon)
        lower_mix = max(0.0, min(lower_bound, sand_top) - max(0.0, mix_start))
        upper_mix = max(0.0, sand_top - max(upper_bound, mix_start))
        low_heavy += min(bh, lower_bound) + lower_mix * mh / mix_total
        low_light += max(0.0, min(bh + bl, lower_bound) - bh) + lower_mix * ml / mix_total
        high_heavy += max(0.0, min(bh, sand_top) - upper_bound) + upper_mix * mh / mix_total
        high_light += (
            max(0.0, min(bh + bl, sand_top) - max(bh, upper_bound))
            + upper_mix * ml / mix_total
        )
    if (
        not has_heavy
        or not has_light
        or low_heavy + low_light <= epsilon
        or high_heavy + high_light <= epsilon
    ):
        return {"lowerHeavyShare": None, "upperHeavyShare": None, "separationRatio": None}
    lower_heavy_share = low_heavy / (low_heavy + low_light)
    upper_heavy_share = high_heavy / (high_heavy + high_light)
    return {
        "lowerHeavyShare": lower_heavy_share,
        "upperHeavyShare": upper_heavy_share,
        "separationRatio": upper_heavy_share / max(lower_heavy_share, epsilon),
    }


def conservation_status(values: dict[str, Any]) -> str:
    return "PASS" if all(abs(float(value)) < 0.001 for value in values.values()) else "FAIL"


def sorting_status(value: Any) -> str:
    if value is None:
        return "N/A"
    return "PASS" if float(value) <= 0.5 else "FAIL"


def evaluate_conservation(records: list[dict[str, Any]]) -> CheckResult:
    rows = metric_records(records)
    ws_snapshots = snapshot_records(records)
    fields = ("driftWater", "driftHeavy", "driftLight")
    missing = ["WS_SNAPSHOT"] if not ws_snapshots else []
    if not rows:
        missing.append("WS_METRIC")
    elif any(
        field not in row["data"] or row["data"].get("ledgerVersion") != 2
        for row in rows
        for field in ("ledgerVersion", *fields)
    ):
        missing.extend(fields)
    if missing:
        return blocked("conservation", missing)
    for row in ws_snapshots:
        snapshot_missing, snapshot_invalid = validate_snapshot(row["data"])
        if snapshot_missing or snapshot_invalid:
            return CheckResult(
                name="conservation",
                status="BLOCKED",
                measured={},
                threshold={},
                evidence_count=len(ws_snapshots),
                note="BLOCKED(missing_hook): " + ", ".join(snapshot_missing + snapshot_invalid),
            )
    initial = snapshot_totals(ws_snapshots[0]["data"])
    snapshot_maxima = {
        phase: max(snapshot_drift(row["data"], initial)[phase] for row in ws_snapshots)
        for phase in ("water", "heavy", "light")
    }
    metric_maxima = {
        field: max(abs(float(row["data"][field])) for row in rows) for field in fields
    }
    mismatched = 0
    for row in ws_snapshots:
        metric = nearest_metric_record(row, records)
        if metric is None:
            mismatched += 1
            continue
        metric_values = {field: metric["data"][field] for field in fields}
        snapshot_values = snapshot_drift(row["data"], initial)
        if conservation_status(snapshot_values) != conservation_status(metric_values):
            mismatched += 1
    passed = (
        all(value < 0.001 for value in snapshot_maxima.values())
        and all(value < 0.001 for value in metric_maxima.values())
        and mismatched == 0
    )
    return CheckResult(
        name="conservation",
        status="PASS" if passed else "FAIL",
        measured={
            "snapshotDriftWater": snapshot_maxima["water"],
            "snapshotDriftHeavy": snapshot_maxima["heavy"],
            "snapshotDriftLight": snapshot_maxima["light"],
            "metricDriftWater": metric_maxima["driftWater"],
            "metricDriftHeavy": metric_maxima["driftHeavy"],
            "metricDriftLight": metric_maxima["driftLight"],
        },
        threshold={field: "< 0.001" for field in fields},
        evidence_count=len(ws_snapshots),
        note="" if mismatched == 0 else f"HG-06 conclusion mismatch on {mismatched} snapshots.",
    )


def evaluate_sorting(records: list[dict[str, Any]]) -> CheckResult:
    rows = metric_records(records)
    ws_snapshots = snapshot_records(records)
    fields = ("separationRatio", "lowerHeavyShare", "upperHeavyShare")
    missing = ["WS_SNAPSHOT"] if not ws_snapshots else []
    if not rows:
        missing.append("WS_METRIC")
    elif any(
        field not in row["data"] or row["data"].get("ledgerVersion") != 2
        for row in rows
        for field in ("ledgerVersion", *fields)
    ):
        missing.extend(fields)
    if missing:
        return blocked("sorting", missing)
    for row in ws_snapshots:
        snapshot_missing, snapshot_invalid = validate_snapshot(row["data"])
        if snapshot_missing or snapshot_invalid:
            return CheckResult(
                name="sorting",
                status="BLOCKED",
                measured={},
                threshold={},
                evidence_count=len(ws_snapshots),
                note="BLOCKED(missing_hook): " + ", ".join(snapshot_missing + snapshot_invalid),
            )
    final_metric = nearest_metric_record(ws_snapshots[-1], records)
    if final_metric is None:
        return blocked("sorting", ["WS_METRIC"])
    snapshot_profile = snapshot_separation_profile(ws_snapshots[-1]["data"])
    metric_profile = {field: final_metric["data"][field] for field in fields}
    snapshot_conclusion = sorting_status(snapshot_profile["separationRatio"])
    metric_conclusion = sorting_status(metric_profile["separationRatio"])
    mismatched = 0
    for row in ws_snapshots:
        metric = nearest_metric_record(row, records)
        if metric is None:
            mismatched += 1
            continue
        row_profile = snapshot_separation_profile(row["data"])
        if sorting_status(row_profile["separationRatio"]) != sorting_status(metric["data"]["separationRatio"]):
            mismatched += 1
    if snapshot_conclusion == "N/A":
        return CheckResult(
            name="sorting",
            status="N/A",
            measured={"snapshot": snapshot_profile, "metric": metric_profile},
            threshold={"separationRatio": "<= 0.5"},
            evidence_count=len(ws_snapshots),
            note="Single phase or empty sorting regions.",
        )
    value = snapshot_profile["separationRatio"]
    passed = (
        value is not None
        and value <= 0.5
        and snapshot_conclusion == metric_conclusion
        and mismatched == 0
    )
    return CheckResult(
        name="sorting",
        status="PASS" if passed else "FAIL",
        measured={"snapshot": snapshot_profile, "metric": metric_profile},
        threshold={"separationRatio": "<= 0.5"},
        evidence_count=len(ws_snapshots),
        note=""
        if mismatched == 0
        else f"HG-06 conclusion mismatch on {mismatched} snapshots.",
    )


def evaluate_settling(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): WS_METRIC must expose t90Heavy and t90Light.
    rows = metrics(records)
    final = rows[-1] if rows else {}
    if "t90Heavy" not in final or "t90Light" not in final:
        return blocked("settling", ["t90Heavy", "t90Light"])
    heavy = float(final["t90Heavy"])
    light = float(final["t90Light"])
    if light <= 0:
        return CheckResult(
            name="settling",
            status="FAIL",
            measured={"t90Heavy": heavy, "t90Light": light},
            threshold={"t90Heavy/t90Light": "<= 0.7"},
            evidence_count=len(rows),
            note="t90Light must be positive.",
        )
    ratio = heavy / light
    passed = heavy < light and ratio <= 0.7
    return CheckResult(
        name="settling",
        status="PASS" if passed else "FAIL",
        measured={"t90Heavy": heavy, "t90Light": light, "ratio": ratio},
        threshold={"t90Heavy/t90Light": "<= 0.7", "order": "t90Heavy < t90Light"},
        evidence_count=len(rows),
    )


def evaluate_angle(records: list[dict[str, Any]], angle_deg: float = 30.0) -> CheckResult:
    # TODO(product-hook): WS_METRIC must expose slope and curvature fields.
    rows = metrics(records)
    final = rows[-1] if rows else {}
    if "maxSlopeDeg" not in final or "maxCurvature" not in final:
        return blocked("angle", ["maxSlopeDeg", "maxCurvature"])
    slope = float(final["maxSlopeDeg"])
    curvature = float(final["maxCurvature"])
    passed = slope <= angle_deg + 2.0 and curvature <= 0.35
    return CheckResult(
        name="angle",
        status="PASS" if passed else "FAIL",
        measured={"maxSlopeDeg": slope, "maxCurvature": curvature},
        threshold={"maxSlopeDeg": f"<= {angle_deg + 2.0:.1f}", "maxCurvature": "<= 0.35"},
        evidence_count=len(rows),
    )


def evaluate_water(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): WS_METRIC must expose water slope and amplitude.
    rows = metrics(records)
    final = rows[-1] if rows else {}
    if "waterSlopeDeg" not in final or "waterAmplitudePx" not in final:
        return blocked("water", ["waterSlopeDeg", "waterAmplitudePx"])
    slope = float(final["waterSlopeDeg"])
    amplitude = float(final["waterAmplitudePx"])
    passed = slope <= 2.0 and amplitude <= 2.0
    return CheckResult(
        name="water",
        status="PASS" if passed else "FAIL",
        measured={"waterSlopeDeg": slope, "waterAmplitudePx": amplitude},
        threshold={"waterSlopeDeg": "<= 2.0", "waterAmplitudePx": "<= 2.0"},
        evidence_count=len(rows),
    )


def evaluate_invert(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): WS_EVENT invert180 and WS_METRIC phaseOrderScore.
    rows = metrics(records)
    has_invert = any(event.get("type") == "invert180" for event in events(records))
    final = rows[-1] if rows else {}
    if not has_invert or "phaseOrderScore" not in final:
        return blocked("invert", ["WS_EVENT invert180", "phaseOrderScore"])
    score = float(final["phaseOrderScore"])
    return CheckResult(
        name="invert",
        status="PASS" if score >= 0.30 else "FAIL",
        measured={"phaseOrderScore": score},
        threshold={"phaseOrderScore": ">= 0.30"},
        evidence_count=len(rows),
    )


def evaluate_shake(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): WS_EVENT shake and WS_METRIC suspensionFraction.
    rows = metrics(records)
    has_shake = any(event.get("type") == "shake" for event in events(records))
    values = [float(row["suspensionFraction"]) for row in rows if "suspensionFraction" in row]
    if not has_shake or not values:
        return blocked("shake", ["WS_EVENT shake", "suspensionFraction"])
    peak = max(values)
    residual = values[-1]
    passed = peak >= 0.5 and residual <= 0.05
    return CheckResult(
        name="shake",
        status="PASS" if passed else "FAIL",
        measured={"peakSusp": peak, "residualSusp": residual},
        threshold={"peakSusp": ">= 0.5", "residualSusp": "<= 0.05"},
        evidence_count=len(values),
    )


def evaluate_long_rest(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): expose bedDrift, or two timestamped WS_SNAPSHOT records.
    rows = metrics(records)
    final = rows[-1] if rows else {}
    if "bedDrift" not in final:
        return blocked("long_rest", ["bedDrift or timestamped WS_SNAPSHOT.sandTop"])
    drift = float(final["bedDrift"])
    return CheckResult(
        name="long_rest",
        status="PASS" if drift <= 0.25 else "FAIL",
        measured={"bedDrift": drift},
        threshold={"bedDrift": "<= 0.25"},
        evidence_count=len(rows),
    )


def percentile_95(values: list[float]) -> float:
    if not values:
        return float("nan")
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    return quantiles(ordered, n=100, method="inclusive")[94]


def evaluate_fps(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): WS_FRAME.frameTimeMs; record environment refresh rate separately.
    rows = frames(records)
    values = [float(row["frameTimeMs"]) for row in rows if "frameTimeMs" in row]
    if len(values) < 100:
        return blocked("fps", [f"WS_FRAME samples (have {len(values)}, need 100)"])
    p95 = percentile_95(values)
    fps = 1000.0 / p95 if p95 > 0 else 0.0
    return CheckResult(
        name="fps",
        status="PASS" if fps >= 100.0 else "FAIL",
        measured={"frameTimeMsP95": p95, "fpsP95": fps},
        threshold={"fpsP95": ">= 100"},
        evidence_count=len(values),
    )


def self_check_defect() -> list[CheckResult]:
    metric = {
        "ledgerVersion": 2,
        "driftWater": 0.0,
        "driftHeavy": 0.0,
        "driftLight": 0.0,
        "lowerHeavyShare": 0.5,
        "upperHeavyShare": 0.5,
        "separationRatio": 1.0,
    }
    synthetic = [
        {
            "type": "METRIC",
            "subtype": None,
            "line": 1,
            "data": metric,
        },
        {
            "type": "SNAPSHOT",
            "subtype": None,
            "line": 2,
            "data": synthetic_snapshot(
                [4.0, 4.0],
                [1.0, 1.0],
                [1.0, 1.0],
                [0.0, 0.0],
                [0.0, 0.0],
                [0.0, 0.0],
                [0.0, 0.0],
            ),
        },
        {
            "type": "METRIC",
            "subtype": None,
            "line": 3,
            "data": metric,
        },
        {
            "type": "SNAPSHOT",
            "subtype": None,
            "line": 4,
            "data": synthetic_snapshot(
                [4.0, 4.0],
                [1.5, 1.5],
                [0.5, 0.5],
                [0.0, 0.0],
                [0.0, 0.0],
                [0.0, 0.0],
                [0.0, 0.0],
            ),
        },
    ]
    result = evaluate_conservation(synthetic)
    if result.status != "FAIL":
        result.status = "ERROR"
        result.note = "Defect sentinel unexpectedly did not fail."
    else:
        result.note = "Algorithm v2 synthetic phase-transfer defect correctly failed."
    return [result]


def synthetic_snapshot(
    water_top: list[float],
    bh: list[float],
    bl: list[float],
    mh: list[float],
    ml: list[float],
    susp_h: list[float],
    susp_l: list[float],
) -> dict[str, Any]:
    hs = [bh[index] + mh[index] for index in range(len(bh))]
    hl = [bl[index] + ml[index] for index in range(len(bl))]
    return {
        "schemaVersion": 2,
        "cols": len(bh),
        "capacity": 10,
        "waterTop": water_top,
        "bh": bh,
        "bl": bl,
        "mh": mh,
        "ml": ml,
        "suspH": susp_h,
        "suspL": susp_l,
        "hs": hs,
        "hl": hl,
        "sandTop": [hs[index] + hl[index] for index in range(len(hs))],
    }


def selected_results(
    records: list[dict[str, Any]], selectors: set[str], angle_deg: float
) -> list[CheckResult]:
    evaluators = {
        "conservation": lambda: evaluate_conservation(records),
        "sorting": lambda: evaluate_sorting(records),
        "settling": lambda: evaluate_settling(records),
        "angle": lambda: evaluate_angle(records, angle_deg),
        "water": lambda: evaluate_water(records),
        "invert": lambda: evaluate_invert(records),
        "shake": lambda: evaluate_shake(records),
        "long_rest": lambda: evaluate_long_rest(records),
        "fps": lambda: evaluate_fps(records),
    }
    names = set(evaluators) if "all" in selectors else selectors
    return [evaluators[name]() for name in sorted(names)]


def write_json(path: Path, results: list[CheckResult]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps([asdict(result) for result in results], ensure_ascii=False, indent=2)
        + "\n",
        encoding="utf-8",
    )


def write_markdown(path: Path, results: list[CheckResult]) -> None:
    lines = ["# WaterSand objective check", "", "| Check | Status | Measured | Threshold | Evidence |", "|---|---|---|---|---:|"]
    for result in results:
        measured = json.dumps(result.measured, ensure_ascii=False, sort_keys=True)
        threshold = json.dumps(result.threshold, ensure_ascii=False, sort_keys=True)
        lines.append(f"| {result.name} | {result.status} | `{measured}` | `{threshold}` | {result.evidence_count} |")
        if result.note:
            lines.append(f"- {result.note}")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path)
    parser.add_argument("--report", type=Path, default=Path("test/reports/ws_check.json"))
    parser.add_argument("--markdown", type=Path, default=Path("test/reports/ws_check.md"))
    parser.add_argument("--angle", type=float, default=30.0)
    parser.add_argument("--self-check-defect", action="store_true")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--all", action="store_true")
    group.add_argument(
        "--selectors",
        nargs="+",
        choices=[
            "conservation",
            "sorting",
            "settling",
            "angle",
            "water",
            "invert",
            "shake",
            "long_rest",
            "fps",
        ],
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.self_check_defect:
        results = self_check_defect()
    else:
        if args.input is None:
            print("--input is required unless --self-check-defect is used", file=sys.stderr)
            return 2
        records = parse_lines(args.input.read_text(encoding="utf-8", errors="replace").splitlines())
        selectors = {"all"} if args.all or args.selectors is None else set(args.selectors)
        results = selected_results(records, selectors, args.angle)

    write_json(args.report, results)
    write_markdown(args.markdown, results)
    for result in results:
        print(f"{result.name}: {result.status}")
        if result.note:
            print(f"  {result.note}")
    print(f"JSON: {args.report}")
    print(f"Markdown: {args.markdown}")

    if args.self_check_defect:
        return 0 if results[0].status == "FAIL" else 1
    return 0 if all(result.status in {"PASS", "N/A"} for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
