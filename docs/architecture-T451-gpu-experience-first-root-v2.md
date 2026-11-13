# T451 · 显式入口首 root 归因与接续 v2

2026-11-10，architect，P5，不使用真机。消费 T456/T457 新证据，接续 [v1](architecture-T451-gpu-experience-state-lost-v1.md)。本稿交付首次归因及工程接力，不判体验、物理速度或 30fps 通过。

## 1. 已证切点

来源为 `test/reports/t457/t457_boot_{want,menu,rerun}.json`，包为 T456-experiment-finalized，最终 SHA256 `96ae7ca2a0d78dc8f47bc0796eb7cee96f67236d9adc37fb427821778a1a9310`。已重算最终包和其中 libgrsm1 的哈希并核签包收据；冻结源码位于 `.codex/local-test/t456-experiment-final/`。提取值、原件与源码哈希见 [本轮证据索引](analysis-T451-T462-T463-T464-T465-evidence-v1.json)。Want 的 rerun 档重含同一首错，不能算第三次独立复现。

| 项 | Want | 菜单 |
|---|---|---|
| PID / GL owner TID / 首错观察 TID | 31861 / 31965 / **31861** | 33895 / 34600 / **33895** |
| 有效 epoch / published run | 2 / 1 | 3 / 1 |
| actualClock | V1 | V1 |
| viewport | 1280×1471 | 1280×1609 |
| transport Create 实际区间 | 56,021,752,500ns | 20,247,062,656ns |
| READY → 首 root | READY_COMMIT 后 ROOT_INTENT kind=2，QUANTUM | 同左 |
| 撤权 anchor / 观察时刻 | 3690618959498766 / 3690619009227203 | 3690926141582360 / 3690926181808610 |
| 实际 GAP 差值 | **49,728,437ns** | **40,226,250ns** |
| 第二批 Flush 区间 | 3690618960303610–3690618991905485 | 3690926142487516–3690926172920485 |
| 第二批 Flush 实际耗时 | **31,601,875ns** | **30,432,969ns** |

两档有效代 EGL 各调用成功、Create 返回 OK，READY 的 epoch/window 身份有效。首个拒绝为 `ClockGapLocked` 锁存的 **V1_ANCHOR_CLOCK_GAP**；由 UI TID 观察，而非报告所称“下一 owner 检查点”。原件未记录首 root 接受、Draw、Swap，因此阻断位于首次 QUANTUM 的分批提交期间。早先 window=0 的 SURFACE_CREATED 终态表示窗口交接使旧尝试换代，不是该有效代 EGL 创建失败。

按 `submit_observer.h` 的枚举，TAIL a=44 是 Flush，a=43 是 BatchCall，a=45 是 Continuation，SUBMIT a=24/25 是 SubmitEnter/Exit，结果16是 OK_SUBMITTING。第二批调用区间包含约30–32ms Flush，批次只在 GL 调用返回后检查预算；8ms Pump 与批次预算都不能抢占一个正在驱动内部执行的调用。原档先后顺序和算术成立，但区间包含线程停调，尚不能把全部 wall 时间认成 GPU 执行时间。首错后没有该批完整尾部，不补造其结果。

## 2. 归因和报告勘误

工程上已定位 V1 的控制 anchor 与 owner 计划推进耦合。`app_bridge.cpp` 在 `Advance` 成功后更新 `controlAnchor=last`，正在分批提交/等待时不以页面 pulse 刷新该 anchor；UI `ObserveControlLocked` 仍依据此 anchor 的33,333,333ns期限撤权。首次提交时出现的长 Flush 与 owner 未能继续推进叠加，触发真实控制期限拒绝。fail-closed 应保留。

创建阶段的20.25/56.02秒均发生于 READY 与运行 anchor 之前，不能用 T448 转述的40.9秒编译解释这次 GAP。原档无同期完整驱动日志，也未证明 EGL wrapper、队列重建或内存不足是首因。两页面 viewport 不同，不能据此推入口性能等价。T410/T437 是实际 V2 的另外一条控制路径，恢复缺口见 [T464](architecture-T464-gpu-v2-recovery-v1.md)，不能定为同因。

T457 的“V2 只会同样失败”无实测支持。T466 已排实际 V2 补证，沿该卡取证，不重复开 T457。本轮首次归因到此已完成，后续取证用于判断 V2 能否消除 owner 耦合，以及是否还存在真实 UI GAP/250ms deadline。

