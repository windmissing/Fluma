#!/usr/bin/env python3
"""P-001 liquid-ca-m1p-v1.0 objective checker for TC-11..TC-13."""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any


CASES = ("TC-11", "TC-12", "TC-13")
PASS_LINE = 6
MAX_CENTROID_DISPLACEMENT_CELLS = 1.0
MAX_PIXEL_CLASSIFICATION_CHANGE = 0.001
TAG_PATTERN = re.compile(r'(?:LIQ|LIQUID)_(EVENT|METRIC|FRAME)\s+(\{.*\})')
FRAME_EXTENSIONS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}


def finite_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def number_or_none(value: Any) -> float | None:
    return float(value) if finite_number(value) else None


def time_value(record: dict[str, Any]) -> float | None:
    for key in ("time", "timestamp", "physicsTime"):
        value = number_or_none(record.get(key))
        if value is not None:
            return value
    return None


def case_id(record: dict[str, Any]) -> str | None:
    value = record.get("caseId", record.get("case"))
    return value if isinstance(value, str) else None


def parse_lines(lines: list[str]) -> tuple[list[dict[str, Any]], list[str]]:
    records: list[dict[str, Any]] = []
    errors: list[str] = []
    for line_number, line in enumerate(lines, 1):
        if "LIQ_" not in line and "LIQUID_" not in line:
            continue
        match = TAG_PATTERN.search(line)
        if match is None:
            errors.append(f"line {line_number}: LIQUID tag without JSON object")
            continue
        try:
            payload = json.loads(match.group(2))
        except json.JSONDecodeError as error:
            errors.append(f"line {line_number}: invalid JSON ({error.msg})")
            continue
        if not isinstance(payload, dict):
            errors.append(f"line {line_number}: JSON payload is not an object")
            continue
        payload["recordTag"] = f"LIQUID_{match.group(1)}"
        payload["sourceLine"] = line_number
        if match.group(1) != "FRAME" and time_value(payload) is None:
            errors.append(f"line {line_number}: missing finite time/timestamp/physicsTime")
            continue
        records.append(payload)
    return records, errors


def normalize_event(raw: dict[str, Any]) -> dict[str, Any] | None:
    kind = raw.get("type", raw.get("name"))
    if not isinstance(kind, str):
        return None
    kind = kind.lower().replace("-", "_")
    state = raw.get("state", raw.get("enabled", raw.get("active")))
    if kind in {"invert", "invert180", "force_invert"}:
        if state is True or str(state).lower() in {"on", "true", "inverted"}:
            kind = "invert_on"
        elif state is False or str(state).lower() in {"off", "false", "upright"}:
            kind = "invert_off"
    elif kind in {"shake", "shake_active"} and state is not None:
        kind = "shake_start" if state is True or str(state).lower() == "true" else "shake_stop"
    timestamp = time_value(raw)
    if timestamp is None:
        return None
    return {"time": timestamp, "type": kind, "caseId": case_id(raw), "raw": raw}


def split_records(records: list[dict[str, Any]], selected_case: str) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    metrics: list[dict[str, Any]] = []
    events: list[dict[str, Any]] = []
    for record in records:
        if selected_case != "ALL" and case_id(record) != selected_case:
            continue
        if record.get("recordTag") == "LIQUID_EVENT":
            event = normalize_event(record)
            if event is not None:
                events.append(event)
        elif record.get("recordTag") == "LIQUID_METRIC":
            metrics.append(record)
    metrics.sort(key=lambda item: time_value(item) or 0.0)
    events.sort(key=lambda item: item["time"])
    return metrics, events


def interface_amplitude(metric: dict[str, Any]) -> float | None:
    direct = number_or_none(metric.get("interfaceAmplitude"))
    if direct is not None:
        return direct
    rows = metric.get("interfaceRows")
    if not isinstance(rows, list) or len(rows) < 3:
        return None
    values = [number_or_none(value) for value in rows]
    if any(value is None for value in values):
        return None
    trimmed = values[math.floor(len(values) * 0.1):math.ceil(len(values) * 0.9)]
    if len(trimmed) < 3:
        trimmed = values
    return max(trimmed) - min(trimmed)


def interface_step(metrics: list[dict[str, Any]]) -> float | None:
    if not metrics:
        return None
    direct = [number_or_none(metric.get("maxInterfaceStep")) for metric in metrics]
    if direct and all(value is not None for value in direct):
        return max(value for value in direct if value is not None)
    amplitudes = [interface_amplitude(metric) for metric in metrics]
    if any(value is None for value in amplitudes):
        return None
    steps = [abs(current - previous) for previous, current in zip(amplitudes, amplitudes[1:])]
    return max(steps, default=0.0)


def mixed_share(metric: dict[str, Any]) -> float | None:
    for key in ("mixedBandShare", "mixedFraction"):
        value = number_or_none(metric.get(key))
        if value is not None:
            return value
    return None


def shares(metric: dict[str, Any]) -> tuple[dict[str, float | None], list[str]]:
    result: dict[str, float | None] = {
        "particleLight": None,
        "particleHeavy": None,
        "pixelLight": None,
        "pixelHeavy": None,
    }
    missing: list[str] = []
    light_count = number_or_none(metric.get("lightCount"))
    heavy_count = number_or_none(metric.get("heavyCount"))
    light_area = number_or_none(metric.get("lightPixelArea"))
    heavy_area = number_or_none(metric.get("heavyPixelArea"))
    if light_count is None or heavy_count is None or light_count + heavy_count <= 0:
        missing.extend(["lightCount", "heavyCount"])
    else:
        result["particleLight"] = light_count / (light_count + heavy_count)
        result["particleHeavy"] = heavy_count / (light_count + heavy_count)
    if light_area is None or heavy_area is None or light_area + heavy_area <= 0:
        missing.extend(["lightPixelArea", "heavyPixelArea"])
    else:
        result["pixelLight"] = light_area / (light_area + heavy_area)
        result["pixelHeavy"] = heavy_area / (light_area + heavy_area)
    return result, missing


def conservation(metrics: list[dict[str, Any]]) -> tuple[dict[str, Any], list[str]]:
    if not metrics:
        return {"particleDriftPct": None, "pixelAreaDriftPct": None, "maxDualShareDeviationPct": None}, ["LIQUID_METRIC"]
    initial = metrics[0]
    initial_shares, missing = shares(initial)
    light_initial = number_or_none(initial.get("lightCount"))
    heavy_initial = number_or_none(initial.get("heavyCount"))
    light_area_initial = number_or_none(initial.get("lightPixelArea"))
    heavy_area_initial = number_or_none(initial.get("heavyPixelArea"))
    if light_initial is None or heavy_initial is None:
        missing.extend(["lightCount", "heavyCount"])
    if light_area_initial is None or heavy_area_initial is None:
        missing.extend(["lightPixelArea", "heavyPixelArea"])
    particle_drifts: list[float] = []
    pixel_drifts: list[float] = []
    dual_deviations: list[float] = []
    for metric in metrics:
        current_shares, current_missing = shares(metric)
        missing.extend(current_missing)
        light_count = number_or_none(metric.get("lightCount"))
        heavy_count = number_or_none(metric.get("heavyCount"))
        light_area = number_or_none(metric.get("lightPixelArea"))
        heavy_area = number_or_none(metric.get("heavyPixelArea"))
        if light_initial is not None and heavy_initial is not None and light_count is not None and heavy_count is not None:
            if light_initial > 0:
                particle_drifts.append(abs(light_count - light_initial) / light_initial * 100)
            if heavy_initial > 0:
                particle_drifts.append(abs(heavy_count - heavy_initial) / heavy_initial * 100)
        if light_area_initial is not None and heavy_area_initial is not None and light_area is not None and heavy_area is not None:
            initial_area = light_area_initial + heavy_area_initial
            current_area = light_area + heavy_area
            if initial_area > 0:
                pixel_drifts.append(abs(current_area - initial_area) / initial_area * 100)
        for phase in ("Light", "Heavy"):
            particle_share = initial_shares[f"particle{phase}"]
            pixel_share = current_shares[f"pixel{phase}"]
            if particle_share is not None and pixel_share is not None:
                dual_deviations.append(abs(pixel_share - particle_share) * 100)
    return {
        "particleDriftPct": max(particle_drifts) if particle_drifts else None,
        "pixelAreaDriftPct": max(pixel_drifts) if pixel_drifts else None,
        "maxDualShareDeviationPct": max(dual_deviations) if dual_deviations else None,
        "initialParticleShares": {key: value for key, value in initial_shares.items() if key.startswith("particle")},
    }, sorted(set(missing))


