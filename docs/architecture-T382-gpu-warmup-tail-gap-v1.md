# T382 · GPU 预热后批尾停顿与上屏断链归因

2026-11-06，architect，P5。离线核对 T376 原卡、正式报告、T375 签包身份与 T379 独立读器。本稿修订 T341 的设备判读口径，并给出下一版最小取证改动。

## 1. 逐批对齐

`test/reports/t376/tl-manifest.json` 所列三份 NDJSON 的字节数和 SHA256 均相符。对三份合并运行 `node test/t379/cold_timeline_reader.cjs ndjson <part0> <part1> <part2>`，334 行连续、无丢失，同组原始 ns 单调。该工具可复算批次与预热；它没有从旧字典判出 GAP 归属，接受与 token 判为 `NOT_JUDGABLE`。下表额外按原始 serial、`PauseBegin.last/delta` 与源码对齐。TID 均为 3475，root nonce 均为各自 run 的 1。

| epoch/run | stage42 原始 ns，随后 Ready 与首 AnchorSet | 批1 serial / ms | 批2 serial / ms | 批2末 op→Exit ms | 后续 serial 与 GAP 原始 ns |
|---|---|---|---|---|---|
| 2/1 | 49/50，23,378,125；51、55 | 71→93 / 0.433 | 99→120 / 46.788 | 119→120 / 46.685 | 121 PumpEnd → 122 RootPoll → 123 PauseBegin → 124 Cancel → 125 PauseEnd，52,403,281 ns |
| 3/2 | 150/151，27,684,688；152、156 | 172→194 / 0.373 | 200→221 / 41.816 | 220→221 / 41.721 | 222→223→224→225→226，50,321,875 ns |
| 4/3 | 251/252，28,265,937；253、257 | 280→302 / 0.346 | 308→329 / 48.218 | 328→329 / 48.127 | 330→331→332→333→334，56,699,531 ns |

- **W1**：三对 stage42 都在 Ready 和首 AnchorSet 前，carry=0/1、root=0，完成行 delta 为原始 ns。结构通过。23.38–28.27ms 低于 T341 的 33–41ms 量级预期，登记偏离，设备档无法单独证明节省的机制。
- **W2**：按 T341 同批 24→25 的首批定义，三轮首批均满足 ≤8ms。T376 的 52/50/57ms 量级不能作首批或 poll 耗时。若要另报从首批 Enter 到第二批 Exit 的跨 Pump 窗，原行得 52.381/50.290/56.670ms。逐批 ≤8ms 的回归要求仍在第二批失败。
- **W3**：首批满足 ≤1ms，第二批三轮超线。两批均实际发出 10 个 top-level op，停顿集中在第二批末 op Exit 后。T337 旧包的首批异常与本包位置不同，现有边界不足以证明同一成本从批1转移到批2。
- **W4/W5**：第二批 `SubmitExit.code=16` 为 `OK_SUBMITTING`，紧随的 `PumpEnd.code=0` 表示 continuation 返回到桥层。旧 stage23 在 `receipt.active` 或 accepted 变化时都能发出；本组三轮均无 stage36–39（fence 或真实 Poll），而 stage41 的 code=1 是 `BATCH_ACTIVE`。因此 root 未封尾、未进入 inflight/accepted，stage23 只记录了仍 active 的 Pump。T379 的旧档字段限制继续适用，取消状态与调用源码足以排除这三个 root 的接受。归档 `statusAfter` 亦为 `acceptedCount=0`、两种 commit=0、`PAUSED_CLOCK/CLOCK_GAP`。全档四种 Draw/Swap 行均为零，Offer/Consume 也为零；触摸记录只证明 UI 动作，双源准入和呈现仍未取证。K5 完整导出成立，其余依赖上屏的回归继续按缺证登记。

## 2. 第一处失效和取证改动

`m1/full_transport.cpp::ExecuteBatch` 的第二批走非末批分支。由取消时仍为 `BATCH_ACTIVE`、无 fence 行和 10 个 op 可交叉确认。末 DispatchExit 到 SubmitExit 覆盖末 op 返回后处理、墙钟/计数更新、原有非末批 `glFlush()` 和调用返回/观察开销；CPU 脱调度同样会被墙钟计入。该区间没有末批 fence 或 transport Poll。现有 `lastBatchWallNs` 在 flush 前采样，不能替代逐批外层墙钟；状态快照的 `submitPumpWallNs=287344` 同样不能反证超窗。

`m1/run.cpp::Pump` 的 8ms guard 在 `ContinueSubmit` 返回后检查，只能阻止继续发下一批。`app_bridge.cpp` 随后以最近一次成功 Advance 对应的 anchor 计算墙钟差，GAP 分支先撤销许可并 Pause，`Run::Pause` 取消 unsealed root。桥层尚未到读取 accepted 和调用 shader 的分支。这里的上屏断点归工程提交路径，当前无需对 Draw/Swap、材质或双源 GLSL 派猜测性修复。

1. developer 在原有 `glFlush` 调用点取紧邻前后的 steady ns。非末批与末批分开标角色，并附同 run/epoch/surface/TID/nonce、批号、pass cursor 区间。保留末 op Exit、逐批 24/25 和真实 Fence/Poll 行。对现有非末批尾和函数调用边界分别记录原始 ns，使长段可归到 flush 内或其前后；若 call 内长，报告 GL 调用墙时。只有拿到匹配 TID 的调度证据才进一步拆驱动执行、排队或脱调度。
2. 旧 stage23 按历史定义兼容读取。新版显式区分 continuation 返回、封尾进入 inflight、真实 Poll 返回、accepted 递增以及 GAP 撤销。记录真实 Poll 返回码和接受计数前后值；成功链用有界冷回执关联 root nonce、publication slot/generation、source/physics commit、Draw/Swap 和完整 requested/rendered token。记录 accepted 发生于 GAP 前还是之后。被 GAP 撤销的样本不作为呈现成功。
3. 优先复用 T295 固定缓冲、stage42 和 T306 K5 冷归档。新增事件版本化，旧编号/原始 ns 的语义保持一致，T379 按字典区分历史 stage23 和真实 Poll；缺配对、截断、跨身份必须显式缺证。采样时间紧贴调用，事件写入不夹在两次采样之间。观测使用现有 owner 线程、有界存储与同步策略，常态帧没有日志 I/O 或额外 GL 调用。分开记录仪表开销；真机以探针开关区分原路径复现和定位。

## 3. 后续交付

- developer 可离线验证同 nonce 批次配对、未封尾 GAP 取消、封尾后 Poll BUSY/成功、accepted 与交换回执关联及旧档兼容；交可用签包。保留 120Hz、carry/nonce、8ms guard、33,333,333ns GAP、每批 flush、封尾/验收与事务顺序。两阶段仍按原 owner 执行，不新增同步 GL 操作或以重置时钟压掉 GAP。
- tester 在同设备串行冷启三个新 run，先复现并采到第二批尾调用边界和首 GAP；某轮成功越过第二批，再取同一 run 的真实 sand/water offer→consume→准入→至少一次 accepted quantum→同 token XComponent+shader Draw/Swap。故障显示 FAULT 或明确 STALE 的完整旧帧，前景与背景都经同一新渲染管线。W2 首批、W3 各批、W4 双源、W5 回归分别记账；未过的产品判据与 T380 收益前置继续保留。
