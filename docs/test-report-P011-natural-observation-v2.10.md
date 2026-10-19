# P-011 natural-observation-v2.10 独立复核报告

- 日期：2026-10-19
- 结论：`ruleDriveTooWeak` 归因成立，未发现把真实位移误判为 0 的测量假象。
- 被测证据：`test/artifacts/p011-v210-natural-observation-20261019-run7/`
- 被测 HAP SHA256：`330604d1f9a9e2a7982db7ad528752b4754d42cf409e6099f0cf794a2e841326`
- 复核依据：`docs/algorithm-P011.md` §6.16/§7，`docs/architecture-P011.md` natural-observation-v2.10 章节。

## 1. 结论摘要

| 项目 | 结论 |
|---|---|
| schema / 守恒 / 账本一致性 | PASS |
| A_B± / A_H / t50 / t90 / τ / 残余 | PASS，与报告一致 |
| bottom request vs actual grant | PASS，5522.6356→10.0245 与 5540.3750→9.7183 |
| raster 标定与密度缩放 | PASS，未发现真实 ≥2px 位移被压成 0 的机制 |
| WaterSandField Probe 零侵入 | PASS，功能态只读；性能开销另行评价 |
| `ruleDriveTooWeak` 判定 | PASS |
| FPS 掉帧归因 | 不是 v2.10 Probe/worker 引起；v2.8 无 Probe/worker 基线已出现同型掉帧 |

复核未修改业务代码、观测代码、参数或任何门槛。

## 2. schema、守恒与账本一致性

独立流式解析 `ledger.ndjson`（244,522,171 bytes）：

| 检查 | 实测 | 判定 |
|---|---:|---|
| 记录数 | 1470 | PASS |
| heartbeat `P011_OBS_METRIC` | 1470 | PASS |
| runId 唯一性 | 1 个 runId | PASS |
| sequence | 1..1470 连续 | PASS |
| phase 计数 | baseline 444 / positive 262 / positiveReturn 218 / negative 262 / negativeReturn 284 | PASS |
| `stateAfter[i]` → `stateBefore[i+1]` 最大差 | 全部 8 个状态向量均为 0 | PASS |
| heavy 总量残差最大值 | 0 | PASS |
| light 总量残差最大值 | 3.637978807091713e-12 | PASS |
| water 总量残差最大值（audit 字段复算） | 7.275957614183426e-12 | PASS |
| finite / range / capacity / stock | 无违规 | PASS |
| bottom / total / local budget excess | 0 | PASS |
| `maxSubstepBh` / `maxSubstepBl` | 0.00032301766381692687 / 0.034008372431429734 | PASS |
| sandTop invariant 最大误差 | 0.3381 < tan30°=0.5773502691896258 | PASS |

`summary.json`、`channel-grants.json` 的 phase 计数与驱动诊断均与账本独立重算一致。`channel-grants.json` 的 12 类通道字段逐相重算，最大绝对误差不超过 `4.888534022029489e-12`。

hilog 中 `P011_OBS_STORE` 与 `P011_RASTER_LAYOUT` 各出现 1 次，`P011_OBS_ERROR` 为 0 次。`source-manifest.json` 的源码与 HAP 哈希均与当前文件复算一致。

## 3. 关键指标独立重算

| 指标 | 报告值 | 独立重算值 | 判定 |
|---|---:|---:|---|
| `A_B_plus_px` | 0.0 | 0.0 | PASS |
| `A_B_minus_px` | 0.0 | 0.0 | PASS |
| `A_H_pixels` | 1 | 1 | PASS |
| `t50_plus_ms` / `t90_plus_ms` | null | null（正相峰值为 0） | PASS |
| `t50_minus_ms` / `t90_minus_ms` | null | null（负相峰值为 0） | PASS |
| `tau_ms` | null | null（正相峰值为 0） | PASS |
| positiveReturn residual | 0.0 | 0.0 | PASS |
| negativeReturn residual | 0.0 | 0.0 | PASS |

说明：baseline 自身相对末帧 baseline 有 1.0004556624826468 px 的瞬时阈值化噪声，低于 2px 可见判据，也未进入 `A_B+`/`A_B-`。`A_H_pixels=1` 来自 baseline 首帧绿色像素数 2945 与末帧 2944 的 1 px 差；正/负相均为 2944。

## 4. 驱动诊断独立重算

| phase | bottom requested | bottom used | limited | relaxOn |
|---|---:|---:|---:|---:|
| positive | 5522.635623525368 | 10.024506202660008 | 262/262 | 262/262 |
| negative | 5540.375000000009 | 9.718263512627333 | 262/262 | 262/262 |

