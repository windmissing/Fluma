#!/usr/bin/env python3
"""Generate and audit the TC-P011-02 continuous linear tilt schedule."""

from __future__ import annotations

import argparse
import csv
from contextlib import contextmanager
import hashlib
import io
import json
import math
import re
import time
from pathlib import Path
from typing import Any


HOOK_RE = re.compile(r"P011_EVENT (input(?:_state)?)\s+(\{.*\})")
PHASES = (
    ("initial_hold", 3000.0, lambda progress: 0.0),
    ("zero_to_positive", 2000.0, lambda progress: 20.0 * progress),
    ("positive_hold", 1000.0, lambda progress: 20.0),
    ("positive_to_negative", 4000.0, lambda progress: 20.0 - 40.0 * progress),
    ("negative_hold", 1000.0, lambda progress: -20.0),
    ("negative_to_zero", 2000.0, lambda progress: -20.0 + 20.0 * progress),
)
ROUND_COUNT = 5

# P-011 v2.8: one canonical injection/capture identity for both runner modes.
SHELL_INTERVAL_SECONDS = 0.2
HILOG_PARAMETERS = {'reset': ['shell', 'hilog', '-r'],
                    'buffer': ['shell', 'hilog', '-G', '16M', '-t', 'app'],
                    'capture': ['shell', 'hilog']}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def canonical_capture(interval_ms=100.0, bundle='com.example.helloworld',
                      ability='EntryAbility', seed=20261016, target='62T0225B10010020'):
    for token in (bundle, ability, target):
        if not re.fullmatch(r'[\w.:-]+', token):
            raise ValueError('unsafe canonical command parameter')
    schedule = build_schedule(interval_ms)
    output = io.StringIO(newline='')
    writer = csv.DictWriter(output, fieldnames=('scheduledTimeMs', 'phase', 'step', 'targetTilt'))
    writer.writeheader()
    writer.writerows(schedule)
    template = (f'aa start -b {bundle} -a {ability} --ps rendermode 4 --ps testmode true '
                f'--ps forceinvert false --ps seed {seed} --ps wspreset thick '
                '--ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 '
                '--ps wssandtopmax 30 --ps wstilt {targetTilt}')
    lines = [f'aa force-stop {bundle}', 'sleep 1']
    for row in schedule:
        lines.extend([template.format(targetTilt=row['targetTilt']) + ' || exit 1',
                      f'sleep {SHELL_INTERVAL_SECONDS}'])
    lines.append(f'echo P011_INJECTION_COMPLETED:{len(schedule)}')
    script = ('\n'.join(lines) + '\n').encode('ascii')
    identity = {'version': 1, 'runner': 'test/p011_tc02_formal_suite.ps1',
                'bundle': bundle, 'ability': ability, 'seed': seed, 'target': target,
                'scheduleIntervalMs': float(interval_ms),
                'shellIntervalSeconds': SHELL_INTERVAL_SECONDS,
                'scheduleSha256': sha256(output.getvalue().encode('utf-8')),
                'injectionScriptSha256': sha256(script), 'injectionCommandTemplate': template,
                'hilogParameters': HILOG_PARAMETERS,
                'hilogParametersSha256': sha256(json.dumps(HILOG_PARAMETERS, sort_keys=True).encode())}
    identity['identitySha256'] = sha256(json.dumps(identity, sort_keys=True).encode())
    return identity, output.getvalue().encode('utf-8'), script


def prepare_capture(schedule_path: Path, script_path: Path, **parameters) -> dict[str, Any]:
    identity, schedule, script = canonical_capture(**parameters)
    for path, data in ((schedule_path, schedule), (script_path, script)):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    return identity


def capture_manifest_path(log_path: Path) -> Path:
    return log_path.with_suffix('.capture.json')


def file_snapshot(path: Path) -> dict[str, Any]:
    stat = path.stat()
    if not path.is_file():
        raise ValueError('captureNotFrozen: input must be a regular closed file')
    return {'size': stat.st_size, 'mtime': stat.st_mtime_ns,
            'sha256': sha256(path.read_bytes())}


