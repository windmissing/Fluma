#!/usr/bin/env python3
import argparse
import json
import math
import re
from pathlib import Path

from p011_v210_natural_observation_check import validate_observation_contract


TAN_30 = math.tan(math.radians(30.0))
OVERSHOOT_PHASES = ("positiveOvershoot", "negativeOvershoot")
EPS = 1e-9


def load_json(path):
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def parse_metrics(path):
    metrics = {}
    pattern = re.compile(r"P011_METRIC (\{.*)")
    with path.open("r", encoding="utf-8", errors="replace") as file:
        for line in file:
            match = pattern.search(line)
            if not match:
                continue
            try:
                payload = json.loads(match.group(1))
            except json.JSONDecodeError:
                continue
            if isinstance(payload, dict) and isinstance(payload.get("sequence"), int):
                metrics[payload["sequence"]] = payload
    return metrics


def ols_slope(heights):
    count = len(heights)
    if count < 2:
        return 0.0, 0.0, 0.0
    mean_x = (count - 1) / 2.0
    mean_y = sum(heights) / count
    denominator = sum((index - mean_x) ** 2 for index in range(count))
    slope = (
        sum((index - mean_x) * (heights[index] - mean_y) for index in range(count))
        / denominator
        if denominator
        else 0.0
    )
    predicted = [mean_y + slope * (index - mean_x) for index in range(count)]
    residual_sum = sum((heights[index] - predicted[index]) ** 2 for index in range(count))
    total_sum = sum((value - mean_y) ** 2 for value in heights)
    r_squared = 1.0 - residual_sum / total_sum if total_sum > EPS else 1.0
    rmse = math.sqrt(residual_sum / count)
    return slope, r_squared, rmse


def signed_degrees(heights):
    return math.degrees(math.atan(ols_slope(heights)[0]))


def raster_surface(path, layout):
    from PIL import Image

    with Image.open(path) as image:
        rgb = image.convert("RGB")
        width, height = rgb.size
        pixels = rgb.load()
    scale_x = width / float(layout["canvasWidth"])
    scale_y = height / float(layout["canvasHeight"])
    box_x = int(float(layout["boxX"]) * scale_x)
    box_y = int(float(layout["boxY"]) * scale_y)
    box_width = int(float(layout["boxWidth"]) * scale_x)
    box_height = int(float(layout["boxHeight"]) * scale_y)
    cols = int(layout["cols"])
    capacity = float(layout["capacity"])
    result = []
    for col in range(cols):
        center_x = box_x + int((col + 0.5) * box_width / cols)
        sample_xs = [value for value in (center_x - 1, center_x, center_x + 1) if 0 <= value < width]
        found = None
        for y in range(box_y + 8, box_y + box_height - 8):
            def is_surface(pixel):
                red, green, blue = pixel
                luminance = (red + green + blue) / 3.0
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


def raster_phase_results(manifest_path):
    manifest = load_json(manifest_path)
    layout = manifest.get("layout", {})
    result = {}
    errors = []
    for capture in manifest.get("captures", []):
        phase = str(capture.get("phaseD") or capture.get("phase"))
        relative_ms = float(capture.get("relativePhaseTimeMs", 0))
        path = manifest_path.parent / capture.get("file", capture.get("path", ""))
        heights = raster_surface(path, layout)
        if heights is None or len(heights) < 2:
            errors.append("rasterUnreadable:" + phase + ":" + capture.get("path", ""))
            continue
        capture_path = capture.get("file", capture.get("path", ""))
        slope, r_squared, rmse = ols_slope(heights)
        result.setdefault(phase, {"samples": []})["samples"].append(
            {
                "relativeMs": relative_ms,
                "path": capture_path,
                "slopeRadPerCol": slope,
                "signedSlopeDeg": math.degrees(math.atan(slope)),
                "linearSlopeDeg": math.degrees(math.atan(abs(slope))),
                "rSquared": r_squared,
                "rmseRadPerCol": rmse,
            }
        )
    for phase in result:
        result[phase]["samples"].sort(key=lambda sample: sample["relativeMs"])
    return result, errors


