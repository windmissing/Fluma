# P-011 双角度标定补采排期（effector 执行单）

## 目的与结论

为每个 candidate pair 补齐双角度协议所需的独立 fit/holdout 证据。现有 run `p011-run-1792420248512-3` 保持原样，作为**历史 calibration / 诊断证据**归档；因 d5a4e4f 边身份污染、439 个 hard range failure 及 stop 遥测无法独立排除污染，不计入新 run 数、fit、holdout 或 bracket 配额。不得把它改派为 fit/holdout，也不得重写 artifact。

**最小补采批次：34 个新的、合格的完整七相 run：24 fit + 10 holdout。** 这是最低 run 数，不保证实际转换/cell 配额一定够；若任一 cell 或每方向 start/stop 数不足，按本单末尾的 top-up 规则继续补，不得降低门槛。

## 冻结采集 schedule

每个 run 一次完整执行，不删相位、不调顺序、不改角度或 hold：

```text
0 / +15 / 0 / +45 / 0 / -45 / 0
每 phase holdMs=5100
```

同一 run 必须满足冻结的设备/build、几何、观测路线、candidate pair、raster 采集与 ledger schema 身份；每 run 同时覆盖 `+45°` 和 `-45°`。正式补采前先校验新 HAP/source manifest 不再是 d5a4e4f 污染构建，并通过 edge identity 与 stop 状态语义 preflight。

### Run-level 预检闸门

每条新 run 都要独立做资格判定；任一项失败即标 `invalid/excluded`，保留原始文件并按原组别补 run，不移动其它 run：

1. 七相完整，schedule/command 身份一一对应，ledger schema、runId、设备/build 和 candidate pair 完整。
2. 每条实际 slip grant 满足 `interfaceIndex=min(donor,receiver)` 且 donor/receiver 相邻；转换 bracket 前后属于同一物理 edge。不得用遍历数组槽号替代物理边身份。
3. 每个 stop 都是实际 `activeBefore=true → activeAfter=false` 状态转换；限流期间 `requested>granted`（包含 grant=0）仍保持 active。任何限流误标 stop 都使该 run 遥测无效。
4. 所有提交子步 hard-gate violation 为 0，预算、守恒、库存、容量及 telemetry 对齐检查通过。
5. v2.12 稳定平台首转折过滤字段齐全：phase、`targetTilt/effectiveTilt/effectiveTiltRate`、连续 `substepId`、edge identity、`r/d`、active 前后状态、request/grant 均可复核。

**采集资格与随机分组顺序**：用新 run 的第一条完整 schedule 记录做资格 preflight；只检查来源/build、字段身份、边身份、预算/守恒/hard gate 等预注册有效性项，不查看阈值拟合结果。通过则留在合格候选池，失败则标记 invalid 并保留证据。收齐 34 条合格 run 后，按不可变 runId 排序、seed `20261019` 在冻结分层内分 fit/holdout，并在任何阈值分析前冻结 randomization manifest；无效 run 不计数、不替换其它 run 的组别。既有坏 run 永不重新分组。

## Fit / holdout 分组

严格执行 algorithm-P011.md 标定协议原规则：

- 分层键：`thresholdCalibrationId/candidate pair`、schedule version、device/build identity、倾角方向覆盖；每个分层至少 34 个合格 run，目标 `24 fit / 10 holdout`。
- 仅对**新 run**按不可变 runId 排序后，以预注册 seed `20261019` 在层内随机化；在揭盲/分析前保存含 runId、组别、随机种子和分层键的 randomization manifest。
- 既有 calibration run 不重新随机、不转组；它已被排除，不占新的 34 个名额。
- 每个完整 run 自带正负倾角。最终确认 fit 与 holdout 各至少 5 个 `+tilt` run、5 个 `-tilt` run；不得把同一 run 的边、子步或 phase 拆到两组。
- holdout 结果保持隔离，不能用于拟合或选择参数。若 holdout 的 cell/转换配额不足，只追加 holdout run；fit 缺额只追加 fit run，既有 run 不移组。

## 单 run bracket 产出与配额风险

本次 d5a4e4f run 的**过滤前诊断计数**如下；它不是新 run 的可靠预测，且正式合格 bracket 数为 0：

| 方向 | 平台候选转换（首转折过滤前） | 每 edge 每 phase 首转折候选 | 本 run 正式合格 bracket |
|---|---:|---:|---:|
| +45° start | 1 | 1 | 0 |
| +45° stop | 2 | 1 | 0 |
| -45° start | 1 | 0 | 0 |
| -45° stop | 4 | 3 | 0 |

因此这份坏 run **不能给出新合格 run 的期望 bracket/run**；当前可审计的 per-run 合格产出基线为 0，而不是把上表候选数冒充产出。新 fit gate run 通过后，先从 fit-only 数据按稳定平台与首转折过滤统计真实 yield，再判断 24/10 是否够。每个组均需每个倾角方向 `start≥20`、`stop≥20`；每个 24-cell 还需 `≥30` 状态样本且来自 `≥10` 个不同 run（同 run 每 cell 最多3个）。

作为容量预警：即使暂按原坏 run 的“每方向约1个 start candidate/run”作纯算术假设，10 个 holdout 也只有约10个 start，达不到每方向20个；按约70/30比例扩为至少 `47 fit + 20 holdout = 67 runs` 才能覆盖这种候选产出。坏 run 的 -45 首转折候选为0，故此67-run情景也**不能保证**满足负向 start。不得据此先开67-run真机批次；先完成一个 clean fit gate。若 clean pilot 的稳定平台过滤后某一方向无 start，按冻结 schedule 继续重复不会自动解决其可辨识性，停止扩批并将该 schedule 的证据缺口提交 manager，不能现场改 schedule 或伪造 bracket。

## 真机时长

`calibration/schedule.csv` 七个 phase 命令的观测时间戳中，首命令至末命令间隔 `40.913 s`；末 phase 另保持 `5.100 s`。因此同一 runner 路径下计划物理采集时长约 **46.013 s/run**（不含设备启动、reset、安装/导出、无效 run 重跑）：

| 计划 | Run 数 | 估算采集时长 |
|---|---:|---:|
| 最小有效批次：24 fit + 10 holdout | 34 | `1,564.442 s` ≈ **26分04秒** |
| 条件情景：47 fit + 20 holdout | 67 | `3,082.871 s` ≈ **51分23秒** |

排期至少预留 **35 分钟**用于最小批次真机运行，再额外安排设备重置、安装、文件冻结与校验时间；不合格 run 每替换一条，采集时间至少再加约46秒。67-run 仅为观察到每 run start 候选约1时的条件算术场景，实际数量以 clean fit yield 和预注册扩批规则为准。

## 交付与停止条件

- Effector 冻结每个 run 的 HAP/source digest、设备/build、schedule hash、randomization manifest、完整 ledger/raster 和有效性结果；旧 run 单独留档，不混入新组。
- fit-only 先报告 24 个 cell 的样本数/独立 run 数、四类方向×转换配额、稳定平台 first-event bracket yield 和 hard-gate/身份审计；之后才解封 holdout。
- 若 34 个合格 run 后仍有不足，按不足项追加原组别的新完整 schedule run；不得把 fit 移成 holdout、holdout 移成 fit、降低 `≥30/≥10/≥20` 门槛或更改冻结 schedule。
- 本补采计划不解除观测③ `BLOCKED`，不输出 `thetaStart/thetaStop`，不授权改物理或实现。