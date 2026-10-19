import argparse
import json
import math
import re
from pathlib import Path

from PIL import Image


ROUTE_ID = 'bottom-creep-v2.8-heavy-interface'
METRIC_RE = re.compile(r'P011_METRIC\s+(\{.*\})')


def parse_metrics(log_path):
    metrics = []
    for line in log_path.read_text(encoding='utf-8', errors='replace').splitlines():
        match = METRIC_RE.search(line)
        if not match:
            continue
        try:
            metrics.append(json.loads(match.group(1)))
        except json.JSONDecodeError:
            continue
    return metrics


def metric_summary(metrics, sign, segment=None):
    phase = [
        metric for metric in metrics
        if metric.get('routeId') == ROUTE_ID
        and metric.get('commandPhaseSign') == sign
        and metric.get('physicsDt', 0) > 0
    ]
    if segment is not None:
        command_time = segment.get('commandTimeMs', 0)
        capture_time = segment.get('captureTimeMs', 0)
        phase = [
            metric for metric in phase
            if command_time <= metric.get('physicsTime', 0) <= capture_time
        ]
        if not phase and metrics:
            nearest = min(
                metrics,
                key=lambda metric: abs(metric.get('physicsTime', 0) - capture_time)
            )
            if nearest.get('commandPhaseSign') == sign:
                phase = [nearest]
    return {
        'heavyFluxNonZeroSamples': sum(metric.get('heavyFlux', 0) > 0 for metric in phase),
        'heavyShapeRms': max((metric.get('heavyShapeRms', 0) for metric in phase), default=0.0),
        'heavyShapeMaxDelta': max((metric.get('heavyShapeMaxDelta', 0) for metric in phase), default=0.0),
    }


def green_centroid(image_path):
    image = Image.open(image_path).convert('RGB')
    width, height = image.size
    left = math.floor(width * 0.10)
    right = width - left
    top = math.floor(height * 0.10)
    bottom = height - top
    pixels = image.load()
    x_sum = 0.0
    count = 0
    for y in range(top, bottom):
        for x in range(left, right):
            red, green, blue = pixels[x, y]
            if 20 <= red <= 80 and 65 <= green <= 115 and 15 <= blue <= 65 \
                    and green > red + 15 and green > blue + 15:
                x_sum += x
                count += 1
    return x_sum / count if count else None


def physical_gates(summary):
    """Existing TC-P011-V28-00 minima; ROI presence alone is insufficient."""
    thresholds = {
        'heavyFluxNonZeroSamples': 30,
        'heavyShapeRms': 0.5,
        'heavyShapeMaxDelta': 1.0,
    }
    return {
        field: isinstance(summary.get(field), (int, float))
        and math.isfinite(summary[field]) and summary[field] >= minimum
        for field, minimum in thresholds.items()
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--positive', type=Path, required=True)
    parser.add_argument('--negative', type=Path, required=True)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--output', type=Path,
                        default=Path('demo/effect/P011-v28-heavy-interface/preflight/preflight.json'))
    args = parser.parse_args()
    metrics = parse_metrics(args.log)
    segments = {}
    if args.manifest is not None:
        manifest = json.loads(args.manifest.read_text(encoding='utf-8-sig'))
        segments = {
            item.get('segment'): item
            for item in (manifest if isinstance(manifest, list) else [])
        }
    baseline = green_centroid(args.baseline)
    positive = green_centroid(args.positive)
    negative = green_centroid(args.negative)
    if baseline is None or positive is None or negative is None:
        status = 'BLOCKED(roiObservation)'
        positive_delta = 0.0
        negative_delta = 0.0
    else:
        status = 'OBSERVATION_READY'
        positive_delta = positive - baseline
        negative_delta = negative - baseline
    result = {
        'schemaVersion': 1,
        'routeId': ROUTE_ID,
        'status': status,
        'positive': {
            'greenCentroidDeltaPx': positive_delta,
            **metric_summary(metrics, 1, segments.get('positive')),
        },
        'negative': {
            'greenCentroidDeltaPx': negative_delta,
            **metric_summary(metrics, -1, segments.get('negative')),
        },
    }
    result['roiReady'] = status == 'OBSERVATION_READY'
    result['physicalGates'] = {
        direction: physical_gates(result[direction])
        for direction in ('positive', 'negative')
    }
    result['physicalGatePassed'] = all(
        passed for gates in result['physicalGates'].values() for passed in gates.values()
    )
    if result['roiReady'] and not result['physicalGatePassed']:
        result['status'] = 'FAIL(heavyObservation)'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))
    return 0 if result['roiReady'] and result['physicalGatePassed'] else 2


if __name__ == '__main__':
    raise SystemExit(main())
