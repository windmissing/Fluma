# T369 停源新生粒子同包真机补证（T367 cohort 装置，逐 ID 锚点）（真机，完成——冻结未复现、沉降持续中、29.3 物理秒在案）

2026-11-06，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：**T367 交付签包 SHA256 `e20d4dddfdc0215a127e4937dccd0b3743fd099a27342a4cc59c591435a66dad`**（离线 9/9；T367_STOP_COHORT_V1 装置：锁定新生离床 ID 逐 ID 三锚点 stop/first/last + 72 词行 63 字段）。依据 T353 §6.2。

## 1. 臂执行（10 分钟上限内）

- 单 run（`p011-run-1793932161133-3`，cohortEpoch 2，admissionTransactionId 11），V2 入口冷启 pid 30225。
- 真实触摸 6×1.5s 出生 → UP（stopReason=UP，stopTimeMs 1793932191817）→ 90s 沉降观测 → 右倾 setTilt → 前后台恢复 → 三次冷导出（10:31 / 10:33 观测 / 10:37 终态）。
- 零 jscrash/THREAD_BLOCK；scene CURRENT 201/201。

## 2. Cohort 锚点（ids [86,87,88]，三粒同快照出生，独立解码 F64 位型双词）

| 锚点 | y | vy | 模拟秒 | motion | state | 床 |
|---|---|---|---|---|---|---|
| **stopAnchor**（UP 切点，frame 353/commit 717） | **147.2425** | -0.2829 | 11.73 | 94 | 1 | 0 |
| **firstAnchor**（首成功子步，frame 354/commit 718） | 147.2355 | -0.4244 | 11.75 | 79 | 1 | 0 |
| **lastAnchor**（末端，frame 1233/commit 2475） | **31.399** | **-44.2741** | **41.03** | 0* | 1 | 0（未触床）|

*lastAnchor motionCount=0 为该切点行采样的增量计数（非累计），y/vy 为权威位型。

## 3. 判定

1. **T350 冻结未复现——粒子持续主动沉降**：UP 后 29.3 成功物理秒（11.73→41.03，≥8s 判据以物理秒满足）内 y 从 147.24 降至 31.399（**116 units 主动下落**），末端 vy=-44.27 仍在下落（非冻结、非悬停）。
2. **全额 LAND 未达（如实）**：臂上限（10 分钟墙钟）内未触床——1.2fps 下模拟时间累积极慢（T347 慢速沉降机制），沉降尚需更多模拟秒；不冒完成。
3. live 曲线：98→94→90→87（UP 前后分批退役，与 T360 臂1 同形态）；终态 87 恒定（观察窗内无泄漏）。
4. setTilt 流向：finalAcceptedDeg=30.0（+15 请求被限幅合同钳位，与 T350 R8/T360 一致，归 T353）。
5. 装置语义：72 词行 63 字段含 mass/bedHeight/netHeight/cumulativeAcceptedMass/finalAcceptedDeg/bedMassDelta 全链证据；stop/first/last 三锚点逐 ID 独立可复核（回 T353 分流）。

## 4. 移交 T353

- T350 冻结在 T367 同包装置下**未复现**（T360 两臂 + T369 本臂三锚点均显示持续沉降）——冻结维持"条件特异/转录级证据"（T358 裁定），归因可聚焦 T350 特有条件（多次 run 重启残留）。
- **新定量事实**：UP 后沉降速率极慢（29.3 模拟秒降 116 units ≈ 4 units/sim-s，而 vy 字段读数 -44.27——**vy 单位与位移速率的对应关系需 architect 判读**（候选：per-substep 系数/单位不一致），此为新增可判别证据。

证据：`test/reports/t369/`（v2-budget-t369.json `65819da5…8d39`、v2-budget-t369-final.json `24a7c533…fb00d9`、hilog_full.txt 201 scene 行）；签包 T367.hap。
