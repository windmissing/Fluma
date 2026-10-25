# T38 peak 独立客观验证报告

- 任务卡：`6adda18ae4b03273ad679626`（【T38·peak独立客观验证】tester：同账本双算、采样边界与反例回归）
- 日期：2026-10-25。角色：tester。模式：纯离线只读验证，未使用真机，未改任何实现或判据。
- 依据：冻结协议 `docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md`（SHA-256 `3f043d404224b8290248d04ea600e5a7ac08b763625f28385d3c3063755ebfe9`，与 09:25 验收版逐字节一致）、T07 rev2 `docs/architecture-P011-peak-fields-revision-v1.md`、T08 实现（commit `41b90bf`）。
- 执行证据：`test/reports/p038_peak_independent_verification.json`（执行器 `test/p038_peak_independent_verification.py`，46/46 MATCH）。

## 1. 总结论

**PASS（附 1 条观察项，不改变判定）。** T08 交付的 peak 采样/流式峰值/checker 与冻结协议在口径层、身份层、边界层一致；独立复算与独立探针未发现口径偏离或假 PASS 路径。协议正式生效权仍在 manager，本报告不宣布 `peakProtocolEffective`。

## 2. 验证范围与方法（三层独立）

| 层 | 方法 | 结果 |
| :--- | :--- | :--- |
| A 独立复算 | 不看交付 checker 代码路径，按协议正文重写参考算式，对交付双算夹具（`test/reports/p011_peak_dual_fixture/`）逐量复算 27 项，与 `comparison.json` 比对 | 27/27 一致 |
| B 独立探针 | 自建合成账本 15 例，覆盖开发者套件未覆盖的协议条款，经交付 checker 执行验证其符合性 | 15/15 符合 |
| C 证据可信度 | 协议源哈希比对、身份串、无正式生效声明、node 套件复跑 | 4/4 通过 |

辅助：预注册反例执行器 `test/p011_peak_protocol_v1_execution.py` 复跑 **8/8 MATCH**（分档表、可达性恒等式残差 0.0、分相规则、legacy 全序列假 PASS 演示均 MATCH）；冻结协议、反例执行器、legacy checker（`test/water_sand_check.py`）相对 `61e5ac4` **git diff 为空**（legacy 原式未变）；开发者 Python 套件 13/13 OK。

## 3. 协议要点逐项核对

| 协议条款 | 验证方式 | 结果 |
| :--- | :--- | :--- |
| 固定 3s 半开窗 `W=[t_on,t_on+3000ms)` | 层 B2/B4/B9：恰等于右端的记录（含全窗最大 M=900）被排除；晚 off 不延长窗口 | 符合 |
| 每成功提交采样 | 源码走查 `recordPeakCommit()` 挂在 post-commit stamp 成功后；层 B7 覆盖缺口被 checker 捕获 | 符合（见观察 O-1） |
| 分子含 state1/2、state0 除外 | 层 B6 双向：正确排除 state=0 槽 → PASS；摘要计入死槽质量 → INVALID_LEDGER | 符合 |
| 分母 `E=VH0+VL0+IH+IL` | 层 A 逐量复算（E=1000）；无 EPS、无默认比值 | 符合 |
| 最早达峰身份 | 层 B1（并列最大取较早提交）+ 开发者 node 套件（真实 ArkTS 采样器，并列 0.52 报首笔 sequence 1） | 符合 |
| 同 ms 顺序 | 源码：事件与提交共用单一 `nextOrder()` 计数器，时钟回退抛 `PEAK_CLOCK_INVALID`；层 B3：order 低于 on 事件的提交无资格；层 B11：时钟回退 → INVALID_LEDGER | 符合 |
| 分相 N/A | 层 A（EL=0 → light N/A）+ 层 B5（EH=0 → heavy N/A，新量仍由 EL 计算） | 符合 |
| 0.5 及格线与分档 | 层 B8：M=0.5E 恰等 → PASS score 6；层 A：分档 6 分一致；无新容差 | 符合 |
| 共同 250ms 双算 | 层 A：2 个共同样本，newPeak 0.48 / historicalPeak 0.35，`OBSERVED_SAMPLED_MAX=0.48` 为下界 | 符合 |
| legacy 原式不变 | `legacyPeakSuspReported` 原值透传（层 B14：0.8）；`legacyPeakHistoricalReference`=max U/(D+U)=0.4；`legacyPeakLiveFormula`=max U/max(D+U+P,EPS)=0.32；`sameReferenceSuspOnlyPeak`=max U/E=0.32 | 符合 |
| 差异归因分离 | 层 A：窗口 0.45 / 密度 0.04 / 分子分母 0.13，与交付值逐一相等 | 符合 |
| 状态分流与判定顺序 | 层 B：覆盖缺口/窗未覆盖/早 off → INCOMPLETE；时钟回退/重复键/死槽计数 → INVALID_LEDGER；未见过假 PASS | 符合 |

层 A 关键复算值（全部与交付一致）：峰 `peakNonDepositedFraction=0.52`、`peakStockMass=520`、`peakReferenceMass=1000`、峰键 sequence 3 / order 4 / t=200ms、窗内样本 3、`peakStatus=PASS`、`peakScore=6`、`baselineNonDepositedFraction=0.8`。

## 4. 观察项（不改变判定，交责任方）

- **O-1 峰值覆盖与 endpoint stamp 成功耦合**：`recordPeakCommit()` 仅在 endpoint post-commit stamp 成功后调用；stamp 失败分支只推进 endpoint 序号，峰值该提交样本缺失。后果：窗内序号跳变被 checker `COMMIT_COVERAGE_GAP` 捕获 → INCOMPLETE，**不会产生假 PASS**；且该提交的 endpoint 账本样本同样缺失，run 已受损。属覆盖保证的稳健性改进项，责任方 T08 developer（`6adcd3e6e4b06ee357206d26`）， informational。
- **O-2 交付夹具的右边界无判别力**：双算夹具中 t=3100 记录的 M=0，包含它不改变峰值，边界排除在该夹具上不可观测（开发者 Python P8 已用高值记录覆盖，本报告层 B9 再次以 M=900 覆盖）。仅夹具增强建议，非实现缺陷。

## 5. 边界与未涉及

未使用真机；未改实现/判据/协议；未宣布 `peakProtocolEffective`（manager 裁决项）；未阻塞 T20/T34/T36；DUALFIELD-* estimand 未触碰；本报告不影响 endpoint 协议任何条款。

## 6. 复跑

```bash
python test/p038_peak_independent_verification.py          # 46/46
python test/p011_peak_protocol_v1_execution.py             # 8/8 反例
python -m unittest test.test_p011_peak_check               # 13/13
node test/run_p011_peak_units.cjs                          # 8/8（真实 ArkTS 采样器）
```

- 2026-10-25，v1：首次发布。三层独立验证 46/46 + 预注册反例 8/8 + 实现级最早达峰身份确认；结论 PASS，观察 O-1/O-2 交责任方。
