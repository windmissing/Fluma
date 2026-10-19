# P-011 natural-observation-v2.10 观测报告

## 运行身份

- Run ID: `p011-run-1792383705846-3`
- 设备: `62T0225B10010020`
- 场景: `128/192/28`, `wspreset=thick`, `dual-input-v1`
- 相位: `0/+15/0/-15/0`, 持续 `3000/5100/2000/5100/3000 ms`
- HAP SHA256: `330604d1f9a9e2a7982db7ad528752b4754d42cf409e6099f0cf794a2e841326`
- 证据目录: `test/artifacts/p011-v210-natural-observation-20261019-run7`

## 判定

`summary.json` 状态为 `ruleDriveTooWeak`: 现有自然规则的重沙界面驱动太弱，而不是重沙变化本身已经接近自然平衡。

## 关键结果

| 指标 | 结果 |
|---|---:|
| 正向可见幅度 `A_B_plus_px` | `0.0 px` |
| 负向可见幅度 `A_B_minus_px` | `0.0 px` |
| 重沙像素总量变化 `A_H_pixels` | `1 px` |
| `t50/t90` | 均未定义，因为幅度峰值为 `0` |
| 正向回零残余 | `0.0 px` |
| 负向回零残余 | `0.0 px` |
| 模型单位界面最大位移 | `4.2833e-4` |
| ledger 记录数 | `1470` |
| heartbeat 记录数 | `1470` |
| phase / audit / raster | 全部通过 |
| 观测错误 | `[]` |

图表: `test/artifacts/p011-v210-natural-observation-20261019-run7/raster-amplitude.png`

完整数据: `test/artifacts/p011-v210-natural-observation-20261019-run7/summary.json`

## 驱动诊断

| 相位 | bottom requested | bottom used | 限流子步 | heavy grant | light grant |
|---|---:|---:|---:|---:|---:|
| `+15` | `5522.6356` | `10.0245` | `262/262` | `5.0355` | `4.9890` |
| `-15` | `5540.3750` | `9.7183` | `262/262` | `5.0886` | `4.6297` |

- `transportDirectionCorrect=true`
- `rateLimitDominant=true`
- `nearNaturalEquilibrium=false`
- `+15` 相位 relax 请求 `3125.5977`, 实际应用 `718.1018`
- `-15` 相位 relax 请求 `3287.7663`, 实际应用 `731.6357`

## 通道级 H/L grant

| 通道 | `+15` H | `+15` L | `-15` H | `-15` L |
|---|---:|---:|---:|---:|
| `settle` | `0` | `154.3793` | `0` | `157.7979` |
| `lift` | `0` | `423.7944` | `0` | `410.8481` |
| `percolate` | `0` | `0` | `0` | `0` |
| `bottomCreep` | `5.0355` | `4.9890` | `5.0886` | `4.6297` |
| `slip` | `0` | `718.1018` | `0` | `731.6357` |
| `suspensionDiffusion` | `0` | `64.2682` | `0` | `66.3385` |

完整通道数据: `test/artifacts/p011-v210-natural-observation-20261019-run7/channel-grants.json`

## 结论

在 `5.1 s` 的正负倾斜窗口内，重沙界面没有形成任何可见位移；两个方向的 raster 幅度均为 `0 px`，回零残余也为 `0 px`。重沙的 `settle/lift/percolate/slip/suspensionDiffusion` grant 全为 `0`，仅 `bottomCreep` 获得约 `5` model units；而 bottom transport 的请求量超过 `5500` model units，实际使用只有约 `10` model units，且所有有效子步都被限流。因此证据支持 `规则弱`，不支持 `变化本来就慢`。
