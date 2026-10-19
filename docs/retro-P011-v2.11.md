# P-011 v2.11 复盘

> 日期：2026-10-19。候选路线：`gravity-advection-v2.11`。状态：`DESIGN_REVIEW_REQUIRED`（外部评审发现冷启动、符号与局部通量散度未闭合；不交 architect/developer）。

## 1. 本轮输入

- v2.10 观测报告：`docs/observation-report-P011-natural-observation-v2.10.md`
- 证据目录：`test/artifacts/p011-v210-natural-observation-20261019-run7/`
- 关键判定：`ruleDriveTooWeak`
- 关键事实：`A_B=0 px`，模型单位最大位移 `4.2833e-4`；`+15/-15` 两相位 `262/262` 子步全部受限；重沙仅 `bottomCreep` 约 `5` model units，其余重沙通道全为 `0`。

## 2. 候选与取舍

### 2.1 双角度滞回

- **结论**：不采用。
- **被否原因**：观测显示 `relaxAngle` 已持续请求并应用，问题不在触发突然，而在重沙无法进入有效搬运通道；引入第二角度还会带来新的内部阈值语义，容易与“不改门槛”约束冲突。
- **保留条件**：若 v2.11 出现坡度振荡或与 `relaxAngle` 冲突，再单独评估。

### 2.2 重力直接驱动项

- **结论**：采用。
- **选型理由**：方向正确、非自然平衡、瓶颈在重沙可达性与 bottom 限流；重力项可把 bottom 通道改为由局部坡度与重沙富余共同驱动的重/轻对向交换，且不改预算、几何、门槛、守恒或崩塌约束。
- **核心机制**：`potential = surfaceDrive + heavyExcess`；`q` 受 `bottomCreepVelocityGain` 与 `vBottomCreepMax` 约束；交换后每列 `bh+bl` 不变，`sandTop` 不变。

## 3. 暴露的问题

1. 现有 `bottomCreepVelocityGain` 参数未被使用，但测试 `p011_velocityGainDoesNotOverrideShapeRateCap` 断言其无状态影响；该测试属于旧契约，须由 manager/tester 修订，不能由 designer 私自改。
2. v2.10 证明 top/slip 只搬轻沙，`bh` 可达性是关键瓶颈；任何只改表层崩塌的方案都难以解决重沙界面运动。
3. 直接增加预算或放宽门槛不属于可选项，必须在现有约束内改物理。

## 4. 下一版方向

1. 若重力交换产生可见重沙界面位移但 `t90` 未达 100ms，优先诊断 bottom/total 预算竞争与接口扫描顺序，不直接调预算。
2. 若出现振荡或与 `relaxAngle` 冲突，再评估双角度滞回，作为独立候选，不与重力项混合。
3. 若 tester/effector 发现观测证据或主观基线与本设计冲突，先修订设计，不通过放宽门槛或改预算解决。

## 5. 外部专家建议复核与设计师判定

外部评审 §12.4 指出四项问题：均匀初态的冷启动势为零、可见性依赖未经标定的 model-unit→pixel 假设、原 bottom 速率上限仍在、`surfaceDrive` 与 `heavyExcess` 的物理意义及相对尺度未交代。前两项属于进入 architect 前的阻断问题；后两项必须纳入量纲与 raster 证据，不能用请求量/总 heavyFlux 代替局部界面位移。该评审的完整原文见 `docs/review-external-20261014.md` §12.4–§12.5。

### 5.1 冷启动与符号复核

默认 thick 场景实际由 `SandCanvas.waterSandScene()` 初始化每列 `bh=72, bl=72, mh=ml=suspH=suspL=0`，`sandTop=144`。原 v2.11 公式第一个 collect plan 因而在任意接口上都是 `surfaceDrive=0, heavyExcess=0, potential=0, q=0`。重力 plan 首步不启动是代数结论；既有 apply 通道可能改变后续状态，因此文档不把“永久死锁”冒充已证明结论。

方向也未闭合：`relaxAngle()` 的目标为 `S[c+1]-S[c]=tan(effectiveTilt)`，但原 `sign(tilt)*(S_donor-S_receiver)` 在对齐该坡度时给负 surfaceDrive。算法 §8.9 记录了带重力坡面残差的候选 `surfaceHead`，只作为下一轮推导，不作为已批准公式。`heavyExcess` 的质量差虽与列高使用同一 model-unit 数值体系，作为重力/浓度势的权重仍未由密度差或侧压机制推导。

