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
            payload = json.loads(match.group(2))
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
        note="TODO(product-hook): missing " + ", ".join(missing),
    )


def evaluate_conservation(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): WS_METRIC must expose driftWater, driftHeavy, driftLight.
    rows = metrics(records)
    fields = ("driftWater", "driftHeavy", "driftLight")
    if not rows or any(field not in row for row in rows for field in fields):
        return blocked("conservation", list(fields))
    maxima = {field: max(abs(float(row[field])) for row in rows) for field in fields}
    passed = all(value < 0.001 for value in maxima.values())
    return CheckResult(
        name="conservation",
        status="PASS" if passed else "FAIL",
        measured=maxima,
        threshold={field: "< 0.001" for field in fields},
        evidence_count=len(rows),
    )


def evaluate_sorting(records: list[dict[str, Any]]) -> CheckResult:
    # TODO(product-hook): WS_METRIC must expose separationRatio and both shares.
    rows = metrics(records)
    fields = ("separationRatio", "lowerHeavyShare", "upperHeavyShare")
    if not rows or any(field not in row for row in rows for field in fields):
        return blocked("sorting", list(fields))
    final = rows[-1]
    if final.get("notApplicable") is True:
        return CheckResult(
            name="sorting",
            status="N/A",
            measured={field: final[field] for field in fields},
            threshold={"separationRatio": "<= 0.5"},
            evidence_count=len(rows),
            note="Single phase or empty sorting regions.",
        )
    value = float(final["separationRatio"])
    return CheckResult(
        name="sorting",
        status="PASS" if value <= 0.5 else "FAIL",
        measured={field: final[field] for field in fields},
        threshold={"separationRatio": "<= 0.5"},
        evidence_count=len(rows),
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
    synthetic = [
        {
            "type": "METRIC",
            "data": {
                "driftWater": 0.0,
                "driftHeavy": 0.02,
                "driftLight": -0.02,
            },
        }
    ]
    result = evaluate_conservation(synthetic)
    if result.status != "FAIL":
        result.status = "ERROR"
        result.note = "Defect sentinel unexpectedly did not fail."
    else:
        result.note = "Algorithm v1 §3.3 synthetic defect correctly exposed."
    return [result]


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
