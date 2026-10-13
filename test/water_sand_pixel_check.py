#!/usr/bin/env python3
"""Render/model cross-check for WaterSand three-phase classification."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

from water_sand_check import parse_lines


@dataclass
class FrameResult:
    frame: str
    status: str
    render_shares: dict[str, float]
    model_shares: dict[str, float]
    differences: dict[str, float]
    note: str = ""


def load_calibration(path: Path) -> dict[str, Any]:
    calibration = json.loads(path.read_text(encoding="utf-8"))
    required = {"box", "water", "heavy", "light", "mixed"}
    missing = sorted(required - set(calibration))
    if missing:
        raise ValueError(f"Calibration missing keys: {', '.join(missing)}")
    return calibration


def model_shares(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    shares: list[dict[str, Any]] = []
    for record in records:
        if record["type"] not in {"METRIC", "SNAPSHOT"}:
            continue
        data = record["data"]
        # TODO(product-hook): expose renderAreaShareWater/Heavy/Light, or the
        # six-state arrays needed to derive model shares from WS_SNAPSHOT.
        if all(key in data for key in ("renderAreaShareWater", "renderAreaShareHeavy", "renderAreaShareLight")):
            shares.append(
                {
                    "line": record["line"],
                    "water": float(data["renderAreaShareWater"]),
                    "heavy": float(data["renderAreaShareHeavy"]),
                    "light": float(data["renderAreaShareLight"]),
                }
            )
    return shares


def classify_frame(frame_path: Path, calibration: dict[str, Any]) -> dict[str, float]:
    # TODO(product-hook/test-calibration): agree final water/heavy/light/mixed
    # BGR thresholds after MODE_WATERSAND rendering is implemented. The box and
    # threshold rules must come from calibration JSON, not hard-coded guesses.
    raise NotImplementedError(f"Color classification pending WaterSand renderer: {frame_path}")


def check_frames(
    frame_paths: list[Path], records: list[dict[str, Any]], calibration: dict[str, Any]
) -> list[FrameResult]:
    models = model_shares(records)
    results: list[FrameResult] = []
    for frame_path in frame_paths:
        if not models:
            results.append(
                FrameResult(
                    frame=frame_path.name,
                    status="BLOCKED",
                    render_shares={},
                    model_shares={},
                    differences={},
                    note="TODO(product-hook): model area shares are unavailable.",
                )
            )
            continue
        try:
            render = classify_frame(frame_path, calibration)
        except NotImplementedError as error:
            results.append(
                FrameResult(
                    frame=frame_path.name,
                    status="BLOCKED",
                    render_shares={},
                    model_shares={},
                    differences={},
                    note=str(error),
                )
            )
            continue
        model = models[-1]
        differences = {
            phase: abs(render.get(phase, float("nan")) - model.get(phase, float("nan")))
            for phase in ("water", "heavy", "light")
        }
        passed = all(value <= 0.05 for value in differences.values())
        results.append(
            FrameResult(
                frame=frame_path.name,
                status="PASS" if passed else "FAIL",
                render_shares=render,
                model_shares=model,
                differences=differences,
            )
        )
    return results


def write_outputs(results: list[FrameResult], report: Path, markdown: Path) -> None:
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(
        json.dumps([asdict(result) for result in results], ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    lines = [
        "# WaterSand pixel/model cross-check",
        "",
        "| Frame | Status | Water diff | Heavy diff | Light diff | Note |",
        "|---|---|---:|---:|---:|---|",
    ]
    for result in results:
        lines.append(
            "| {frame} | {status} | {water:.4f} | {heavy:.4f} | {light:.4f} | {note} |".format(
                frame=result.frame,
                status=result.status,
                water=result.differences.get("water", float("nan")),
                heavy=result.differences.get("heavy", float("nan")),
                light=result.differences.get("light", float("nan")),
                note=result.note,
            )
        )
    markdown.parent.mkdir(parents=True, exist_ok=True)
    markdown.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--frames", type=Path, required=True, help="Directory or glob of captured frames")
    parser.add_argument("--hilog", type=Path, required=True)
    parser.add_argument("--calibration", type=Path, required=True)
    parser.add_argument("--report", type=Path, default=Path("test/reports/ws_pixel_check.json"))
    parser.add_argument("--markdown", type=Path, default=Path("test/reports/ws_pixel_check.md"))
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    frame_paths = sorted(path for path in args.frames.iterdir() if path.is_file()) if args.frames.is_dir() else [args.frames]
    records = parse_lines(args.hilog.read_text(encoding="utf-8", errors="replace").splitlines())
    calibration = load_calibration(args.calibration)
    results = check_frames(frame_paths, records, calibration)
    write_outputs(results, args.report, args.markdown)
    for result in results:
        print(f"{result.frame}: {result.status}")
    print(f"JSON: {args.report}")
    print(f"Markdown: {args.markdown}")
    return 0 if all(result.status in {"PASS", "N/A"} for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