@contextmanager
def closed_capture(path: Path):
    """On Windows, deny an existing or new writer while parsing frozen evidence."""
    import os
    if os.name != 'nt':
        with path.open('r', encoding='utf-8', errors='replace') as handle:
            yield handle
        return
    import ctypes
    import msvcrt
    from ctypes import wintypes
    create = ctypes.WinDLL('kernel32', use_last_error=True).CreateFileW
    create.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                       wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    create.restype = wintypes.HANDLE
    handle = create(str(path.resolve()), 0x80000000, 1, None, 3, 0, None)
    if handle == wintypes.HANDLE(-1).value:
        raise ValueError('captureNotFrozen: active writer or inaccessible capture')
    fd = msvcrt.open_osfhandle(handle, os.O_RDONLY)
    with os.fdopen(fd, 'r', encoding='utf-8', errors='replace') as stream:
        yield stream


def freeze_capture(log_path: Path, identity: dict[str, Any], context: dict[str, Any]):
    if (context.get('injectionCompleted') is not True or context.get('collectorStopped') is not True
            or context.get('residualCollectors') != 0 or not context.get('collectorPid')):
        raise ValueError('captureNotFrozen: incomplete injection or live/residual collector')
    with closed_capture(log_path):
        first = file_snapshot(log_path)
        start = time.monotonic()
        time.sleep(1.0)
        second = file_snapshot(log_path)
        elapsed = (time.monotonic() - start) * 1000
    if first != second:
        raise ValueError('captureNotFrozen: size/mtime/SHA256 changed during stability interval')
    audit = dict(context, logPath=str(log_path.resolve()), stabilityIntervalMs=elapsed)
    for prefix, snapshot in (('preParse', first), ('postParse', second)):
        for field in ('size', 'mtime', 'sha256'):
            audit[prefix + field[0].upper() + field[1:]] = snapshot[field]
    result = {'captureFrozen': True, 'capturePath': identity, 'captureAudit': audit}
    capture_manifest_path(log_path).write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def verify_capture(log_path: Path, interval_ms: float, expected_identity=None):
    manifest = json.loads(capture_manifest_path(log_path).read_text(encoding='utf-8-sig'))
    identity = manifest['capturePath']
    canonical, _, _ = canonical_capture(interval_ms, identity['bundle'], identity['ability'],
                                         identity['seed'], identity['target'])
    if identity != canonical or (expected_identity is not None and identity != expected_identity):
        raise ValueError('capturePathMismatch: canonical schedule/injection/hilog identity differs')
    audit = manifest['captureAudit']
    if (manifest.get('captureFrozen') is not True or audit['logPath'] != str(log_path.resolve())
            or audit['stabilityIntervalMs'] < 1000 or audit['injectionCompleted'] is not True
            or audit['collectorStopped'] is not True or audit['residualCollectors'] != 0
            or not audit['collectorPid']):
        raise ValueError('captureNotFrozen: missing or incomplete lifecycle audit')
    current = file_snapshot(log_path)
    for prefix in ('preParse', 'postParse'):
        for field in ('size', 'mtime', 'sha256'):
            if audit[prefix + field[0].upper() + field[1:]] != current[field]:
                raise ValueError('captureNotFrozen: frozen size/mtime/SHA256 differs')
    return manifest, current


def build_schedule(interval_ms: float) -> list[dict[str, Any]]:
    schedule: list[dict[str, Any]] = []
    elapsed_ms = 0.0
    for round_index in range(ROUND_COUNT):
        round_phases = PHASES if round_index == 0 else PHASES[1:]
        for phase_index, (phase, duration_ms, angle) in enumerate(round_phases):
            if round_index == 0 and phase == "initial_hold":
                phase_name = phase
            else:
                phase_name = f"round{round_index + 1}.{phase}"
            step_count = int(round(duration_ms / interval_ms))
            if step_count <= 0:
                raise ValueError(f"interval {interval_ms} is too large for phase {phase}")
            start_step = 0 if phase == "initial_hold" else 1
            for step in range(start_step, step_count + 1):
                progress = min(1.0, step / step_count)
                schedule.append({
                    "scheduledTimeMs": round(elapsed_ms + step * interval_ms, 3),
                    "phase": phase_name,
                    "step": step,
                    "targetTilt": round(angle(progress), 3),
                })
            elapsed_ms += duration_ms
    return schedule


