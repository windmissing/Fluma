#!/usr/bin/env python3
"""Recording orchestrator skeleton for WaterSand objective cases."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


@dataclass
class Scenario:
    case_id: str
    preset: str
    want: dict[str, Any]
    actions: list[dict[str, Any]]


SCENARIOS: dict[str, Scenario] = {
    "TC-WS-01": Scenario(
        "TC-WS-01",
        "sorting",
        {"rendermode": 4, "testmode": True, "wssnapshot": True, "wscols": 128, "wscapacity": 192},
        [
            {"action": "rest", "duration_s": 8},
            {"action": "tilt", "target_deg": 15, "duration_s": 5},
            {"action": "tilt", "target_deg": -30, "duration_s": 5},
            {"action": "tilt", "target_deg": 0, "duration_s": 3},
            {"action": "forceinvert", "value": True, "duration_s": 4},
            {"action": "forceinvert", "value": False, "duration_s": 4},
            {"action": "shake", "input": 1.0, "duration_s": 3},
            {"action": "rest", "duration_s": 8},
            {"action": "repeat", "count": 3},
        ],
    ),
    "TC-WS-02": Scenario("TC-WS-02", "sorting", {"rendermode": 4, "testmode": True, "wssnapshot": True}, [{"action": "rest", "duration_s": 10}]),
    "TC-WS-03": Scenario("TC-WS-03", "settling", {"rendermode": 4, "testmode": True, "wssnapshot": True}, [{"action": "rest", "duration_s": 20}, {"action": "repeat", "count": 3}]),
    "TC-WS-04": Scenario("TC-WS-04", "angle", {"rendermode": 4, "testmode": True, "wssnapshot": True}, [{"action": "rest", "duration_s": 10}]),
    "TC-WS-05": Scenario("TC-WS-05", "default", {"rendermode": 4, "testmode": True, "wssnapshot": True}, [{"action": "rest", "duration_s": 8}]),
    "TC-WS-06": Scenario(
        "TC-WS-06",
        "default",
        {"rendermode": 4, "testmode": True, "wssnapshot": True},
        [
            {"action": "tilt", "target_deg": 15, "duration_s": 9},
            {"action": "tilt", "target_deg": 30, "duration_s": 9},
            {"action": "tilt", "target_deg": 0, "duration_s": 5},
        ],
    ),
    "TC-WS-07": Scenario(
        "TC-WS-07",
        "default",
        {"rendermode": 4, "testmode": True, "wssnapshot": True},
        [
            {"action": "forceinvert", "value": True, "duration_s": 7},
            {"action": "forceinvert", "value": False, "duration_s": 7},
            {"action": "repeat", "count": 2},
        ],
    ),
    "TC-WS-08": Scenario(
        "TC-WS-08",
        "default",
        {"rendermode": 4, "testmode": True, "wssnapshot": True},
        [{"action": "rest", "duration_s": 3}, {"action": "shake", "input": 1.0, "duration_s": 3}, {"action": "rest", "duration_s": 8}],
    ),
    "TC-WS-09": Scenario(
        "TC-WS-09",
        "default",
        {"rendermode": 4, "testmode": True, "wssnapshot": True},
        [{"action": "shake", "input": 1.0, "duration_s": 3}, {"action": "rest", "duration_s": 10}],
    ),
    "TC-WS-10": Scenario("TC-WS-10", "sorting", {"rendermode": 4, "testmode": True, "wssnapshot": True}, [{"action": "rest", "duration_s": 30}]),
    "TC-WS-11": Scenario(
        "TC-WS-11",
        "default",
        {"rendermode": 4, "testmode": True, "wssnapshot": True},
        [
            {"action": "tilt", "target_deg": 15, "duration_s": 5},
            {"action": "tilt", "target_deg": -30, "duration_s": 5},
            {"action": "shake", "input": 1.0, "duration_s": 3},
            {"action": "rest", "duration_s": 5},
            {"action": "forceinvert", "value": True, "duration_s": 4},
            {"action": "forceinvert", "value": False, "duration_s": 4},
            {"action": "dynamic_tilt", "duration_s": 4},
        ],
    ),
    "TC-WS-12": Scenario("TC-WS-12", "default", {"rendermode": 4, "testmode": True, "wssnapshot": True, "wsmixband": 1}, [{"action": "capture", "sources": ["TC-WS-01", "TC-WS-05", "TC-WS-08", "TC-WS-09"]}]),
}


def install_hap(hap: Path, serial: str | None) -> None:
    # TODO(test-infra): replace with the project's verified hdc install command.
    command = ["hdc"]
    if serial:
        command += ["-t", serial]
    command += ["install", "-r", str(hap)]
    subprocess.run(command, check=True)


def start_app(scenario: Scenario, serial: str | None) -> None:
    # TODO(test-infra): build the aa start command from scenario.want.
    raise NotImplementedError(f"App startup orchestration pending Want support: {scenario.case_id}")


def run_actions(scenario: Scenario) -> None:
    # TODO(product-hook): inject tilt/shake/invert through testmode controls.
    raise NotImplementedError(f"Action orchestration pending product hooks: {scenario.case_id}")


def collect_outputs(output_dir: Path, scenario: Scenario) -> None:
    # TODO(test-infra): capture WaterSand hilog, screenshots, WS_FRAME, and refresh rate.
    raise NotImplementedError(f"Capture orchestration pending infrastructure: {scenario.case_id}")


def run_checkers(output_dir: Path) -> None:
    # TODO(test-infra): invoke water_sand_check.py and water_sand_pixel_check.py.
    raise NotImplementedError(f"Checker invocation pending captured artifacts: {output_dir}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", choices=sorted(SCENARIOS), required=True)
    parser.add_argument("--hap", type=Path)
    parser.add_argument("--serial")
    parser.add_argument("--output-dir", type=Path, default=Path("test/artifacts/ws"))
    parser.add_argument("--skip-install", action="store_true")
    parser.add_argument("--plan", action="store_true", help="Only print the scenario plan")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    scenario = SCENARIOS[args.case]
    if args.plan:
        print(json.dumps(asdict(scenario), ensure_ascii=False, indent=2))
        return 0
    if args.hap and not args.skip_install:
        install_hap(args.hap, args.serial)
    try:
        start_app(scenario, args.serial)
        run_actions(scenario)
        collect_outputs(args.output_dir, scenario)
        run_checkers(args.output_dir)
    except NotImplementedError as error:
        print(f"BLOCKED: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