def longest_zero_run(metrics: list[dict[str, Any]]) -> int:
    longest = 0
    current = 0
    for metric in metrics:
        if number_or_none(metric.get("changedCells")) == 0:
            current += 1
            longest = max(longest, current)
        else:
            current = 0
    return longest


def percentile_95(values: list[float]) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    return ordered[math.ceil(len(ordered) * 0.95) - 1]


def performance_frames(records: list[dict[str, Any]], selected_case: str) -> list[dict[str, Any]]:
    tagged = [record for record in records if record.get("recordTag") == "LIQUID_FRAME"]
    case_frames = [record for record in tagged if case_id(record) == selected_case]
    return case_frames if case_frames else [record for record in tagged if case_id(record) is None]


def validate_frame_gate(
    records: list[dict[str, Any]], checks: list[dict[str, Any]], measured: dict[str, Any], missing: list[str]
) -> None:
    frames = [record for record in records if record.get("recordTag") == "LIQUID_FRAME"]
    if not frames:
        missing.append("LIQ_FRAME")
    valid_flags = [
        record.get("valid") is True or record.get("valid") == 1
        for record in frames
    ]
    valid_count = sum(1 for value in valid_flags if value)
    valid_ratio = valid_count / len(frames) if frames else None
    measured["validFrameCount"] = valid_count if frames else None
    measured["validFrameRatio"] = valid_ratio
    add_check(
        checks, "HG-VALID-FRAME-COUNT", "MUST-PASS", measured["validFrameCount"],
        ">= 100 valid frames", None if not frames else valid_count >= 100,
    )
    add_check(
        checks, "HG-VALID-FRAME-RATIO", "MUST-PASS", valid_ratio,
        "100% valid frames", None if valid_ratio is None else valid_ratio == 1.0,
    )

    valid_frame_records = [
        record for record in frames
        if (record.get("valid") is True or record.get("valid") == 1)
    ]
    frame_times = [
        number_or_none(record.get("frame_time_ms"))
        for record in valid_frame_records
    ]
    frame_times = [value for value in frame_times if value is not None and value > 0]
    fps_p95 = percentile_95([1000.0 / value for value in frame_times])
    measured["fpsP95"] = fps_p95
    add_check(
        checks, "HG-FPS-P95", "MUST-PASS", fps_p95, ">= 100 fps",
        None if not frames else fps_p95 is not None and fps_p95 >= 100,
    )

    physics_times = [
        number_or_none(record.get("physics_time_ms"))
        for record in valid_frame_records
    ]
    physics_times = [value for value in physics_times if value is not None]
    physics_p95 = percentile_95(physics_times)
    measured["physicsTimeMsP95"] = physics_p95
    add_check(
        checks, "HG-PHYSICS-TIME-P95", "MUST-PASS", physics_p95, "<= 4 ms",
        None if not frames else physics_p95 is not None and physics_p95 <= 4,
    )

    invalid_frames = [
        record for record in frames
        if not (record.get("valid") is True or record.get("valid") == 1)
    ]
    invalid_clipped = [number_or_none(record.get("clipped_time_ms")) for record in invalid_frames]
    clipped_complete = all(value is not None and value > 0 for value in invalid_clipped)
    measured["invalidFramesWithClippedTime"] = (
        sum(1 for value in invalid_clipped if value is not None and value > 0)
        if invalid_frames else 0
    )
    add_check(
        checks, "HG-BACKPRESSURE-FRAME-FIELDS", "MUST-PASS",
        measured["invalidFramesWithClippedTime"], "every invalid frame has clipped_time_ms > 0",
        True if not invalid_frames else clipped_complete,
    )


def phase_order(metric: dict[str, Any], target: str) -> bool | None:
    light_row = number_or_none(metric.get("lightCentroidRow"))
    heavy_row = number_or_none(metric.get("heavyCentroidRow"))
    if light_row is None or heavy_row is None:
        return None
    return light_row < heavy_row if target == "upright" else heavy_row < light_row


def final_metric(metrics: list[dict[str, Any]], duration: float = 0.5) -> dict[str, Any] | None:
    if not metrics:
        return None
    end_time = time_value(metrics[-1])
    if end_time is None:
        return None
    candidates = [metric for metric in metrics if (time_value(metric) or 0.0) >= end_time - duration]
    return candidates[-1] if candidates else None


def linear_slope(values: list[float], times: list[float]) -> float | None:
    if len(values) < 2:
        return None
    mean_x = sum(times) / len(times)
    mean_y = sum(values) / len(values)
    denominator = sum((value - mean_x) ** 2 for value in times)
    if denominator == 0:
        return None
    return sum((x - mean_x) * (y - mean_y) for x, y in zip(times, values)) / denominator


def stable_time_after_event(
    metrics: list[dict[str, Any]], event_time: float, target: str, hold_seconds: float = 0.5,
    mixed_limit: float | None = None
) -> tuple[float | None, dict[str, Any] | None]:
    for index, metric in enumerate(metrics):
        timestamp = time_value(metric)
        if timestamp is None or timestamp < event_time:
            continue
        amplitude = interface_amplitude(metric)
        ordered = phase_order(metric, target)
        mixed = mixed_share(metric)
        if amplitude is None or amplitude > 3 or ordered is not True:
            continue
        if mixed_limit is not None and (mixed is None or mixed > mixed_limit):
            continue
        hold_metrics = []
        for following in metrics[index:]:
            following_time = time_value(following)
            if following_time is None or following_time > timestamp + hold_seconds:
                break
            following_amplitude = interface_amplitude(following)
            following_ordered = phase_order(following, target)
            following_mixed = mixed_share(following)
            if following_amplitude is None or following_amplitude > 3 or following_ordered is not True:
                break
            if mixed_limit is not None and (following_mixed is None or following_mixed > mixed_limit):
                break
            hold_metrics.append(following)
        if hold_metrics and time_value(hold_metrics[-1]) is not None:
            hold_end = time_value(hold_metrics[-1])
            if hold_end is not None and hold_end - timestamp + 1e-9 >= hold_seconds:
                return hold_end - event_time, hold_metrics[-1]
    return None, None


