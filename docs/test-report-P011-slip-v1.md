# P011 SLIP 正式用例基线报告（slip-v1 历史证据）

- 日期：2026-10-19
- 被测代码 commit：`130a52c74e8964fb024677b672cad1965f5cf1db`
- 校验器 SHA-256：`fdbc3424f34231522f4d29b1225dcab3bbe763215899c66c1d79535cef974dc9`
- 被测对象：`test/artifacts/p011-slip-overshoot-v1/`（只读历史证据，不修改物理代码）
- 正式校验器：`test/p011_slip_formal_check.py`
- 判定 JSON：`test/artifacts/p011-slip-formal-v1/formal-checker-output.json`
- 结论：`FAIL`（RED 基线）

## MUST-PASS 结果

### 1. 逐边 tan30 硬约束

- 口径：每个提交子步全部 127 边，`|S[c+1]-S[c]-tan(effectiveTilt_committed)|<=tan30°`。
- `effectiveTilt_committed` 取同一 `sequence/frameId/substepId` 的 `P011_METRIC.effectiveTilt`（post-update）。
- 实测：检查 `319024` 边，违约子步 `484`，违约边 `59413`。
- 判定：**FAIL**。
- 首个违约：`{"recordIndex": 1456, "sequence": 1457, "phase": "positiveOvershoot", "edgeIndex": 126, "delta": 0.1390415789497581, "committedEffectiveTiltDeg": 36.0, "expectedTan": 0.7265425280053608, "violationTan": 0.5875009490556027, "limitTan30": 0.5773502691896257}`。

### 2. 预算逐子步对账

- 实测：违约子步 `1738`；判定：**FAIL**。
- 口径：local/bottom/total 均不放宽、不追加；clipping 与 relax 必须为 0。

### 3. effectiveTilt pre/post 同子步账本对齐

- pre：ledger `context.effectiveTilt`；post：同子步 `P011_METRIC.effectiveTilt`。
- 实测：缺 post 快照 `0`；身份错配 `0`。
- 判定：**PASS**。

## 相位终点稳定性

| 相位 | 最后1s斜率变化° | <=1° | 终点 suspH+suspL | <=5%初始沉积沙 | 判定 |
|---|---:|---|---:|---|---|
| baseline | 0.000000 | true | 0.000000 | true | PASS |
| positive | 0.026823 | true | 270.724945 | true | PASS |
| positiveReturn | 0.000483 | true | 0.682054 | true | PASS |
| positiveOvershoot | 0.065818 | true | 282.228961 | true | PASS |
| positiveOvershootReturn | 0.004031 | true | 6.371000 | true | PASS |
| negativeOvershoot | 0.047340 | true | 227.035312 | true | PASS |
| negativeOvershootReturn | 0.013946 | true | 15.430404 | true | PASS |

## ±45° 相位收敛与 target-based t90

| 相位 | 终点 OLS 坡度° | R² | RMSE rad/col | 30°±2° | R²>=0.90 | 首次27°相对时间ms | t90<=5100ms | 判定 |
|---|---:|---:|---:|---|---|---:|---|---|
| positiveOvershoot | 0.226404 | 0.001293 | 4.057967939 | false | false | null | false | FAIL |
| negativeOvershoot | 0.523789 | 0.301096 | 0.514645250 | false | false | null | false | FAIL |

## RED 基线摘要

1. 逐边 tan30 硬约束失败：不能采用 v1 checker 中滞后 `context.effectiveTilt` 的旧采样口径；正式口径改用同子步 post-update 快照。
2. ±45° raster 未收敛到 30°±2°，且没有 target-based 首次 27° t90 记录；旧 `slope90TimeMs` 以本相位实测终值为基准，不得转正。
3. 相位终点稳定性和悬沙预算纳入 MUST-PASS；本次逐相位实测值见上表。
4. 本报告只固化客观用例与 RED 基线，不修改业务物理、预算、几何或既有门槛。
