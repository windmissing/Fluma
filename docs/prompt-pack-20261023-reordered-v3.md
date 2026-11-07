# 2026-10-23 流水线改排 Prompt v3：撤销 correction 提数与大角度实现排期

日期：2026-10-23。原因：用户明确纠正：**correction 方案已经决定不用**，且大角度算法尚需精读和可达性研究。[v1](prompt-pack-P011-large-angle-v1.md)过早排完整实现链，[v2](prompt-pack-20261023-reordered-v2.md)误排 correction 正式提数，均已标 `NOT_FOR_DISPATCH`。本稿状态：**SUPERSEDED / NOT_FOR_DISPATCH / NO_AGENT_LAUNCHED**。用户进一步重申“先效果、后效率”，v3 将帧时复测排第一仍不符合当前排序；现行建议见 [v4](prompt-pack-20261023-effect-first-v4.md)。任务分工遵循 [.codex/PIPELINE.md](../.codex/PIPELINE.md) 与 `.codex/agents/*.toml` 的文件所有权。

## 本轮取舍

| 方向 | 处理 |
|---|---|
| correction | `ARCHIVED / NOT_PROMOTED`。82/108 run 封存资产保留，**不补采、不正式提数、不排转正评审**。后续工程以自然 relax 基底进行；历史裁决和原始资料不删除。 |
| 大角度 | `RESEARCH_ONLY`。外部姿态短暂超角语义已批准，完整的移动层机制、300/s 可达性、恢复律未证；不派 architect/developer/reviewer，不改 checker。 |
| P-012 E3 性能 | 修复 commit `60d24bf` 已入库，先做同口径复测。 [五件套真机回执](window-receipt-5in1-20261023-v1.md) 的 s134 约 9–10fps 是修复前证据；新结果不能预判。 |
| P-011 S3 湿度首版 | 算法、架构及实现 commit `01cc503` 已有；做独立回归。局部渗透与三工况摇晃粒子仍不在首版适用域。 |
| P-011 掉沙观感 | 物理注入与 tracer 已有代码；V1–V4 因 9–10fps 证据不可判。仅在 E3 帧时恢复后重录，不重复派开发。 |

## 可直接派发的 Prompt

### 1. manager：更新队列（先发）

> 你是本项目 manager。读取本稿、`docs/priority.md`、`docs/gap.md`、`docs/window-receipt-5in1-20261023-v1.md`、`docs/algorithm-P011-S3-moisture-grading-v1.md` 与 `.codex/agents/manager.toml`。按用户最新裁决，将 correction 路线标为已决定不用：保留历史报告和 82-run 封存数据，但**不再安排提数、补采、转正或以它作为 S4/方案 3 的必需前置**；后续 P-011 以自然 relax 基底工作。大角度只留 designer 精读和离线可达性研究，不派 architect/developer/reviewer。当前优先安排：①tester 对 `60d24bf` E3 帧时修复做同口径真机复测；②tester 对 S3 湿度首版做独立回归；③E3 帧时恢复后 effector 重录掉沙 V1–V4。为每项在你独占的 priority/gap 标明前置、负责角色、报告位置和共享设备执行顺序；如已有同等最新回执，先核对并跳过重复任务。阶段一 CLOSED、预算/守恒和阶段二 ±45° 效果目标不改。不要写算法、代码或测试断言。

完成条件：priority 中 correction 无活动任务，大角度无实施接力，本轮只排有现成实现或修复可验的事项。

### 2. tester：E3 性能修复复测（先于视觉重录）