def check_result(
    case_id: str, checks: list[dict[str, Any]], measured: dict[str, Any], missing: list[str]
) -> dict[str, Any]:
    statuses = [check["status"] for check in checks]
    if "FAIL" in statuses:
        overall = "FAIL"
    elif "BLOCKED" in statuses or missing:
        overall = "BLOCKED"
    else:
        overall = "PASS"
    return {
        "caseId": case_id,
        "overall": overall,
        "missing": sorted(set(missing)),
        "measured": measured,
        "checks": checks,
        "passLine": PASS_LINE,
    }


def add_check(
    checks: list[dict[str, Any]], check_id: str, category: str, measured: Any,
    threshold: str, passed: bool | None, score: int | None = None
) -> None:
    status = "BLOCKED" if passed is None else ("PASS" if passed else "FAIL")
    checks.append(
        {
            "id": check_id,
            "category": category,
            "measured": measured,
            "threshold": threshold,
            "status": status,
            "score": score,
        }
    )


def score_dual(deviation: float | None) -> int:
    if deviation is None:
        return 0
    if deviation <= 1:
        return 10
    if deviation <= 2:
        return 8
    if deviation <= 3:
        return 6
    if deviation <= 4:
        return 4
    return 0


def score_amplitude(amplitude: float | None, limit: float) -> int:
    if amplitude is None:
        return 0
    if amplitude <= max(1.0, limit * 0.5):
        return 10
    if amplitude <= limit:
        return 8
    if amplitude <= limit + 1:
        return 6
    if amplitude <= limit + 2:
        return 4
    return 0


def score_rise(delta: float | None) -> int:
    if delta is None:
        return 0
    if delta >= 0.20:
        return 10
    if delta >= 0.10:
        return 8
    if delta >= 0.05:
        return 6
    if delta >= 0.02:
        return 4
    return 0


def score_residual(residual: float | None) -> int:
    if residual is None:
        return 0
    if residual <= 0.005:
        return 10
    if residual <= 0.010:
        return 8
    if residual <= 0.020:
        return 6
    if residual <= 0.030:
        return 4
    return 0


def score_peak_mixed(share: float | None) -> int:
    if share is None:
        return 0
    if share >= 0.30:
        return 10
    if share >= 0.20:
        return 8
    if share >= 0.10:
        return 6
    if share >= 0.05:
        return 4
    if share >= 0.02:
        return 2
    return 0


def score_recovery_time(seconds: float | None) -> int:
    if seconds is None:
        return 0
    if seconds <= 2:
        return 10
    if seconds <= 3:
        return 8
    if seconds <= 4:
        return 6
    if seconds <= 6:
        return 4
    if seconds <= 8:
        return 2
    return 0


def score_final_mixed(share: float | None) -> int:
    if share is None:
        return 0
    if share <= 0.01:
        return 10
    if share <= 0.02:
        return 8
    if share <= 0.05:
        return 6
    if share <= 0.08:
        return 4
    if share <= 0.12:
        return 2
    return 0


def score_smoothness(displacement: float | None) -> int:
    if displacement is None:
        return 0
    if displacement <= 0.5:
        return 10
    if displacement <= 1:
        return 8
    if displacement <= 2:
        return 6
    if displacement <= 3:
        return 4
    if displacement <= 4:
        return 2
    return 0


def score_rearrange(seconds: float | None) -> int:
    if seconds is None:
        return 0
    if 2.5 <= seconds <= 3.5:
        return 10
    if 2.2 <= seconds <= 3.8:
        return 8
    if 2.0 <= seconds <= 4.0:
        return 6
    if 1.5 <= seconds <= 4.5:
        return 4
    if 1.0 <= seconds <= 5.5:
        return 2
    return 0


def validate_common(metrics: list[dict[str, Any]], checks: list[dict[str, Any]], measured: dict[str, Any], missing: list[str]) -> None:
    conservation_values, conservation_missing = conservation(metrics)
    measured.update(conservation_values)
    missing.extend(conservation_missing)
    add_check(
        checks, "HG-COUNT-CONSERVATION", "MUST-PASS",
        conservation_values["particleDriftPct"], "<= 0.1%",
        None if conservation_values["particleDriftPct"] is None else conservation_values["particleDriftPct"] <= 0.1,
    )
    add_check(
        checks, "HG-PIXEL-AREA-CONSERVATION", "MUST-PASS",
        conservation_values["pixelAreaDriftPct"], "<= 0.1%",
        None if conservation_values["pixelAreaDriftPct"] is None else conservation_values["pixelAreaDriftPct"] <= 0.1,
    )
    dual = conservation_values["maxDualShareDeviationPct"]
    add_check(
        checks, "VV-01-DUAL-SHARE", "MUST-PASS / TIERED", dual,
        "<= 3 percentage points; score >= 6",
        None if dual is None else dual <= 3, score_dual(dual),
    )
    times = [time_value(metric) for metric in metrics]
    monotonic = all(
        previous is not None and current is not None and current >= previous
        for previous, current in zip(times, times[1:])
    )
    add_check(
        checks, "HG-TIMESTAMP-ORDER", "MUST-PASS", monotonic, "non-decreasing",
        None if not metrics else monotonic,
    )


def check_tc11(metrics: list[dict[str, Any]], events: list[dict[str, Any]]) -> dict[str, Any]:
    checks: list[dict[str, Any]] = []
    measured: dict[str, Any] = {}
    missing: list[str] = []
    if not metrics:
        missing.append("TC-11 LIQUID_METRIC")
    validate_common(metrics, checks, measured, missing)
    amplitudes = [interface_amplitude(metric) for metric in metrics]
    if not amplitudes or any(value is None for value in amplitudes):
        missing.append("interfaceAmplitude or interfaceRows")
        max_amplitude = None
    else:
        max_amplitude = max(value for value in amplitudes if value is not None)
    measured["maxInterfaceAmplitudeCells"] = max_amplitude
    add_check(
        checks, "HG-INTERFACE-AMPLITUDE", "MUST-PASS / TIERED", max_amplitude,
        "<= 2 cells; score >= 6",
        None if max_amplitude is None else max_amplitude <= 2, score_amplitude(max_amplitude, 2),
    )
    zero_run = longest_zero_run(metrics)
    measured["longestStaticFrameRun"] = zero_run
    add_check(
        checks, "HG-STATIC-300-FRAMES", "MUST-PASS", zero_run, ">= 300 consecutive frames",
        None if not metrics else zero_run >= 300,
    )
    final = final_metric(metrics, 0.5)
    final_order = phase_order(final, "upright") if final is not None else None
    measured["finalPhaseOrder"] = final_order
    add_check(
        checks, "HG-UPRIGHT-PHASE-ORDER", "MUST-PASS", final_order, "light row < heavy row",
        None if not metrics else final_order is True,
    )
    start_time = time_value(metrics[0]) if metrics else None
    end_time = time_value(metrics[-1]) if metrics else None
    duration = end_time - start_time if start_time is not None and end_time is not None else None
    measured["observationSeconds"] = duration
    add_check(checks, "HG-OBSERVATION-DURATION", "MUST-PASS", duration, ">= 30 seconds", None if duration is None else duration >= 30)
    return check_result("TC-11", checks, measured, missing)


