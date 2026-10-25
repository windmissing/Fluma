# 后续工作清单 · 固定编号台账

> 建立人：manager。2026-10-25 09:10。
> 规则：**每个 task 一个固定编号，编号终身不变**（卡关闭后编号退役，不复用给别的卡）。
> 每轮清单**按优先级排序**（P5 → P4 → P3），因此**编号在清单里的出现顺序可以乱序**——编号只用于跨轮次指代同一件事，不是排名。

## 编号 → task 对照

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T01 | D1-R3 M1 生产热路径热接口实现（零分配收口）·S1 已过 | developer | P5 | 6adcc2c1…1cf05 |
| T02 | D2 M2 场集成（九接入点 + 账本 v7 + 13 项单测 = 隔离原型出口） | developer | P5 | 6adc52e4…2a86 |
| T04 | 终点指标 D-4/O-2 修复（D-4 已落，O-2 未落） | developer | P5 | 6adcd497…0569 |
| T05 | 掉沙接线修复入库（六项已落地并通过自测） | developer | P4 | 6adca854…cdcc |
| T06 | peak 协议（独立 protocol + 适配清单 + RED 基线） | tester | P4 | 6adcd3ba…f8c8 |
| T07 | peak 字段 revision（shake-on/off + 子步消费身份） | architect | P5 | 6adcd3d3…51f1 |
| T08 | peak 实现（采样 + 流式峰值 + 双算对照） | developer | P5 | 6adcd3e6…d26 |
| T09 | P1 CPU fixture + 五量 RED 基线（G1 解冻前置之二）·零交付待开工 | tester | P5 | 6adcc00b…0dda |
| T10 | 掉沙 V1-V4 真机返修复测收尾（真机队列第 1 位） | effector | P5 | 6adcd2dd…cb99 |
| T11 | P6 endpoint 合规设备 run（协议生效硬前置，真机队列第 2 位） | effector | P4 | 待建 |
| T12 | S4 sweep 三轮（先冻结 μ_y 量纲定义） | developer | 0 | 6adcd331…a94b2 |
| T13 | 方案3 b 起沙链 B-1/B-2/B-3（已落地待入库） | developer | P4 | 6adc6272…f9dd |
| T14 | 大角度 B：±45° 三测量对象形式化 + 300/s 可达性账本 | designer | 0 | 6adb6d3d…649b |
| T15 | 大角度 C：新 route 客观用例预注册 | tester | 0 | 6adb6d3d…88ef9 |
| T16 | 大角度 D：真实观感用例预注册 | effector | 0 | 6adb6d3d…538e |
| T17 | 大角度 E：新 route 工程化复审 | architect | 0 | 6adb6d3d…7d49 |
| T18 | 大角度 F：默认关闭新 route 实现 | developer | 0 | 6adb6d3d…649c |
| T19 | 大角度 G：只读审核与提交 | reviewer | 0 | 6adb6d3d…88efc |
| T20 | 【T20·D4/D5粒子视觉接线】developer：真实粒子快照渲染、落床可见性与B tracer退出 | developer | P5 | 6adda102e4b017ab33e90fbf |
| T21 | E2 出数 / 参数面板 / S3-S4 真机 | effector | — | 待建 |
| T22 | 方案3 悬浮平流启动裁决 | manager | P3 | 6ad5f713…9cb2 |
| T23 | P1 G1 解冻放行 | manager | P4 | 待建 |
| T24 | P1 实施（G2–G5） | developer | — | 待建 |
| T25 | 掉沙链主观验收闭环 | effector | — | 待建 |
| T26 | goals.md 四子项变更落地 + 连带口径登记 | designer | 0 | 6ad9af6a…055b |
| T27 | P-012 C 补采（层厚增减 mask 裁剪 + DPI 缩放呼吸检查） | effector | 0 | 6ad982ef…f0be |
| T28 | S4 物理定义（pEff(Pa)/rhoEff/参考压力 + 量纲闭合表） | designer | 0 | 6add5bda…8d7b |
| T29 | P1 G0 真机 probe（surface/sRGB/三层z序/透明/输入透传） | effector | P5 | 6add6012…830b |
| T30 | T01后续客观端到端验证（原T01-V-O） | tester | P5 | 6add769ce4b017ab33e71bd7 |
| T31 | T01后续主观端到端验证（原T01-V-E） | effector | P5 | 6add769ce4b06ee35724ea6b |
| T32 | G0帧桥拒绝与绑定时序修复 | developer | P5 | 6add77dfe4b03273ad6591a2 |
| T33 | G0叠层/alpha/色彩工程定案 | architect | P5 | 6add77dfe4b06ee35724fd94 |
| T34 | G0叠层/alpha/色彩实现 | developer | P5 | 6add77dfe4b012783274cf8e |
| T35 | G0补证复测 | effector | P5 | 6add77dfe4b06ee35724fd95 |

