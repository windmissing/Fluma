#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Observation-3 v3 schedule CSV generator (GAP-2 binding artifacts).

Deterministic generation of the v3 pre-registration command schedule
(docs/observation-protocol-P011-v3-preregistration-v1.md §3): platforms
0 -> +15 -> +A -> +15 -> 0 -> -15 -> -A -> -15 -> 0 (H0..H8, 5100 ms holds),
eight ramp legs L1..L8 with a 100 ms command step, rates 0.5 / 1.0 deg/s.
The CSV layout matches the protocol §9 execution binding: UTF-8 no BOM, LF
endings, one trailing LF, columns
commandSequence,offsetMs,segmentId,phaseKind,legId,targetTiltDeg,rateDegPerSec
(hold rows: empty legId, rate 0; ramp rows: leg + positive rate magnitude).

A comes from the shadow audit (15.1..29.9 grid); this tool REFUSES to
default A to 30 or 29.9. Output also prints the schedule SHA-256 for the
manifest binding (never invented by the app side).
"""

import argparse
import hashlib
import sys
from pathlib import Path

PLATFORM_HOLD_MS = 5100
COMMAND_STEP_MS = 100
RATES = (0.5, 1.0)


def build_schedule(peak_a_deg: float, rate_deg_per_s: float):
    if rate_deg_per_s not in RATES:
        raise ValueError(f"rate must be one of {RATES}")
    if not (0 < peak_a_deg < 30):
        raise ValueError("A must satisfy 0 < A < 30 (no 30/29.9 default)")
    angular_step = rate_deg_per_s * COMMAND_STEP_MS / 1000.0
    platforms = [0.0, 15.0, peak_a_deg, 15.0, 0.0, -15.0, -peak_a_deg, -15.0, 0.0]
    leg_pairs = [(0, 1), (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 7), (7, 8)]
    commands = []
    sequence = 0
    offset = 0
    commands.append((sequence + 1, offset, "H0", "platform", "", 0.0, 0.0))
    sequence += 1
    offset += PLATFORM_HOLD_MS
    for leg_index, (frm, to) in enumerate(leg_pairs):
        t0, t1 = platforms[frm], platforms[to]
        leg_id = f"L{leg_index + 1}"
        span = abs(t1 - t0)
        exact_n = span / angular_step
        n = round(exact_n)
        if abs(exact_n - n) > 1e-9:
            raise ValueError(f"leg {leg_id} length not integral: {exact_n}")
        commands.append((sequence + 1, offset, leg_id, "ramp", leg_id, t0, rate_deg_per_s))
        sequence += 1
        offset += COMMAND_STEP_MS
        for step in range(1, n + 1):
            target = t0 + (t1 - t0) * step / n
            commands.append((sequence + 1, offset, leg_id, "ramp", leg_id,
                             target, rate_deg_per_s))
            sequence += 1
            offset += COMMAND_STEP_MS
        platform_id = f"H{to}"
        commands.append((sequence + 1, offset, platform_id, "platform", "", t1, 0.0))
        sequence += 1
        offset += PLATFORM_HOLD_MS
    return commands, offset


def format_angle(value: float) -> str:
    return f"{round(value * 100) / 100:.2f}"


def format_rate(value: float) -> str:
    return "0" if value == 0 else f"{round(value * 100) / 100:.2f}"


def to_csv(commands) -> str:
    lines = ["commandSequence,offsetMs,segmentId,phaseKind,legId,targetTiltDeg,rateDegPerSec"]
    for (seq, off, seg, kind, leg, target, rate) in commands:
        lines.append(f"{seq},{off},{seg},{kind},{leg},"
                     f"{format_angle(target)},{format_rate(rate)}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--peak-a", type=float, required=True,
                        help="A value from the shadow audit (0 < A < 30)")
    parser.add_argument("--rate", type=float, required=True, choices=RATES)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    commands, total_ms = build_schedule(args.peak_a, args.rate)
    csv_text = to_csv(commands)
    data = csv_text.encode("utf-8")
    args.output.write_bytes(data)
    sha = hashlib.sha256(data).hexdigest()
    ideal_total = 9 * PLATFORM_HOLD_MS + 4 * args.peak_a / args.rate * 1000
    print(f"commands={len(commands)} totalMs={total_ms} idealMs={ideal_total:.0f}")
    print(f"scheduleSha256={sha}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