def check_tc12(
    metrics: list[dict[str, Any]], events: list[dict[str, Any]],
    frames: list[dict[str, Any]] | None = None
) -> dict[str, Any]:
    checks: list[dict[str, Any]] = []
    measured: dict[str, Any] = {}
    missing: list[str] = []
    if not metrics:
        missing.append("TC-12 LIQUID_METRIC")
    validate_common(metrics, checks, measured, missing)
    starts = [event for event in events if event["type"] == "shake_start"]
    stops = [event for event in events if event["type"] == "shake_stop"]
    add_check(
        checks, "HG-SHAKE-EVENTS", "MUST-PASS", {"start": len(starts), "stop": len(stops)},
        "one start and one stop",
        None if not metrics else len(starts) == 1 and len(stops) == 1,
    )
    if not starts or not stops:
        missing.append("shake_start/shake_stop events")
        baseline_value = None
        peak_value = None
        final_value = None
        recovery_slope = None
        recovery_time = None
        recovery_duration = None
        recovery_inputs_complete = False
    else:
        start_time = starts[0]["time"]
        stop_time = stops[0]["time"]
        before = [metric for metric in metrics if (time_value(metric) or 0.0) < start_time]
        during = [metric for metric in metrics if start_time <= (time_value(metric) or 0.0) < stop_time]
        after = [metric for metric in metrics if (time_value(metric) or 0.0) >= stop_time]
        frame_before = [frame for frame in (frames or []) if frame["time"] < start_time]
        frame_during = [frame for frame in (frames or []) if start_time <= frame["time"] < stop_time]
        frame_after = [frame for frame in (frames or []) if frame["time"] >= stop_time]
        use_frames = bool(frames)
        baseline_values = (
            [frame["mixedShare"] for frame in frame_before[-8:]]
            if use_frames else [mixed_share(metric) for metric in before[-8:]]
        )
        during_values = (
            [frame["mixedShare"] for frame in frame_during]
            if use_frames else [mixed_share(metric) for metric in during]
        )
        final_values = (
            [frame["mixedShare"] for frame in frame_after[-8:]]
            if use_frames else [mixed_share(metric) for metric in after[-8:]]
        )
        if not baseline_values or any(value is None for value in baseline_values):
            missing.append("frame mixedShare before shake" if use_frames else "mixedBandShare/mixedFraction before shake")
            baseline_value = None
        else:
            baseline_value = sum(value for value in baseline_values if value is not None) / len(baseline_values)
        if not during_values or any(value is None for value in during_values):
            missing.append("frame mixedShare during shake" if use_frames else "mixedBandShare/mixedFraction during shake")
            peak_value = None
        else:
            peak_value = max(value for value in during_values if value is not None)
        if not final_values or any(value is None for value in final_values):
            missing.append("frame mixedShare after recovery" if use_frames else "mixedBandShare/mixedFraction after recovery")
            final_value = None
        else:
            final_value = sum(value for value in final_values if value is not None) / len(final_values)
        recovery_values = [mixed_share(metric) for metric in after]
        recovery_times = [time_value(metric) for metric in after]
        if recovery_values and recovery_times and not any(value is None for value in recovery_values):
            recovery_slope = linear_slope([value for value in recovery_values if value is not None], recovery_times)
        else:
            recovery_slope = None
            missing.append("mixedBandShare/mixedFraction during recovery")
        recovery_duration = (time_value(after[-1]) - stop_time) if after and time_value(after[-1]) is not None else None
        add_check(checks, "HG-RECOVERY-DURATION", "MUST-PASS", recovery_duration, ">= 30 seconds after shake_stop", None if recovery_duration is None else recovery_duration >= 30)
        recovery_time, stable_metric = stable_time_after_event(
            metrics, stop_time, "upright", hold_seconds=1.0, mixed_limit=0.05
        )
        recovery_inputs_complete = bool(after) and all(
            mixed_share(metric) is not None
            and phase_order(metric, "upright") is not None
            and interface_amplitude(metric) is not None
            for metric in after
        )
        if not recovery_inputs_complete:
            missing.append("recovery phase/interface/mixed inputs")
        if stable_metric is not None:
            measured["recoveryStableMetric"] = {
                "time": time_value(stable_metric),
                "mixedShare": mixed_share(stable_metric),
            }
    rise = None if baseline_value is None or peak_value is None else peak_value - baseline_value
    residual = None if baseline_value is None or final_value is None else final_value - baseline_value
    peak_score = score_peak_mixed(peak_value)
    recovery_score = score_recovery_time(recovery_time)
    final_score = score_final_mixed(final_value)
    tier_minimum = min(peak_score, recovery_score, final_score)
    measured.update(
        {
            "baselineMixedFraction": baseline_value,
            "peakMixedFraction": peak_value,
            "mixedRise": rise,
            "recoveryResidual": residual,
            "recoverySlopePerSecond": recovery_slope,
            "recoveryTimeSeconds": recovery_time,
            "recoveryDurationSeconds": recovery_duration,
            "tierScores": {
                "peakMixedShare": peak_score,
                "recoveryTime": recovery_score,
                "finalMixedShare": final_score,
                "minimum": tier_minimum,
            },
        }
    )
    add_check(checks, "HG-MIXED-RISE", "MUST-PASS / TIERED", rise, ">= 0.05; score >= 6", None if rise is None else rise >= 0.05, score_rise(rise))
    add_check(checks, "HG-RECOVERY-RESIDUAL", "MUST-PASS / TIERED", residual, "<= 0.02; score >= 6", None if residual is None else residual <= 0.02, score_residual(residual))
    add_check(checks, "HG-RECOVERY-SLOPE", "MUST-PASS", recovery_slope, "< 0", None if recovery_slope is None else recovery_slope < 0)
    add_check(
        checks, "HG-RECOVERY-TIME", "MUST-PASS / TIERED", recovery_time,
        "<= 4 seconds and stable for 1 second; score >= 6",
        (
            False
            if recovery_time is None and recovery_inputs_complete and recovery_duration is not None and recovery_duration >= 30
            else None if recovery_time is None else recovery_time <= 4
        ),
        recovery_score,
    )
    add_check(checks, "TIERED-PEAK-MIXED-SHARE", "TIERED", peak_value, ">= 10%; score >= 6", None if peak_value is None else peak_score >= 6, peak_score)
    add_check(checks, "TIERED-RECOVERY-TIME", "TIERED", recovery_time, "<= 4 seconds; score >= 6", None if recovery_time is None else recovery_score >= 6, recovery_score)
    add_check(checks, "TIERED-FINAL-MIXED-SHARE", "TIERED", final_value, "<= 5%; score >= 6", None if final_value is None else final_score >= 6, final_score)
    add_check(checks, "TIERED-TC12-MINIMUM", "TIERED", tier_minimum, "minimum score >= 6", None if peak_value is None or recovery_time is None or final_value is None else tier_minimum >= 6, tier_minimum)
    final = final_metric(metrics, 0.5)
    final_order = phase_order(final, "upright") if final is not None else None
    final_amplitude = interface_amplitude(final) if final is not None else None
    if final is not None and final_amplitude is None:
        missing.append("final interfaceAmplitude or interfaceRows")
    if final is not None and final_order is None:
        missing.extend(["final lightCentroidRow", "final heavyCentroidRow"])
    measured["finalPhaseOrder"] = final_order
    measured["finalInterfaceAmplitudeCells"] = final_amplitude
    add_check(
        checks, "HG-FINAL-PHASE-ORDER", "MUST-PASS", final_order, "light row < heavy row",
        None if not metrics else final_order is True,
    )
    add_check(
        checks, "HG-FINAL-INTERFACE-AMPLITUDE", "MUST-PASS / TIERED", final_amplitude,
        "<= 3 cells; score >= 6",
        None if final_amplitude is None else final_amplitude <= 3, score_amplitude(final_amplitude, 3),
    )
    return check_result("TC-12", checks, measured, missing)