def analyze(ledger_path, hilog_path, raster_manifest_path, output_path):
    metrics = parse_metrics(hilog_path)
    manifest = load_json(raster_manifest_path)
    raster_results, raster_errors = raster_phase_results(raster_manifest_path)
    hard_violations = 0
    hard_checked_edges = 0
    hard_violating_edges = 0
    tilt_snapshot_mismatches = 0
    missing_post_tilt = 0
    first_violation = None
    budget_violations = 0
    first_budget_violation = None
    phase_records = {}
    initial_deposit_total = None
    record_count = 0
    first_context = {}
    records = []

    with ledger_path.open("r", encoding="utf-8") as file:
        for line in file:
            if not line.strip():
                continue
            record = json.loads(line)
            records.append(record)
            record_index = record_count
            if record_count == 0:
                first_context = record.get("context", {})
            record_count += 1
            context = record["context"]
            phase = context["phaseD"]
            sequence = int(context["sequence"])
            metric = metrics.get(sequence)
            if metric is None:
                missing_post_tilt += 1
            elif not (
                metric.get("frameId") == context.get("frameId")
                and metric.get("substepId") == context.get("substepId")
            ):
                tilt_snapshot_mismatches += 1

            before = record["stateBefore"]
            after = record["stateAfter"]
            if initial_deposit_total is None:
                initial_deposit_total = sum(before["bh"]) + sum(before["bl"])
            phase_records.setdefault(phase, []).append(
                {
                    "context": context,
                    "suspH": sum(after["suspH"]),
                    "suspL": sum(after["suspL"]),
                    "slopeDeg": signed_degrees(after["S"]),
                    "physicsTime": float(context["physicsTime"]),
                }
            )

            committed_tilt = float(metric["effectiveTilt"]) if metric else None
            substep_violation = None
            if committed_tilt is None:
                for height_index in range(len(before["S"]) - 1):
                    hard_violations += 1
                    hard_checked_edges += 1
                    if first_violation is None:
                        first_violation = {
                            "recordIndex": record_index,
                            "sequence": sequence,
                            "phase": phase,
                            "edgeIndex": height_index,
                            "reason": "missingSameSubstepPostTilt",
                        }
            else:
                expected = math.tan(math.radians(committed_tilt))
                for height_index in range(len(before["S"]) - 1):
                    delta = after["S"][height_index + 1] - after["S"][height_index]
                    error = abs(delta - expected)
                    hard_checked_edges += 1
                    if error > TAN_30 + EPS:
                        substep_violation = {
                            "recordIndex": record_index,
                            "sequence": sequence,
                            "phase": phase,
                            "edgeIndex": height_index,
                            "delta": delta,
                            "committedEffectiveTiltDeg": committed_tilt,
                            "expectedTan": expected,
                            "violationTan": error,
                            "limitTan30": TAN_30,
                        }
                        hard_violating_edges += 1
                if substep_violation is not None:
                    hard_violations += 1
                    if first_violation is None:
                        first_violation = substep_violation

            budgets = record["budgets"]
            audit = record["audit"]
            local_ok = all(
                float(used) <= float(limit) + EPS
                for used, limit in zip(budgets["localUsed"], budgets["localLimit"])
            )
            bottom_ok = (
                float(budgets["bottomUsed"]) <= float(budgets["bottomRequested"]) + EPS
                and float(budgets["bottomUsed"]) <= float(budgets["bottomLimit"]) + EPS
            )
            total_ok = (
                float(budgets["totalUsed"]) <= float(budgets["totalRequested"]) + EPS
                and float(budgets["totalUsed"]) <= float(budgets["totalLimit"]) + EPS
            )
            relax_ok = (
                int(budgets.get("bottomClipped", 0)) == 0
                and int(budgets.get("totalClipped", 0)) == 0
                and audit.get("relaxAppliedTotal", 0) == 0
                and float(audit.get("bottomBudgetExcess", 1)) <= EPS
                and float(audit.get("totalBudgetExcess", 1)) <= EPS
                and float(audit.get("maxLocalBudgetExcess", 1)) <= EPS
            )
            if not (local_ok and bottom_ok and total_ok and relax_ok):
                budget_violations += 1
                if first_budget_violation is None:
                    first_budget_violation = {
                        "recordIndex": record_index,
                        "sequence": sequence,
                        "phase": phase,
                        "localOk": local_ok,
                        "bottomOk": bottom_ok,
                        "totalOk": total_ok,
                        "relaxOk": relax_ok,
                    }

    cols = len(records[0]["stateAfter"]["S"]) if records else 0
    contract_errors = validate_observation_contract(records, cols, manifest)

    phase_endpoint = {}
    for phase, records in phase_records.items():
        end_time = records[-1]["physicsTime"]
        last_1s = [item for item in records if item["physicsTime"] >= end_time - 1000.0]
        first = last_1s[0]
        last = last_1s[-1]
        suspension = last["suspH"] + last["suspL"]
        slope_change = abs(last["slopeDeg"] - first["slopeDeg"])
        phase_endpoint[phase] = {
            "last1sSlopeChangeDeg": slope_change,
            "last1sSlopeStable": slope_change <= 1.0 + EPS,
            "initialDepositTotal": initial_deposit_total,
            "endSuspHPlusSuspL": suspension,
            "suspensionWithin5PercentOfInitialDeposit": suspension
            <= 0.05 * float(initial_deposit_total) + EPS,
        }

    convergence = {}
    for phase in OVERSHOOT_PHASES:
        samples = raster_results.get(phase, {}).get("samples", [])
        final = samples[-1] if samples else None
        target_90 = next(
            (sample for sample in samples if sample["linearSlopeDeg"] >= 27.0), None
        )
        convergence[phase] = {
            "captureCount": len(samples),
            "finalLinearSlopeDeg": final["linearSlopeDeg"] if final else None,
            "finalSlopeR2": final["rSquared"] if final else None,
            "finalSlopeRmseRadPerCol": final["rmseRadPerCol"] if final else None,
            "slopeWithin30PlusMinus2": bool(final and abs(final["linearSlopeDeg"] - 30.0) <= 2.0),
            "rSquaredAtLeast090": bool(final and final["rSquared"] >= 0.90),
            "targetBasedFirst27DegRelativeMs": target_90["relativeMs"] if target_90 else None,
            "targetBasedT90Within5100ms": bool(target_90 and target_90["relativeMs"] <= 5100.0),
        }

    result = {
        "schemaVersion": 1,
        "testCase": "P011-SLIP-FORMAL",
        "routeId": first_context.get("observationRouteId", "natural-observation-v2.10"),
        "observationRunRole": first_context.get("runRole"),
        "thresholdCalibrationId": first_context.get("thresholdCalibrationId"),
        "thetaStartDeg": first_context.get("thetaStartDeg"),
        "thetaStopDeg": first_context.get("thetaStopDeg"),
        "transitionTelemetryAvailable": first_context.get("transitionTelemetryAvailable", False),
        "evidenceExperiment": "slip-overshoot-v1",
        "metrics": {
            "metricCount": len(metrics),
            "firstSequence": min(metrics) if metrics else None,
            "lastSequence": max(metrics) if metrics else None,
        },
        "recordCount": record_count,
        "observationContract": {
            "errors": contract_errors,
            "passed": not contract_errors,
        },
        "hardConstraint": {
            "definition": "per committed substep, every 127 edge satisfies |S[i+1]-S[i]-tan(effectiveTiltCommitted)|<=tan30",
            "committedTiltSource": "P011_METRIC.effectiveTilt at identical sequence/frame/substep/physicsTime",
            "checkedEdges": hard_checked_edges,
            "violatingEdges": hard_violating_edges,
            "violatingSubsteps": hard_violations,
            "passed": hard_violations == 0,
            "missingPostTiltSubsteps": missing_post_tilt,
            "firstViolation": first_violation,
        },
        "tiltLedgerAlignment": {
            "contextSource": "ledger.context.effectiveTilt (post-update after P011-TaskC)",
            "metricSource": "P011_METRIC.effectiveTilt (post-update)",
            "substepIdentityFields": ["sequence", "frameId", "substepId"],
            "missingPostTiltSubsteps": missing_post_tilt,
            "identityMismatches": tilt_snapshot_mismatches,
            "passed": missing_post_tilt == 0 and tilt_snapshot_mismatches == 0,
        },
        "budget": {
            "policy": "localUsed<=localLimit; bottomUsed<=min(bottomRequested,bottomLimit); totalUsed<=min(totalRequested,totalLimit); clipping/relax=0",
            "violatingSubsteps": budget_violations,
            "passed": budget_violations == 0,
            "firstViolation": first_budget_violation,
        },
        "phaseEndpoint": phase_endpoint,
        "convergence": convergence,
        "rasterErrors": raster_errors,
        "rasterSamplePaths": {
            phase: [sample["path"] for sample in data["samples"]]
            for phase, data in raster_results.items()
        },
    }
    overall = (
        result["hardConstraint"]["passed"]
        and result["observationContract"]["passed"]
        and result["tiltLedgerAlignment"]["passed"]
        and result["budget"]["passed"]
        and all(
            item["slopeWithin30PlusMinus2"]
            and item["rSquaredAtLeast090"]
            and item["targetBasedT90Within5100ms"]
            for item in convergence.values()
        )
        and all(
            item["last1sSlopeStable"] and item["suspensionWithin5PercentOfInitialDeposit"]
            for item in phase_endpoint.values()
        )
    )
    result["overall"] = "PASS" if overall else "FAIL"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return result


