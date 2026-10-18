#!/usr/bin/env python3
"""P-011 bottom-creep-v2.7 objective log checker."""

from __future__ import annotations

import argparse
import bisect
from collections import Counter, defaultdict
import json
import math
import re
import sys
from pathlib import Path
from typing import Any

P011_HOOK_RE = re.compile(r'P011_(EVENT|METRIC|BOTTOM)\s+(input|reset)?\s*(\{.*\})')

DEFAULT_ROUTE = "bottom-creep-v2.7.3"
BEHAVIOR_ROLE = "behavior"
BUDGET_ISOLATION_ROLE = "budgetIsolation"
PRESSURE_CLIP_ROLE = "pressureClip"
NOT_APPLICABLE = "NOT_APPLICABLE"
DEAD_ZONE_DEGREES = 1.0
TILT_RESPONSE_RATE_DEG_PER_S = 720.0
BOTTOM_FLUX_EPS = 1e-12
DONOR_FIELD = "bottomCreepDonorEffectiveDisplacement"
RECEIVER_FIELD = "bottomCreepReceiverEffectiveDisplacement"
HOOK_RE = re.compile(r"P011_(EVENT|METRIC|BOTTOM)\s+(?:input\s+)?(\{.*\})")
WS_INIT_RE = re.compile(r"WS_EVENT init\s+(\{.*\})")


def parse_lines(lines: list[str]) -> tuple[dict[str, list[dict[str, Any]]], int, int | None]:
    parsed: dict[str, list[dict[str, Any]]] = {"event": [], "metric": [], "bottom": []}
    parse_errors = 0
    capacity: int | None = None
    for line_number, line in enumerate(lines, start=1):
        match = P011_HOOK_RE.search(line)
        if match:
            try:
                payload = json.loads(match.group(3))
            except json.JSONDecodeError:
                parse_errors += 1
                continue
            if match.group(1) == 'EVENT':
                payload['_eventType'] = match.group(2) or 'input'
            payload["_line"] = line_number
            parsed[match.group(1).lower()].append(payload)
            continue
        init_match = WS_INIT_RE.search(line)
        if init_match and capacity is None:
            try:
                capacity = json.loads(init_match.group(1)).get("capacity")
            except json.JSONDecodeError:
                parse_errors += 1
    return parsed, parse_errors, capacity


def mean(values: list[float]) -> float:
    return sum(values) / len(values) if values else 0.0


def finite_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def pearson(left: list[float], right: list[float]) -> float | None:
    if len(left) != len(right) or len(left) < 2:
        return None
    left_mean = sum(left) / len(left)
    right_mean = sum(right) / len(right)
    numerator = sum((x - left_mean) * (y - right_mean) for x, y in zip(left, right))
    left_square = sum((x - left_mean) ** 2 for x in left)
    right_square = sum((y - right_mean) ** 2 for y in right)
    if left_square <= 0 or right_square <= 0:
        return None
    return numerator / math.sqrt(left_square * right_square)


def percentile_95(values: list[float]) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    index = max(0, math.ceil(0.95 * len(ordered)) - 1)
    return ordered[index]


def nearest_metric_before(
    metrics: list[dict[str, Any]],
    timestamp: float,
    metric_times: list[float] | None = None,
) -> dict[str, Any] | None:
    if not metrics:
        return None
    if metric_times is None:
        metric_times = [metric.get("physicsTime", -math.inf) for metric in metrics]
    index = bisect.bisect_right(metric_times, timestamp) - 1
    return metrics[index] if index >= 0 else None


def reset_epoch_for_time(
    resets: dict[str, list[float]], run_id: Any, timestamp: Any
) -> int | None:
    if not isinstance(run_id, str) or not finite_number(timestamp):
        return None
    times = resets.get(run_id, [])
    return bisect.bisect_right(times, timestamp) if times else 0


def interpolate_bottom(bottoms: list[dict[str, Any]], timestamp: float, key: str) -> list[float] | None:
    if not bottoms:
        return None
    if timestamp <= bottoms[0]["physicsTime"]:
        return list(bottoms[0][key])
    if timestamp >= bottoms[-1]["physicsTime"]:
        return list(bottoms[-1][key])
    for previous, current in zip(bottoms, bottoms[1:]):
        if previous["physicsTime"] <= timestamp <= current["physicsTime"]:
            span = current["physicsTime"] - previous["physicsTime"]
            ratio = 0 if span <= 0 else (timestamp - previous["physicsTime"]) / span
            return [
                a + (b - a) * ratio
                for a, b in zip(previous[key], current[key])
            ]
    return None


def signed_bottom_motion(bottom: dict[str, Any]) -> tuple[float, float]:
    donor = sum(value for value in bottom.get(DONOR_FIELD, []) if finite_number(value))
    receiver = sum(value for value in bottom.get(RECEIVER_FIELD, []) if finite_number(value))
    return donor, receiver


def bottom_motion_sign(bottom: dict[str, Any]) -> int:
    _, receiver = signed_bottom_motion(bottom)
    if receiver > BOTTOM_FLUX_EPS:
        return 1
    if receiver < -BOTTOM_FLUX_EPS:
        return -1
    return 0


def active_indices(activity: list[float]) -> list[int]:
    return [index for index, value in enumerate(activity) if abs(value) > 1e-9]


def valid_identity(record: dict[str, Any]) -> bool:
    values = [record.get(field) for field in ('runId', 'sequence', 'frameId', 'substepId')]
    run_id, sequence, frame_id, substep_id = values
    return (
        isinstance(run_id, str)
        and bool(run_id)
        and all(
            isinstance(value, int) and not isinstance(value, bool)
            for value in (sequence, frame_id, substep_id)
        )
    )


def identity_tuple(record: dict[str, Any]) -> tuple[Any, ...] | None:
    if not valid_identity(record):
        return None
    return (
        record['runId'], record['sequence'], record['frameId'], record['substepId']
    )


def reset_times_by_run(reset_events: list[dict[str, Any]]) -> dict[str, list[float]]:
    result: dict[str, list[float]] = {}
    for event in reset_events:
        run_id = event.get('runId')
        event_time = event.get('eventTime', event.get('physicsTime'))
        if not isinstance(run_id, str) or not finite_number(event_time):
            continue
        result.setdefault(run_id, []).append(event_time)
    return {run_id: sorted(times) for run_id, times in result.items()}


