# T243 V2 床沙不可见与低帧率首次归因

2026-10-29，architect；原卡 `6ae28b60e4b0c3103c1983fb`。消费 T218 正式报告、截图归档索引与 V2 页面/facade/publication/渲染准备源码。**结论：现有设备证据不足以判 LAND、publication 或 renderer 缺陷；低帧率导致物理时间严重少于墙钟时间，是必须先扣除的明确工程边界。** 不使用真机，不重跑测试，不改业务代码。

## 证据与可推导范围

T218 11 次 inject 均成功（单次约 0.54–1.34 mass）、AIR 粒子在出生点附近可见，P013 `CURRENT/swap=true`，frame204→236 与 physicsCommit421→485 都前进，证明注入、某些粒子投影及整帧交换正在运行。它们**不证明**粒子已 LAND、`bh/mh` 已非零，也不证明床层 shader 对非零床量可见。

`pages/P051DerivedV2Demo.ets` 每 frame 送入 `dt=min(1/30, wallDelta)`，首帧为 0；`P051DerivedDemoV2.tick` 在每帧最多调用一次 owner.step(dt)，但 source 注入按手势时钟采样墙钟秒。T218 0.5–8fps 下约 20s 墙钟可能仅约 10–160 次 step，即最多 0.33–5.33s 模拟时间；所报 204→236 的 32 帧窗口最多 1.067s。出生 y≈220、重力约 9.8 单位/s²的自由落体粗估到 y=0 需约 6.7s（实际受碰撞/初速/倾角影响），因此“注入约 50 mass、等 20s 必有约 48 高床”不能由现有数据推出。物理 commit 每帧 +2 还包含 source 与 step，不能当模拟秒数。源码 `copyPublishedRenderStateInto` 在 V2 走 `DerivedCommitV2.copyPublishedInto`；`fillDerivedPublicationV2` 包含 field 与 pool，P013 准备读取 `bh/mh`，未见可直接认定的漏拷字段。仅凭截图不能排除投影/材质/着色层故障。

## 精确补证及分流门

优先交 **tester 真机补证**，对原隔离 V2 route 加只读诊断并由 developer 编译/非真机自测后执行；不要求重跑 T218 全矩阵。一次连续长按及停源后至少保留出生、20s 墙钟、累计接受模拟时间达到 **≥8s**、首次 LAND/床量变化、再 3 个完整 CURRENT 帧五类锚点。每锚点同 `runId/stateEpoch/physicsCommit/frameSerial` 记录：offered/admitted mass、stepCount/每步 dt 与累计 accepted dt、pool live/各 state 数、粒子 y/vy 范围、LAND 事件计数及质量、authoritative `Σbh/bl/mh/ml`、publication 拷贝后的同列 bh/mh 与同 token seal、P013 packed height/material/band、实际新管线床层 draw/GL 错误/viewport/profile 与像素采样。任何 probe 不得改变权威状态或把旧帧称 CURRENT。另按阶段记录 source/step、cold publication、pack/upload、shader draw/swap 的 p50/p95、帧间隔、有效物理秒/墙钟秒，保留原始序列与设备包 SHA，定位 0.5–8fps 的真实耗时段。

- 累计模拟时间不足或粒子仍 AIR：先归低帧率/时间步策略与性能证据，不能开 LAND 返修。性能若证实 render/上传等工程瓶颈，architect 修工程合同后转 developer；若需改变物理时间推进语义，交 designer 定版，不暗改 dt。
- 模拟时间充足而无 LAND，且 y 已触床/运动异常：交 architect 读事件/数值证据；工程实现偏离已定 LAND 合同则先修相应工程文档后 developer，算法合同冲突则 designer。
- 权威床量非零而 publication 为零：publication writer 工程问题；publication/packed 非零而床层 draw 或像素缺失：新 XComponent shader/render 工程问题；先修对应工程文档再 developer。若像素/viewport与可见判断矛盾，tester/effector补同 token 录像与采样，不凭主观截图定因。

本轮不能选 A/B 单因，也不能把 `XComponentType.TEXTURE`、clear 或旧 Canvas 猜测列为已证原因。T218 DD12 产品门保持未通过；后续观感必须等实际可见床与完整新管线帧证据。manager 可先派小型诊断钩子+独立补证，实际修复卡等上述判别结果。无需 manager 另设验收门。