def write_report(result, report_path):
    import hashlib

    checker_sha256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    lines = [
        "# P011 SLIP 正式用例基线报告（slip-v1 历史证据）",
        "",
        "- 日期：2026-10-19",
        "- 被测代码 commit：`130a52c74e8964fb024677b672cad1965f5cf1db`",
        "- 校验器 SHA-256：`" + checker_sha256 + "`",
        "- 被测对象：`test/artifacts/p011-slip-overshoot-v1/`（只读历史证据，不修改物理代码）",
        "- 正式校验器：`test/p011_slip_formal_check.py`",
        "- 判定 JSON：`test/artifacts/p011-slip-formal-v1/formal-checker-output.json`",
        "- 结论：`" + result["overall"] + "`（RED 基线）",
        "",
        "## MUST-PASS 结果",
        "",
        "### 1. 逐边 tan30 硬约束",
        "",
        "- 口径：每个提交子步全部 127 边，`|S[c+1]-S[c]-tan(effectiveTilt_committed)|<=tan30°`。",
        "- `effectiveTilt_committed` 取同一 `sequence/frameId/substepId` 的 `P011_METRIC.effectiveTilt`（post-update）。",
        "- 实测：检查 `" + str(result["hardConstraint"]["checkedEdges"]) + "` 边，违约子步 `" + str(result["hardConstraint"]["violatingSubsteps"]) + "`，违约边 `" + str(result["hardConstraint"]["violatingEdges"]) + "`。",
        "- 判定：**FAIL**。",
        "- 首个违约：`" + json.dumps(result["hardConstraint"]["firstViolation"], ensure_ascii=False) + "`。",
        "",
        "### 2. 预算逐子步对账",
        "",
        "- 实测：违约子步 `" + str(result["budget"]["violatingSubsteps"]) + "`；判定：**" + ("PASS" if result["budget"]["passed"] else "FAIL") + "**。",
        "- 口径：local/bottom/total 均不放宽、不追加；clipping 与 relax 必须为 0。",
        "",
        "### 3. effectiveTilt pre/post 同子步账本对齐",
        "",
        "- ledger `context.effectiveTilt` 与同子步 `P011_METRIC.effectiveTilt` 均为 updateTilt 后参考。",
        "- 实测：缺 post 快照 `" + str(result["tiltLedgerAlignment"]["missingPostTiltSubsteps"]) + "`；身份错配 `" + str(result["tiltLedgerAlignment"]["identityMismatches"]) + "`。",
        "- 判定：**" + ("PASS" if result["tiltLedgerAlignment"]["passed"] else "FAIL") + "**。",
        "",
        "## 相位终点稳定性",
        "",
        "| 相位 | 最后1s斜率变化° | <=1° | 终点 suspH+suspL | <=5%初始沉积沙 | 判定 |",
        "|---|---:|---|---:|---|---|",
    ]
    for phase, item in result["phaseEndpoint"].items():
        passed = item["last1sSlopeStable"] and item["suspensionWithin5PercentOfInitialDeposit"]
        lines.append(
            "| " + phase + " | " + format(item["last1sSlopeChangeDeg"], ".6f") + " | "
            + str(item["last1sSlopeStable"]).lower() + " | "
            + format(item["endSuspHPlusSuspL"], ".6f") + " | "
            + str(item["suspensionWithin5PercentOfInitialDeposit"]).lower() + " | "
            + ("PASS" if passed else "FAIL") + " |"
        )
    lines.extend(
        [
            "",
            "## ±45° 相位收敛与 target-based t90",
            "",
            "| 相位 | 终点 OLS 坡度° | R² | RMSE rad/col | 30°±2° | R²>=0.90 | 首次27°相对时间ms | t90<=5100ms | 判定 |",
            "|---|---:|---:|---:|---|---|---:|---|---|",
        ]
    )
    for phase, item in result["convergence"].items():
        passed = (
            item["slopeWithin30PlusMinus2"]
            and item["rSquaredAtLeast090"]
            and item["targetBasedT90Within5100ms"]
        )
        first_ms = item["targetBasedFirst27DegRelativeMs"]
        lines.append(
            "| " + phase + " | " + format(item["finalLinearSlopeDeg"], ".6f") + " | "
            + format(item["finalSlopeR2"], ".6f") + " | "
            + format(item["finalSlopeRmseRadPerCol"], ".9f") + " | "
            + str(item["slopeWithin30PlusMinus2"]).lower() + " | "
            + str(item["rSquaredAtLeast090"]).lower() + " | "
            + ("null" if first_ms is None else format(first_ms, ".0f")) + " | "
            + str(item["targetBasedT90Within5100ms"]).lower() + " | "
            + ("PASS" if passed else "FAIL") + " |"
        )
    lines.extend(
        [
            "",
            "## RED 基线摘要",
            "",
            "1. 逐边 tan30 硬约束失败：不能采用 v1 checker 中滞后 `context.effectiveTilt` 的旧采样口径；正式口径改用同子步 post-update 快照。",
            "2. ±45° raster 未收敛到 30°±2°，且没有 target-based 首次 27° t90 记录；旧 `slope90TimeMs` 以本相位实测终值为基准，不得转正。",
            "3. 相位终点稳定性和悬沙预算纳入 MUST-PASS；本次逐相位实测值见上表。",
            "4. 本报告只固化客观用例与 RED 基线，不修改业务物理、预算、几何或既有门槛。",
        ]
    )
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ledger", type=Path, required=True)
    parser.add_argument("--hilog", type=Path, required=True)
    parser.add_argument("--raster-manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result = analyze(args.ledger, args.hilog, args.raster_manifest, args.output)
    if args.report is not None:
        write_report(result, args.report)
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0 if result["overall"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
