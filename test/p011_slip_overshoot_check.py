#!/usr/bin/env python3
import argparse
import hashlib
import json
import math
from pathlib import Path
from typing import Any

import matplotlib.pyplot as plt
from PIL import Image

from p011_v210_natural_observation_check import (
    load_json,
    parse_hilog,
    raster,
    validate,
    validate_observation_contract,
)


ROUTE = 'natural-observation-v2.10'
EXPERIMENT = 'slip-overshoot-v1'
DEVICE = '62T0225B10010020'
PHASES = [
    'baseline',
    'positive',
    'positiveReturn',
    'positiveOvershoot',
    'positiveOvershootReturn',
    'negativeOvershoot',
    'negativeOvershootReturn',
]
EXPECTED_PHASES = {
    'baseline': (1, 0.0),
    'positive': (2, 15.0),
    'positiveReturn': (3, 0.0),
    'positiveOvershoot': (4, 45.0),
    'positiveOvershootReturn': (5, 0.0),
    'negativeOvershoot': (6, -45.0),
    'negativeOvershootReturn': (7, 0.0),
}
OVERSHOOT_PHASES = ['positiveOvershoot', 'negativeOvershoot']
CONTROL_PHASES = ['positive']


def surface(path: Path, layout: dict[str, Any]) -> list[float] | None:
    with Image.open(path) as image:
        rgb = image.convert('RGB')
        width, height = rgb.size
        pixels = rgb.load()
    scale_x = width / float(layout['canvasWidth'])
    scale_y = height / float(layout['canvasHeight'])
    box_x = int(float(layout['boxX']) * scale_x)
    box_y = int(float(layout['boxY']) * scale_y)
    box_width = int(float(layout['boxWidth']) * scale_x)
    box_height = int(float(layout['boxHeight']) * scale_y)
    cols = int(layout['cols'])
    capacity = float(layout['capacity'])
    result: list[float] = []
    for col in range(cols):
        center_x = box_x + int((col + 0.5) * box_width / cols)
        sample_xs = [x for x in (center_x - 1, center_x, center_x + 1) if 0 <= x < width]
        found = None
        for y in range(box_y + 8, box_y + box_height - 8):
            def is_surface(pixel: tuple[int, int, int]) -> bool:
                red, green, blue = pixel
                luminance = (red + green + blue) / 3
                return luminance > 35 and not (blue > red and blue > green)

            if not any(is_surface(pixels[x, y]) for x in sample_xs):
                continue
            if all(
                any(is_surface(pixels[x, yy]) for x in sample_xs)
                for yy in range(y, min(y + 3, box_y + box_height))
            ):
                found = y
                break
        if found is None:
            return None
        result.append(capacity - (found + 0.5 - box_y) * capacity / box_height)
    return result


def signed_slope(heights: list[float]) -> float:
    count = len(heights)
    if count < 2:
        return 0.0
    mean_x = (count - 1) / 2
    mean_y = sum(heights) / count
    numerator = sum((index - mean_x) * (heights[index] - mean_y) for index in range(count))
    denominator = sum((index - mean_x) ** 2 for index in range(count))
    return numerator / denominator if denominator else 0.0


def slope_degrees(heights: list[float]) -> tuple[float, float]:
    linear = math.degrees(math.atan(abs(signed_slope(heights))))
    count = len(heights)
    window = 5
    half = window // 2
    smoothed = [
        sum(heights[max(0, index - half):min(count, index + half + 1)]) /
        len(heights[max(0, index - half):min(count, index + half + 1)])
        for index in range(count)
    ]
    differences = [smoothed[index + 1] - smoothed[index] for index in range(count - 1)]
    smoothed_max = max((abs(value) for value in differences), default=0.0)
    return linear, math.degrees(math.atan(smoothed_max))


def direction_name(value: float, tolerance: float = 1e-9) -> str:
    if value > tolerance:
        return 'right/up'
    if value < -tolerance:
        return 'left/down'
    return 'none'


def centroid(values: list[float]) -> float:
    total = sum(values)
    if abs(total) <= 1e-12:
        return 0.0
    return sum(index * value for index, value in enumerate(values)) / total


def heavy_delta_summary(
    initial: list[float],
    final: list[float],
    pixels_per_model_unit: float,
) -> dict[str, Any]:
    deltas = [final[index] - initial[index] for index in range(len(initial))]
    largest_index = max(range(len(deltas)), key=lambda index: abs(deltas[index]))
    largest_delta = deltas[largest_index]
    centroid_shift = centroid(final) - centroid(initial)
    return {
        'maxDeltaModelUnits': max(abs(value) for value in deltas),
        'maxDeltaPx': max(abs(value) for value in deltas) * pixels_per_model_unit,
        'largestSignedDeltaModelUnits': largest_delta,
        'largestSignedDeltaPx': largest_delta * pixels_per_model_unit,
        'largestDeltaColumn': largest_index,
        'meanSignedDeltaModelUnits': sum(deltas) / len(deltas),
        'meanSignedDeltaPx': sum(deltas) * pixels_per_model_unit / len(deltas),
        'centroidShiftColumns': centroid_shift,
        'direction': direction_name(centroid_shift),
    }


def first_reach(samples: list[tuple[float, float]], fraction: float) -> float | None:
    if not samples:
        return None
    peak = max(abs(value) for _, value in samples)
    if peak <= 1e-12:
        return None
    threshold = fraction * peak
    return next((time_ms for time_ms, value in samples if abs(value) >= threshold), None)


