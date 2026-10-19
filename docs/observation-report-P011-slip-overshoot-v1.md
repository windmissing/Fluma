# P-011 slip-overshoot-v1 观测报告

## 运行身份

- Run ID: `p011-run-1792397031250-3`
- 设备: `62T0225B10010020`
- 场景: `128/192/28`, `wspreset=thick`, `dual-input-v1`
- 相位: `0/+15/0/+45/0/-45/0`，每段 `5.1 s`
- 时序口径: `5.1 s` 为 runner `holdMs`；沿用 v2.10 的命令派发与 raster 采集开销，实际 command interval 见 `schedule.csv`。
- 观测路线: `natural-observation-v2.10`，实验 ID: `slip-overshoot-v1`
- HAP SHA256: `a0ed37048ce0bee0391021e424b6c9d82e3d3248ea33a47d8d2d0c7b0c21f34d`
- 证据目录: `test/artifacts/p011-slip-overshoot-v1`

## 三问答案

1. **超角滑移触发与 30° 收敛**：`+45` 触发=yes，最终可见线性坡度=0.23°；`-45` 触发=yes，最终可见线性坡度=0.52°。按 `±2°` 判定，`+45/-45` 收敛到 30° 分别为 no/no。同时，ledger 中有 `484` 个子步 `audit.range=false`，因此这不是一个满足正式硬门槛的 30° 安息构型。
2. **重沙界面可见移动**：以 raster `>=2 px` 为可见阈值，`+45/-45` 分别为 no/no；幅度分别为 0.00 px / 0.00 px。方向、时间与回零残差见下表；ledger 的重沙 grant 同步列在通道表中。
3. **每单位超角滑移速率**：`+45` 与 `-45` 的表面坡度速率分别为 0.0027 / 0.0021 deg/(s·超角)，slip 体积速率分别为 8.6726 / 6.6061 model units/(s·超角)。`+15` 对照无超角，坡度变化速率为 0.0111 deg/s，slip 体积速率为 104.5883 model units/s。

本七相协议实际只包含 `+15` 对照，不包含 `-15`；`-45` 是反向超角段，不是 `-15` 对照段。

## 逐相 ledger 数据

| 相位 | 记录 | 目标° | 超角° | 初始坡度° | 最终坡度° | slip请求 | slip应用 | 重沙最大位移 | 质心方向 | 预算受限子步 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|
| baseline | 774 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 px | none | 0 |
| positive | 264 | 15.00 | 0.00 | 0.00 | -0.08 | 3151.13 | 724.27 | 0.25 px | right/up | 264 |
| positiveReturn | 416 | 0.00 | 0.00 | -0.08 | 0.20 | 9.67 | 6.63 | 0.00 px | none | 49 |
| positiveOvershoot | 270 | 45.00 | 15.00 | 0.20 | -0.07 | 15363.97 | 886.17 | 0.07 px | right/up | 270 |
| positiveOvershootReturn | 320 | 0.00 | 0.00 | -0.07 | 0.37 | 146.49 | 52.99 | 0.00 px | none | 91 |
| negativeOvershoot | 208 | -45.00 | 15.00 | 0.37 | 0.59 | 11912.33 | 677.49 | 0.06 px | left/down | 208 |
| negativeOvershootReturn | 260 | 0.00 | 0.00 | 0.59 | 0.23 | 209.28 | 96.74 | 0.00 px | none | 89 |

## raster 收敛曲线数据

| 相位 | 采样 | 初始坡度° | 最终坡度° | 坡度50%ms | 坡度90%ms | 重沙50%ms | 重沙90%ms | 重沙峰值ms | 重沙最大位移 | 重沙方向 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| baseline | 7 | 0.01 | 0.01 | - | - | - | - | 2191 | 0.00 px | none |
| positive | 7 | 0.94 | 0.94 | 1646 | 1646 | - | - | 699 | 0.00 px | none |
| positiveReturn | 7 | 0.83 | 0.16 | 1666 | 1666 | - | - | 718 | 0.00 px | none |
| positiveOvershoot | 7 | 0.00 | 0.23 | 4530 | 6494 | - | - | 677 | 0.00 px | none |
| positiveOvershootReturn | 8 | 0.80 | 0.32 | 1541 | 1541 | - | - | 674 | 0.00 px | none |
| negativeOvershoot | 8 | 0.31 | 0.52 | 4035 | 6583 | - | - | 637 | 0.00 px | none |
| negativeOvershootReturn | 7 | 0.47 | 0.23 | 3291 | 4995 | - | - | 667 | 0.00 px | none |

## 超角后回零残差

| 超角相位 | 参考相位 | 回零相位 | 经过时间ms | 参考坡度° | 回零坡度° | 重沙最大残差 | 重沙质心方向 |
|---|---|---|---:|---:|---:|---:|---|
| positiveOvershoot | positiveReturn | positiveOvershootReturn | 13565 | 0.20 | 0.37 | 0.07 px | right/up |
| negativeOvershoot | positiveOvershootReturn | negativeOvershootReturn | 12887 | 0.37 | 0.23 | 0.06 px | left/down |