方向检查强于“bottomUsed>0”：

- positive：33274 条 bottom edge 全部 `directionSign=+1` 且 donor index < receiver index。
- negative：33274 条 bottom edge 全部 `directionSign=-1` 且 donor index > receiver index。

因此 `transportDirectionCorrect=true`、`rateLimitDominant=true`、`nearNaturalEquilibrium=false` 与账本一致。报告中的 `ruleDriveTooWeak` 分支成立。

## 5. raster 标定与截图密度缩放

独立复算 28 张真实截图：

| phase | captures | SHA256 mismatch | A_B |
|---|---:|---:|---:|
| baseline | 5 | 0 | — |
| positive | 7 | 0 | 0.0 px |
| positiveReturn | 4 | 0 | 0.0 px |
| negative | 7 | 0 | 0.0 px |
| negativeReturn | 5 | 0 | 0.0 px |

`hilog` 与 manifest 中的 raster layout 完全一致。标定为 128 列逐列 heavy 绿色界面，不是 whole-green 质心替代；转换使用同一截图高度与 layout 的 `pixelsPerModelUnitY`，正/负相均得到 0 px。`raster-amplitude.png` 的 baseline 首点约 1.0005 px、正/负相 0、bottom request/used 柱状图与账本重算一致。JPEG 阈值化会引入约 1 px/1 绿色像素噪声，但不存在能把 ≥2px 界面位移系统性压成 0 的路径。

## 6. Probe 零侵入检查

`WaterSandField.ets` 的插入点满足只读契约：

1. `beginNaturalObservation()` 在 `updateSandTop()` 后复制状态并初始化 Probe 上下文。
2. `finishNaturalObservation()` 复制 after 状态、计算 audit、调用 `Probe.finishSubstep()`。
3. Probe 内部只保存状态、通道 grant 和 audit，最后调用 sink，不写回物理状态。
4. `naturalObservationState()` 调用的 `updateSandTop()` 是由当前 `bh/bl/mh/ml` 决定的确定性重算，且 `step()` / `applyMotionChannels()` 已在相邻位置调用，属冗余同步而非物理修改。
5. 观测相关新增变量不参与预算裁剪、通道优先级、守恒或状态转移。

因此 Probe 对模拟结果功能零侵入。后台 worker 每 record `JSON.stringify` 后写文件并 `fsyncSync`，这是有意的持久化设计；它可能带来性能开销，但不改变模拟状态。

## 7. FPS 掉帧补充复核

效应基线 `docs/effect-baseline-P011-natural-v1.md` 记录 P0 为 60 FPS，+15° 为 20 FPS，-15° 为 16 FPS，回正阶段为 31/39 FPS。对应证据截图可见 `FPS=60` 与 `FPS=20`。

关键控制组：该效应基线使用 `test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap`，其 `source-manifest.json` 不包含 `P011NaturalObservation*.ets` 等 v2.10 Probe/worker 模块。也就是说，在没有 v2.10 后台 ledger sink 和 `fsyncSync` 的情况下，同设备、同五相序列仍出现 60→20/16 的掉帧。

结论：

1. FPS 掉帧不是 v2.10 Probe/worker 引起的主要现象。
2. v2.10 run7 的正相截图显示 `FPS=19`，与既有掉帧模式一致，而非 Probe 打开后新引入的突变。
3. 每条 record 约 166KB、共 1470 条且逐条 `fsyncSync`，这确实是有潜在代价的观测 sink 设计；但目前证据不支持把掉帧根因归到它。
4. 尚无同版本 v2.10、`wssnapshot=false` 的直接 A/B，因此只能排除“主因”，不能精确量化 Probe/worker 的边际开销。

严重度：Medium。性能现象真实且影响体验；但因 v2.8 无 Probe/worker 基线已复现，不应阻塞 `ruleDriveTooWeak` 证据链结论。

## 8. 问题与保留意见

| 问题 | 严重度 | 说明 |
|---|---|---|
| FPS 根因未完全分解 | Medium | 已排除 v2.10 Probe/worker 主因，但缺少同版本 Probe-off A/B 来量化边际开销。 |
| 观测 sink 成本较高 | Low | 逐 record `JSON.stringify` + `fsyncSync` 在移动设备上有潜在 IO/内存压力，后续如继续采集应做性能评估。 |
| `bottomRequested` 语义需保持清楚 | Info | 它是逐子步所有 bottom edge request 的总和，不是单列或单边请求；报告未将其误用为位移。 |

除上述非阻断事项外，未发现影响 `ruleDriveTooWeak` 归因的证据链缺陷。