## 3. 工程接力

1. developer 先交付有界 V2 启动选择：实验入口冷选择 V2，页面须展示实际 revision，不以包支持或按钮文字代替实际选择。普通 V1 基线继续明确标 V1。不要先改正式默认，再声称问题已解决；正式默认切换须消费实际 V2 证据，由 manager 裁决。
2. 完善现有冷探针，仍在同一 GL owner 执行。每一 V1 AnchorSet、每一 V2 pulse 接受/拒绝及撤权写观察角色、native stamp、UI 原域 stamp、epoch/session/permissionRevision、root/plan、旧新 anchor；ROOT_INTENT 只记录首次值不足以解释后续 anchor，需保留实际许可修订。批次记录 from/to/stage、enter/exit，Flush 增 enter 锁存和 exit；对最后未返回调用可导出当前 in-call 阶段。固定容量、溢出显式 incomplete，禁止对每 pass 写字符串/同步日志。
3. 首错终态仍允许附加该代只读尾部证据，不能覆写首因；晚到 accept/Swap 必须依旧 permit 被拒。STATE_LOST 与首因并列展示，旧 public run 不充作当前 READY。不得伪造 pulse/heartbeat、在 Flush 内重置 anchor、靠降低显示率放宽 GAP，或去掉批次间 Flush/屏障来凑通过。
4. 实际 V2 的 owner 单次长调用仍受独立页面控制与 root 250ms绝对期限约束；恢复实施按 T464。若实际 V2 健康 pulse 下仍 deadline 超限，带具体 pass/Flush 区间回 architect，再判断工程切分或 designer 算法成本；当前证据不足以要求拆 kernel。

developer 必要检查为实际生成 bridge 接线的控制桩：健康 pulse + owner 40–50ms阻塞在 V2 下不能被 V1 anchor误撤权；真实 UI/native GAP 超阈值必须撤权；旧 root 同 nonce continuation 不续期限，250ms前后边界保留；跨 epoch晚尾不接受，不从旧 token 上屏。完成编译、非机检查、签包即可交付，独立设备验证不作开发关闭门。

T466 tester 串行消费同包实际 V2，记录 actualClock、同档位、viewport与 cold/已准备阶段。先取 READY→首 root→Accept→Draw/Swap，后取真实 GAP 与迟到尾负控。区分 controlNativeGap、UI callbackGap、owner阻塞区间和 root age，绝不跨时钟域相减。未成功进入 V2 就回报配置切点，不能拿 V1 再解释 V2。T448体验、T410采集、T437前缀收益各保原范围，字节收益不推 fps。

执行接力共用约束：不使用真机的开发与带“真机”标签的 tester 分卡；同设备串行。要么完成本卡约定范围及本角色必要检查，要么具体阻断才停止，局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改滴答同名清单。全部背景/前景只走 XComponent+shader，失败 FAULT 或有旧 token 且标 STALE 的上一有效完整帧，禁止 Canvas/软件 GL 产品回退。CPU停投。成功回原卡并仅改本人行结束待验收；失败或关键缺证写复现、原错、证据、尝试、未完项、所需前置与接手角色，仅改本人行看评论。

## 4. 交付边界

本轮未运行设备或产品测试，已完成两独立入口原件、冻结源码、签包身份和区间算术核对。确定首次撤权切点、V1控制耦合及工程接力；V2实际行为、驱动内部成本和体验指标仍未证，不影响本卡归因交付。

## 2026-11-10 用户要求补齐编译产物持久化

此前仅安排日志落盘，遗漏成功program binary复用。新增 [编译产物持久化工程合同](architecture-T462-program-binary-persistence-v1.md)，明确自动落盘、跨启动加载、环境/代码失效及开发/独立验证prompt。manager据此新增P5实施，不扩正在执行的T468诊断关闭门；当前尚未实施，不能称为已缓存。

## 2026-11-13 T448 v2首root归因修订

[T490工程裁定](architecture-T490-m1-first-root-budget-v1.md)核实际T467签包及冻结源：首QUANTUM在245.51875ms真实ACCEPT，后续呈现共享deadline仅余4.48ms并晚拒；旧报告2455ms单位错误。timeout后boot普通root记录被terminal过滤，缺SWAP行不证明没有回调。graphics冷准备沿T480/T485，首次撤权锁存和有界尾部另作工程接续；不修改250ms/GAP/配额语义。