def write_schedule(schedule: list[dict[str, Any]], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=(
            "scheduledTimeMs", "phase", "step", "targetTilt"
        ))
        writer.writeheader()
        writer.writerows(schedule)


def parse_events(log_text: str) -> tuple[list[dict[str, Any]], int, int, int]:
    events: list[dict[str, Any]] = []
    input_state_count = 0
    malformed_count = 0
    unknown_protocol_count = 0
    for line_number, line in enumerate(log_text.splitlines(), start=1):
        match = HOOK_RE.search(line)
        if not match:
            continue
        try:
            payload = json.loads(match.group(2))
        except json.JSONDecodeError:
            malformed_count += 1
            continue
        payload["_line"] = line_number
        if match.group(1) == "input_state":
            input_state_count += 1
            continue
        if payload.get("source") == "script" and payload.get("sequence") is not None:
            events.append(payload)
        else:
            unknown_protocol_count += 1
    return events, input_state_count, malformed_count, unknown_protocol_count


def validate_log(schedule: list[dict[str, Any]], interval_ms: float, log_path: Path,
                 expected_identity=None) -> dict[str, Any]:
    """P-011 v2.8: audit before event checks; discard results if evidence changes."""
    result = {'log': str(log_path.resolve()), 'passed': False, 'captureFrozen': False,
              'capturePath': None, 'captureAudit': {}, 'errors': []}
    try:
        with closed_capture(log_path) as handle:
            manifest, before = verify_capture(log_path, interval_ms, expected_identity)
            result.update(manifest)
            parsed = parse_events(handle.read())
            validated = _validate_events(schedule, interval_ms, log_path, parsed)
            _, after = verify_capture(log_path, interval_ms, expected_identity)
            if before != after:
                raise ValueError('captureNotFrozen: capture changed while parsing')
            result.update(validated)
            for prefix, snapshot in (('preParse', before), ('postParse', after)):
                for field in ('size', 'mtime', 'sha256'):
                    result['captureAudit'][prefix + field[0].upper() + field[1:]] = snapshot[field]
    except (OSError, ValueError, KeyError, TypeError) as error:
        result.update(passed=False, captureFrozen=False,
                      errors=[str(error) if str(error).startswith(('captureNotFrozen:', 'capturePathMismatch:'))
                              else f'captureNotFrozen: {error}'])
    return result