## 已关闭退役编号（不复用）

2026-10-25全局重排新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T36 | 【T36·触摸注沙速率修复】developer：统一时间单位、首帧与停止边界，消除43倍注入放大 | developer | P5 | 6adda18ae4b03273ad679625 |
| T37 | 【T37·双场到GLES接口适配】architect：同帧高度场和真实粒子shader数据通路 | architect | P5 | 6adda18ae4b0e6ce82af0d63 |
| T38 | 【T38·peak独立客观验证】tester：同账本双算、采样边界与反例回归 | tester | P5 | 6adda18ae4b03273ad679626 |

T02、T08、T13按滴答完成状态登记完成，不增加manager验收；端到端/后续验证另卡推进。

| 编号 | task | 关闭时间 |
|---|---|---|
| T37 | 双场到GLES接口架构（6089f98，交原T20/T34接力） | 2026-10-25 |
| T32 | 帧桥修复开发；按用户通知完成登记 | 2026-10-25 |
| T07 | peak字段revision三项对齐（61e5ac4） | 2026-10-25 |
| T33 | G0叠层/alpha/色彩工程定案（e287c47） | 2026-10-25 |
| T01 | M1开发、编译、离线自测（端到端转T30/T31） | 2026-10-25 |
| T09 | CPU fixture与五量RED基线 | 2026-10-25 |
| T10 | 掉沙返修复测取证交付 | 2026-10-25 |
| T29 | G0探针取证交付（G0未全过，后续T32–T35） | 2026-10-25 |
| R01 | 终点指标双报告 | 10-24 22:50 |
| R02 | E4 沙丘占比量测方案 | 10-24 22:50 |
| R03 | 终点指标缺陷修复 D-1/D-3/D-4/D-5 | 10-24 23:30 |
| R04 | 终点指标协议勘误 errata1 | 10-24 23:29 |
| R05 | 终点指标字段对齐（D-2/D-6 裁定） | 10-24 23:32 |
| R06 | 终点指标双报告补充1 | 10-24 23:58 |
| R07 | D1-R3 迁移设计（architect） | 10-24 23:41 |
| R08 | t90 epoch 修复 | 10-24 23:41 |
| R09 | S4 代理升级建议（architect） | 10-24 23:41 |
| R10 | E1 C 层理锚点 v2（designer） | 10-24 23:18 |
| R11 | 大角度 A：新 route 登记（manager 自办） | 10-24 23:48 |
| R12 | peak 协议（独立 protocol + 适配清单 + RED 基线 + §15 用例条目） | 10-25 09:30 |
| R13 | 掉沙接线修复入库（六项 + 单测） | 10-25 09:44 |
| R14 | 终点指标 D-2/D-6 实施（诊断三具名 + 事件级校验归 checker） | 10-25 09:46 |

## 未编号的一次性工作

不占 T 编号（无独立跟踪价值，或完成后即消失）：
- 终点指标双报告勘误与补充（已并入 R04/R06）
- 大角度 A 卡正文改写与 sortOrder 修正（manager 自办）

## 使用规则

1. 每轮输出清单时，**沿用本表的编号**，不重新编号。
2. 清单**按优先级排序**（P5 在前），所以编号顺序与优先级顺序**不一致是正常的**。
3. 新卡入表时**取下一个未用的最大编号 +1**，退役编号不复用。
4. 卡被关闭 → 在「已关闭退役编号」表补一行，编号退役。
5. 用户按编号指代时（如「T05 验收」），直接查本表定位卡。