## 硬门槛诊断

- `audit.range` 违约子步：`484`。
- 逐相位违约数：`{"negativeOvershoot": 206, "negativeOvershootReturn": 5, "positiveOvershoot": 268, "positiveOvershootReturn": 5}`。
- 首个违约：record `1456` / sequence `1457` / phase `positiveOvershoot`；context `effectiveTilt=24°`，`maxSandTopInvariantError=0.729596`，正式上限 `tan30=0.577350`。
- 判定：观测流程完整，但超角运行未满足正式 `tan30°` 硬门槛；本次任务未修改任何物理或约束来掩盖该结果。

## 实际通道 H/L grant

| 相位 | settle H | settle L | percolate H | percolate L | lift H | lift L | slip H | slip L | bottom H | bottom L | suspension H | suspension L |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| positive | 0.00 | 156.50 | 0.00 | 0.00 | 0.00 | 427.22 | 0.00 | 724.27 | 5.08 | 5.03 | 0.00 | 65.57 |
| positiveReturn | 0.00 | 402.63 | 0.00 | 0.00 | 0.00 | 132.58 | 0.00 | 6.63 | 0.00 | 0.00 | 0.00 | 46.09 |
| positiveOvershoot | 0.00 | 64.24 | 0.00 | 0.00 | 0.00 | 345.79 | 0.00 | 886.17 | 1.42 | 1.41 | 0.00 | 192.37 |
| positiveOvershootReturn | 0.00 | 488.67 | 0.00 | 0.00 | 0.00 | 212.81 | 0.00 | 52.99 | 0.00 | 0.00 | 0.00 | 144.40 |
| negativeOvershoot | 0.00 | 44.90 | 0.00 | 0.00 | 0.00 | 265.57 | 0.00 | 677.49 | 1.12 | 1.07 | 0.00 | 104.01 |
| negativeOvershootReturn | 0.00 | 425.66 | 0.00 | 0.00 | 0.00 | 214.05 | 0.00 | 96.74 | 0.00 | 0.00 | 0.00 | 144.97 |

## 预算与驱动诊断

| 相位 | relax触发 | relax受限 | total请求 | total应用 | total上限 | bottom请求 | bottom应用 | bottom上限 | effectiveTilt范围° | shake均值 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| baseline | 0/774 | 0/774 | 0.00 | 0.00 | 2343.51 | 0.00 | 0.00 | 78.12 | 0.00/0.00 | 0.0000 |
| positive | 264/264 | 264/264 | 5761.83 | 1318.10 | 1318.10 | 5572.89 | 10.11 | 43.94 | 0.00/15.00 | 0.1658 |
| positiveReturn | 416/416 | 49/416 | 675.97 | 541.84 | 2061.30 | 0.00 | 0.00 | 68.71 | 0.00/15.00 | 0.0113 |
| positiveOvershoot | 270/270 | 270/270 | 22204.74 | 1299.04 | 1350.00 | 5663.74 | 2.83 | 45.00 | 0.00/45.00 | 0.4956 |
| positiveOvershootReturn | 320/320 | 91/320 | 1448.84 | 754.48 | 1598.80 | 0.00 | 0.00 | 53.29 | 0.00/45.00 | 0.0436 |
| negativeOvershoot | 208/208 | 208/208 | 17087.67 | 990.15 | 1040.00 | 4308.10 | 2.19 | 34.67 | -45.00/0.00 | 0.4942 |
| negativeOvershootReturn | 260/260 | 89/260 | 1426.59 | 736.45 | 1300.00 | 0.00 | 0.00 | 43.33 | -45.00/0.00 | 0.0534 |

## 图表

![surface slope and heavy interface movement](test/artifacts/p011-slip-overshoot-v1/surface-slope.png)

完整曲线数据: `test/artifacts/p011-slip-overshoot-v1/summary.json`。

## 证据

- ledger: `test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`
- hilog: `test/artifacts/p011-slip-overshoot-v1/hilog.txt`
- raster manifest: `test/artifacts/p011-slip-overshoot-v1/raster-manifest.json`
- source manifest: `test/artifacts/p011-slip-overshoot-v1/source-manifest.json`
- summary: `test/artifacts/p011-slip-overshoot-v1/summary.json`

## 结论

本次运行状态为 `overshootSlipTriggeredWithHardRangeViolation`。观测代码仅记录状态与实际 grant，不改变物理、预算、几何、门槛或守恒约束；上述数值直接来自真机 ledger 与 raster。
`auditPassed=no`，`hardRangeViolation=yes`；若硬门槛失败，本报告只作为观测证据，不作为验收通过证据。