def _validate_events(schedule, interval_ms, log_path, parsed):
    events, input_state_count, malformed_count, unknown_protocol_count = parsed
    errors: list[str] = []
    run_ids = {str(event.get("runId")) for event in events}
    sequences = [event.get("sequence") for event in events]
    expected_sequence = list(range(1, len(schedule) + 1))
    if malformed_count:
        errors.append(f"malformed telemetry event count={malformed_count}")
    if unknown_protocol_count:
        errors.append(f"unknown or mixed telemetry event count={unknown_protocol_count}")
    if len(run_ids) != 1:
        errors.append(f"script run id count={len(run_ids)}, expected=1")
    if sequences != expected_sequence:
        errors.append("script sequence is not exactly [1..expectedEventCount] without duplicates")
    accepted_times = [float(event["acceptedTime"]) for event in events]
    accepted_gap_violations = 0
    for index, (previous, current) in enumerate(zip(accepted_times, accepted_times[1:]), start=1):
        gap = current - previous
        if gap < interval_ms:
            accepted_gap_violations += 1
            errors.append(
                f"event {index} acceptedTime gap={gap:.3f}ms is below {interval_ms:.3f}ms"
            )
    if len(events) != len(schedule):
        errors.append(f"script event count={len(events)}, expected={len(schedule)}")
    else:
        for index, (expected, actual) in enumerate(zip(schedule, events), start=1):
            target = actual.get("targetTilt")
            if not isinstance(target, (int, float)) or not math.isfinite(float(target)):
                errors.append(f"event {index} has non-finite targetTilt")
            elif abs(float(target) - float(expected["targetTilt"])) > 0.001:
                errors.append(
                    f"event {index} targetTilt={target}, expected={expected['targetTilt']}"
                )
    phase_spans: list[dict[str, Any]] = []
    if len(events) == len(schedule):
        for index, expected in enumerate(schedule):
            phase = str(expected["phase"]).split(".", 1)[-1]
            if not phase_spans or phase_spans[-1]["phase"] != phase:
                phase_spans.append({
                    "phase": phase,
                    "startTime": accepted_times[index],
                    "endTime": accepted_times[index],
                    "sampleCount": 1,
                })
            else:
                phase_spans[-1]["endTime"] = accepted_times[index]
                phase_spans[-1]["sampleCount"] += 1
    return {
        "log": str(log_path),
        "scriptEventCount": len(events),
        "scriptRunIdCount": len(run_ids),
        "scriptRunId": next(iter(run_ids), None),
        "scriptSequence": [event.get("sequence") for event in events] if len(events) <= 8 else "1..N",
        "scriptSequenceValid": sequences == expected_sequence,
        "inputStateEventCount": input_state_count,
        "unknownTelemetryEventCount": unknown_protocol_count,
        "expectedEventCount": len(schedule),
        "minAcceptedTimeGapMs": min(
            (current - previous for previous, current in zip(accepted_times, accepted_times[1:])),
            default=None,
        ),
        "acceptedTimeGapViolations": accepted_gap_violations,
        "phaseSpans": phase_spans,
        "errors": errors,
        "passed": not errors,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--frame-ms", type=float, default=100.0,
                        help="Want command period; must be no less than one display frame")
    parser.add_argument("--min-display-frame-ms", type=float, default=8.333)
    parser.add_argument("--output", type=Path, default=Path("test/artifacts/p011-tc02-schedule.csv"))
    parser.add_argument("--validate-log", type=Path)
    parser.add_argument('--prepare-capture', action='store_true')
    parser.add_argument('--device-script', type=Path)
    parser.add_argument('--capture-identity', type=Path)
    parser.add_argument('--freeze-context', type=Path)
    parser.add_argument('--validation-json', type=Path)
    parser.add_argument('--bundle', default='com.example.helloworld')
    parser.add_argument('--ability', default='EntryAbility')
    parser.add_argument('--seed', type=int, default=20261016)
    parser.add_argument('--target', default='62T0225B10010020')
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.frame_ms < args.min_display_frame_ms:
        print(
            f"frame-ms={args.frame_ms} is below min-display-frame-ms={args.min_display_frame_ms}",
            file=__import__("sys").stderr,
        )
        return 2
    schedule = build_schedule(args.frame_ms)
    identity = None
    if args.prepare_capture:
        if args.device_script is None or args.capture_identity is None:
            raise ValueError('--prepare-capture requires --device-script and --capture-identity')
        identity = prepare_capture(args.output, args.device_script, interval_ms=args.frame_ms,
                                   bundle=args.bundle, ability=args.ability, seed=args.seed, target=args.target)
        args.capture_identity.write_text(json.dumps(identity, indent=2) + '\n', encoding='utf-8')
    elif not args.validate_log:
        write_schedule(schedule, args.output)
    summary = {
        "schedule": str(args.output),
        "frameMs": args.frame_ms,
        "minDisplayFrameMs": args.min_display_frame_ms,
        "rounds": ROUND_COUNT,
        "rowCount": len(schedule),
        "durationMs": schedule[-1]["scheduledTimeMs"] + args.frame_ms,
        "maxAdjacentAngleStepDeg": max(
            abs(float(current["targetTilt"]) - float(previous["targetTilt"]))
            for previous, current in zip(schedule, schedule[1:])
        ),
    }
    if args.validate_log:
        if args.capture_identity:
            identity = json.loads(args.capture_identity.read_text(encoding='utf-8-sig'))
        if args.freeze_context:
            try:
                context = json.loads(args.freeze_context.read_text(encoding='utf-8-sig'))
                if identity is None:
                    raise ValueError('capturePathMismatch: --capture-identity required to freeze')
                freeze_capture(args.validate_log, identity, context)
            except (OSError, ValueError, KeyError, TypeError) as error:
                print(f'FAIL(captureNotFrozen): {error}')
                return 1
        result = validate_log(schedule, args.frame_ms, args.validate_log, identity)
        summary["validation"] = result
        result_path = args.validation_json or args.validate_log.with_suffix(".tc02-validation.json")
        result_path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        summary["validationJson"] = str(result_path)
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    validation = summary.get("validation")
    return 0 if validation is None or validation["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
