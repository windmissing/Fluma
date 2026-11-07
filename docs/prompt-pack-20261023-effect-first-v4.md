# 2026-10-23 效果优先改排 v4：离床沙的真实运动

日期：2026-10-23。原因：用户重申**先效果、后效率**；大角度另行调研，correction 已弃用。本稿替代 [v3](prompt-pack-20261023-reordered-v3.md) 中“E3 帧时复测排第一”的建议。状态：**SUPERSEDED / NOT_FOR_DISPATCH / NO_AGENT_LAUNCHED**。用户再次指出本稿把早已讨论的“高度场＋粒子”重新包装成新 gap；下方 A/B/C 重选路 prompt 撤销。现有 [B 路线架构](architecture-P011-falling-sand-B-route-v1.md)已登记 manager 10:26 裁决 **B 首版、C 后备**，且 B 的注入/tracer 已有实现。本稿保留作历史纠错，不改 manager 独占的 `priority.md` 或 tester/effector 用例。

## 为什么选这个技术 gap

现行 `WaterSandField` 的离床沙仅为每列 `suspH/suspL` 总量，没有高度、速度和空气/水介质身份。[S3 设计 §9](algorithm-P011-S3-moisture-grading-v1.md)已指出：无自由水的干沙也可能被记入名义“水中悬沙”；潮湿团簇和被淹没颗粒均无法由同一列总量表示。[掉沙三路线设计](design-P011-falling-sand-vertical-routes-v1.md)给出 B 只读 tracer 与 C 真粒子+高度场，但 C 仍待 manager 选路，B 不能证明粒子落底与沉积同一物理事件。这一缺口同时影响手指掉沙、摇晃扬沙和干/潮湿/水下效果。解决方向是：**大宗沉积层仍用 128 列高度场，只让离床且需要可见轨迹的少量沙取得独立、有质量的活动状态**；具体表示与范围需设计/评审，不能把候选 C 写成已批准实现。

## 当前工作顺序

1. manager 登记“离床沙运动”效果 gap，选最小可验证场景；tester/effector 先预注册安全与观感用例。
2. designer 在 S3 §9 与掉沙三路线基础上统一干/潮湿/水下身份、床↔活动层质量转换和最小版本；manager 据对比裁决选路。
3. architect 只在选路与用例就绪后细化接口；developer/reviewer 再按正常流水线接力。大角度保持调研；correction 保持归档。
4. E3 帧时修复 `60d24bf` 的复测作为**后续性能门**。设计、离线原型、效果场景定义不等它；真机若仍约 9–10fps，短时粒子录像标 `EFFECT_EVIDENCE_BLOCKED`，可用隔离效果构建做探索，不能把探索结果冒充产品构建验收。

## 可复制 Prompt

### manager：效果 gap 与选路门

> 你是 P-011 manager。用户已定“先效果、后效率”；大角度另行调研，correction 已决定不用。读取 `docs/algorithm-P011-S3-moisture-grading-v1.md` §9、`docs/design-P011-falling-sand-vertical-routes-v1.md`、`docs/algorithm-P011-gesture-sand-source-v1.md` 及最新效果报告。把“离床沙没有可见真实轨迹和介质身份”登记为本轮效果 gap：覆盖手指注沙、干沙摇晃、潮湿团簇、水下扬沙，但一轮先选最小可验证集合，避免同时实施四种机制。先派 tester/effector 补可验证用例，再派 designer 统一方案；对 B 视觉 tracer、A 有质量 cohort、C 有界真粒子的目标适用域做选路裁决。不得把 B 的视觉通过写成 C 的物理通过；不重启 correction，也不将 E3 帧时优化设为设计前置。只维护 goals/gap/priority，若需修改冻结目标再提交用户裁决。

### effector：先定义用户看见什么

> 你是 P-011 effector。固定同一摇晃强度、沙量、初始床形和画面档，定义干、潮湿、被水淹没三场景，以及手指注沙的短时掉落场景；分别记录起沙延迟、可见颗粒/团簇、运动路径、落地或浑浊回落，以及同屏云雾/沙面遮蔽。给区分四种效果的主观量表与可复现录像窗口，数字阈值仅作候选，采前再冻结。B tracer 的录像可以作视觉探索，但须单列“与真实沉降错时”；不据 tracer 判真粒子库存。真机帧率不足以辨识时明确 BLOCKED，可先用隔离低负载效果 demo 探索，不把它冒充正式产品效果 PASS。维护自己的 effect-cases/报告/素材，不改物理代码。

### tester：先写守恒与转换用例

> 你是 P-011 tester。为“沉积床↔离床活动沙↔沉积床”设计客观用例，覆盖 H/L 守恒、外源注入只加一次、干态不得记为水下 susp、潮湿团簇的 H/L 分账、水面跨介质、落床失败余量保留、粒子池满额降级、跨列预算、§19.5 guard、preview/apply、reset/invert/TEST/EXPERIENCE、OFF 旧轨迹不退化。用例分别标 A/B/C 适用路线，B 只读 tracer 不额外计质量，不能拿 B 的外观测试替 C 的物理断言。未实现时出 RED/NOT_IMPLEMENTED 基线，先不改 checker 判定使旧 run 变绿。维护 test-cases/测试代码/独立报告，发现目标歧义交 manager。

### designer：统一最小算法版本

> 你是 P-011 designer。读取 S3 §9、掉沙三路线设计、方案 3 悬浮平流设计与已实现的 gesture injection。比较 A 有质量列内 cohort、B 无质量只读 tracer、C 有界真粒子+128 列沉积床在四种效果场景的能力；给最小版与后续版的明确范围。必须定义每相库存唯一所有权 `deposited/air/aggregate/waterParticle/susp`、源准入、摇晃起沙、跨介质、移动、撞床、被 guard/预算截断后的去向和质量闭合式。给伪代码、复杂度、粒子/包数量扫描域与可测观感指标；数值参数标待定标。说明与 S3 首版湿度代理、方案 3 仅搬已有悬沙的接口，不把湿度代理称为已标定真实含水率。只写 P-011 算法/复盘文档，交 manager 选路；不写代码或改测试阈值。

### architect / developer / reviewer：条件接力，不立即启动

> 只有 manager 完成选路、designer 算法稿与 tester/effector 用例就绪后，architect 才细化状态字段、快照、预算/guard、P-012 只读渲染 adapter、生命周期和回退；developer 再做 routeId 隔离的最小实现并完成构建/运行；reviewer 核对算法/架构/测试一致性。正式产品效果仍须之后的 tester+effector 双报告。三角色当前均无立即派发任务。

## 仍需保留的约束

- 不提高 `300/s`、不放宽 tan30 writer guard、守恒/库存/容量/有限值，旧 route 与历史报告身份不变。
- 方案 3 advection stage-a 已有代码，仅搬已有 `susp`；不能自行生成干沙空中粒子或潮湿团簇。
- S3 湿度首版已实现，可按现有用例独立验收；它不代表三工况摇晃已完成。
- E3 帧时回归保持独立 OPEN 问题，效果设计先行；正式产品体验验收时仍须有可辨识的真实帧序列。
