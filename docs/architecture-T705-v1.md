# T705 · M2 SOURCE 链接开放截尾归因

2026-11-20，architect，不使用真机。T591 v4 新增一份 **SOURCE_V3 的单 API `glLinkProgram` 已进入且截尾仍开放**原件。同设备报告的两次单 API 开放分属 SOURCE 与 FINAL_Q；同角色的重复单 API 长尾、最终耗时和永久死锁仍未证明。 [机器裁定](architecture-T705-contract-v1.json) [原件复核](analysis-T702-T705-evidence-v1.json)

## 三份证据各自的边界

| 原件 | 实际角色/对象 | 到达边界 |
| --- | --- | --- |
| T591 v4 · 新快照 | SOURCE_V3 obj0，shader 1 → program 2；源码 b7d0a027… / 85,839B | COMPILE 成功约 4.821495s，ATTACH 成功。apiCursor rev1 seq15、api6、edge1，`glLinkProgram(2)` 在采样时开放至少 **32.233489688s**。LINK_STATUS、link glGetError、link log、反射及 Create 终态未见。 |
| T590 v3 · T682 原件 | QUANTUM_FINAL_V3 obj2，shader 3 → program 4；源码 0ef4f045… / 54,857B | api6/edge1 `glLinkProgram(4)` 开放至少 **20.567682344s**。同一 T590 尝试的 SOURCE 复合 LINK 已成功返回，用时 **73.431850156s**，有 REFLECT/PREPARE 退出。 |
| T591 v3 · T662/T682 原件 | SOURCE_V3 obj0，program 2（另一作用域） | **40.140833750s** 为复合 LINK 开放截尾。该段包括链接调用及 status/error 读取，原件未获得单 API 游标。 |

v4 当前作用域 bootSerial 1/epoch 3/window 387037432160/context 387043882336/ownerTid 37305/candidateRun 1。当前 LINK ENTER 331578978574846ns，游标 ENTER 331578978575627ns，相差 781ns。以 stage ENTER + 32,233,490,469ns 反推采样 331611212065315ns，再减游标 ENTER；整个 Create 尝试到采样为 37,055,495,157ns。原生时间计算取本档，不用文件名墙钟、设备 UI 等待、collector 上限或 250ms root 期限推算。快照 `sealed=false/nativeEndNs=0/firstFailure=null`，cursor `returned=-1/rawGl=-1/endNs=0`；开放的 stage rawGl 默认值 0 无函数返回含义。program 列表中的 obj1/obj2 仅角色声明，源码 SHA/长度为空，运行入口未见。

交接五件中 native 原件及三个 0B 文件均按 manifest 校验 SHA。obj0 的 compileLog 是实际查询得到的空日志；obj0 link log 和 obj1/obj2 尚无相应 query，0B 保留为容器原样。导出按同 serial 先取 native 后分次取对象 log，并非原子。外部声明为 `DECLARATION_MISSING`，签包 T590-cursor2 6b1faa1d… 与 T663 同版 native 7a0bcbaa…/BuildId c23e231c… 可离线核，实际设备 PID/启动点与加载模块映射仍缺。v4 SOURCE hash 与包内嵌入源码相符，支持角色/源码对照。

**归因**：当前证据支持 M2 冷创建的长尾跨 SOURCE 和 FINAL_Q 两个角色进入同一 GL API；旧 SOURCE 另有复合 LINK 长尾。这三份原件不能登记成三次单 API 复现、同角色单 API 重复或永久死锁。对象 handle 会复用，进程间不能按 handle/bootSerial 或组合源码 SHA 拼接。实际驱动内部停点、cache 的命中/失败、shader 形态与代价之间的因果都待证。G9 DM/RX 的 1728 行编译诊断、V5 NOT_READY 与正式 M1 取证各属自己的入口和源码。

## 复用 T694/T676，补已有出口的最小原件

1. 优先恢复 v4 同进程原始安装/模块映射、真实页 preArm/窗口及同 serial 连续采样或终态。绑定设备/driver、签包、进程启动、可读的实际加载 ELF path+BuildId/backing SHA、五字段 scope、角色源码 SHA/长度、cursor 序和原生时刻。没有该身份链的历史资料保独立观察。若缺件已不可恢复，直接用 T676 已签交付的 `T676-original-a`（038d1b04…，原 T663 五份 ELF，更新 ArkTS，见 `test/reports/t676-original-a/`）。effector 在实际 GpuSandWaterExperience 同页点击一次原采集入口 pre-arm，再触发一次原 XComponent mount。沿 T676 现有 reader/夹具，无需重复准备或等待 B 域。
2. 原预算 60s/64 次 native 诊断查询含锚点及日志，单持久记录 ≤16KiB、原串切片 ≤8192B、目录 ≤1MiB。验证真实最新 serial 后锁 scope，保存游标边、确实发生的下游 status/error 与日志、首失败、可得终态。严格区别窗口结束仍开放、函数返回（确认同序 EXIT 或原生匹配事件）、完整 Create 已终止（`sealed && nativeEndNs>0`）。后续同进程另时刻快照可补独立边界，保原窗口及观测时间，时序跳过 EXIT 只记录阶段有进展。
3. T696 的两程序裁剪及持久化/偿还对照保持原验收。v4 在进入 obj1 前已长时间停在 SOURCE，省去 intermediate 本身无法解释或消除该前序等待。用现有 T696 冷/warm/损坏/失配以及 SOURCE/FINAL_Q 按角色计时记录真 cache 路径和原 API 首错；无符合条件的设备证据时保观察态。现场需要产品修订时按真实同作用域首个已返回失败或可归属缺口回 architect。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。tester 离线，effector 真机串行。GPU 驻留、CPU 停投，呈现仅 XComponent+shader，故障保 FAIL/明确 STALE。