def pair_crosses_event(previous_time: float, current_time: float, event_times: list[float]) -> bool:
    return any(previous_time < event_time <= current_time for event_time in event_times)


def metric_centroid(metric: dict[str, Any], phase: str) -> tuple[float, float] | None:
    row = number_or_none(metric.get(f"{phase}CentroidRow"))
    if row is None:
        return None
    column = number_or_none(metric.get(f"{phase}CentroidCol"))
    return row, column if column is not None else 0.0


def frame_centroid(frame: dict[str, Any], phase: str) -> tuple[float, float] | None:
    row = number_or_none(frame.get(f"{phase}CentroidPixels", {}).get("row"))
    column = number_or_none(frame.get(f"{phase}CentroidPixels", {}).get("column"))
    cell_size = number_or_none(frame.get("cellPx"))
    if row is None or column is None or cell_size is None or cell_size <= 0:
        return None
    return row / cell_size, column / cell_size


def wrapped_metric_centroid(record: dict[str, Any], phase: str) -> tuple[float, float] | None:
    return metric_centroid(record["metric"], phase)


def max_centroid_displacement(
    records: list[dict[str, Any]], event_times: list[float], centroid_reader
) -> float | None:
    displacements: list[float] = []
    complete = True
    for previous, current in zip(records, records[1:]):
        if pair_crosses_event(previous["time"], current["time"], event_times):
            continue
        previous_light = centroid_reader(previous, "light")
        previous_heavy = centroid_reader(previous, "heavy")
        current_light = centroid_reader(current, "light")
        current_heavy = centroid_reader(current, "heavy")
        if None in (previous_light, previous_heavy, current_light, current_heavy):
            complete = False
            continue
        for previous_centroid, current_centroid in ((previous_light, current_light), (previous_heavy, current_heavy)):
            displacements.append(math.hypot(
                current_centroid[0] - previous_centroid[0],
                current_centroid[1] - previous_centroid[1],
            ))
    if not complete:
        return None
    return max(displacements, default=0.0)


def metric_pixel_change(metric: dict[str, Any]) -> float | None:
    changed = number_or_none(metric.get("changedCells", metric.get("changedPixelCount")))
    total = number_or_none(metric.get("totalCells", metric.get("liquidPixelCount")))
    if changed is None or total is None or total <= 0:
        return None
    return changed / total


def max_pixel_classification_change(
    metrics: list[dict[str, Any]], frames: list[dict[str, Any]] | None,
    event_times: list[float]
) -> tuple[float | None, list[str]]:
    sources: list[str] = []
    values: list[float] = []
    metric_values: list[float] = []
    for previous, current in zip(metrics, metrics[1:]):
        previous_time = time_value(previous)
        current_time = time_value(current)
        if previous_time is None or current_time is None or pair_crosses_event(previous_time, current_time, event_times):
            continue
        value = metric_pixel_change(current)
        if value is None:
            continue
        metric_values.append(value)
    if metric_values:
        sources.append("LIQUID_METRIC changedCells/totalCells")
        values.extend(metric_values)
    frame_values: list[float] = []
    for previous, current in zip(frames or [], (frames or [])[1:]):
        if pair_crosses_event(previous["time"], current["time"], event_times):
            continue
        value = number_or_none(current.get("adjacentClassificationChange"))
        if value is not None:
            frame_values.append(value)
    if frame_values:
        sources.append("frame pixel classification")
        values.extend(frame_values)
    return (max(values) if values else None), sources


def check_tc13(
    metrics: list[dict[str, Any]], events: list[dict[str, Any]],
    frames: list[dict[str, Any]] | None = None
) -> dict[str, Any]:
    checks: list[dict[str, Any]] = []
    measured: dict[str, Any] = {}
    missing: list[str] = []
    if not metrics:
        missing.append("TC-13 LIQUID_METRIC")
    validate_common(metrics, checks, measured, missing)
    transitions: list[dict[str, Any]] = []
    for event in events:
        if event["type"] not in {"invert_on", "invert_off"}:
            continue
        target = "inverted" if event["type"] == "invert_on" else "upright"
        seconds, stable_metric = stable_time_after_event(metrics, event["time"], target)
        transitions.append(
            {
                "event": event["type"],
                "eventTime": event["time"],
                "target": target,
                "rearrangeSeconds": seconds,
                "stablePhaseOrder": phase_order(stable_metric, target) if stable_metric is not None else None,
            }
        )
    event_times = [event["time"] for event in events if event["type"] in {"invert_on", "invert_off"}]
    metric_records = [
        {"time": time_value(metric), "metric": metric}
        for metric in metrics
        if time_value(metric) is not None
    ]
    metric_displacement = max_centroid_displacement(metric_records, event_times, wrapped_metric_centroid)
    frame_displacement = max_centroid_displacement(frames or [], event_times, frame_centroid)
    displacement_values = [value for value in (metric_displacement, frame_displacement) if value is not None]
    centroid_displacement = max(displacement_values) if displacement_values else None
    if metric_displacement is None and frame_displacement is None:
        missing.append("adjacent light/heavy centroid rows and columns")
    pixel_change, pixel_sources = max_pixel_classification_change(metrics, frames, event_times)
    if pixel_change is None:
        missing.append("adjacent pixel classification change (metric or frames)")
    measured["transitions"] = transitions
    measured["maxAdjacentCentroidDisplacementCells"] = centroid_displacement
    measured["centroidDisplacementSources"] = {
        "metric": metric_displacement,
        "frames": frame_displacement,
    }
    measured["maxAdjacentPixelClassificationChange"] = pixel_change
    measured["pixelClassificationSources"] = pixel_sources
    add_check(
        checks, "HG-TRANSITION-EVENTS", "MUST-PASS", len(transitions), ">= 2 invert transitions",
        None if not metrics else len(transitions) >= 2,
    )
    for index, transition in enumerate(transitions, 1):
        seconds = transition["rearrangeSeconds"]
        add_check(
            checks, f"HG-REARRANGE-WINDOW-{index}", "MUST-PASS / TIERED", seconds,
            "2.0 <= seconds <= 4.0; score >= 6",
            None if seconds is None else 2.0 <= seconds <= 4.0, score_rearrange(seconds),
        )
        add_check(
            checks, f"HG-PHASE-ORDER-{index}", "MUST-PASS", transition["stablePhaseOrder"],
            f"target {transition['target']}",
            None if not metrics else transition["stablePhaseOrder"] is True,
        )
    if any(transition["rearrangeSeconds"] is None for transition in transitions):
        missing.append("stable centroid order and interface amplitude")
    step = interface_step(metrics)
    if step is None:
        missing.append("maxInterfaceStep or consecutive interfaceAmplitude")
    measured["maxInterfaceStepCells"] = step
    add_check(checks, "HG-NO-TELEPORT", "MUST-PASS", step, "<= 2 cells per physics frame", None if step is None else step <= 2)
    smoothness_score = score_smoothness(centroid_displacement)
    add_check(
        checks, "HG-CENTROID-DISPLACEMENT", "MUST-PASS", centroid_displacement,
        "<= 1 cell per adjacent frame (event boundary excluded)",
        None if centroid_displacement is None else centroid_displacement <= MAX_CENTROID_DISPLACEMENT_CELLS,
    )
    add_check(
        checks, "TIERED-PROCESS-SMOOTHNESS", "TIERED", centroid_displacement,
        "<= 2 cells for score >= 6", None if centroid_displacement is None else smoothness_score >= 6,
        smoothness_score,
    )
    add_check(
        checks, "HG-PIXEL-CLASSIFICATION-CONTINUITY", "MUST-PASS", pixel_change,
        "<= 0.10% adjacent classified pixels (event boundary excluded)",
        None if pixel_change is None else pixel_change <= MAX_PIXEL_CLASSIFICATION_CHANGE,
    )
    rearrange_scores = [
        score_rearrange(transition["rearrangeSeconds"])
        for transition in transitions
        if transition["rearrangeSeconds"] is not None
    ]
    tier_minimum = min([smoothness_score, *rearrange_scores]) if transitions and centroid_displacement is not None else None
    measured["tierScores"] = {
        "rearrange": rearrange_scores,
        "smoothness": smoothness_score,
        "minimum": tier_minimum,
    }
    add_check(
        checks, "TIERED-TC13-MINIMUM", "TIERED", tier_minimum,
        "minimum score >= 6", None if tier_minimum is None else tier_minimum >= 6, tier_minimum,
    )
    invert_times = [event["time"] for event in events if event["type"] in {"invert_on", "invert_off"}]
    if invert_times and metrics:
        end_time = time_value(metrics[-1])
        last_event = max(invert_times)
        post_duration = end_time - last_event if end_time is not None else None
    else:
        post_duration = None
    measured["postLastEventSeconds"] = post_duration
    add_check(checks, "HG-POST-EVENT-OBSERVATION", "MUST-PASS", post_duration, ">= 4.5 seconds", None if post_duration is None else post_duration >= 4.5)
    return check_result("TC-13", checks, measured, missing)