def first_slope_reach(
    samples: list[tuple[float, float, float, float]],
    initial: float,
    final: float,
    fraction: float,
) -> float | None:
    change = final - initial
    if abs(change) <= 1e-12 or not samples:
        return None
    sign = 1.0 if change > 0 else -1.0
    threshold = initial + sign * fraction * abs(change)
    return next(
        (time_ms for time_ms, _, _, signed in samples if sign * (signed - threshold) >= 0),
        None,
    )


def first_target_slope_reach(
    samples: list[tuple[float, float, float, float]],
    target_degrees: float,
) -> float | None:
    threshold = math.radians(target_degrees)
    return next(
        (time_ms for time_ms, linear, _, _ in samples if abs(math.radians(linear)) >= threshold),
        None,
    )


def phase_records(records: list[dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    grouped = {phase: [] for phase in PHASES}
    for record in records:
        phase = record['context']['phaseD']
        if phase in grouped:
            grouped[phase].append(record)
    return grouped


def edge_totals(records: list[dict[str, Any]], channel: str) -> dict[str, float]:
    total = {
        'allowed': 0.0,
        'heavy': 0.0,
        'light': 0.0,
        'heavyRightMinusLeft': 0.0,
        'lightRightMinusLeft': 0.0,
    }
    for record_index, record in enumerate(records):
        for grant in record[channel]:
            total['allowed'] += float(grant['allowed'])
            total['heavy'] += float(grant['heavy'])
            total['light'] += float(grant['light'])
            signed = float(grant['directionSign'])
            total['heavyRightMinusLeft'] += signed * float(grant['heavy'])
            total['lightRightMinusLeft'] += signed * float(grant['light'])
    return total


def ledger_phase_summary(
    records: list[dict[str, Any]],
    pixels_per_model_unit: float,
) -> dict[str, Any]:
    grouped = phase_records(records)
    summaries: dict[str, Any] = {}
    for phase, phase_list in grouped.items():
        if not phase_list:
            summaries[phase] = None
            continue
        first = phase_list[0]
        last = phase_list[-1]
        initial_surface = first['stateBefore']['S']
        final_surface = last['stateAfter']['S']
        initial_heavy = first['stateBefore']['bh']
        final_heavy = last['stateAfter']['bh']
        initial_slope = slope_degrees(initial_surface)
        final_slope = slope_degrees(final_surface)
        initial_signed = math.degrees(math.atan(signed_slope(initial_surface)))
        final_signed = math.degrees(math.atan(signed_slope(final_surface)))
        target_tilt = float(first['context']['targetTilt'])
        duration_ms = float(last['context']['physicsTime'] - first['context']['physicsTime'])
        duration_s = duration_ms / 1000.0
        overshoot_deg = max(0.0, abs(target_tilt) - 30.0)
        slope_change = final_signed - initial_signed
        slope_change_rate = abs(slope_change) / duration_s if duration_s > 0 else None
        surface_rate_per_overshoot = (
            slope_change_rate / overshoot_deg
            if overshoot_deg > 0 and slope_change_rate is not None
            else None
        )
        relax_requested = sum(float(record['audit']['relaxRequestedTotal']) for record in phase_list)
        relax_applied = sum(float(record['audit']['relaxAppliedTotal']) for record in phase_list)
        slip_rate = relax_applied / duration_s if duration_s > 0 else None
        slip_rate_per_overshoot = (
            slip_rate / overshoot_deg
            if overshoot_deg > 0 and slip_rate is not None
            else None
        )
        settle_heavy = sum(float(value) for record in phase_list for value in record['settle']['heavy'])
        settle_light = sum(float(value) for record in phase_list for value in record['settle']['light'])
        percolate_heavy = sum(float(value) for record in phase_list for value in record['percolate']['heavyDown'])
        percolate_light = sum(float(value) for record in phase_list for value in record['percolate']['lightDown'])
        lift_heavy = sum(float(value) for record in phase_list for value in record['lift']['heavy'])
        lift_light = sum(float(value) for record in phase_list for value in record['lift']['light'])
        slip = edge_totals(phase_list, 'slip')
        bottom = edge_totals(phase_list, 'bottomCreep')
        suspension = edge_totals(phase_list, 'suspensionDiffusion')
        effective_tilts = [float(record['context']['effectiveTilt']) for record in phase_list]
        shake_values = [float(record['context']['shakeEnergy']) for record in phase_list]
        budgets = [record['budgets'] for record in phase_list]
        summaries[phase] = {
            'recordCount': len(phase_list),
            'targetTilt': target_tilt,
            'durationMs': duration_ms,
            'overshootDeg': overshoot_deg,
            'initialSurfaceSlopeSignedDeg': initial_signed,
            'finalSurfaceSlopeSignedDeg': final_signed,
            'initialSurfaceSlopeLinearDeg': initial_slope[0],
            'finalSurfaceSlopeLinearDeg': final_slope[0],
            'initialSurfaceSlopeSmoothedMaxDeg': initial_slope[1],
            'finalSurfaceSlopeSmoothedMaxDeg': final_slope[1],
            'surfaceSlopeChangeSignedDeg': slope_change,
            'surfaceSlopeChangeRateDegPerSec': slope_change_rate,
            'surfaceSlopeRatePerOvershootDeg': surface_rate_per_overshoot,
            'slipRequested': relax_requested,
            'slipApplied': relax_applied,
            'slipAppliedRateModelUnitsPerSec': slip_rate,
            'slipAppliedRatePerOvershootDeg': slip_rate_per_overshoot,
            'relaxTriggeredCount': sum(record['audit']['relaxTriggered'] for record in phase_list),
            'relaxLimitedCount': sum(
                float(record['audit']['relaxRequestedTotal']) >
                float(record['audit']['relaxAppliedTotal']) + 1e-9
                for record in phase_list
            ),
            'effectiveTiltMinDeg': min(effective_tilts),
            'effectiveTiltMaxDeg': max(effective_tilts),
            'effectiveTiltMeanDeg': sum(effective_tilts) / len(effective_tilts),
            'shakeEnergyMean': sum(shake_values) / len(shake_values),
            'budget': {
                'bottomRequested': sum(float(value['bottomRequested']) for value in budgets),
                'bottomUsed': sum(float(value['bottomUsed']) for value in budgets),
                'bottomLimit': sum(float(value['bottomLimit']) for value in budgets),
                'totalRequested': sum(float(value['totalRequested']) for value in budgets),
                'totalUsed': sum(float(value['totalUsed']) for value in budgets),
                'totalLimit': sum(float(value['totalLimit']) for value in budgets),
            },
            'channels': {
                'settleHeavy': settle_heavy,
                'settleLight': settle_light,
                'percolateHeavy': percolate_heavy,
                'percolateLight': percolate_light,
                'liftHeavy': lift_heavy,
                'liftLight': lift_light,
                'slipHeavy': slip['heavy'],
                'slipLight': slip['light'],
                'slipHeavyRightMinusLeft': slip['heavyRightMinusLeft'],
                'slipLightRightMinusLeft': slip['lightRightMinusLeft'],
                'bottomHeavy': bottom['heavy'],
                'bottomLight': bottom['light'],
                'bottomHeavyRightMinusLeft': bottom['heavyRightMinusLeft'],
                'bottomLightRightMinusLeft': bottom['lightRightMinusLeft'],
                'suspensionHeavy': suspension['heavy'],
                'suspensionLight': suspension['light'],
                'suspensionHeavyRightMinusLeft': suspension['heavyRightMinusLeft'],
                'suspensionLightRightMinusLeft': suspension['lightRightMinusLeft'],
            },
            'heavyInterface': heavy_delta_summary(initial_heavy, final_heavy, pixels_per_model_unit),
        }
    return summaries


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as file:
        for block in iter(lambda: file.read(1048576), b''):
            digest.update(block)
    return digest.hexdigest()


def raster_phase_summary(
    manifest_path: Path,
    layout: dict[str, Any],
    pixels_per_model_unit: float,
) -> tuple[dict[str, Any], dict[str, list[tuple[float, float, float, float]]], list[str]]:
    manifest = load_json(manifest_path)
    entries = manifest.get('captures', [])
    base_dir = manifest_path.parent
    grouped: dict[str, list[tuple[float, list[float], list[float], str]]] = {
        phase: [] for phase in PHASES
    }
    errors: list[str] = []
    for entry in entries:
        phase = entry.get('phase')
        if phase not in grouped:
            continue
        path = base_dir / str(entry['file'])
        if not path.is_file():
            errors.append('raster missing:' + str(entry['file']))
            continue
        if file_sha256(path) != entry.get('sha256'):
            errors.append('raster hash mismatch:' + str(entry['file']))
        surface_heights = surface(path, layout)
        heavy_result = raster(path, layout)
        if surface_heights is None or heavy_result is None:
            errors.append('surface or heavy boundary missing:' + str(entry['file']))
            continue
        grouped[phase].append((
            float(entry.get('relativePhaseTimeMs', 0)),
            surface_heights,
            heavy_result[0],
            str(entry['file']),
        ))
    summaries: dict[str, Any] = {}
    series: dict[str, list[tuple[float, float, float, float]]] = {}
    for phase, samples in grouped.items():
        if not samples:
            summaries[phase] = None
            series[phase] = []
            continue
        initial_surface = samples[0][1]
        initial_heavy = samples[0][2]
        initial_signed = math.degrees(math.atan(signed_slope(initial_surface)))
        final_surface = samples[-1][1]
        final_heavy = samples[-1][2]
        final_signed = math.degrees(math.atan(signed_slope(final_surface)))
        initial_slope = slope_degrees(initial_surface)
        final_slope = slope_degrees(final_surface)
        duration_s = (samples[-1][0] - samples[0][0]) / 1000.0
        slope_samples: list[tuple[float, float, float, float]] = []
        heavy_samples: list[tuple[float, float]] = []
        for time_ms, surface_heights, heavy_heights, _ in samples:
            slopes = slope_degrees(surface_heights)
            signed = math.degrees(math.atan(signed_slope(surface_heights)))
            heavy_delta = max(
                abs(heavy_heights[index] - initial_heavy[index])
                for index in range(len(initial_heavy))
            ) * pixels_per_model_unit
            slope_samples.append((time_ms, slopes[0], heavy_delta, signed))
            heavy_samples.append((time_ms, heavy_delta))
        slope_change = final_signed - initial_signed
        slope_rate = abs(slope_change) / duration_s if duration_s > 0 else None
        summaries[phase] = {
            'captureCount': len(samples),
            'firstCaptureRelativeMs': samples[0][0],
            'lastCaptureRelativeMs': samples[-1][0],
            'initialSurfaceSlopeSignedDeg': initial_signed,
            'finalSurfaceSlopeSignedDeg': final_signed,
            'initialSurfaceSlopeLinearDeg': initial_slope[0],
            'finalSurfaceSlopeLinearDeg': final_slope[0],
            'initialSurfaceSlopeSmoothedMaxDeg': initial_slope[1],
            'finalSurfaceSlopeSmoothedMaxDeg': final_slope[1],
            'surfaceSlopeChangeSignedDeg': slope_change,
            'surfaceSlopeChangeRateDegPerSec': slope_rate,
            'slope50TimeMs': first_slope_reach(slope_samples, initial_signed, final_signed, 0.5),
            'slope90TimeMs': first_target_slope_reach(slope_samples, 27.0),
            'slope90RelativeFinalTimeMs': first_slope_reach(
                slope_samples, initial_signed, final_signed, 0.9
            ),
            'heavyInterface': heavy_delta_summary(initial_heavy, final_heavy, pixels_per_model_unit),
            'heavy50TimeMs': first_reach(heavy_samples, 0.5),
            'heavy90TimeMs': first_reach(heavy_samples, 0.9),
            'peakHeavyTimeMs': max(heavy_samples, key=lambda sample: sample[1])[0],
        }
        series[phase] = slope_samples
    return summaries, series, errors


def overshoot_residuals(
    records: list[dict[str, Any]],
    pixels_per_model_unit: float,
) -> dict[str, Any]:
    grouped = phase_records(records)
    result: dict[str, Any] = {}
    references = {
        'positiveOvershoot': ('positiveReturn', 'positiveOvershootReturn'),
        'negativeOvershoot': ('positiveOvershootReturn', 'negativeOvershootReturn'),
    }
    for phase, (before_name, after_name) in references.items():
        before = grouped[before_name]
        after = grouped[after_name]
        if not before or not after:
            result[phase] = None
            continue
        before_last = before[-1]['stateAfter']
        after_last = after[-1]['stateAfter']
        result[phase] = {
            'beforePhase': before_name,
            'returnPhase': after_name,
            'elapsedMs': float(
                after[-1]['context']['physicsTime'] - before[-1]['context']['physicsTime']
            ),
            'surfaceSlopeBeforeDeg': math.degrees(math.atan(signed_slope(before_last['S']))),
            'surfaceSlopeAfterDeg': math.degrees(math.atan(signed_slope(after_last['S']))),
            'heavyInterface': heavy_delta_summary(
                before_last['bh'], after_last['bh'], pixels_per_model_unit
            ),
        }
    return result


def make_chart(
    output_path: Path,
    raster_series: dict[str, list[tuple[float, float, float, float]]],
) -> None:
    figure, axes = plt.subplots(2, 1, figsize=(12, 8))
    colors = {
        'baseline': '#666666',
        'positive': '#d62728',
        'positiveReturn': '#ff9896',
        'positiveOvershoot': '#9467bd',
        'positiveOvershootReturn': '#c5b0d5',
        'negativeOvershoot': '#1f77b4',
        'negativeOvershootReturn': '#aec7e8',
    }
    for phase, samples in raster_series.items():
        if not samples:
            continue
        times = [sample[0] for sample in samples]
        slopes = [sample[1] for sample in samples]
        axes[0].plot(times, slopes, marker='o', markersize=3, linewidth=1.5,
                     label=phase, color=colors[phase])
    axes[0].axhline(30, color='black', linestyle='--', linewidth=1, label='30 deg rest angle')
    axes[0].set_title('Visible surface slope from raster')
    axes[0].set_ylabel('linear slope (deg)')
    axes[0].grid(True, alpha=0.25)
    axes[0].legend(ncol=2, fontsize=8)
    for phase, samples in raster_series.items():
        if not samples:
            continue
        times = [sample[0] for sample in samples]
        amplitudes = [sample[2] for sample in samples]
        axes[1].plot(times, amplitudes, marker='o', markersize=3, linewidth=1.5,
                     label=phase, color=colors[phase])
    axes[1].axhline(2, color='black', linestyle='--', linewidth=1, label='2 px visible threshold')
    axes[1].set_title('Heavy interface movement from raster')
    axes[1].set_xlabel('relative phase time (ms)')
    axes[1].set_ylabel('max |delta| (px)')
    axes[1].grid(True, alpha=0.25)
    axes[1].legend(ncol=2, fontsize=8)
    figure.tight_layout()
    figure.savefig(output_path, dpi=160)
    plt.close(figure)


def validate_phase_contract(records: list[dict[str, Any]]) -> list[str]:
    errors: list[str] = []
    grouped = phase_records(records)
    for phase, (sequence, target) in EXPECTED_PHASES.items():
        phase_list = grouped[phase]
        if not phase_list:
            continue
        if any(record['context']['commandSequence'] != sequence for record in phase_list):
            errors.append(phase + ':commandSequence')
        if any(abs(float(record['context']['targetTilt']) - target) > 1e-6 for record in phase_list):
            errors.append(phase + ':targetTilt')
    return errors


def audit_failure_summary(records: list[dict[str, Any]]) -> dict[str, Any]:
    fields = ['finite', 'range', 'capacity', 'stock', 'conservation']
    counts = {field: 0 for field in fields}
    phase_counts: dict[str, int] = {}
    first_range = None
    budget_excess = 0
    for record_index, record in enumerate(records):
        audit = record['audit']
        for field in fields:
            if audit[field] is not True:
                counts[field] += 1
        if max(
            float(audit['bottomBudgetExcess']),
            float(audit['totalBudgetExcess']),
            float(audit['maxLocalBudgetExcess']),
        ) > 1e-9:
            budget_excess += 1
        if audit['range'] is not True:
            phase = record['context']['phaseD']
            phase_counts[phase] = phase_counts.get(phase, 0) + 1
            if first_range is None:
                first_range = {
                    'recordIndex': record_index,
                    'sequence': record['context']['sequence'],
                    'phase': phase,
                    'targetTiltDeg': record['context']['targetTilt'],
                    'contextEffectiveTiltDeg': record['context']['effectiveTilt'],
                    'maxSandTopInvariantError': audit['maxSandTopInvariantError'],
                    'formalRangeLimitTan30': math.tan(math.radians(30.0)),
                }
    return {
        'fieldFailureCounts': counts,
        'rangeFailurePhaseCounts': phase_counts,
        'firstRangeViolation': first_range,
        'budgetExcessSubsteps': budget_excess,
    }


def analyze(args: argparse.Namespace) -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    with args.ledger.open('r', encoding='utf-8') as file:
        for line in file:
            if line.strip():
                records.append(json.loads(line))
    errors: list[str] = []
    if not records:
        errors.append('ledger empty')
        cols = 128
    else:
        cols = len(records[0]['stateAfter']['bh'])
    errors.extend(validate(records, cols))
    log = parse_hilog(args.hilog)
    errors.extend(log['errors'])
    if not log['store']:
        errors.append('P011_OBS_STORE missing')
    if not log['layout']:
        errors.append('P011_RASTER_LAYOUT missing')
    if log['metric'] < len(records):
        errors.append('heartbeat count below ledger')
    if len({record['context']['runId'] for record in records}) != 1:
        errors.append('multiple runIds')
    phases = {record['context']['phaseD'] for record in records}
    phase_complete = all(phase in phases for phase in PHASES)
    if not phase_complete:
        errors.append('phase sequence incomplete')
    command_sequences = sorted({record['context']['commandSequence'] for record in records})
    if command_sequences != list(range(1, 8)):
        errors.append('command sequences invalid')
    errors.extend(validate_phase_contract(records))
    audit_failures = audit_failure_summary(records)
    manifest = load_json(args.raster_manifest)
    errors.extend(validate_observation_contract(records, cols, manifest))
    layout = manifest.get('layout', log['layout'])
    entries = manifest.get('captures', [])
    capture_height = (
        float(entries[0].get('height', layout['canvasHeight']))
        if entries else float(layout['canvasHeight'])
    )
    pixels_per_model_unit = (
        float(layout.get('pixelsPerModelUnitY', 1)) *
        capture_height / float(layout['canvasHeight'])
    )
    ledger_summary = ledger_phase_summary(records, pixels_per_model_unit)
    raster_summary, raster_series, raster_errors = raster_phase_summary(
        args.raster_manifest, layout, pixels_per_model_unit
    )
    errors.extend(raster_errors)
    residuals = overshoot_residuals(records, pixels_per_model_unit)
    slip_triggered = {
        phase: bool(
            ledger_summary[phase] and
            (ledger_summary[phase]['relaxTriggeredCount'] > 0 or
             ledger_summary[phase]['slipApplied'] > 0)
        )
        for phase in OVERSHOOT_PHASES
    }
    final_slopes = {
        phase: (
            raster_summary[phase]['finalSurfaceSlopeLinearDeg']
            if raster_summary[phase] else None
        )
        for phase in OVERSHOOT_PHASES
    }
    converged_to_30 = {
        phase: value is not None and abs(value - 30.0) <= 2.0
        for phase, value in final_slopes.items()
    }
    heavy_amplitudes = {
        phase: (
            raster_summary[phase]['heavyInterface']['maxDeltaPx']
            if raster_summary[phase] else None
        )
        for phase in OVERSHOOT_PHASES
    }
    heavy_interface_moved = {
        phase: value is not None and value >= 2.0
        for phase, value in heavy_amplitudes.items()
    }
    surface_rate_per_overshoot = {
        phase: (
            ledger_summary[phase]['surfaceSlopeRatePerOvershootDeg']
            if ledger_summary[phase] else None
        )
        for phase in OVERSHOOT_PHASES
    }
    slip_rate_per_overshoot = {
        phase: (
            ledger_summary[phase]['slipAppliedRatePerOvershootDeg']
            if ledger_summary[phase] else None
        )
        for phase in OVERSHOOT_PHASES
    }
    control_rates = {
        phase: {
            'surfaceSlopeChangeRateDegPerSec': (
                ledger_summary[phase]['surfaceSlopeChangeRateDegPerSec']
                if ledger_summary[phase] else None
            ),
            'slipAppliedRateModelUnitsPerSec': (
                ledger_summary[phase]['slipAppliedRateModelUnitsPerSec']
                if ledger_summary[phase] else None
            ),
        }
        for phase in CONTROL_PHASES
    }
    audit_ok = not any('audit.' in error or error.endswith(':errorCode') for error in errors)
    non_audit_errors = [
        error for error in errors
        if not error.startswith('record[') or ':audit.' not in error
    ]
    raster_valid = all(raster_summary[phase] is not None for phase in PHASES)
    status = 'invalid'
    hard_range_violation = audit_failures['fieldFailureCounts']['range'] > 0
    if not non_audit_errors and phase_complete and raster_valid:
        if hard_range_violation and any(slip_triggered.values()):
            status = 'overshootSlipTriggeredWithHardRangeViolation'
        elif hard_range_violation:
            status = 'overshootSlipNotTriggeredWithHardRangeViolation'
        elif all(slip_triggered.values()) and all(converged_to_30.values()):
            status = 'overshootSlipConvergedTo30'
        elif any(slip_triggered.values()):
            status = 'overshootSlipTriggeredWithout30Convergence'
        else:
            status = 'overshootSlipNotTriggered'
    source_manifest = None
    if args.source_manifest is not None and args.source_manifest.is_file():
        source_manifest = load_json(args.source_manifest)
    artifact_root = args.output.parent.as_posix()
    first_context = records[0]['context'] if records else {}
    return {
        'schemaVersion': 1,
        'observationRouteId': first_context.get('observationRouteId', ROUTE),
        'observationRunRole': first_context.get('runRole'),
        'thresholdCalibrationId': first_context.get('thresholdCalibrationId'),
        'thetaStartDeg': first_context.get('thetaStartDeg'),
        'thetaStopDeg': first_context.get('thetaStopDeg'),
        'transitionTelemetryAvailable': first_context.get('transitionTelemetryAvailable', False),
        'experimentId': EXPERIMENT,
        'device': args.device,
        'runId': log['store'].get('runId') if log['store'] else None,
        'status': status,
        'phaseComplete': phase_complete,
        'auditPassed': audit_ok,
        'hardRangeViolation': hard_range_violation,
        'auditFailureSummary': audit_failures,
        'rasterValid': raster_valid,
        'answers': {
            'question1': {
                'slipTriggered': slip_triggered,
                'finalVisibleSurfaceSlopeLinearDeg': final_slopes,
                'convergedTo30Deg': converged_to_30,
            },
            'question2': {
                'heavyInterfaceMovedVisible': heavy_interface_moved,
                'heavyInterfaceMaxDeltaPx': heavy_amplitudes,
                'residuals': residuals,
            },
            'question3': {
                'surfaceSlopeRatePerOvershootDeg': surface_rate_per_overshoot,
                'slipAppliedRatePerOvershootDeg': slip_rate_per_overshoot,
                'controlRates': control_rates,
                'controlNote': 'The seven-phase schedule contains +15 only; it does not contain -15.',
            },
        },
        'ledgerPhaseSummary': ledger_summary,
        'rasterPhaseSummary': raster_summary,
        'rasterSeries': {
            phase: [
                {
                    'timeMs': sample[0],
                    'surfaceSlopeLinearDeg': sample[1],
                    'heavyInterfaceMaxDeltaPx': sample[2],
                    'surfaceSlopeSignedDeg': sample[3],
                }
                for sample in samples
            ]
            for phase, samples in raster_series.items()
        },
        'overshootResiduals': residuals,
        'evidence': {
            'artifactRoot': artifact_root,
            'ledger': artifact_root + '/ledger.ndjson',
            'hilog': artifact_root + '/hilog.txt',
            'rasterManifest': artifact_root + '/raster-manifest.json',
            'summary': artifact_root + '/summary.json',
            'chart': artifact_root + '/surface-slope.png',
            'sourceManifest': artifact_root + '/source-manifest.json',
        },
        'sourceManifest': source_manifest,
        'errors': errors,
        'recordCount': len(records),
        'heartbeatCount': log['metric'],
    }


def fmt(value: Any, digits: int = 2) -> str:
    if value is None:
        return '-'
    if isinstance(value, bool):
        return 'yes' if value else 'no'
    if isinstance(value, float):
        return f'{value:.{digits}f}'
    return str(value)


def phase_row(name: str, data: dict[str, Any] | None) -> str:
    if data is None:
        return '| ' + name + ' | - | - | - | - | - | - | - | - | - | - |'
    heavy = data['heavyInterface']
    return (
        '| ' + name + ' | ' + str(data['recordCount']) + ' | ' + fmt(data['targetTilt']) +
        ' | ' + fmt(data['overshootDeg']) +
        ' | ' + fmt(data['initialSurfaceSlopeSignedDeg']) +
        ' | ' + fmt(data['finalSurfaceSlopeSignedDeg']) +
        ' | ' + fmt(data['slipRequested']) +
        ' | ' + fmt(data['slipApplied']) +
        ' | ' + fmt(heavy['maxDeltaPx']) + ' px | ' + heavy['direction'] +
        ' | ' + str(data['relaxLimitedCount']) + ' |'
    )


def raster_row(name: str, data: dict[str, Any] | None) -> str:
    if data is None:
        return '| ' + name + ' | - | - | - | - | - | - | - | - | - | - |'
    heavy = data['heavyInterface']
    return (
        '| ' + name + ' | ' + str(data['captureCount']) +
        ' | ' + fmt(data['initialSurfaceSlopeLinearDeg']) +
        ' | ' + fmt(data['finalSurfaceSlopeLinearDeg']) +
        ' | ' + fmt(data['slope50TimeMs'], 0) +
        ' | ' + fmt(data['slope90TimeMs'], 0) +
        ' | ' + fmt(data['slope90RelativeFinalTimeMs'], 0) +
        ' | ' + fmt(data['heavy50TimeMs'], 0) +
        ' | ' + fmt(data['heavy90TimeMs'], 0) +
        ' | ' + fmt(data['peakHeavyTimeMs'], 0) +
        ' | ' + fmt(heavy['maxDeltaPx']) + ' px | ' + heavy['direction'] + ' |'
    )


def channel_row(name: str, data: dict[str, Any] | None) -> str:
    if data is None:
        return '| ' + name + ' | - | - | - | - | - | - | - | - | - | - | - |'
    channels = data['channels']
    values = [
        channels['settleHeavy'], channels['settleLight'],
        channels['percolateHeavy'], channels['percolateLight'],
        channels['liftHeavy'], channels['liftLight'],
        channels['slipHeavy'], channels['slipLight'],
        channels['bottomHeavy'], channels['bottomLight'],
        channels['suspensionHeavy'], channels['suspensionLight'],
    ]
    return '| ' + name + ' | ' + ' | '.join(fmt(value) for value in values) + ' |'


def budget_row(name: str, data: dict[str, Any] | None) -> str:
    if data is None:
        return '| ' + name + ' | - | - | - | - | - | - | - | - | - | - | - |'
    budget = data['budget']
    return (
        '| ' + name + ' | ' + str(data['relaxTriggeredCount']) + '/' + str(data['recordCount']) +
        ' | ' + str(data['relaxLimitedCount']) + '/' + str(data['recordCount']) +
        ' | ' + fmt(budget['totalRequested']) +
        ' | ' + fmt(budget['totalUsed']) +
        ' | ' + fmt(budget['totalLimit']) +
        ' | ' + fmt(budget['bottomRequested']) +
        ' | ' + fmt(budget['bottomUsed']) +
        ' | ' + fmt(budget['bottomLimit']) +
        ' | ' + fmt(data['effectiveTiltMinDeg']) + '/' + fmt(data['effectiveTiltMaxDeg']) +
        ' | ' + fmt(data['shakeEnergyMean'], 4) + ' |'
    )


def write_report(summary: dict[str, Any], report_path: Path, chart_path: Path) -> None:
    question1 = summary['answers']['question1']
    question2 = summary['answers']['question2']
    question3 = summary['answers']['question3']
    evidence = summary['evidence']
    source = summary.get('sourceManifest') or {}
    lines = [
        '# P-011 slip-overshoot-v1 观测报告',
        '',
        '## 运行身份',
        '',
        '- Run ID: `' + str(summary['runId']) + '`',
        '- 设备: `' + summary['device'] + '`',
        '- 场景: `128/192/28`, `wspreset=thick`, `dual-input-v1`',
        '- 相位: `0/+15/0/+45/0/-45/0`，每段 `5.1 s`',
        '- 时序口径: `5.1 s` 为 runner `holdMs`；沿用 v2.10 的命令派发与 raster 采集开销，实际 command interval 见 `schedule.csv`。',
        '- 观测路线: `' + summary['observationRouteId'] + '`，实验 ID: `' + summary['experimentId'] + '`',
        '- HAP SHA256: `' + str(source.get('hapSha256', '-')) + '`',
        '- 证据目录: `' + evidence['artifactRoot'] + '`',
        '',
        '## 三问答案',
        '',
        '1. **超角滑移触发与 30° 收敛**：`+45` 触发=' +
        fmt(question1['slipTriggered']['positiveOvershoot']) +
        '，最终可见线性坡度=' + fmt(question1['finalVisibleSurfaceSlopeLinearDeg']['positiveOvershoot']) +
        '°；`-45` 触发=' + fmt(question1['slipTriggered']['negativeOvershoot']) +
        '，最终可见线性坡度=' + fmt(question1['finalVisibleSurfaceSlopeLinearDeg']['negativeOvershoot']) +
        '°。按 `±2°` 判定，`+45/-45` 收敛到 30° 分别为 ' +
        fmt(question1['convergedTo30Deg']['positiveOvershoot']) + '/' +
        fmt(question1['convergedTo30Deg']['negativeOvershoot']) +
        '。同时，ledger 中有 `' +
        str(summary['auditFailureSummary']['fieldFailureCounts']['range']) +
        '` 个子步 `audit.range=false`，因此这不是一个满足正式硬门槛的 30° 安息构型。',
        '2. **重沙界面可见移动**：以 raster `>=2 px` 为可见阈值，`+45/-45` 分别为 ' +
        fmt(question2['heavyInterfaceMovedVisible']['positiveOvershoot']) + '/' +
        fmt(question2['heavyInterfaceMovedVisible']['negativeOvershoot']) +
        '；幅度分别为 ' + fmt(question2['heavyInterfaceMaxDeltaPx']['positiveOvershoot']) +
        ' px / ' + fmt(question2['heavyInterfaceMaxDeltaPx']['negativeOvershoot']) +
        ' px。方向、时间与回零残差见下表；ledger 的重沙 grant 同步列在通道表中。',
        '3. **每单位超角滑移速率**：`+45` 与 `-45` 的表面坡度速率分别为 ' +
        fmt(question3['surfaceSlopeRatePerOvershootDeg']['positiveOvershoot'], 4) +
        ' / ' + fmt(question3['surfaceSlopeRatePerOvershootDeg']['negativeOvershoot'], 4) +
        ' deg/(s·超角)，slip 体积速率分别为 ' +
        fmt(question3['slipAppliedRatePerOvershootDeg']['positiveOvershoot'], 4) +
        ' / ' + fmt(question3['slipAppliedRatePerOvershootDeg']['negativeOvershoot'], 4) +
        ' model units/(s·超角)。`+15` 对照无超角，坡度变化速率为 ' +
        fmt(question3['controlRates']['positive']['surfaceSlopeChangeRateDegPerSec'], 4) +
        ' deg/s，slip 体积速率为 ' +
        fmt(question3['controlRates']['positive']['slipAppliedRateModelUnitsPerSec'], 4) +
        ' model units/s。',
        '',
        '本七相协议实际只包含 `+15` 对照，不包含 `-15`；`-45` 是反向超角段，不是 `-15` 对照段。',
        '',
        '## 逐相 ledger 数据',
        '',
        '| 相位 | 记录 | 目标° | 超角° | 初始坡度° | 最终坡度° | slip请求 | slip应用 | 重沙最大位移 | 质心方向 | 预算受限子步 |',
        '|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|',
    ]
    for phase in PHASES:
        lines.append(phase_row(phase, summary['ledgerPhaseSummary'].get(phase)))
    lines.extend([
        '',
        '## raster 收敛曲线数据',
        '',
        '| 相位 | 采样 | 初始坡度° | 最终坡度° | 坡度50%ms | target27°ms | 旧终值90%ms | 重沙50%ms | 重沙90%ms | 重沙峰值ms | 重沙最大位移 | 重沙方向 |',
        '|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|',
    ])
    for phase in PHASES:
        lines.append(raster_row(phase, summary['rasterPhaseSummary'].get(phase)))
    lines.extend([
        '',
        '## 超角后回零残差',
        '',
        '| 超角相位 | 参考相位 | 回零相位 | 经过时间ms | 参考坡度° | 回零坡度° | 重沙最大残差 | 重沙质心方向 |',
        '|---|---|---|---:|---:|---:|---:|---|',
    ])
    for phase in OVERSHOOT_PHASES:
        residual = summary['overshootResiduals'].get(phase)
        if residual is None:
            lines.append('| ' + phase + ' | - | - | - | - | - | - | - |')
            continue
        heavy = residual['heavyInterface']
        lines.append(
            '| ' + phase + ' | ' + residual['beforePhase'] + ' | ' + residual['returnPhase'] +
            ' | ' + fmt(residual['elapsedMs'], 0) +
            ' | ' + fmt(residual['surfaceSlopeBeforeDeg']) +
            ' | ' + fmt(residual['surfaceSlopeAfterDeg']) +
            ' | ' + fmt(heavy['maxDeltaPx']) + ' px | ' + heavy['direction'] + ' |'
        )
    lines.extend([
        '',
        '## 硬门槛诊断',
        '',
    ])
    audit_failures = summary['auditFailureSummary']
    first_range = audit_failures['firstRangeViolation']
    if first_range is None:
        lines.append('- `audit.range` 违约子步：`0`。')
    else:
        lines.extend([
            '- `audit.range` 违约子步：`' + str(audit_failures['fieldFailureCounts']['range']) + '`。',
            '- 逐相位违约数：`' + json.dumps(
                audit_failures['rangeFailurePhaseCounts'], ensure_ascii=False, sort_keys=True
            ) + '`。',
            '- 首个违约：record `' + str(first_range['recordIndex']) +
            '` / sequence `' + str(first_range['sequence']) +
            '` / phase `' + first_range['phase'] +
            '`；context `effectiveTilt=' + fmt(first_range['contextEffectiveTiltDeg']) +
            '°`，`maxSandTopInvariantError=' + fmt(first_range['maxSandTopInvariantError'], 6) +
            '`，正式上限 `tan30=' + fmt(first_range['formalRangeLimitTan30'], 6) + '`。',
            '- 判定：观测流程完整，但超角运行未满足正式 `tan30°` 硬门槛；本次任务未修改任何物理或约束来掩盖该结果。',
        ])
    lines.extend([
        '',
        '## 实际通道 H/L grant',
        '',
        '| 相位 | settle H | settle L | percolate H | percolate L | lift H | lift L | slip H | slip L | bottom H | bottom L | suspension H | suspension L |',
        '|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|',
    ])
    for phase in PHASES:
        lines.append(channel_row(phase, summary['ledgerPhaseSummary'].get(phase)))
    lines.extend([
        '',
        '## 预算与驱动诊断',
        '',
        '| 相位 | relax触发 | relax受限 | total请求 | total应用 | total上限 | bottom请求 | bottom应用 | bottom上限 | effectiveTilt范围° | shake均值 |',
        '|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|',
    ])
    for phase in PHASES:
        lines.append(budget_row(phase, summary['ledgerPhaseSummary'].get(phase)))
    lines.extend([
        '',
        '## 图表',
        '',
        '![surface slope and heavy interface movement](' + evidence['chart'] + ')',
        '',
        '完整曲线数据: `' + evidence['summary'] + '`。',
        '',
        '## 证据',
        '',
        '- ledger: `' + evidence['ledger'] + '`',
        '- hilog: `' + evidence['hilog'] + '`',
        '- raster manifest: `' + evidence['rasterManifest'] + '`',
        '- source manifest: `' + evidence['sourceManifest'] + '`',
        '- summary: `' + evidence['summary'] + '`',
        '',
        '## 结论',
        '',
        '本次运行状态为 `' + summary['status'] + '`。观测代码仅记录状态与实际 grant，不改变物理、预算、几何、门槛或守恒约束；上述数值直接来自真机 ledger 与 raster。',
        '`auditPassed=' + fmt(summary['auditPassed']) + '`，`hardRangeViolation=' +
        fmt(summary['hardRangeViolation']) + '`；若硬门槛失败，本报告只作为观测证据，不作为验收通过证据。',
    ])
    report_path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--ledger', type=Path, required=True)
    parser.add_argument('--hilog', type=Path, required=True)
    parser.add_argument('--raster-manifest', type=Path, required=True)
    parser.add_argument('--source-manifest', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--chart', type=Path)
    parser.add_argument('--device', default=DEVICE)
    args = parser.parse_args()
    if args.source_manifest is None:
        candidate = args.output.parent / 'source-manifest.json'
        args.source_manifest = candidate if candidate.is_file() else None
    summary = analyze(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + '\n', encoding='utf-8'
    )
    chart_path = args.chart if args.chart is not None else args.output.with_name('surface-slope.png')
    manifest = load_json(args.raster_manifest)
    layout = manifest.get('layout', {})
    entries = manifest.get('captures', [])
    capture_height = (
        float(entries[0].get('height', layout.get('canvasHeight', 1)))
        if entries else float(layout.get('canvasHeight', 1))
    )
    pixels_per_model_unit = (
        float(layout.get('pixelsPerModelUnitY', 1)) *
        capture_height / float(layout.get('canvasHeight', 1))
    )
    _, raster_series, _ = raster_phase_summary(args.raster_manifest, layout, pixels_per_model_unit)
    make_chart(chart_path, raster_series)
    if args.report is not None:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        write_report(summary, args.report, chart_path)
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0 if summary['status'] != 'invalid' else 1


if __name__ == '__main__':
    raise SystemExit(main())
