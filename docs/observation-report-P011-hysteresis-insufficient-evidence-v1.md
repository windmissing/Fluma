# P-011 双角度标定正式报告：insufficientEvidence

- **状态**：`insufficientEvidence`；`thetaStart=null`、`thetaStop=null`。本报告不发布任何阈值数值。
- **目的**：对现有单个 calibration run 执行 `algorithm-P011.md` 的双角度标定协议，并裁定该 run 的证据可用性。
- **run**：`p011-run-1792420248512-3`；设备 `62T0225B10010020`；candidate pair 字段 `28°/22.911°` 仅为历史占位值，不是估计值。
- **schedule**：冻结七相 `0/+15/0/+45/0/-45/0`，各 phase `holdMs=5100`。
- **证据**：`test/artifacts/p011-v212-relax-hysteresis/calibration/ledger.ndjson`、`summary.json`、`schedule.csv`、`source-manifest.json`、`effect-report-calib-hysteresis-calib-v1.md`。

## 正式可用性裁定

裁定为：**部分可用作采集流程与缺陷诊断证据；弃用为 theta fit/holdout 样本。** 不移动、不重分或覆盖该既有 run；保留其 `runRole=calibration` 身份和全部原始产物。

1. **hard-gate 不合格**：ledger 中 `audit.range=false` 共 `439` 个子步（positiveOvershoot 234、positiveOvershootReturn 3、negativeOvershoot 198、negativeOvershootReturn 4）。冻结协议排除含 hard-gate violation 的 run 对阈值拟合的贡献，因此正式 cell、transition、fit/holdout 有效计数均为 0。
2. **边身份污染**：扫描 `slip[]` 的 `105,145` 条逐边 grant 记录，`36,236` 条的 `interfaceIndex != min(donor,receiver)`，占 `34.46%`。数组槽位不能替代物理 edge 身份，故转换 bracket 无法可信地跨同一物理 edge 配对；这条 run 不满足逐边身份完整性。任务提供的源码身份为 `d5a4e4f`；artifact manifest 固定了本次 HAP/source digest，但未单独写 Git commit 字段。
3. **限流误标 stop 检查**：序列化数据中有 `353` 条 stop；逐行检查未发现 `requested>granted` 的 stop（0/353）。这只能说明本 ledger 没有记录到“正 request 被裁剪后仍标 stop”的直接例子，不能洗清该版本 bug：物理 edge join 已不可信，且 run 本身违反 hard gate。因此不把 stop 事件计入拟合，也不据此判定限流 stop 缺陷不存在。
4. **历史报告中的稳定平台数只作诊断**：按本次正式规则筛选 `effectiveTilt=±45°`、`effectiveTiltRate=0` 且同平台连续子步后，序列化候选转换为 `+45°: start 1/stop 2`、`-45°: start 1/stop 4`。每 edge 每 phase 只保留首个转换后为 `+45°: start 1/stop 1`、`-45°: start 0/stop 3`；这些仍是未通过来源与 hard-gate 检查的原始候选数，不是合格 bracket。

因此本 run **可部分保留**七相完成、runner/raster 路径、时序、aggregate 转换计数和 bug 复核信息；不得用于阈值点估计、bootstrap CI、24-cell 充分性、stop 转折拟合或 holdout 验证。

## 24-cell 状态样本量审计

下表的 raw count 是 ledger 中逐子步逐 edge 的序列化行数，按历史 placeholder `thetaStart=28°`、`thetaStop=22.911°` 和一个 run 的探索性 `wNoise=0.215701°` 分层；它们不是有效独立样本。计数用 `alpha=atan(abs(r))`，inactive/active 分别按候选 start/stop 角计算到阈值的距离，并按协议 near/shoulder/far-control 分类。此处探索性 `sigmaNoise=0.001265°`、`qResolution=0.215701°`，故 `wNoise=max(1.96*sigmaNoise,qResolution)=0.215701°`；这些数值只为展示 raw cell 落点，不是正式噪声定标。每个 run 每 cell 最多计 3 个状态样本，所以本 run 对任一 cell 的理论贡献上限为 `min(raw,3)`。