def check_case(
    records: list[dict[str, Any]], selected_case: str, frames: list[dict[str, Any]] | None = None
) -> dict[str, Any]:
    metrics, events = split_records(records, selected_case)
    frame_checks: list[dict[str, Any]] = []
    frame_measured: dict[str, Any] = {}
    frame_missing: list[str] = []
    validate_frame_gate(performance_frames(records, selected_case), frame_checks, frame_measured, frame_missing)
    if selected_case == "TC-11":
        result = check_tc11(metrics, events)
    if selected_case == "TC-12":
        result = check_tc12(metrics, events, frames)
    if selected_case == "TC-13":
        result = check_tc13(metrics, events, frames)
    result["checks"].extend(frame_checks)
    result["measured"].update(frame_measured)
    result["missing"].extend(frame_missing)
    result["missing"] = sorted(set(result["missing"]))
    statuses = [check["status"] for check in result["checks"]]
    if "FAIL" in statuses:
        result["overall"] = "FAIL"
    elif "BLOCKED" in statuses or result["missing"]:
        result["overall"] = "BLOCKED"
    else:
        result["overall"] = "PASS"
    return result
    raise ValueError(f"unsupported case: {selected_case}")


class InputError(ValueError):
    pass


def load_json_object(path: Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise InputError(f"cannot read {label} {path}: {error}") from error
    if not isinstance(value, dict):
        raise InputError(f"{label} {path} must contain a JSON object")
    return value


def manifest_time(entry: dict[str, Any], time_unit: str) -> float:
    milliseconds = time_unit.lower() in {"ms", "millisecond", "milliseconds"}
    entry_unit = entry.get("timeUnit", entry.get("unit"))
    if isinstance(entry_unit, str):
        milliseconds = entry_unit.lower() in {"ms", "millisecond", "milliseconds"}
    for key in ("timeSeconds", "time", "timestamp", "physicsTime", "frameTimeMs", "physicsTimeMs"):
        value = number_or_none(entry.get(key))
        if value is None:
            continue
        if milliseconds or key.endswith("Ms"):
            return value / 1000
        return value
    raise InputError(f"manifest entry {json.dumps(entry, ensure_ascii=False)} has no finite timestamp")


def load_frame_manifest(path: Path, frames_dir: Path | None) -> list[dict[str, Any]]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise InputError(f"cannot read frame manifest {path}: {error}") from error
    time_unit = "seconds"
    raw_entries: Any
    if isinstance(value, list):
        raw_entries = value
    elif isinstance(value, dict) and isinstance(value.get("frames"), list):
        time_unit = str(value.get("timeUnit", time_unit))
        raw_entries = value["frames"]
    elif isinstance(value, dict):
        time_unit = str(value.get("timeUnit", time_unit))
        raw_entries = [
            {"file": filename, **(entry if isinstance(entry, dict) else {"time": entry})}
            for filename, entry in value.items()
            if filename not in {"timeUnit", "colorSpace"}
        ]
    else:
        raise InputError(f"frame manifest {path} must be an array, frames array, or filename mapping")

    entries: list[dict[str, Any]] = []
    seen_paths: set[Path] = set()
    previous_time: float | None = None
    for raw in raw_entries:
        if not isinstance(raw, dict):
            raise InputError(f"frame manifest entry must be an object: {raw!r}")
        filename = raw.get("file", raw.get("frame", raw.get("path")))
        if not isinstance(filename, str) or not filename:
            raise InputError(f"frame manifest entry has no file/frame/path: {raw!r}")
        timestamp = manifest_time(raw, time_unit)
        if previous_time is not None and timestamp <= previous_time:
            raise InputError("frame manifest timestamps must be strictly increasing")
        base = frames_dir if frames_dir is not None else path.parent
        frame_path = Path(filename)
        if not frame_path.is_absolute():
            frame_path = base / frame_path
        if not frame_path.is_file():
            raise InputError(f"manifest frame does not exist: {frame_path}")
        if frame_path.suffix.lower() not in FRAME_EXTENSIONS:
            raise InputError(f"unsupported frame extension: {frame_path}")
        resolved = frame_path.resolve()
        if resolved in seen_paths:
            raise InputError(f"duplicate manifest frame: {frame_path}")
        seen_paths.add(resolved)
        entries.append({"path": frame_path, "time": timestamp})
        previous_time = timestamp
    if frames_dir is not None:
        directory_files = {
            item.resolve()
            for item in frames_dir.iterdir()
            if item.is_file() and item.suffix.lower() in FRAME_EXTENSIONS
        }
        if directory_files != seen_paths:
            missing = sorted(str(item) for item in directory_files - seen_paths)
            extra = sorted(str(item) for item in seen_paths - directory_files)
            raise InputError(f"frame directory/manifest mismatch; directory-only={missing}, manifest-only={extra}")
    if not entries:
        raise InputError("frame manifest contains no frames")
    return entries


def calibration_color(calibration: dict[str, Any], name: str, aliases: tuple[str, ...]) -> list[float]:
    classes = calibration.get("classes", calibration.get("colors", calibration))
    if not isinstance(classes, dict):
        raise InputError("color calibration classes must be an object")
    value = None
    for key in (name, *aliases):
        if key in classes:
            value = classes[key]
            break
    if isinstance(value, dict):
        value = value.get("color", value.get("rgb", value.get("bgr")))
    if not isinstance(value, list) or len(value) != 3:
        raise InputError(f"color calibration requires a 3-channel color for {name}")
    color = [number_or_none(channel) for channel in value]
    if any(channel is None or channel < 0 or channel > 255 for channel in color):
        raise InputError(f"color calibration color for {name} must contain finite 0..255 channels")
    return [float(channel) for channel in color if channel is not None]


def load_color_calibration(path: Path) -> dict[str, Any]:
    calibration = load_json_object(path, "color calibration")
    colors = {
        "background": calibration_color(calibration, "background", ("backgroundColor",)),
        "light": calibration_color(calibration, "light", ("lightPhase", "oil")),
        "heavy": calibration_color(calibration, "heavy", ("heavyPhase", "water")),
        "mixed": calibration_color(calibration, "mixed", ("mixedBand", "mix")),
    }
    cell_size = number_or_none(calibration.get("cellPx", calibration.get("cellSizePx")))
    if cell_size is None or cell_size <= 0:
        raise InputError("color calibration requires finite cellPx > 0 for cell-unit continuity")
    ignore_columns = number_or_none(calibration.get("ignoreColumnsPct", 10))
    if ignore_columns is None or not 0 <= ignore_columns < 50:
        raise InputError("ignoreColumnsPct must be finite and in [0, 50)")
    ignore_border = number_or_none(calibration.get("ignoreBorderPx", 0))
    if ignore_border is None or ignore_border < 0:
        raise InputError("ignoreBorderPx must be finite and >= 0")
    configured_change_limit = number_or_none(calibration.get("maxAdjacentClassificationChange"))
    if configured_change_limit is not None and configured_change_limit > MAX_PIXEL_CLASSIFICATION_CHANGE:
        raise InputError("maxAdjacentClassificationChange cannot exceed the hard 0.10% limit")
    color_space = str(calibration.get("colorSpace", "rgb")).lower()
    if color_space not in {"rgb", "bgr"}:
        raise InputError("colorSpace must be rgb or bgr")
    return {
        "colors": colors,
        "colorSpace": color_space,
        "cellPx": cell_size,
        "ignoreColumnsPct": ignore_columns,
        "ignoreBorderPx": ignore_border,
        "maxAdjacentClassificationChange": configured_change_limit or MAX_PIXEL_CLASSIFICATION_CHANGE,
    }


def analyze_frames(entries: list[dict[str, Any]], calibration: dict[str, Any]) -> list[dict[str, Any]]:
    try:
        import numpy as np
        from PIL import Image
    except ImportError as error:
        raise InputError("frame input requires numpy and Pillow") from error

    color_order = list(calibration["colors"].keys())
    colors = np.asarray([calibration["colors"][name] for name in color_order], dtype=np.float64)
    previous_labels: Any = None
    records: list[dict[str, Any]] = []
    for entry in entries:
        try:
            with Image.open(entry["path"]) as image:
                frame = np.asarray(image.convert("RGB"), dtype=np.float64)
        except (OSError, ValueError) as error:
            raise InputError(f"cannot decode frame {entry['path']}: {error}") from error
        if frame.ndim != 3 or frame.shape[2] != 3:
            raise InputError(f"frame {entry['path']} is not a 3-channel image")
        if calibration["colorSpace"] == "bgr":
            frame = frame[:, :, ::-1]
        border = int(calibration["ignoreBorderPx"])
        if border:
            frame = frame[border:-border, border:-border, :]
        width = frame.shape[1]
        left = math.ceil(width * calibration["ignoreColumnsPct"] / 100)
        right = math.floor(width * (1 - calibration["ignoreColumnsPct"] / 100))
        frame = frame[:, left:right, :]
        distances = np.stack(
            [np.sum(np.abs(frame - color), axis=2) for color in colors], axis=2
        )
        labels = np.argmin(distances, axis=2)
        counts = {name: int(np.count_nonzero(labels == index)) for index, name in enumerate(color_order)}
        liquid_pixels = counts["light"] + counts["heavy"] + counts["mixed"]
        if liquid_pixels <= 0:
            raise InputError(f"frame {entry['path']} has no calibrated liquid pixels")
        centroids: dict[str, dict[str, float]] = {}
        for phase in ("light", "heavy", "mixed"):
            rows, columns = np.nonzero(labels == color_order.index(phase))
            if rows.size == 0:
                raise InputError(f"frame {entry['path']} has no calibrated {phase} pixels")
            centroids[phase] = {"row": float(rows.mean()), "column": float(columns.mean())}
        adjacent_change = (
            float(np.count_nonzero(labels != previous_labels)) / labels.size
            if previous_labels is not None else None
        )
        records.append(
            {
                "path": str(entry["path"]),
                "time": entry["time"],
                "cellPx": calibration["cellPx"],
                "counts": counts,
                "mixedShare": counts["mixed"] / liquid_pixels,
                "lightCentroidPixels": centroids["light"],
                "heavyCentroidPixels": centroids["heavy"],
                "mixedCentroidPixels": centroids["mixed"],
                "adjacentClassificationChange": adjacent_change,
            }
        )
        previous_labels = labels
    return records


def load_frame_inputs(
    frames_dir: Path | None, manifest_path: Path | None, calibration_path: Path | None
) -> list[dict[str, Any]]:
    provided = [value is not None for value in (frames_dir, manifest_path, calibration_path)]
    if not any(provided):
        return []
    if not all(provided):
        raise InputError("--frames, --frame-manifest, and --color-calibration must be provided together")
    if not frames_dir or not manifest_path or not calibration_path:
        raise InputError("frame input paths are incomplete")
    if not frames_dir.is_dir():
        raise InputError(f"frame directory does not exist: {frames_dir}")
    entries = load_frame_manifest(manifest_path, frames_dir)
    calibration = load_color_calibration(calibration_path)
    return analyze_frames(entries, calibration)


def self_check_log() -> str:
    lines: list[str] = []

    def emit(tag: str, payload: dict[str, Any]) -> None:
        lines.append(f"10-15 00:00:00.000 {tag} {json.dumps(payload, separators=(',', ':'))}")

    def metric(
        case: str, timestamp: float, light_row: float, heavy_row: float,
        mixed: float, changed: int, step: float = 0.0
    ) -> None:
        emit(
            "LIQUID_METRIC",
            {
                "caseId": case,
                "time": round(timestamp, 3),
                "lightCount": 2400,
                "heavyCount": 3360,
                "lightPixelArea": 24000,
                "heavyPixelArea": 33600,
                "lightCentroidRow": round(light_row, 3),
                "heavyCentroidRow": round(heavy_row, 3),
                "lightCentroidCol": 40.0,
                "heavyCentroidCol": 40.0,
                "interfaceAmplitude": 1.0,
                "interfaceRows": [50] * 10,
                "mixedFraction": round(mixed, 4),
                "changedCells": changed,
                "totalCells": 57600,
                "maxInterfaceStep": step,
            },
        )

    def frame(case: str, sequence: int, valid: bool = True) -> None:
        emit(
            "LIQUID_FRAME",
            {
                "caseId": case,
                "sequence": sequence,
                "frame_time_ms": 1000 / 120,
                "physics_time_ms": 1.5,
                "snapshot_time_ms": 0.2,
                "substeps": 1,
                "clipped_time_ms": 0 if valid else 8.333,
                "valid": 1 if valid else 0,
            },
        )

    emit("LIQUID_EVENT", {"caseId": "TC-11", "time": 0, "type": "case_start"})
    for index in range(3720):
        metric("TC-11", index / 120, 20, 80, 0.02, 1 if index < 100 else 0)
        frame("TC-11", index + 1)

    emit("LIQUID_EVENT", {"caseId": "TC-12", "time": 0, "type": "case_start"})
    emit("LIQUID_EVENT", {"caseId": "TC-12", "time": 1, "type": "shake_start"})
    emit("LIQUID_EVENT", {"caseId": "TC-12", "time": 7, "type": "shake_stop"})
    for index in range(4441):
        timestamp = index / 120
        if timestamp < 1:
            mixed = 0.02
        elif timestamp < 7:
            progress = (timestamp - 1) / 6
            mixed = 0.02 + 0.33 * math.sin(math.pi * progress)
        else:
            progress = min(1.0, (timestamp - 7) / 2.5)
            mixed = 0.35 - 0.325 * progress
        metric("TC-12", timestamp, 20, 80, mixed, 1 if timestamp < 7 else 0)
        frame("TC-12", index + 1)

    emit("LIQUID_EVENT", {"caseId": "TC-13", "time": 0, "type": "case_start"})
    emit("LIQUID_EVENT", {"caseId": "TC-13", "time": 1, "type": "invert_on"})
    emit("LIQUID_EVENT", {"caseId": "TC-13", "time": 5, "type": "invert_off"})
    for index in range(1200):
        timestamp = index / 120
        light_row, heavy_row = 20.0, 80.0
        if 1 <= timestamp < 4:
            progress = (timestamp - 1) / 3
            eased_progress = progress ** 2
            light_row, heavy_row = 20 + 60 * eased_progress, 80 - 60 * eased_progress
        elif 4 <= timestamp < 5:
            light_row, heavy_row = 80, 20
        elif 5 <= timestamp < 8:
            progress = (timestamp - 5) / 3
            eased_progress = progress ** 2
            light_row, heavy_row = 80 - 60 * eased_progress, 20 + 60 * eased_progress
        elif timestamp >= 8:
            light_row, heavy_row = 20, 80
        metric("TC-13", timestamp, light_row, heavy_row, 0.02, 0, 0.5)
        frame("TC-13", index + 1)
    return "\n".join(lines) + "\n"


def commit_from_metadata() -> str:
    git_path = Path.cwd() / ".git"
    if git_path.is_file():
        content = git_path.read_text(encoding="utf-8", errors="replace").strip()
        if not content.startswith("gitdir:"):
            return "unavailable"
        git_path = Path(content.removeprefix("gitdir:").strip())
        if not git_path.is_absolute():
            git_path = Path.cwd() / git_path
    if not git_path.is_dir():
        return "unavailable"
    head_path = git_path / "HEAD"
    if not head_path.is_file():
        return "unavailable"
    head = head_path.read_text(encoding="utf-8", errors="replace").strip()
    if not head.startswith("ref:"):
        return head
    reference = head.removeprefix("ref:").strip()
    reference_path = git_path / reference
    if reference_path.is_file():
        return reference_path.read_text(encoding="utf-8", errors="replace").strip()
    packed_refs = git_path / "packed-refs"
    if packed_refs.is_file():
        for line in packed_refs.read_text(encoding="utf-8", errors="replace").splitlines():
            parts = line.split()
            if len(parts) == 2 and parts[1] == reference:
                return parts[0]
    return "unavailable"


def git_commit() -> str:
    environment_commit = os.environ.get("GIT_COMMIT")
    if environment_commit:
        return environment_commit
    try:
        completed = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            capture_output=True, text=True, check=False, timeout=2,
        )
    except (OSError, subprocess.TimeoutExpired):
        return commit_from_metadata()
    if completed.returncode == 0 and completed.stdout.strip():
        return completed.stdout.strip()
    return commit_from_metadata()