def build_phase_index(
    input_events: list[dict[str, Any]], reset_events: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    resets = reset_times_by_run(reset_events)
    phases: list[dict[str, Any]] = []
    for event in input_events:
        run_id = event.get('runId')
        accepted_time = event.get('acceptedTime')
        sequence = event.get('sequence')
        if not isinstance(run_id, str) or not finite_number(accepted_time):
            continue
        reset_epoch = sum(time <= accepted_time for time in resets.get(run_id, []))
        phases.append({
            'key': f'{run_id}:{reset_epoch}:{sequence}',
            'runId': run_id,
            'resetEpoch': reset_epoch,
            'sequence': sequence,
            'acceptedTime': accepted_time,
        })
    return phases


def build_phase_lookup(
    phases: list[dict[str, Any]]
) -> dict[tuple[str, int], tuple[list[float], list[dict[str, Any]]]]:
    grouped: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
    for phase in phases:
        grouped[(phase['runId'], phase['resetEpoch'])].append(phase)
    return {
        key: (
            [phase['acceptedTime'] for phase in candidates],
            sorted(candidates, key=lambda phase: phase['acceptedTime'])
        )
        for key, candidates in grouped.items()
    }


def phase_for_metric(
    metric: dict[str, Any],
    phases: list[dict[str, Any]],
    resets: dict[str, list[float]],
    phase_index: dict[tuple[str, int], tuple[list[float], list[dict[str, Any]]]] | None = None,
) -> str | None:
    run_id = metric.get('runId')
    physics_time = metric.get('physicsTime')
    if not isinstance(run_id, str) or not finite_number(physics_time):
        return None
    reset_epoch = reset_epoch_for_time(resets, run_id, physics_time)
    if reset_epoch is None:
        return None
    if phase_index is not None:
        accepted_times, candidates = phase_index.get((run_id, reset_epoch), ([], []))
        index = bisect.bisect_right(accepted_times, physics_time) - 1
        return candidates[index]['key'] if index >= 0 else None
    candidates = [
        phase for phase in phases
        if phase['runId'] == run_id
        and phase['resetEpoch'] == reset_epoch
        and phase['acceptedTime'] <= physics_time
    ]
    return candidates[-1]['key'] if candidates else None


def run_checker_regressions() -> dict[str, bool]:
    positive = {DONOR_FIELD: [-1.0, 0.0], RECEIVER_FIELD: [1.0, 0.0]}
    negative = {DONOR_FIELD: [0.0, 1.0], RECEIVER_FIELD: [0.0, -1.0]}
    asymmetric = {DONOR_FIELD: [0.0, 0.0, -0.4], RECEIVER_FIELD: [0.0, 0.6, 0.0]}
    signs = [bottom_motion_sign(item) for item in (positive, negative)]
    sign_switches = sum(
        1 for previous, current in zip(signs, signs[1:])
        if previous != 0 and current != 0 and previous != current
    )
    indices = active_indices([1.0, 0.0, 0.0, 2.0])
    donor, receiver = signed_bottom_motion(asymmetric)
    return {
        "positiveNegativeDirection": signs == [1, -1] and sign_switches == 1,
        "zeroActivityExcluded": indices == [0, 3],
        "asymmetricHalfDistribution": donor < 0 and receiver > 0,
    }


def segmentation_regression_metric(
    run_id: str, sequence: int, frame_id: int, substep_id: int,
    physics_time: float, physics_dt: float, effective_tilt: float
) -> dict[str, Any]:
    return {
        'routeId': DEFAULT_ROUTE, 'runId': run_id, 'sequence': sequence,
        'frameId': frame_id, 'substepId': substep_id,
        'physicsTime': physics_time, 'physicsDt': physics_dt,
        'targetTilt': effective_tilt, 'effectiveTilt': effective_tilt,
        'effectiveTiltRate': 0.0, 'bottomCreepRate': 0.0, 'bottomFlux': 0.0,
        'bottomCreepBudgetUsed': 0.0, 'bottomCreepBudgetLimit': 0.0,
        'bottomCreepBudgetClipped': 0.0, 'totalMotionBudgetUsed': 0.0,
        'totalMotionBudgetLimit': 0.0, 'totalMotionBudgetClipped': 0.0,
        'directionCorrect': True, 'waterTotal': 1.0, 'heavyTotal': 1.0,
        'lightTotal': 1.0,
    }


def segmentation_regression_bottom(
    metric: dict[str, Any], bottom_value: float, frame_override: int | None = None
) -> dict[str, Any]:
    return {
        'routeId': DEFAULT_ROUTE, 'runId': metric['runId'],
        'sequence': metric['sequence'],
        'frameId': frame_override if frame_override is not None else metric['frameId'],
        'substepId': metric['substepId'], 'physicsTime': metric['physicsTime'],
        'bh': [bottom_value], 'bl': [bottom_value],
        DONOR_FIELD: [0.0], RECEIVER_FIELD: [0.0],
    }


def segmentation_regression_log(
    samples: list[tuple[str, int, int, int, float, float, float, float]],
    inputs: list[tuple[str, int, float, float, float]],
    resets: list[tuple[str, int, float]],
    mismatch_frame: bool = False
) -> str:
    lines = ['WS_EVENT init ' + json.dumps({'capacity': 1, 'routeId': DEFAULT_ROUTE})]
    for run_id, sequence, event_time in resets:
        lines.append('P011_EVENT reset ' + json.dumps({
            'routeId': DEFAULT_ROUTE, 'runId': run_id, 'sequence': sequence,
            'eventTime': event_time, 'physicsTime': event_time,
            'reason': 'scene-reset', 'boundary': True,
        }))
    for run_id, sequence, accepted_time, physics_time, target_tilt in inputs:
        lines.append('P011_EVENT input ' + json.dumps({
            'routeId': DEFAULT_ROUTE, 'runId': run_id, 'sequence': sequence,
            'commandTime': accepted_time, 'acceptedTime': accepted_time,
            'physicsTime': physics_time, 'targetTilt': target_tilt,
            'source': 'script',
        }))
    for index, sample in enumerate(samples):
        run_id, sequence, frame_id, substep_id, physics_time, physics_dt, tilt, bottom_value = sample
        metric = segmentation_regression_metric(
            run_id, sequence, frame_id, substep_id, physics_time, physics_dt, tilt
        )
        bottom = segmentation_regression_bottom(
            metric, bottom_value, frame_override=(999 if mismatch_frame and index == 1 else None)
        )
        lines.append('P011_METRIC ' + json.dumps(metric))
        lines.append('P011_BOTTOM ' + json.dumps(bottom))
    return '\n'.join(lines) + '\n'


def run_segmentation_regressions() -> dict[str, bool]:
    sample = (lambda run_id, sequence, time, value, dt=0.008: (
        run_id, sequence, sequence, sequence, time, dt, 1.0, value
    ))
    continuous = check(segmentation_regression_log(
        [sample('run-a', 1, 10.0, 0.0), sample('run-a', 2, 20.0, 10.0)],
        [('run-a', 1, 0.0, 10.0, 1.0)], [('run-a', 0, 0.0)]
    ), DEFAULT_ROUTE)
    run_switch = check(segmentation_regression_log(
        [sample('run-a', 1, 10.0, 0.0), sample('run-b', 1, 40.0, 100.0)],
        [('run-a', 1, 0.0, 10.0, 1.0), ('run-b', 1, 30.0, 40.0, 1.0)],
        [('run-a', 0, 0.0), ('run-b', 0, 30.0)]
    ), DEFAULT_ROUTE)
    reset_switch = check(segmentation_regression_log(
        [sample('run-a', 1, 10.0, 0.0), sample('run-a', 2, 40.0, 100.0)],
        [('run-a', 1, 0.0, 10.0, 1.0), ('run-a', 2, 30.0, 40.0, 1.0)],
        [('run-a', 0, 0.0), ('run-a', 1, 30.0)]
    ), DEFAULT_ROUTE)
    phase_switch = check(segmentation_regression_log(
        [sample('run-a', 1, 10.0, 0.0), sample('run-a', 2, 40.0, 100.0)],
        [('run-a', 1, 0.0, 10.0, 1.0), ('run-a', 2, 30.0, 40.0, 1.0)],
        [('run-a', 0, 0.0)]
    ), DEFAULT_ROUTE)
    dormant = check(segmentation_regression_log(
        [sample('run-a', 1, 10.0, 0.0), sample('run-a', 2, 20.0, 100.0, 0.0)],
        [('run-a', 1, 0.0, 10.0, 1.0)], [('run-a', 0, 0.0)]
    ), DEFAULT_ROUTE)
    identity = check(segmentation_regression_log(
        [sample('run-a', 1, 10.0, 0.0), sample('run-a', 2, 20.0, 10.0)],
        [('run-a', 1, 0.0, 10.0, 1.0)], [('run-a', 0, 0.0)], mismatch_frame=True
    ), DEFAULT_ROUTE)
    sequence_regression = check(segmentation_regression_log(
        [sample('run-a', 2, 10.0, 0.0), sample('run-a', 1, 20.0, 10.0)],
        [('run-a', 1, 0.0, 10.0, 1.0)], [('run-a', 0, 0.0)]
    ), DEFAULT_ROUTE)
    multi_reset_same_target = check(segmentation_regression_log(
        [
            sample('run-a', 1, 10.0, 0.0, dt=0.008),
            sample('run-a', 2, 40.0, 100.0, dt=0.008),
            sample('run-a', 3, 70.0, 0.0, dt=0.008),
        ],
        [
            ('run-a', 1, 0.0, 10.0, 1.0),
            ('run-a', 2, 30.0, 40.0, 1.0),
            ('run-a', 3, 60.0, 70.0, 1.0),
        ],
        [('run-a', 0, 0.0), ('run-a', 1, 30.0), ('run-a', 2, 60.0)]
    ), DEFAULT_ROUTE)
    max_values = lambda result: (
        result['tcP01103']['maxSubstepBh'], result['tcP01103']['maxSubstepBl']
    )
    return {
        'sameRunContinuousSample': max_values(continuous) == (10.0, 10.0),
        'runSwitchExcluded': max_values(run_switch) == (0.0, 0.0),
        'resetSwitchExcluded': max_values(reset_switch) == (0.0, 0.0),
        'targetPhaseSwitchExcluded': max_values(phase_switch) == (0.0, 0.0),
        'dormantNoopExcluded': max_values(dormant) == (0.0, 0.0),
        'metricBottomIdentityMismatch': identity['dataCompleteness']['identityMismatchCount'] == 1,
        'sequenceRegressionBlocked': (
            sequence_regression['dataCompleteness']['sequenceRegressionCount'] == 1
            and sequence_regression['overall'] == 'BLOCKED(missing_hook)'
        ),
        'sameRunMultiResetSameTargetExcluded': (
            multi_reset_same_target['tcP01103']['maxUncommandedTiltStep'] == 0.0
        ),
    }


FULL_LOAD_REQUEST_FIELDS = (
    "settlementMotionRequest",
    "percolationMotionRequest",
    "slipMotionRequest",
    "suspensionMotionRequest",
)


def best_lag(activity: list[float], response: list[float], times: list[float]) -> tuple[float, float | None]:
    if len(activity) < 3 or len(response) < 3:
        return 0.0, None
    intervals = [b - a for a, b in zip(times, times[1:]) if b > a]
    step = max(1.0, min(intervals) if intervals else 1.0)
    best = (0.0, -math.inf)
    minimum_matches = max(3, math.ceil(len(activity) * 0.8))
    lag = -200.0
    pointers = [0] * len(times)
    first_lag = True
    while lag <= 200.0 + 1e-9:
        left: list[float] = []
        right: list[float] = []
        for index, time in enumerate(times):
            target_time = time + lag
            if first_lag:
                target_index = bisect.bisect_left(times, target_time)
                if target_index == len(times):
                    target_index -= 1
                elif target_index > 0 and abs(times[target_index - 1] - target_time) <= abs(times[target_index] - target_time):
                    target_index -= 1
                pointers[index] = target_index
            else:
                target_index = pointers[index]
                while (
                    target_index + 1 < len(times)
                    and abs(times[target_index + 1] - target_time)
                    < abs(times[target_index] - target_time)
                ):
                    target_index += 1
                pointers[index] = target_index
            if abs(times[target_index] - target_time) <= step * 0.51:
                left.append(activity[index])
                right.append(response[target_index])
        first_lag = False
        if len(left) >= minimum_matches:
            correlation = pearson(left, right)
            if correlation is not None and correlation > best[1]:
                best = (lag, correlation)
        lag += step
    return best[0], best[1]


def following_error(activity: list[float], response: list[float]) -> float | None:
    if not activity or len(activity) != len(response):
        return None
    activity_total = sum(activity)
    response_total = sum(response)
    if activity_total <= 0 or response_total <= 0:
        return None
    activity_cumulative = 0.0
    response_cumulative = 0.0
    maximum = 0.0
    for a_value, b_value in zip(activity, response):
        activity_cumulative += a_value / activity_total
        response_cumulative += b_value / response_total
        maximum = max(maximum, abs(activity_cumulative - response_cumulative))
    return maximum


def tiered_score_tc1(p95_t90: float | None, direction_error: int) -> int:
    if direction_error != 0 or p95_t90 is None:
        return 0
    if p95_t90 <= 50:
        return 10
    if p95_t90 <= 75:
        return 8
    if p95_t90 <= 100:
        return 6
    if p95_t90 <= 150:
        return 4
    if p95_t90 <= 200:
        return 2
    return 0


def tiered_score_tc2(correlation: float | None, lag: float, following: float | None) -> int:
    if correlation is None or following is None:
        return 0
    absolute_lag = abs(lag)
    if correlation >= 0.90 and absolute_lag <= 16.7 and following <= 0.05:
        return 10
    if correlation >= 0.80 and absolute_lag <= 33.3 and following <= 0.10:
        return 8
    if correlation >= 0.70 and absolute_lag <= 66.7 and following <= 0.15:
        return 6
    if correlation >= 0.55 and absolute_lag <= 100 and following <= 0.25:
        return 4
    if correlation >= 0.40 and absolute_lag <= 150 and following <= 0.35:
        return 2
    return 0


def tiered_score_tc3(max_step: float) -> int:
    if max_step <= 0.5:
        return 10
    if max_step <= 1.0:
        return 8
    if max_step <= 1.5:
        return 6
    if max_step <= 2.5:
        return 4
    if max_step <= 4.0:
        return 2
    return 0


def check(
    log_text: str,
    expected_route: str,
    include_regressions: bool = False,
    suite_role: str = BEHAVIOR_ROLE,
) -> dict[str, Any]:
    lines = log_text.splitlines()
    parsed, parse_errors, capacity = parse_lines(lines)
    all_p011_events = parsed['event']
    reset_events = [item for item in all_p011_events if item.get('_eventType') == 'reset']
    events = [
        item for item in all_p011_events if item.get('_eventType', 'input') == 'input'
    ]
    events.sort(key=lambda item: item.get('commandTime', 0))
    events = sorted(parsed["event"], key=lambda item: item.get("commandTime", 0))
    metrics = sorted(parsed["metric"], key=lambda item: item.get("physicsTime", 0))
    bottoms = sorted(parsed["bottom"], key=lambda item: item.get("physicsTime", 0))

    events = [
        item for item in all_p011_events if item.get('_eventType', 'input') == 'input'
    ]
    events.sort(key=lambda item: item.get('commandTime', 0))
    missing: list[str] = []
    if not events:
        missing.append("P011_EVENT input")
    if not metrics:
        missing.append("P011_METRIC")
    if not bottoms:
        missing.append("P011_BOTTOM")
    bottom_by_time = {item.get("physicsTime"): item for item in bottoms}
    for metric in metrics:
        for field in ("routeId", "physicsTime", "physicsDt", "targetTilt", "effectiveTilt", "effectiveTiltRate",
                      "bottomCreepRate", "bottomFlux", "bottomCreepBudgetUsed",
                      "bottomCreepBudgetLimit", "bottomCreepBudgetClipped",
                      "totalMotionBudgetUsed", "totalMotionBudgetLimit",
                      "totalMotionBudgetClipped", "directionCorrect", "waterTotal",
                      "heavyTotal", "lightTotal"):
            if field not in metric:
                missing.append(f"P011_METRIC.{field}")
                break
        if not isinstance(metric.get("directionCorrect"), bool):
            missing.append("P011_METRIC.directionCorrect(bool)")
            break
    if metrics and not all(valid_identity(metric) for metric in metrics):
        missing.append('P011_METRIC.runId/sequence/frameId/substepId')
    for bottom in bottoms:
        for key in ("bh", "bl", DONOR_FIELD, RECEIVER_FIELD):
            value = bottom.get(key)
            expected_length = len(bottoms[0].get(key, [])) if bottoms else None
            if not isinstance(value, list) or (expected_length is not None and len(value) != expected_length):
                missing.append(f"P011_BOTTOM.{key}")
                break
    if bottoms and not all(valid_identity(bottom) for bottom in bottoms):
        missing.append('P011_BOTTOM.runId/sequence/frameId/substepId')
    if capacity is None:
        missing.append("WS_EVENT init.capacity")
    for event in events:
        for field in ("routeId", "commandTime", "acceptedTime", "physicsTime", "targetTilt", "source"):
            if field not in event:
                missing.append(f"P011_EVENT input.{field}")
                break
        if event.get("source") not in {"script", "sensor", "gesture"}:
            missing.append("P011_EVENT input.source")
            break
    route_mismatch = any(item.get("routeId") != expected_route for item in events + metrics + bottoms)
    if events and not all(
        isinstance(event.get('runId'), str)
        and isinstance(event.get('sequence'), int)
        and not isinstance(event.get('sequence'), bool)
        for event in events
    ):
        missing.append('P011_EVENT input.runId/sequence')
    if reset_events and not all(
        isinstance(event.get('runId'), str)
        and isinstance(event.get('sequence'), int)
        and not isinstance(event.get('sequence'), bool)
        and finite_number(event.get('eventTime', event.get('physicsTime')))
        for event in reset_events
    ):
        missing.append('P011_EVENT reset.runId/sequence/eventTime')
    route_mismatch = route_mismatch or any(
        item.get('routeId') != expected_route for item in reset_events
    )
    if route_mismatch:
        missing.append(f"routeId={expected_route}")

    metric_times = [item.get("physicsTime") for item in metrics]
    bottom_times = [item.get("physicsTime") for item in bottoms]
    metric_by_time = {}
    for metric in metrics:
        metric_by_time.setdefault(metric.get("physicsTime"), metric)
    timestamp_order_errors = 0
    timestamp_order_errors += sum(
        1 for previous, current in zip(metric_times, metric_times[1:])
        if not finite_number(previous) or not finite_number(current) or current <= previous
    )
    timestamp_order_errors += sum(
        1 for previous, current in zip(bottom_times, bottom_times[1:])
        if not finite_number(previous) or not finite_number(current) or current <= previous
    )
    for event in events:
        command_time = event.get("commandTime")
        accepted_time = event.get("acceptedTime")
        physics_time = event.get("physicsTime")
        if not all(finite_number(value) for value in (command_time, accepted_time, physics_time)):
            timestamp_order_errors += 1
            continue
        if not command_time <= accepted_time <= physics_time:
            timestamp_order_errors += 1
        if physics_time not in metric_times:
            timestamp_order_errors += 1
    if (
        len(metrics) != len(bottoms)
        or len(set(metric_times)) != len(metric_times)
        or len(set(bottom_times)) != len(bottom_times)
        or set(metric_times) != set(bottom_times)
    ):
        missing.append("P011_METRIC/P011_BOTTOM.physicsTime alignment")

    identity_missing_count = sum(
        not valid_identity(metric) or not valid_identity(bottom_by_time.get(metric.get('physicsTime'), {}))
        for metric in metrics
    )
    identity_mismatch_count = sum(
        valid_identity(metric)
        and identity_tuple(metric) != identity_tuple(bottom_by_time.get(metric.get('physicsTime'), {}))
        for metric in metrics
    )
    sequence_regression_count = 0
    frame_id_regression_count = 0
    substep_id_regression_count = 0
    observed_run_ids = list(dict.fromkeys(
        metric.get('runId') for metric in metrics if isinstance(metric.get('runId'), str)
    ))
    metrics_by_run: dict[Any, list[dict[str, Any]]] = defaultdict(list)
    for metric in metrics:
        metrics_by_run[metric.get('runId')].append(metric)
    for run_id in observed_run_ids:
        run_metrics = metrics_by_run[run_id]
        for previous, current in zip(run_metrics, run_metrics[1:]):
            if current.get('sequence', 0) <= previous.get('sequence', 0):
                sequence_regression_count += 1
            if current.get('frameId', 0) < previous.get('frameId', 0):
                frame_id_regression_count += 1
            if current.get('substepId', 0) < previous.get('substepId', 0):
                substep_id_regression_count += 1
    reset_run_ids = {
        event.get('runId') for event in reset_events if isinstance(event.get('runId'), str)
    }
    run_without_reset_count = sum(run_id not in reset_run_ids for run_id in observed_run_ids)
    resets = reset_times_by_run(reset_events)
    phases = build_phase_index(events, reset_events)
    phase_index = build_phase_lookup(phases)
    metric_phase_keys = [
        phase_for_metric(metric, phases, resets, phase_index) for metric in metrics
    ]
    metric_scope_keys = []
    for metric in metrics:
        reset_epoch = reset_epoch_for_time(
            resets, metric.get('runId'), metric.get('physicsTime')
        )
        metric_scope_keys.append(
            (metric.get('runId'), reset_epoch) if reset_epoch is not None else None
        )
    metric_scope_by_time = {
        metric.get('physicsTime'): key for metric, key in zip(metrics, metric_scope_keys)
    }
    metric_phase_by_time = {
        metric.get('physicsTime'): key for metric, key in zip(metrics, metric_phase_keys)
    }
    phase_counts = Counter(metric_phase_keys)
    phase_sample_counts = {
        key: phase_counts[key] for key in dict.fromkeys(metric_phase_keys) if key is not None
    }
    if identity_missing_count:
        missing.append('P011 identity fields')

    bottom_response_values: list[float] = []
    direction_error = 0
    reverse_direction_error = 0
    bottom_direction_error = 0
    bottom_direction_samples = 0
    bottom_direction_sign_switches = 0
    positive_to_negative_switches = 0
    negative_to_positive_switches = 0
    last_non_zero_command_direction = 0
    last_non_zero_bottom_direction = 0
    zero_residual_samples: list[float] = []
    max_uncommanded_tilt_step = 0.0
    bottom_scopes = [metric_scope_by_time.get(bottom.get("physicsTime")) for bottom in bottoms]
    bottom_active = []
    for bottom in bottoms:
        donor_motion, receiver_motion = signed_bottom_motion(bottom)
        bottom_active.append(abs(donor_motion) > BOTTOM_FLUX_EPS and abs(receiver_motion) > BOTTOM_FLUX_EPS)
    bottom_scope_run_end = [0] * len(bottoms)
    next_active_bottom_in_scope = [len(bottoms)] * len(bottoms)
    for bottom_index in range(len(bottoms) - 1, -1, -1):
        next_index = bottom_index + 1
        bottom_scope_run_end[bottom_index] = bottom_scope_run_end[next_index] if next_index < len(bottoms) and bottom_scopes[bottom_index] == bottom_scopes[next_index] else next_index
        if next_index < len(bottoms) and bottom_scope_run_end[bottom_index] == bottom_scope_run_end[next_index]:
            next_active_bottom_in_scope[bottom_index] = next_active_bottom_in_scope[next_index]
        if bottom_active[bottom_index]:
            next_active_bottom_in_scope[bottom_index] = bottom_index
    adjacent_tilt_deltas = [
        current.get("effectiveTilt", 0) - previous.get("effectiveTilt", 0)
        for previous, current in zip(metrics, metrics[1:])
    ]
    negative_delta_prefix = [0]
    positive_delta_prefix = [0]
    for delta in adjacent_tilt_deltas:
        negative_delta_prefix.append(negative_delta_prefix[-1] + (delta < 0))
        positive_delta_prefix.append(positive_delta_prefix[-1] + (delta > 0))

    for index, event in enumerate(events):
        command_time = event.get("commandTime")
        target = event.get("targetTilt")
        if not finite_number(command_time) or not finite_number(target):
            continue
        baseline_metric = nearest_metric_before(metrics, command_time, metric_times)
        baseline = baseline_metric.get("effectiveTilt", 0) if baseline_metric else 0
        change = target - baseline
        direction = 1 if change > 0 else (-1 if change < 0 else 0)
        if direction == 0:
            direction = 1 if target > 0 else (-1 if target < 0 else 0)
        if target > 1e-9:
            command_direction = 1
        elif target < -1e-9:
            command_direction = -1
        else:
            command_direction = 0
        if direction != 0:
            response_start = max(1, bisect.bisect_left(metric_times, command_time))
            response_end = min(
                len(metrics) - 1,
                bisect.bisect_right(metric_times, command_time + 100) - 1,
            )
            if response_start <= response_end:
                delta_prefix = (
                    negative_delta_prefix if direction > 0 else positive_delta_prefix
                )
                direction_error += (
                    delta_prefix[response_end] - delta_prefix[response_start - 1]
                )
        previous_target = events[index - 1].get("targetTilt", 0) if index else 0
        if previous_target * target < 0:
            if response_start <= response_end:
                delta_prefix = (
                    negative_delta_prefix if direction > 0 else positive_delta_prefix
                )
                reverse_direction_error += (
                    delta_prefix[response_end] - delta_prefix[response_start - 1]
                )
        if abs(target) <= 1e-9:
            next_command = events[index + 1].get("commandTime", math.inf) if index + 1 < len(events) else math.inf
            residual_start = bisect.bisect_left(metric_times, command_time + 300)
            residual_end = bisect.bisect_left(metric_times, next_command)
            zero_residual_samples.extend(
                abs(metrics[sample_index].get("effectiveTilt", 0))
                for sample_index in range(residual_start, residual_end)
            )
    for current_index, current in enumerate(metrics[1:], start=1):
        previous = metrics[current_index - 1]
        if (
            metric_scope_keys[current_index] is None
            or metric_scope_keys[current_index] != metric_scope_keys[current_index - 1]
        ):
            continue
        crossed_dead_zone = (
            abs(previous.get("effectiveTilt", 0)) <= DEAD_ZONE_DEGREES
            and abs(current.get("effectiveTilt", 0)) > DEAD_ZONE_DEGREES
        )
        crossed_dead_zone = crossed_dead_zone or (
            previous.get("effectiveTilt", 0) * current.get("effectiveTilt", 0) < 0
            and abs(current.get("effectiveTilt", 0)) > DEAD_ZONE_DEGREES
        )
        if not crossed_dead_zone:
            continue
        crossing_time = current.get("physicsTime", 0)
        crossing_scope = metric_scope_keys[current_index]
        bottom_index = max(1, bisect.bisect_left(bottom_times, crossing_time))
        bottom_index = (
            next_active_bottom_in_scope[bottom_index]
            if bottom_index < len(bottoms)
            else len(bottoms)
        )
        response = bottoms[bottom_index].get("physicsTime", 0) - crossing_time if bottom_index < len(bottoms) else None
        if response is not None:
            bottom_response_values.append(response)

    for current_index, current in enumerate(metrics[1:], start=1):
        previous = metrics[current_index - 1]
        if (
            metric_scope_keys[current_index] is None
            or metric_scope_keys[current_index] != metric_scope_keys[current_index - 1]
        ):
            continue
        time_delta = current.get("physicsDt", 0)
        if not finite_number(time_delta) or time_delta <= 0 or not finite_number(previous.get("effectiveTilt", 0)):
            continue
        expected_step = current.get("targetTilt", 0) - previous.get("effectiveTilt", 0)
        expected_step = max(
            -TILT_RESPONSE_RATE_DEG_PER_S * time_delta,
            min(TILT_RESPONSE_RATE_DEG_PER_S * time_delta, expected_step),
        )
        residual = current.get("effectiveTilt", 0) - (previous.get("effectiveTilt", 0) + expected_step)
        max_uncommanded_tilt_step = max(max_uncommanded_tilt_step, abs(residual))

    times: list[float] = []
    activity: list[float] = []
    response_h: list[float] = []
    response_l: list[float] = []
    response_cumulative_h = 0.0
    response_cumulative_l = 0.0
    current_phase_key: tuple[str, int] | None = None
    for current_index, (previous, current) in enumerate(zip(metrics, metrics[1:]), start=1):
        if (
            metric_scope_keys[current_index] is None
            or metric_scope_keys[current_index] != metric_scope_keys[current_index - 1]
        ):
            continue
        time = current.get("physicsTime", 0)
        previous_time = previous.get("physicsTime", 0)
        bottom = bottom_by_time.get(time)
        if bottom is None or current.get("runId") is None:
            continue
        reset_epoch = reset_epoch_for_time(resets, current.get("runId"), current.get("physicsTime")) or 0
        target_sign = 1 if current.get("targetTilt", 0) > DEAD_ZONE_DEGREES else (-1 if current.get("targetTilt", 0) < -DEAD_ZONE_DEGREES else 0)
        if target_sign == 0:
            continue
        phase_key = (f'{current["runId"]}:{reset_epoch}', target_sign)
        if phase_key != current_phase_key:
            current_phase_key = phase_key
            response_cumulative_h = 0.0
            response_cumulative_l = 0.0
        _, receiver_motion = signed_bottom_motion(bottom)
        signed_displacement = receiver_motion
        response_cumulative_h += signed_displacement
        response_cumulative_l += signed_displacement
        times.append(time)
        activity.append(target_sign)
        response_h.append(response_cumulative_h)
        response_l.append(response_cumulative_l)
    active_sample_indices = active_indices(activity)
    active_times = [times[index] for index in active_sample_indices]
    active_a = [activity[index] for index in active_sample_indices]
    active_h = [response_h[index] for index in active_sample_indices]
    active_l = [response_l[index] for index in active_sample_indices]
    correlation_h = pearson(active_a, active_h)
    correlation_l = pearson(active_a, active_l)
    lag_h, correlated_h = best_lag(active_a, active_h, active_times)
    lag_l, correlated_l = best_lag(active_a, active_l, active_times)
    fe_h = following_error([abs(value) for value in active_a], [abs(value) for value in active_h])
    fe_l = following_error([abs(value) for value in active_a], [abs(value) for value in active_l])

    max_substep_bh = 0.0
    max_substep_bl = 0.0
    bottom_budget_violations = 0
    total_budget_violations = 0
    bottom_independent_clips = 0
    total_motion_clips = 0
    non_finite_count = 0
    out_of_range_count = 0
    non_dead_zone_samples = 0
    direction_correct_samples = 0
    for previous, current in zip(bottoms, bottoms[1:]):
        previous_phase = metric_phase_by_time.get(previous.get('physicsTime'))
        current_phase = metric_phase_by_time.get(current.get('physicsTime'))
        current_metric = metric_by_time.get(current.get('physicsTime'))
        allow_difference = (
            previous_phase is not None
            and previous_phase == current_phase
            and current_metric is not None
            and finite_number(current_metric.get('physicsDt'))
            and current_metric.get('physicsDt', 0) > 0
        )
        for key in ("bh", "bl"):
            for a, b in zip(previous[key], current[key]):
                if not finite_number(a) or not finite_number(b):
                    non_finite_count += 1
                    continue
                difference = abs(b - a) if allow_difference else 0.0
                if key == "bh":
                    max_substep_bh = max(max_substep_bh, difference)
                else:
                    max_substep_bl = max(max_substep_bl, difference)
                if a < -1e-9 or b < -1e-9 or (capacity is not None and (a > capacity + 1e-9 or b > capacity + 1e-9)):
                    out_of_range_count += 1
    for metric in metrics:
        if abs(metric.get("effectiveTilt", 0)) > DEAD_ZONE_DEGREES:
            non_dead_zone_samples += 1
            if metric.get("directionCorrect") is True:
                direction_correct_samples += 1
        for field in ("effectiveTilt", "effectiveTiltRate", "bottomCreepRate", "bottomFlux",
                      "waterTotal", "heavyTotal", "lightTotal"):
            if not finite_number(metric.get(field)):
                non_finite_count += 1
        for field in ('bottomCreepBudgetUsed', 'bottomCreepBudgetLimit',
                      'bottomCreepBudgetClipped', 'totalMotionBudgetUsed',
                      'totalMotionBudgetLimit', 'totalMotionBudgetClipped'):
            if not finite_number(metric.get(field)):
                non_finite_count += 1
        if metric.get('bottomCreepBudgetUsed', 0) > metric.get('bottomCreepBudgetLimit', 0) + 1e-9:
            bottom_budget_violations += 1
        if metric.get('totalMotionBudgetUsed', 0) > metric.get('totalMotionBudgetLimit', 0) + 1e-9:
            total_budget_violations += 1
        if metric.get('bottomCreepBudgetClipped', 0) > 1e-9:
            bottom_independent_clips += 1
        if metric.get('totalMotionBudgetClipped', 0) > 1e-9:
            total_motion_clips += 1
        for field in (DONOR_FIELD, RECEIVER_FIELD):
            if any(not finite_number(value) for value in bottom_by_time.get(metric.get("physicsTime"), {}).get(field, [])):
                non_finite_count += 1

    for current_index, metric in enumerate(metrics[1:], start=1):
        previous_metric = metrics[current_index - 1]
        if (
            metric_scope_keys[current_index] is None
            or metric_scope_keys[current_index] != metric_scope_keys[current_index - 1]
        ):
            last_non_zero_bottom_direction = 0
        effective_tilt = metric.get("effectiveTilt", 0)
        direction_sign = 1 if effective_tilt > DEAD_ZONE_DEGREES else (-1 if effective_tilt < -DEAD_ZONE_DEGREES else 0)
        if direction_sign == 0:
            continue
        if metric.get("bottomCreepBudgetUsed", 0) <= 1e-9:
            continue
        previous_bottom = bottom_by_time.get(previous_metric.get("physicsTime"))
        current_bottom = bottom_by_time.get(metric.get("physicsTime"))
        if previous_bottom is None or current_bottom is None:
            continue
        donor_motion, receiver_motion = signed_bottom_motion(current_bottom)
        expected_receiver_motion = direction_sign * receiver_motion > 1e-9
        expected_donor_motion = direction_sign * donor_motion < -1e-9
        observed_direction = bottom_motion_sign(current_bottom)
        if observed_direction != 0:
            if last_non_zero_bottom_direction != 0 and observed_direction != last_non_zero_bottom_direction:
                if last_non_zero_bottom_direction == 1 and observed_direction == -1:
                    positive_to_negative_switches += 1
                elif last_non_zero_bottom_direction == -1 and observed_direction == 1:
                    negative_to_positive_switches += 1
            last_non_zero_bottom_direction = observed_direction
        bottom_direction_samples += 1
        if not (expected_donor_motion and expected_receiver_motion):
            bottom_direction_error += 1

    bottom_direction_sign_switches = positive_to_negative_switches + negative_to_positive_switches
    initial = metrics[0] if metrics else {}
    drifts = {"water": 0.0, "heavy": 0.0, "light": 0.0}
    for phase, field in drifts.items():
        initial_total = initial.get(f"{phase}Total", 0)
        if initial_total:
            drifts[phase] = max(
                abs(metric.get(f"{phase}Total", 0) - initial_total) / abs(initial_total)
                for metric in metrics
            )
    model_error_count = sum("WS_EVENT model_error" in line for line in lines)
    crash_count = sum(marker in line for marker in ("FATAL", "Fatal", "app freeze") for line in lines)
    zero_residual = max(zero_residual_samples, default=0.0)
    p95_t90 = percentile_95(bottom_response_values)
    positive_bottom_limits = [
        metric.get("bottomCreepBudgetLimit", 0) for metric in metrics
        if metric.get("bottomCreepBudgetLimit", 0) > 0
    ]
    solo_budget_limit = min(positive_bottom_limits) if positive_bottom_limits else 0.0
    full_load_limit_error_rate = 0.0
    full_load_samples = 0
    full_load_isolation_violations = 0
    total_motion_clip_runs = 0
    total_motion_clip_segments: list[dict[str, Any]] = []
    previous_total_motion_clip = False
    for metric in metrics:
        total_motion_clip = metric.get("totalMotionBudgetClipped", 0) > 1e-9
        if total_motion_clip and not previous_total_motion_clip:
            total_motion_clip_runs += 1
            total_motion_clip_segments.append({
                "segmentId": f"totalClip-{total_motion_clip_runs}",
                "startTime": metric.get("physicsTime"),
                "endTime": metric.get("physicsTime"),
                "clipSampleCount": 0,
                "maxClipped": 0.0,
                "maxTotalUsed": 0.0,
            })
        previous_total_motion_clip = total_motion_clip
        if not total_motion_clip:
            continue
        if total_motion_clip_segments:
            segment = total_motion_clip_segments[-1]
            segment["endTime"] = metric.get("physicsTime")
            segment["clipSampleCount"] += 1
            segment["maxClipped"] = max(segment["maxClipped"], metric.get("totalMotionBudgetClipped", 0))
            segment["maxTotalUsed"] = max(segment["maxTotalUsed"], metric.get("totalMotionBudgetUsed", 0))
        request_fields_present = any(field in metric for field in FULL_LOAD_REQUEST_FIELDS)
        requests_concurrent = (
            not request_fields_present
            or all(metric.get(field, 0) > 1e-9 for field in FULL_LOAD_REQUEST_FIELDS)
        )
        if requests_concurrent:
            full_load_samples += 1
        limit = metric.get("bottomCreepBudgetLimit", 0)
        if solo_budget_limit > 0 and limit < solo_budget_limit * 0.98:
            full_load_isolation_violations += 1
    if full_load_samples:
        full_load_limit_error_rate = full_load_isolation_violations / full_load_samples
    direction_accuracy = (
        direction_correct_samples / non_dead_zone_samples
        if non_dead_zone_samples else None
    )
    score_tc1 = tiered_score_tc1(p95_t90, direction_error)
    score_tc2_h = tiered_score_tc2(correlation_h, lag_h, fe_h)
    score_tc2_l = tiered_score_tc2(correlation_l, lag_l, fe_l)
    score_tc2 = min(score_tc2_h, score_tc2_l)
    score_tc3 = tiered_score_tc3(max(max_substep_bh, max_substep_bl))
    regression_checks = (
        run_checker_regressions() | run_segmentation_regressions()
        if include_regressions else None
    )

    must_pass = {
        "driftBelow0_1Percent": max(drifts.values()) < 0.001,
        "nonFiniteCountZero": non_finite_count == 0,
        "outOfRangeCountZero": out_of_range_count == 0,
        "modelErrorZero": model_error_count == 0,
        "crashZero": crash_count == 0,
        "maxSubstepBelow5": max(max_substep_bh, max_substep_bl) <= 5,
        "maxUncommandedTiltStepBelow2": max_uncommanded_tilt_step <= 2,
        "zeroResidualBelow1": zero_residual <= 1,
        "reverseDirectionErrorZero": reverse_direction_error == 0,
        "bottomBudgetViolationZero": bottom_budget_violations == 0,
        "totalBudgetViolationZero": total_budget_violations == 0,
        "directionCorrectAll": (
            non_dead_zone_samples >= 200
            and direction_correct_samples == non_dead_zone_samples
        ),
        "bottomDirectionCorrectAll": (
            bottom_direction_samples >= 200
            and bottom_direction_error == 0
        ),
        "independentBudgetIsolation": (
            solo_budget_limit > 0
            and full_load_samples > 0
            and full_load_isolation_violations == 0
        ) if BUDGET_ISOLATION_ROLE in suite_role.split('+') else NOT_APPLICABLE,
    }
    if regression_checks is not None:
        must_pass["checkerRegressionCoverage"] = all(regression_checks.values())
    data_complete = (
        not missing
        and parse_errors == 0
        and not route_mismatch
        and timestamp_order_errors == 0
        and identity_missing_count == 0
        and identity_mismatch_count == 0
        and sequence_regression_count == 0
        and frame_id_regression_count == 0
        and substep_id_regression_count == 0
        and run_without_reset_count == 0
    )
    applicable_must_pass = [
        value for value in must_pass.values() if value != NOT_APPLICABLE
    ]
    overall = "BLOCKED(missing_hook)" if not data_complete else (
        "PASS" if all(value is True for value in applicable_must_pass) and (
            suite_role != BEHAVIOR_ROLE or min(score_tc1, score_tc2, score_tc3) >= 6
        ) else "FAIL"
    )
    per_log_gate_pass = data_complete and all(value is True for value in applicable_must_pass)
    result = {
        'budgetMetrics': {
            'bottomCreepBudgetViolationRate': bottom_budget_violations / len(metrics) if metrics else None,
            'totalMotionBudgetViolationRate': total_budget_violations / len(metrics) if metrics else None,
            'bottomCreepIndependentClipRate': bottom_independent_clips / len(metrics) if metrics else None,
            'totalMotionClipRate': total_motion_clips / len(metrics) if metrics else None,
        },
        "schemaVersion": 1,
        "routeId": expected_route,
        "suiteRole": suite_role,
        "overall": overall,
        "perLogGatePass": per_log_gate_pass,
        "dataCompleteness": {
            "missingHooks": sorted(set(missing)),
            "parseErrors": parse_errors,
            "timestampOrderErrors": timestamp_order_errors,
            "routeMismatch": route_mismatch,
            "eventCount": len(events),
            "metricCount": len(metrics),
            "bottomCount": len(bottoms),
        },
        "tcP01101": {
            "bottomCreepResponseCount": len(bottom_response_values),
            "t90P95Ms": p95_t90,
            "directionAccuracy": direction_accuracy,
            "directionError": direction_error,
            "bottomDirectionSamples": bottom_direction_samples,
            "bottomDirectionError": bottom_direction_error,
            "bottomDirectionSignSwitches": bottom_direction_sign_switches,
            "score": score_tc1 if suite_role == BEHAVIOR_ROLE else NOT_APPLICABLE,
        },
        "tcP01102": {
            "activitySamples": len(active_a),
            "varianceACorr": sum((value - sum(active_a) / len(active_a)) ** 2 for value in active_a) / len(active_a) if active_a else None,
            "varianceRCorrH": sum((value - sum(active_h) / len(active_h)) ** 2 for value in active_h) / len(active_h) if active_h else None,
            "varianceRCorrL": sum((value - sum(active_l) / len(active_l)) ** 2 for value in active_l) / len(active_l) if active_l else None,
            "positivePhaseSamples": sum(value > 0 for value in active_a),
            "negativePhaseSamples": sum(value < 0 for value in active_a),
            "correlationSamplesH": len(active_a),
            "correlationSamplesL": len(active_a),
            "pearsonH": correlation_h,
            "pearsonL": correlation_l,
            "lagHMs": lag_h,
            "lagLMs": lag_l,
            "feH": fe_h,
            "feL": fe_l,
            "score": score_tc2 if suite_role == BEHAVIOR_ROLE else NOT_APPLICABLE,
        },
        "tcP01103": {
            "maxSubstepBh": max_substep_bh,
            "maxSubstepBl": max_substep_bl,
            "maxUncommandedTiltStep": max_uncommanded_tilt_step,
            "zeroResidual": zero_residual,
            "reverseDirectionError": reverse_direction_error,
            "score": score_tc3,
        },
        "tcP01104": {
            "driftWater": drifts["water"],
            "driftHeavy": drifts["heavy"],
            "driftLight": drifts["light"],
            "nonFiniteCount": non_finite_count,
            "outOfRangeCount": out_of_range_count,
            "modelErrorCount": model_error_count,
            "crashCount": crash_count,
        },
        "budgetAcceptance": {
            "soloBottomCreepBudgetLimit": solo_budget_limit,
            "fullLoadSamples": full_load_samples,
            "fullLoadLimitErrorRate": full_load_limit_error_rate,
            "totalMotionClipRuns": total_motion_clip_runs,
            "totalMotionClipSegments": total_motion_clip_segments,
        },
        "regressionChecks": regression_checks,
        "mustPass": must_pass,
    }


    result['dataCompleteness'].update({
        'identityMissingCount': identity_missing_count,
        'identityMismatchCount': identity_mismatch_count,
        'sequenceRegressionCount': sequence_regression_count,
        'frameIdRegressionCount': frame_id_regression_count,
        'substepIdRegressionCount': substep_id_regression_count,
        'runWithoutResetCount': run_without_reset_count,
    })
    result['identityObservability'] = {
        'runIds': observed_run_ids,
        'resetCountByRun': {
            run_id: len(resets.get(run_id, [])) for run_id in observed_run_ids
        },
        'sequenceRangeByRun': {
            run_id: {
                'min': min(
                    metric.get('sequence', 0) for metric in metrics if metric.get('runId') == run_id
                ),
                'max': max(
                    metric.get('sequence', 0) for metric in metrics if metric.get('runId') == run_id
                ),
            }
            for run_id in observed_run_ids
        },
        'frameIdRangeByRun': {
            run_id: {
                'min': min(
                    metric.get('frameId', 0) for metric in metrics if metric.get('runId') == run_id
                ),
                'max': max(
                    metric.get('frameId', 0) for metric in metrics if metric.get('runId') == run_id
                ),
            }
            for run_id in observed_run_ids
        },
        'substepIdRangeByRun': {
            run_id: {
                'min': min(
                    metric.get('substepId', 0) for metric in metrics if metric.get('runId') == run_id
                ),
                'max': max(
                    metric.get('substepId', 0) for metric in metrics if metric.get('runId') == run_id
                ),
            }
            for run_id in observed_run_ids
        },
        'targetPhaseCount': len(phases),
        'phaseSampleCounts': phase_sample_counts,
    }
    result['tcP01101']['bottomDirectionPositiveToNegativeSwitches'] = positive_to_negative_switches
    result['tcP01101']['bottomDirectionNegativeToPositiveSwitches'] = negative_to_positive_switches
    return result


def self_check_log() -> str:
    lines = [f'WS_EVENT init {{"capacity":192,"routeId":"{DEFAULT_ROUTE}"}}']
    phases = ((1000, -5, 20, 1820), (2000, 20, -20, 2820), (3000, -20, -5, 3820))
    bottom_left_offset = 0.0
    bottom_right_offset = 0.0
    ramp_sample_counts = (30, 8, 30)
    for phase_index, (command_time, start, target, end_time) in enumerate(phases):
        ramp = ramp_increments(start, target, ramp_sample_counts[phase_index], phase_index)
        metric_target = start
        lines.append("P011_EVENT input " + json.dumps({
            "routeId": DEFAULT_ROUTE,
            "commandTime": command_time,
            "acceptedTime": command_time + 2,
            "physicsTime": command_time + 8,
            "targetTilt": target,
            "source": "gesture",
        }, separators=(",", ":")))
        current = start
        baseline_metric = {
            'bottomCreepBudgetUsed': 0.0,
            'bottomCreepBudgetLimit': 10 * 0.008,
            'bottomCreepBudgetClipped': 0.0,
            'totalMotionBudgetUsed': 0.2,
            'totalMotionBudgetLimit': 300 * 0.008,
            'totalMotionBudgetClipped': 0.0,
            'directionCorrect': True,
            "routeId": DEFAULT_ROUTE,
            "physicsTime": command_time,
            "physicsDt": 0.0,
            "targetTilt": start,
            "effectiveTilt": start,
            "effectiveTiltRate": 0.0,
            "bottomCreepRate": 96,
            "bottomFlux": 0.0,
            "waterTotal": 500,
            "heavyTotal": 400,
            "lightTotal": 400,
        }
        lines.append("P011_METRIC " + json.dumps(baseline_metric, separators=(",", ":")))
        lines.append("P011_BOTTOM " + json.dumps({
            "routeId": DEFAULT_ROUTE,
            "physicsTime": command_time,
            "bh": [50 + bottom_left_offset if column < 4 else 50 + bottom_right_offset for column in range(8)],
            "bl": [50 + bottom_left_offset if column < 4 else 50 + bottom_right_offset for column in range(8)],
            DONOR_FIELD: [0.0 for _column in range(8)],
            RECEIVER_FIELD: [0.0 for _column in range(8)],
        }, separators=(",", ":")))
        time = command_time + 8
        sample_index = 0
        while time <= end_time:
            step = ramp[sample_index] if sample_index < len(ramp) else 0.0
            sample_index += 1
            current += step
            if sample_index >= len(ramp):
                current = target
            metric_target += step
            activity = abs(step)
            direction_sign = 1 if current > DEAD_ZONE_DEGREES else (-1 if current < -DEAD_ZONE_DEGREES else 0)
            bottom_requested = activity / 40 + 0.008 if direction_sign != 0 else 0.0
            bottom_limit = 10 * 0.008
            bottom_used = min(bottom_requested, bottom_limit)
            total_limit = 300 * 0.008
            total_clipped = 0.0
            total_used = bottom_used + 0.2
            full_load = time in (1608.0, 1624.0, 2608.0, 2624.0, 3608.0, 3624.0)
            if full_load:
                total_used = total_limit
                total_clipped = 0.01
            bottom_left_offset -= bottom_used * direction_sign
            bottom_right_offset += bottom_used * direction_sign
            bh = [50 + (bottom_left_offset if column < 4 else bottom_right_offset) for column in range(8)]
            bl = [50 + (bottom_left_offset if column < 4 else bottom_right_offset) for column in range(8)]
            donor_displacement = [-direction_sign * bottom_used if column < 4 else 0.0 for column in range(8)]
            receiver_displacement = [0.0 if column < 4 else direction_sign * bottom_used for column in range(8)]
            metric = {
                'bottomCreepBudgetUsed': bottom_used,
                'bottomCreepBudgetLimit': bottom_limit,
                'bottomCreepBudgetClipped': max(0.0, bottom_requested - bottom_used),
                'totalMotionBudgetUsed': total_used,
                'totalMotionBudgetLimit': total_limit,
                'totalMotionBudgetClipped': total_clipped,
                'directionCorrect': True,
                "routeId": DEFAULT_ROUTE,
                "physicsTime": time,
                "physicsDt": 0.008,
                "targetTilt": metric_target,
                "effectiveTilt": current,
                "effectiveTiltRate": activity / 0.008,
                "bottomCreepRate": 96 + activity,
                "bottomFlux": bottom_used,
                "waterTotal": 500,
                "heavyTotal": 400,
                "lightTotal": 400,
            }
            metric.update({
                field: (0.5 if full_load else 0.0)
                for field in FULL_LOAD_REQUEST_FIELDS
            })
            lines.append("P011_METRIC " + json.dumps(metric, separators=(",", ":")))
            bottom = {
                "routeId": DEFAULT_ROUTE,
                "physicsTime": time,
                "bh": bh,
                "bl": bl,
            DONOR_FIELD: donor_displacement,
            RECEIVER_FIELD: receiver_displacement,
            }
            lines.append("P011_BOTTOM " + json.dumps(bottom, separators=(",", ":")))
            time += 8
    run_id = 'p011-selfcheck-run-1'
    input_sequence = 0
    sample_sequence = 0
    frame_id = 0
    substep_id = 0
    last_identity = None
    identity_lines: list[str] = []
    for line in lines:
        identity_lines.append(line)
        if line.startswith('WS_EVENT init '):
            identity_lines.append('P011_EVENT reset ' + json.dumps({
                'routeId': DEFAULT_ROUTE, 'runId': run_id, 'sequence': 0,
                'eventTime': 0, 'physicsTime': 0, 'reason': 'self-check',
                'boundary': True,
            }))
        elif line.startswith('P011_EVENT input '):
            input_sequence += 1
            payload = json.loads(line.split(' ', 3)[2])
            payload['runId'] = run_id
            payload['sequence'] = input_sequence
            identity_lines[-1] = 'P011_EVENT input ' + json.dumps(payload)
        elif line.startswith('P011_METRIC '):
            sample_sequence += 1
            frame_id += 1
            substep_id += 1
            payload = json.loads(line.split(' ', 1)[1])
            payload['runId'] = run_id
            payload['sequence'] = sample_sequence
            payload['frameId'] = frame_id
            payload['substepId'] = substep_id
            last_identity = {
                'runId': payload['runId'], 'sequence': payload['sequence'],
                'frameId': payload['frameId'], 'substepId': payload['substepId'],
            }
            identity_lines[-1] = 'P011_METRIC ' + json.dumps(payload)
        elif line.startswith('P011_BOTTOM '):
            payload = json.loads(line.split(' ', 1)[1])
            if last_identity is not None:
                payload.update(last_identity)
            identity_lines[-1] = 'P011_BOTTOM ' + json.dumps(payload)
    return '\n'.join(identity_lines) + '\n'
    return "\n".join(lines) + "\n"


def check_suite(
    log_texts: list[str],
    expected_route: str,
    include_regressions: bool = False,
    expected_log_count: int = 5,
    pressure_log_count: int = 4,
    log_roles: list[str] | None = None,
) -> dict[str, Any]:
    if log_roles is None:
        log_roles = [BEHAVIOR_ROLE, "budgetIsolation+pressureClip"] + [PRESSURE_CLIP_ROLE] * (expected_log_count - 2)
    results = [
        check(
            log_text,
            expected_route,
            include_regressions=include_regressions,
            suite_role=role,
        )
        for log_text, role in zip(log_texts, log_roles)
    ]
    behavior_results = [result for result, role in zip(results, log_roles) if role == BEHAVIOR_ROLE]
    budget_results = [
        result for result, role in zip(results, log_roles)
        if BUDGET_ISOLATION_ROLE in role.split('+')
    ]
    pressure_results = [
        result for result, role in zip(results, log_roles)
        if PRESSURE_CLIP_ROLE in role.split('+')
    ]
    positive_to_negative = sum(
        result['tcP01101']['bottomDirectionPositiveToNegativeSwitches'] for result in behavior_results
    )
    negative_to_positive = sum(
        result['tcP01101']['bottomDirectionNegativeToPositiveSwitches'] for result in behavior_results
    )
    pressure_total_motion_clip_runs = sum(
        result['budgetAcceptance']['totalMotionClipRuns'] for result in pressure_results
    )
    coverage = {
        'positiveToNegativeSwitches': positive_to_negative,
        'negativeToPositiveSwitches': negative_to_positive,
        'covered': positive_to_negative >= 1 and negative_to_positive >= 1,
    }
    high_pressure_coverage = {
        'pressureLogCount': len(pressure_results),
        'totalMotionClipRuns': pressure_total_motion_clip_runs,
        'perLogTotalMotionClipRuns': [
            result['budgetAcceptance']['totalMotionClipRuns']
            for result in pressure_results
        ],
        'passed': pressure_log_count == 0 or pressure_total_motion_clip_runs >= 3,
    }
    budget_result = budget_results[0] if len(budget_results) == 1 else None
    independent_budget_coverage = {
        'budgetIsolationLogCount': len(budget_results),
        'fullLoadSamples': budget_result['budgetAcceptance']['fullLoadSamples'] if budget_result else 0,
        'isolationViolations': (
            budget_result['budgetAcceptance']['fullLoadLimitErrorRate']
            * budget_result['budgetAcceptance']['fullLoadSamples'] if budget_result else None
        ),
        'bottomBudgetViolationRate': (
            budget_result['budgetMetrics']['bottomCreepBudgetViolationRate'] if budget_result else None
        ),
        'totalBudgetViolationRate': (
            budget_result['budgetMetrics']['totalMotionBudgetViolationRate'] if budget_result else None
        ),
        'passed': bool(
            budget_result
            and budget_result['budgetAcceptance']['fullLoadSamples'] > 0
            and budget_result['budgetAcceptance']['fullLoadLimitErrorRate'] == 0
            and budget_result['budgetMetrics']['bottomCreepBudgetViolationRate'] == 0
            and budget_result['budgetMetrics']['totalMotionBudgetViolationRate'] == 0
        ),
    }
    log_summaries = [
        {
            'logIndex': index,
            'suiteRole': role,
            'perLogGate': 'PASS' if result['perLogGatePass'] else 'FAIL',
            'perLogGatePass': result['perLogGatePass'],
            'identityObservability': result.get('identityObservability', {}),
            'bottomDirectionSignSwitches': result['tcP01101'][
                'bottomDirectionSignSwitches'
            ],
            'bottomDirectionPositiveToNegativeSwitches': result['tcP01101'][
                'bottomDirectionPositiveToNegativeSwitches'
            ],
            'bottomDirectionNegativeToPositiveSwitches': result['tcP01101'][
                'bottomDirectionNegativeToPositiveSwitches'
            ],
        }
        for index, (result, role) in enumerate(zip(results, log_roles))
    ]
    suite_must_pass = {
        'suiteInputComplete': len(results) == expected_log_count,
        'suiteRolesComplete': (
            len(log_roles) == len(results)
            and sum(role == BEHAVIOR_ROLE for role in log_roles) == 1
            and (expected_log_count == 1 or sum(BUDGET_ISOLATION_ROLE in role.split('+') for role in log_roles) == 1)
            and sum(PRESSURE_CLIP_ROLE in role.split('+') for role in log_roles) == pressure_log_count
        ),
        'allLogsComplete': bool(results) and all(
            result['overall'] != 'BLOCKED(missing_hook)' for result in results
        ),
        'allLogsPass': bool(results) and all(
            result['perLogGatePass'] for result in results
        ),
        'behaviorTierMinimum': bool(behavior_results) and all(
            result['overall'] == 'PASS' for result in behavior_results
        ),
        'highPressureEvidenceCount': pressure_log_count == 0 or high_pressure_coverage['passed'],
        'independentBudgetCoverage': expected_log_count == 1 or independent_budget_coverage['passed'],
        'suiteSignSwitchCoverage': coverage['covered'],
    }
    suite_input_complete = len(results) == expected_log_count
    overall = (
        'INCOMPLETE'
        if not suite_input_complete
        else ('PASS' if all(suite_must_pass.values()) else 'FAIL')
    )
    return {
        'suite': True,
        'schemaVersion': 1,
        'routeId': expected_route,
        'overall': overall,
        'logResults': results,
        'logSummaries': log_summaries,
        'highPressureEvidenceCount': high_pressure_coverage['passed'],
        'highPressureCoverage': high_pressure_coverage,
        'independentBudgetCoverage': independent_budget_coverage,
        'suiteRoles': log_roles,
        'suiteSignSwitchCoverage': coverage,
        'mustPass': suite_must_pass,
    }


def write_json(path: Path, result: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def ramp_increments(start: float, target: float, sample_count: int, phase_index: int) -> list[float]:
    total = target - start
    weights = [
        0.5 + ((index * (7 + phase_index * 5) + phase_index * 3) % 17) / 17
        for index in range(sample_count)
    ]
    scale = total / sum(weights)
    return [weight * scale for weight in weights]


def write_markdown(path: Path, result: dict[str, Any]) -> None:
    if result.get('suite'):
        lines = [
            "# P-011 objective suite check",
            "",
            f"- Route: `{result['routeId']}`",
            f"- Overall: **{result['overall']}**",
            f"- Pressure total clip runs: {result['highPressureCoverage']['totalMotionClipRuns']}",
            f"- Positive to negative switches: {result['suiteSignSwitchCoverage']['positiveToNegativeSwitches']}",
            f"- Negative to positive switches: {result['suiteSignSwitchCoverage']['negativeToPositiveSwitches']}",
            "",
            "## Log summaries",
            "",
            "| Log | Per-log gate | Positive to negative | Negative to positive | Reset count | Target phases |",
            "|---|---|---:|---:|---:|---:|",
        ]
        lines.extend(
            (
                f"| {summary['logIndex']} | `{'PASS' if summary['perLogGatePass'] else 'FAIL'}` | "
                f"{summary['bottomDirectionPositiveToNegativeSwitches']} | "
                f"{summary['bottomDirectionNegativeToPositiveSwitches']} | "
                f"{summary['identityObservability'].get('resetCountByRun', {})} | "
                f"{summary['identityObservability'].get('targetPhaseCount', 0)} |"
            )
            for summary in result['logSummaries']
        )
        lines.extend(["", "## Suite MUST-PASS", ""])
        lines.extend(
            f"- {name}: {'PASS' if passed else 'FAIL'}"
            for name, passed in result['mustPass'].items()
        )
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        return

    lines = [
        "# P-011 objective check",
        "",
        f"- Route: `{result['routeId']}`",
        f"- Overall: **{result['overall']}**",
        "",
        "| Case | Measured | Score |",
        "|---|---|---:|",
    ]
    for key in ("tcP01101", "tcP01102", "tcP01103", "tcP01104"):
        lines.append(f"| {key} | `{json.dumps(result[key], ensure_ascii=False, sort_keys=True)}` | {result[key].get('score', '-')} |")
    lines.extend([
        "",
        "## High-pressure total-clip segments",
        "",
        "Independent segments require `totalMotionBudgetClipped > 0` bounded by `totalMotionBudgetClipped = 0` or separate command phases.",
        "",
    ])
    segments = result.get("budgetAcceptance", {}).get("totalMotionClipSegments", [])
    lines.append(f"- Segment count: {len(segments)}")
    lines.extend(
        f"- {index + 1}: `{json.dumps(segment, ensure_ascii=False, sort_keys=True)}`"
        for index, segment in enumerate(segments)
    )
    lines.extend(["", "## MUST-PASS", ""])
    lines.extend(f"- {name}: {'PASS' if passed else 'FAIL'}" for name, passed in result["mustPass"].items())
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path)
    parser.add_argument("--route-id", default=DEFAULT_ROUTE)
    parser.add_argument("--json", type=Path, default=Path("test/artifacts/p011_check_v2.json"))
    parser.add_argument("--markdown", type=Path, default=Path("docs/test-report-P011-bottom-creep-v2.7-20261016.md"))
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument('--suite-input', action='append', type=Path, default=[])
    parser.add_argument('--suite-role', action='append', default=[])
    return parser.parse_args()


def suite_role_for_path(path: Path, explicit_roles: list[str], index: int) -> str:
    if index < len(explicit_roles):
        return explicit_roles[index]
    formal_roles = {
        'p011-tc02-formal.log': BEHAVIOR_ROLE,
        'p011-full-load.log': f'{BUDGET_ISOLATION_ROLE}+{PRESSURE_CLIP_ROLE}',
        'p011-clip-segment-a-positive.log': PRESSURE_CLIP_ROLE,
        'p011-clip-segment-b-negative.log': PRESSURE_CLIP_ROLE,
        'p011-clip-segment-c-positive-repeat.log': PRESSURE_CLIP_ROLE,
    }
    role = formal_roles.get(path.name)
    if role is None:
        raise ValueError(f'unknown formal suite log role: {path}')
    return role


def main() -> int:
    args = parse_args()
    if args.self_check:
        args.json = Path("test/artifacts/p011_selfcheck_v2.json")
        args.markdown = Path("test/artifacts/p011_selfcheck_v2.md")
        log_text = self_check_log()
    else:
        if args.suite_input:
            suite_texts = [
                path.read_text(encoding='utf-8', errors='replace')
                for path in args.suite_input
            ]
            suite_roles = [
                suite_role_for_path(path, args.suite_role, index)
                for index, path in enumerate(args.suite_input)
            ]
            result = check_suite(suite_texts, args.route_id, log_roles=suite_roles)
            write_json(args.json, result)
            write_markdown(args.markdown, result)
            print('overall: ' + result['overall'])
            print(f'JSON: {args.json}')
            print(f'Markdown: {args.markdown}')
            return 0 if result['overall'] == 'PASS' else 1
        if args.input is None:
            print("--input is required unless --self-check is used", file=sys.stderr)
            return 2
        log_text = sys.stdin.read() if str(args.input) == "-" else args.input.read_text(encoding="utf-8", errors="replace")
    if args.self_check:
        result = check_suite(
            [log_text],
            args.route_id,
            include_regressions=True,
            expected_log_count=1,
            pressure_log_count=0,
            log_roles=[BEHAVIOR_ROLE],
        )
    else:
        result = check(log_text, args.route_id, include_regressions=args.self_check)
    write_json(args.json, result)
    write_markdown(args.markdown, result)
    print(f"overall: {result['overall']}")
    print(f"JSON: {args.json}")
    print(f"Markdown: {args.markdown}")
    if args.self_check:
        return 0 if result["overall"] == "PASS" else 1
    return 0 if result["overall"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