| Cell：状态 × d 符号 × 倾角符号 × 噪声层 | Raw 行数 | 单 run 计数上限 | Fit：有效/要求30（缺口） | Fit：run/要求10（缺口） | Holdout：有效/要求30（缺口） | Holdout：run/要求10（缺口） |
|---|---:|---:|---:|---:|---:|---:|
| inactive/start × d<0 × tilt>0 × near | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d<0 × tilt>0 × shoulder | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d<0 × tilt>0 × far-control | 419 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d<0 × tilt<0 × near | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d<0 × tilt<0 × shoulder | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d<0 × tilt<0 × far-control | 3 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d>0 × tilt>0 × near | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d>0 × tilt>0 × shoulder | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d>0 × tilt>0 × far-control | 6 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d>0 × tilt<0 × near | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d>0 × tilt<0 × shoulder | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| inactive/start × d>0 × tilt<0 × far-control | 567 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d<0 × tilt>0 × near | 5 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d<0 × tilt>0 × shoulder | 11 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d<0 × tilt>0 × far-control | 56,435 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d<0 × tilt<0 × near | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d<0 × tilt<0 × shoulder | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d<0 × tilt<0 × far-control | 9 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d>0 × tilt>0 × near | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d>0 × tilt>0 × shoulder | 0 | 0 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d>0 × tilt>0 × far-control | 20 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d>0 × tilt<0 × near | 4 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d>0 × tilt<0 × shoulder | 5 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |
| active/stop × d>0 × tilt<0 × far-control | 25,193 | 3 | 0/30（30） | 0/10（10） | 0/30（30） | 0/10（10） |

**Cell 汇总**：24 个 cell 全部正式计入 `0/30` 个状态样本、`0/10` 个有效 run；每 cell 的缺口都是至少 30 个样本与 10 个独立 run。raw count 高不等于有效样本，不能用同一 run 的数万子步替代 run-cluster 数量。

## 转换与候选 bracket 缺口

协议要求 fit 与 holdout **各自**在每个倾角方向均至少 20 个有效 start、20 个有效 stop；候选 pair 至少 24 fit + 10 holdout run，且每组正负方向各至少 5 个 run。

| 倾角方向 | 转换类型 | 稳定平台序列化候选（首转折过滤前） | 每 edge 首转折候选 | 正式有效 fit | Fit 要求/缺口 | 正式有效 holdout | Holdout 要求/缺口 |
|---|---|---:|---:|---:|---:|---:|---:|
| +45° | start | 1 | 1 | 0 | 20 / 缺20 | 0 | 20 / 缺20 |
| +45° | stop | 2 | 1 | 0 | 20 / 缺20 | 0 | 20 / 缺20 |
| -45° | start | 1 | 0 | 0 | 20 / 缺20 | 0 | 20 / 缺20 |
| -45° | stop | 4 | 3 | 0 | 20 / 缺20 | 0 | 20 / 缺20 |


`r=1.398352976` 是稳定平台序列化槽位 124 的后续 start；按每 edge 首转折过滤不进入阈值/分位数数据。首转折候选仍含 `+45° start r=0.541469`、`+45° stop r=0.427579`、`-45° stop r=0.707728/0.909317/1.347801`，并无首转折 `-45° start`。以上 residual 因物理边身份冲突及 hard-gate failure 全部仅作诊断，不可解释为 thetaStart/thetaStop。

## Run 数与方向覆盖

| 范围 | 当前原始 run 数 | 当前有效 run 数 | 要求 | 有效缺口 |
|---|---:|---:|---:|---:|
| Candidate pair 合计 | 1 | 0 | 34（24 fit + 10 holdout） | 34 |
| Fit 组 | 该 run 未通过资格门，且无可审计的 fit/holdout randomization manifest | 0 | 24 | 24 |
| Holdout 组 | 同上 | 0 | 10 | 10 |
| Fit +45°方向 run | 原始轨迹覆盖 | 0 | ≥5 | 5 |
| Fit -45°方向 run | 原始轨迹覆盖 | 0 | ≥5 | 5 |
| Holdout +45°方向 run | 原始轨迹覆盖 | 0 | ≥5 | 5 |
| Holdout -45°方向 run | 原始轨迹覆盖 | 0 | ≥5 | 5 |

## 结论

本次按冻结协议输出 **`insufficientEvidence`**。`thetaStart` 与 `thetaStop` 均不估计、不填默认值；不运行或发布有效 bootstrap CI。该 calibration run 对采集与 bug 归因部分可用，但在 hard-gate、edge identity 与限流状态可信度资格门失败，fit/holdout、24-cell 与转换计数正式贡献为零。下一步按补采清单采集新的合格 run；本报告不改变观测③任务本体的 `BLOCKED` 状态。