def build_result(
    records: list[dict[str, Any]], parse_errors: list[str], case: str, self_check: bool,
    frames: list[dict[str, Any]] | None = None
) -> dict[str, Any]:
    case_results = (
        {case_id: check_case(records, case_id, frames) for case_id in CASES}
        if case == "ALL"
        else {case: check_case(records, case, frames)}
    )
    statuses = [result["overall"] for result in case_results.values()]
    overall = "FAIL" if "FAIL" in statuses else ("BLOCKED" if "BLOCKED" in statuses else "PASS")
    return {
        "problemId": "P-001",
        "case": case,
        "overall": overall,
        "selfCheck": self_check,
        "commit": git_commit(),
        "generatedAt": datetime.now().astimezone().isoformat(timespec="seconds"),
        "parseErrors": parse_errors,
        "frameInputs": {
            "frameCount": len(frames or []),
            "firstTime": frames[0]["time"] if frames else None,
            "lastTime": frames[-1]["time"] if frames else None,
        },
        "cases": case_results,
    }


def write_json(path: Path, result: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def markdown_case(case_result: dict[str, Any]) -> list[str]:
    lines = [
        f"### {case_result['caseId']} — {case_result['overall']}",
        "",
        "| 指标 | 类别 | 实测 | 判定 | 分档 |",
        "|---|---|---:|---|---:|",
    ]
    for check in case_result["checks"]:
        measured = "-" if check["measured"] is None else check["measured"]
        lines.append(
            f"| `{check['id']}` | {check['category']} | `{json.dumps(measured, ensure_ascii=False)}` | "
            f"**{check['status']}** | {check['score'] if check['score'] is not None else '-'} |"
        )
    if case_result["missing"]:
        lines.extend(["", f"缺失输入：{', '.join(f'`{item}`' for item in case_result['missing'])}"])
    lines.extend(["", f"实测汇总：`{json.dumps(case_result['measured'], ensure_ascii=False, sort_keys=True)}`", ""])
    return lines


def write_markdown(path: Path, result: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        f"## P-001 liquid-ca-m1p-v1.0 objective check — {result['generatedAt']}",
        "",
        f"- Case: `{result['case']}`",
        f"- Commit: `{result['commit']}`",
        f"- Overall: **{result['overall']}**",
        f"- Self-check: `{result['selfCheck']}`",
        f"- Frame inputs: `{result['frameInputs']['frameCount']}`",
        f"- Parse errors: {len(result['parseErrors'])}",
        "",
    ]
    for case_result in result["cases"].values():
        lines.extend(markdown_case(case_result))
    with path.open("a", encoding="utf-8") as stream:
        stream.write("\n".join(lines).rstrip() + "\n\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check P-001 liquid-mode TC-11..TC-13")
    parser.add_argument("--input", type=Path)
    parser.add_argument("--frames", type=Path, help="Directory containing recorded frames")
    parser.add_argument("--frame-manifest", type=Path, help="JSON manifest with frame filenames and timestamps")
    parser.add_argument("--color-calibration", type=Path, help="JSON color calibration for background/light/heavy/mixed")
    parser.add_argument("--case", choices=(*CASES, "ALL"), default="ALL")
    parser.add_argument("--json", type=Path, default=Path("test/reports/p001_liquid_mode_check.json"))
    parser.add_argument("--markdown", type=Path, default=Path("docs/test-report-P001-liquid-ca-m1p-v1.md"))
    parser.add_argument("--self-check", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.self_check:
        log_text = self_check_log()
    else:
        if args.input is None:
            print("--input is required unless --self-check is used", file=sys.stderr)
            return 2
        log_text = sys.stdin.read() if str(args.input) == "-" else args.input.read_text(encoding="utf-8", errors="replace")
    try:
        frames = [] if args.self_check else load_frame_inputs(
            args.frames, args.frame_manifest, args.color_calibration
        )
    except InputError as error:
        print(str(error), file=sys.stderr)
        return 2
    records, parse_errors = parse_lines(log_text.splitlines())
    result = build_result(records, parse_errors, args.case, args.self_check, frames)
    write_json(args.json, result)
    write_markdown(args.markdown, result)
    print(f"overall: {result['overall']}")
    print(f"JSON: {args.json}")
    print(f"Markdown: {args.markdown}")
    return 0 if result["overall"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
