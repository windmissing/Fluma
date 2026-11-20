# T703 · T693 原映射身份与三时域补证合同

2026-11-20，architect，不使用真机。现存原件通过窄范围的逐根和动作配对。T684 完整证明仍为 INCOMPLETE，32cycle 保 NOT_MEASURED。首缺在 tester 的冻结 host wrapper/trace 与关联检查，复用 T692 已有 host-only accepted read-back。 [机器裁定](architecture-T703-contract-v1.json) [原件复核](analysis-T702-T705-evidence-v1.json)

## 冻结已证层

既有 parser 离线重读四臂原件。主 OFF/ON 独立正常退出，动作均 52/52，分别 144.328s 和 139.813s，trace 为 9,911,001B/86,854 行与 9,893,138B/86,296 行。相同 PE、模块及缓存输入锚、动作原字节、2 SOURCE + 90 Q 的 ledger/token 和排空终态支持主臂比较；排空只剔除各进程绝对时刻。源目录短暂编辑与实际 PE 分别记载。ON 留 55 完整 FROZEN 页。独立重放两臂凭 T692 实际读回的 bank/event/completion 按 nonce/提交身份均配回主臂 92 根，核过 trace 中的原 1024B 与重放原 1024B，读回干预分开登记。原检查只是非空 map 加同线程后续 unmap 成功并与 completion 配对，用它保留窄结果。

## 在原调用边界绑定实际对象

1. 修改 `test/t496/gl_forward_sink.cpp`、`argument_shadow.h` 与现有 `ScopedCall`/sink 包装。一次分配调用序，ENTER、原函数 RETURNED、原字节记录与 EXIT 共用该序。移除 map 的 `CallSeq().load()` 猜号。原调用 id 和 map token 分开；记录 GL target、带符号原 offset/length、完整 access 位、返回 non-null/原 unmap 的 GLboolean。原取得 1024B 条件为 offset==0、length==1024、READ 位被置，保实际参数以区分完整 access。原函数实际调用次数与顺序保持，覆盖当前直调和 proc-address 转发路径，复用同一个包装入口。
2. 用实际 `eglMakeCurrent` 成功/解绑/退休事件建立线程当前 EGL context generation 及可知 share-group lineage，覆盖同句柄重生。为每份可认的 GL buffer 保 share-group generation + GL name + object lifetime generation，重分配另增 storage generation。用实际 Gen/Delete、BindBuffer、BindBufferBase/Range、BufferData 等所经调用维护通用/索引目标别名、绑定代与存储代，按实际 GL 绑定路径建 shadow。void 调用回返只表示观察到调用结束，既有原生产错误原件可用于否定；缺 context、跨 share-group、未知分配/重生或原参数歧义时标 UNKNOWN 并拒绝强证明。保存证据来源为 wrapper，禁止额外 GL 状态查询或消费 glGetError。
3. map ENTER 固定不可变对象、存储和绑定代及实际 owner/thread；仅真实有效指针按原切点复制 1024B，保存原字节与 SHA。未匹配的退出/重复 token、nullptr、同 target 改绑后的 unmap、不同对象代、超界或原 GL_FALSE 均具名。用同 context/target/对象/存储及 token 绑定实际 unmap 调用和成功返回，允许在实际可证明的 context 所有权转移下按 token 解析，单靠 tid 邻接无法证明。
4. 冻结主臂实际 completion map 和独立 replay 的 bank/event/额外 completion map 分类。以实际 `AcceptedRoot` 的 run scope、kind、nonce、frame/control、source/physics commit、profile、bank/publication、eventArena/sequence 关联原 map 与同接受切点的实际 1024B。检查原字段 w[4..]、w[18..21]、w[27]、w[51..54] 及 T684 完整键，保 ledger/token、拒绝、终态。跨进程用动作/根顺序与语义角色、各自显式对象代映射对齐，两侧独立存原值。

## 在 32B trace 行引用实际时域

保 `steadyNs` 和原 seq/function/edge，用现有 32B 行的 `stampId`/flags 引版本化定长键字典或有界范围锚。保以下三个权威观测角色的时钟 ID、代、原值和适用掩码，异步动作同时保动作 ID 和 owner 实际消费的 checkpoint。字典压缩可引用已提交同一值，读取逻辑须逐行解析实际上下文。对跨动作/permit 的 ENTER/RETURNED/EXIT 各保当时上下文。

| 角色 | 权威值和适用域 |
| --- | --- |
| 脚本动作 | `BuildScript` 中原 actionId/declared stamp 与真实送达/消费锚；`g_scriptBase+stamp` 用于声明计划时钟，观察值取现场实际捕获，归属不能用最近全局动作猜。 |
| 观测墙钟/守护 | `t496sink::WallNs` 的进程本地 steady 采样及父级 watchdog 的原起点/边界。父子关联用 PID/start、ActivePhase 和原握手；各自计时。 |
| transport / root | 已冻结 host 的 `RootIsolation.stamp → App::Stamp`；`GRS_BRIDGE_HOST_TEST` 下可直用 `hostNow`，T496 D1 此回调绑定 `g_hostNowNs`。因此可能与脚本角色数值相同，仍需真实 permit start/deadline/revision、root nonce 与在 authorize/map/accept 时实际读取的值或锚。另存 transport 内若确实调用 `TailObserver::Now` 的实际绝对样本及实现种类；它使用 steady 并无独立硬件时源。 |

没有活动 root 或脚本尚未开始用 NOT_APPLICABLE；处于适用范围但缺权威锚用 UNKNOWN。用 owner 原许可切点已取得的状态复制/发布轻量上下文，现有观察时间点直接复用。禁止调用额外的 transport clock hook、推进逻辑时间、改期限或拿另一角色的值回填。最新版 reader 对三角色覆盖率、不适用/未知数量、根许可关联及首缺行出可核结果。T693 旧 live 空 stamp/分离业务记录仍按原格式解释。

## tester 验证与边界

先用保存的旧件回归现有 92 根与格式读取，并校验新格式对同名缓冲重用、同/非共享 context、重分配与改绑、不同 offset/access、空 map、失败 unmap、并发干扰、未知时域/代切换、容量耗尽及损坏 trace 块的拒绝。再以新证据 schema 的同版主双臂与独立重放双臂做整对关联。保 52 动作、原忙态检查、真实原 GL 序列、原 420s 外层/业务 120s/排空 30s，分区 12/16/4MiB、总 32MiB/arm。保持无阻塞热 I/O，有界缓存与可读截尾；达限保首拒绝和已提交前缀，缺证不得标 complete。当前静态核对没有独立证明每根指定 busy cut；本轮验收以实际演出过的合同切点判定。32cycle 继续原独立矩阵，本文四臂无真机或产品通过结论。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。host ANGLE 仅限夹具，产品 GPU 驻留、CPU 停投、仅 XComponent+shader。
