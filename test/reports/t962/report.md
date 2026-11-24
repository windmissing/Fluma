# T962 · 目标 14 解析沙边界集显 GPU 校验（effector）

2026-11-24 · 开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。GPU 驻留、shader 渲染，无 Canvas 回退。工件：`.codex/local-test/t957-dm2-baseline`（PE `8168be2d`、modules `51cb1aaf`、geometry `f002ef06`/transport `e4d49107`/audit `7c3589b5` 五件 SHA 逐一核过，与 developer-T957-v1.md 收据一致）。

## 能力门（卡面首项）

- **shaderFloat64**：`HostDevice::Open(requireFloat64=true)` 的 `G14_FLOAT64_UNAVAILABLE` 拒绝路径 live 存在；本机 UHD 630 **实测支持**——`G14_DEVICE` 行实录 `"shaderFloat64":true`（vendor 32902/device 16024/driver 1656924）。
- **1+2⁻³⁰ 拆字**：FREEZE 内核拆分两 FP32 原字——全部六正臂 Completion 首错（words 16–24）**全零**，无 measured/expected 差异报出（拆字正确时首错为空；负控 A-bad-gauge 反向证明了该通道 live）。
- 原 PE/模块身份：宿主对 manifest、模块、夹具均校验冻结 SHA（`G14_MODULE_SHA`/`G14_FIXTURE_SHA` 路径），全部通过。

## 六正臂（全过）

| 臂 | 状态 | 派发 | 区间 | 时长 | 备注 |
|---|---|---|---|---|---|
| A（固定平床） | ANALYTIC_SEALED | 8 | 1×c0149571 | 3.5s | 参数 L=0.1/H=0.025/ρ=1000/μ=0.01/P0=1.0/ref=0.2 |
| A-reverse | ANALYTIC_SEALED | 8 | 1 封存 | 0.4s | 反向变体 |
| A-gauge | ANALYTIC_SEALED | 8 | 1 封存 | 0.2s | 规约变体接受（vs bad-gauge 拒 9） |
| B（双格开面） | **MANUFACTURED_SEALED** | 8 | 1 封存 | 0.2s | kind=MANUFACTURED；FLUX_BASE 开面条目符号位+Q24 位置 live |
| C-20x10（粗） | ANALYTIC_SEALED | 8 | **25 根合计 30 段** | 0.9s | 全 c0149571 |
| C-40x20（细） | ANALYTIC_SEALED | 8 | **50 根合计 73 段** | 2.4s | 全 c0149571 |

**共同判据（六臂全过）**：`producerFence VkResult=0`（真实提交 fence）、read/retired vk=0、`identity=true`、`reason=0`、**private 无发布**（typedToken/commit/acceptedRoot 全 null）、每根 children 封存带 SHA、input-N 含精确有理时刻与 8 个 binary64 解析参数。

**守恒**：GPU 侧证据=区间封存标记（c0149571）+首错全零+逐根 children SHA+同根原字全档；数值参考（两档最大逐格余额 3.56e−20/1.74e−20 m²）已由 T957 developer 离线核过，本卡不重复（结果只计解析小域）。

## 六负控（全拒、六独立拒因）

| 夹具 | exit | reason | 快照 |
|---|---|---|---|
| A-bad-gauge | 1 | **9** | d0149571 + measured/expected Real64 对（3208139244,854966927 vs 3164854026,2952118927） |
| B-closed | 1 | **6** | d0149571 |
| B-closed-flux | 1 | **4** | d0149571 |
| B-wrong-sign | 1 | **10** | d0149571 |
| B-wrong-time | 1 | **3** | 无区间标记（FREEZE 级拒） |
| B-wrong-generation | 1 | **2** | 无区间标记（身份/代拒） |

六个拒因 9/6/4/10/3/2 各自独立、无串扰；四个带 `d0149571` 拒绝快照（words 16–24 含首错与 Real64 拆字对）。

## 结论

**过**：能力门（shaderFloat64 实录+拆字零首错+资产 SHA 门）→ 六正臂全封存（A/A-reverse/A-gauge/B/C 两档 30/73 段）→ 六负控独立拒因 → 真实 fence → private 无发布。解析小域限定成立；未用 CPU 材质或静态代跑；正式 T611 仍等目标 13。

证据根 `test/reports/t962/runs/`（12 夹具 × roots.jsonl + input-N/root-N 原字 + 逐臂 stdout）。