### 5.2 可见性与 grant 分配

即使给 uniform thick 初态每条边相同正 potential，纯局部计划会对边产生相同 q；内部列 `Δbh=q_left-q_right=0`，总 H grant 不能直接换算成中央 local-boundary 位移。若按 v2.10 实测 bottom 总 grant `10.02` 均分 127 边，理想单边 heavy `q≈0.0395` model unit；ASSUMPTION-3 对应约 `0.079px`，且像素换算尚未标定。该数不是任意自适应 grant 的全局上界，却足以说明原方案没有证明局部 `≥2px`。

因此 v2.11 仍需从现有物理状态推出空间非均匀的边通量与中央 ROI `bh` 离散散度；不能指定目标曲线/green mask，也不能提高 `10*dt` 或 `300*dt`。原 heavyExcess 与坡面项直接相加还需给量纲与相对权重依据。局部 raster、正负倾角的对称性、预算竞争及 `t90` 都须有量化预测或归入实测 gate。

预算插入顺序还有一个具体风险：v2.10 两个有效相位共 `262/262` 子步触发 bottom 限流，正相位 `bottomUsed=10.0245`、负相位 `9.7183`。architecture §18 把 gravity apply 排在 legacy `creepBottom` 后，并让 gravity 只拿 `bottomRemaining`；所以不能默认 gravity 得到额外的 `10*dt`。若继续 v2.11，必须先定义 legacy/gravity 如何在同一独立 bottom cap 内联合分配，并量化 legacy 被挤占的 grant及 total-budget 竞争；否则该 route 可能账面有 gravity request，实际 `q=0`。

### 5.3 流水线处置

v2.11 的守恒不变量和“重沙向一边、轻沙反向交换”仍有设计价值，但目前没有证明默认场景能启动、中央 ROI 能产生不抵消的位移或达到局部 2px，也未解决 legacy bottom 饱和后 gravity grant 的额度来源。因此算法 §8 的 `READY_FOR_ARCHITECT` 与架构 §18 的 `READY_FOR_DEVELOPER` 暂停生效；以本复盘和算法 §8.9 的 `DESIGN_REVIEW_REQUIRED` 为当前状态。designer 下一轮须回答外评两问，并给出首子步、1s、5.1s 状态/预算账本、联合预算分配和非均匀 grant 依据。未完成前不进 architect/developer；不改测试文件、不放宽阈值、不启动 tester。

## 6. v2.11-R1 数值复核结论（2026-10-19）

算法 §8.10 已写入可复算候选：`D_s=sign(θ)*(tan(θ)dx-(S_R-S_L))`，`P=D_s+(bh_d-bh_r)`，`q=min(36*max(0,P)*dt/2, vBottomCreepMax*dt/2, stock)`。它在平坦默认初态将倾角显式带入推力，并修正了旧 `surfaceDrive` 的符号；heavy 组成项权重仍是未标定假设。

以正式首个有效 `dt=0.00835s`、`+15°`、bottom 额度一半分给 gravity 重算：首步 `P=0.267949`、每边 `q=0.00016437`、端部位移 `0.000266px`、中央为 0；1.002s 后中央 104 列仍为 0、全场最大 `bh` 变化 `0.01905`（`0.0309px`）；5.10185s 后中央仍为 0、全场最大 `bh` 变化 `0.08561`（`0.1387px`）。raster 标定为 `1.6197917px/model-unit`，局部 2px 需 `|Δbh|≥1.23472`。这是同步逐边公式推演，不是设备保证；已足以否决“均匀 grant 达成中央 2px”。

额度只能共用现有 `10*dt`：legacy 与 gravity 依各自申请量按比例分享该 cap，未用份额可由另一方用，单列/stock/total `300*dt` 再截断，不重发被截额度。这个方案会压缩 legacy grant；将全部 bottom 给 gravity 也没有解决中央传播问题。

因此此轮交付的是公式与有数值的否证，不是通过方案。`gravity-advection-v2.11` 仍为 `DESIGN_REVIEW_REQUIRED`，architecture 的 developer-ready 状态不生效；没有中央非均匀边 grant、2px 及实际 bh release 证明前，不应开工。代码、测试用例未改。