> 你是 P-012 tester。读取 `docs/window-receipt-5in1-20261023-v1.md` §2、`docs/fps-seg3-P012-v0.md`、E3 修复 commit `60d24bf` 与现有 seg3 原始数据。确认实际被测 HAP 的 commit/源码 hash，隔离工作区未提交改动；同设备、同热态分层、同 settle/tilt/heavy 窗口，做修复前后 paired 帧时复测，报告 FPS、phys/render/present P50/P95、q dirty 命中/重烘频率及 E3 J95/J90/std 默认锚点是否退化。若 s134 的 9–10fps 或 dyn 消融异常仍存在，写 `PERF_BLOCKED` 和定位证据；不得以视觉分数代替帧时，也不得把 E3 三锚点通过外推为全量 VALIDATED。维护测试代码/资产和 P-012 独立 test-report；不要改产品代码。

完成条件：修复效果有同口径、同构建身份可追溯数据；明确掉沙重录是否具备可靠帧率前置。

### 3. tester：S3 湿度首版回归（可离线先行）

> 你是 P-011 tester。读取 `docs/algorithm-P011-S3-moisture-grading-v1.md`、`docs/architecture-P011-S3-moisture-v1.md`、实现 commit `01cc503` 与当前工作区改动。先固定被测源码身份，运行或补齐首版用例：`moistureRoute=OFF` 旧轨迹位精确；ON/default-reference 与 26/20 旧行为全轨迹等价；不同水量和高度格能改变局部 pair；start/stop 顺序与 tan30、300/s、H/L/水守恒、preview/apply、reset 和有限值不退化。报告只判首版湿度→双角。局部渗透求解器、干/潮/水下摇晃三工况及粒子轨迹列 `NOT_IMPLEMENTED / OUT_OF_SCOPE_FOR_V1`，不得因首版通过而追认。写入 tester 自己的 test-cases/测试代码/独立报告；共享真机安排在 E3 性能复测之后。

完成条件：首版验证结论与高级版缺口分开，原始输入和构建身份可复核。

### 4. effector：掉沙 V1–V4 效果重录（条件触发）

> 你是 P-011/P-012 effector。前置：tester 已证明 E3 修复后的真实帧率足以可靠观察短时 tracer；若未证明，继续保持 `EFFECT_EVIDENCE_BLOCKED`，不要采一批同样无法判读的图。满足前置后，在同一真机按 `docs/window-receipt-5in1-20261023-v1.md` §3 原 V1–V4 固定场景重录，并记录喷流是否可见、方向、落点、与真实沉降的时间差以及被拒注入是否无喷流。保留旧 9–10fps 批次“证据不足”的历史状态。维护 effect-cases、素材与独立报告；不要改物理、渲染或客观锚点。与 tester 的设备执行串行。

完成条件：V1–V4 每项有可复现的帧序列和主观判定；无可靠帧率则明确 BLOCKED 而不假 PASS。

## 大角度调研 Prompt（不在本轮实现队列）

> 你是 P-011 designer。等外部智能体的原文精读材料回来后，核对静止/流动层交换、倾角阶跃瞬态与起停滞回的公式和适用条件；独立计算 128 列、300/s、5.1s 下局部崩塌、全宽沉积坡、raster OLS 的质量/一阶矩/跨边预算，区分流动层与沉积终点，给 `0→±45°` 前几个子步、1s 和 5.1s 的条件性预测。保留已批准的外部姿态短暂超角语义，但不能凭该语义宣称材料能跟上。至少比较两种候选机制；若无真实可提交构造，形成量化约束冲突交 manager。只写 P-011 算法/复盘文档，**不交 architect 实施、不改代码、测试用例或 priority**。

完成条件：每个效果判断都有可复核的库存、通量、预算和目标几何；引用论文不能代替项目内可达性证书。

## 后续候选，当前不抢排期

- P-011 方案 3 advection 的 a 段架构仅承诺“搬已有悬沙”，可以在当前验证项后排开发；b 段起沙需 P-3 manager 另批，不借 correction 快车道。
- S4 已有离线原型与 sweep；主线结合 C 层理时按自然 relax 基底，不再等待 correction 提数。需要独立 S4×C 效果与安全验证后才谈产品转正。
- P-013 换栈/E2 仍按自己的前置推进；E3 帧时回归未清前不从 s134 的异常值推算换栈收益。
