# T772 · RX SOURCE DEADLINE 首次归因与阶段补证

2026-11-21 · G20 · P5 · architect · 离线交付。

> T807 复核。T802 v3 双侧车已定位 `PRE_FLUSH→PRE_FENCE_CREATE DEADLINE`。首笔 arena copy 与末批重 flush 合计 287.861 ms，执行 fence 未创建，退役独立 fence 最终到位。先测 RX 分段 wall/owner CPU，T768 轮询和 T792 改序按实证触发。 [T807 首次归因](architecture-T807-v1.md)

**T766 第二轮的 RX 已进入 live Pump，并有一次 guard 判定超过 250 ms 根期限。** 保留原 250 ms、33,333,333 ns PagePulse 新鲜度与命令 receipt 时限。现有证据足以区分根前编译和根内拒绝。须先取 T763 已有的阶段 JSON，才能定位超期落在 owner 回调、copy/dispatch、flush、执行 fence 等待或回读前哪一段。 [T766 原报告](test-report-T766-v1.md) · [T772 原件联核](architecture-T772-evidence-v1.json)

## 两轮原件与身份

| 证据 | 能推出的结论 |
| --- | --- |
| 工件 | T763 未签 HAP `376720d8…` 与 T766 签名输入一致，签后 `0c980f70…`，签前后包内剥离 ELF 字节相同，SHA `6f069065…`，BuildId `4bdd76ddf02618f9579836d437aeca99d9039f63`。从 ELF 核到 BOOT 22,318 B、SOURCE 41,767 B。SOURCE SHA `ee66f5f2…` 是 T736 的 shader 字节，完整 roleSet 为 T763 的 `3312d851…`。 |
| 第一轮 | `l2.json` 显示 ticket 1 的 `PROGRAM_LINK_ENTER`，`l4.json` 显示 `PUMP_FAILED code=5`。按 effector 的同轮记录和已装研究版本，可按 live Pump STALE 处理。该终态布局没有原 ticket/process start、scope 或 guard，缺第一轮原 APPLIED、后续原边和具体拒绝原因。 |
| 第二轮 | `v6/chain.txt` 的 configure ticket **2**，PID **927**，owner TID **1129**，run/**owner**/context **2/1/1**。BOOT 的 `FINISH_EXIT.raw=37146`、原 WORK offset **1,166,976 B**、成功 map/unmap、GL 0 和原尾 `[0,6559303]` 同域成立。字段名是 `raw`，无需假定另有 `fenceWaitRaw`；这是 **BOOT fence**。本次主日志原件在保留的 v6 分段内。T766 将之后的 `l5.json` 的 `PUMP_FAILED code=6` 对应该轮，UI 布局本身仍无 ticket/start tick/nonce。 |

第二轮 SOURCE 编译 `99885830201094 → 99968133127969 ns`，**82.302926875 s**。链接 `99968133177500 → 100066241005469 ns`，**98.107827969 s**。随后 program **4**、40 uniforms、反射合格，`CONFIGURE_RESEARCH_EXIT result=0`，`APPLIED ns=100066243675000`。入队到 APPLIED **180.601356406 s**。`CACHE_PRELOAD_READY` 表示预载阶段结束，该轮 SOURCE 实际走源码编译。截图时间、系统日志时间和其他轮次的 start tick 不能补根内原时间。 [签包回执](../test/reports/t257/T766-rx.json) · [T763 构建回执](../test/reports/t763/compile.json) · [第二轮配置日志](../test/reports/t491/t766/v6/chain.txt) · [BOOT 原日志](../test/reports/t491/t766/v6/boot.txt) · [第一轮终态](../test/reports/t491/t766/l4.json) · [第二轮终态](../test/reports/t491/t766/l5.json)

## 从实装能定到的边界

- 页面只在同票 APPLIED、实际 XComponent extent 合格后激活 scope，执行一次 SOURCE 入队，再发原 22-word FrameCallback pulse。Claim 要求真实已接收 pulse、baseline、freshness、零 pending 和有资格的队首，Freeze 冻结同 nonce/plan 的根开始和截止。只有 `Transport::Begin` 成功上传并置 busy，`Pump` 才能返回所见的 5 或 6。按各轮对应的代码和回执，这些是**阶段推定已到达**。实际首 pulse 时戳及与 Claim 的间隔、INPUT128 原字、上传绑定和 root 证据没有从设备 JSON 取到。SOURCE_BUILD 的 SHA 认证计算程序输入，不能用来核对 SOURCE 命令原字。 [页面入口](../entry/src/main/ets/pages/GpuResidentG9Rx.ets#L93) · [Claim/Freeze](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L644) · [Begin/Pump](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L576)
- T763 BOOT_SOURCE 的 `Allowed` 在 protocol 锁内核当前 scope/permit、快照 pulse/receipt、实读时钟。Pump 内的 `Denied()` 返回最近一次 guard 的具名映射，`SourceReject::Deadline` 才映射 6，`SourceTimeGuard` 判定条件为 `now > deadline`，在 pulse/命令 age 之前。该轮有一次真实根期限拒绝，仍未知同次是否也已失鲜或命令超龄。第一轮的 5 对应其它多种拒绝条件，不能从 5、6 的差异定位同一谓词或复现 T755 的旧竞态。 `PUMP_FAILED` 在读取和验收 typed proof 之前发布。 [具名时限](../entry/src/main/cpp/gpu_resident/g9rx/source_guard.h) · [返回映射](../entry/src/main/cpp/gpu_resident/g9rx/source_timeline.h) · [Pump 失败出口](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L734)
- 第二轮的最小候选范围是 `PUMP_ENTRY / DISPATCH_ENTRY / PRE_COPY / POST_COPY / PRE_DISPATCH / POST_DISPATCH / PRE_FLUSH / PRE_FENCE_CREATE / PRE_FENCE_WAIT / PRE_MAP`。各守卫在原位置实采，前一 GL 调用越过期限也会在下一 guard 表现为 6。现场没有 `firstReject/recentPass`、cursor 与 submitted 的配对、执行或退役 fence 句柄与原返回。代码 6 允许已经看到执行 signal 后在 PRE_MAP 被拒，也允许还未提交首 pass。保原失败未提交，原尾、admittedQ20、冷档和有效 Swap 仍缺证；不能从 UI 给出物理准入量。 [T763 Transport 守卫](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L715)

## 最短补证与保存步骤

1. **先找 T766 原文件。** 在设备队列内查看现有安装上的 `context.filesDir`，并核本地归档和原操作员是否已有未归档文件。本库的 T766 目录、日志、原布局和本地候选文件索引均没有 `g9rx-source-*.json` 的有效导出，也没有出现保存路径或首拒记录。两份通用 h766 日志无此根专用时序，`v6/pump.txt` 是其它进程的系统日志。原进程若仍存活且仍是第二轮 scope，先从原页面保存和转存，不启动新 Configure。如果旧文件存在，以文件内身份联据，文件名时间仅用于找到文件；scope 已替换时标明历史原件无法恢复。
2. **确需重跑时只做一次真入口。** 复用核验的 T766 签包与同台设备，新进程保存实际 PID/start、app/sign SHA、映射 BuildId、owner TID。先连续采 `G9RxConfigure/G9RxSource/G9Boot`，再点一次「启用研究」。保持前台、原尺寸/旋转和真实 FrameCallback。不要额外点「注入粒子」、切页或重进 Configure。可用下方日志采集命令，从触发前持续到导出完成。系统日志时钟和原单调时钟分别归档。
3. **见终态马上保存。** 先保页面终态布局或截图，再点「保存原件」。失败也能导出，页面会将 `g9RxSourceDiagnostics()` 写入 `context.filesDir/g9rx-source-<Date.now()>.json`，fsync 后显示文件路径。应用在检测到 fault 时已自动 Stop，不必再点停止。首次导出后若仍无退役结果，同一活动页面和 ticket 待清理后第二次保存，分别归档，不覆盖。若新文件只见末尾 `RETIRE_COMPLETE`，原 wait 已被 384 条上限截掉，登记退役 signal 证据不足。确有写/读权限或结构错误时留原返回，再用已授权的 Device File Explorer 或已知的沙箱映射取回。 [页面保存原件](../entry/src/main/ets/pages/GpuResidentG9Rx.ets#L25) · [HDC 沙箱导出方法](https://developer.huawei.com/consumer/en/doc/harmonyos-guides/ide-device-file-explorer/)

在仓库根目录执行。第一段开在独立终端持续采集，`$rxTarget` 使用 `hdc list targets` 返回的本次设备，`$rxOut` 为此次本地取证目录。第二段只给 `$rxRemote` 填页面显示的实际沙箱路径，在开头加点转换为 HDC 沙箱相对路径。

```powershell
hdc -t $rxTarget hilog | Select-String '/G9RxConfigure:|/G9RxSource:|/G9Boot:' |
  ForEach-Object { $_.Line } | Tee-Object -FilePath (Join-Path $rxOut 'hilog-rx.txt')
```

```powershell
$rxBundle = 'com.example.helloworld'
hdc -t $rxTarget shell -b $rxBundle ls -l $rxRemote
hdc -t $rxTarget file recv -b $rxBundle $rxRemote $rxOut
$rxTrace = Join-Path $rxOut ([IO.Path]::GetFileName($rxRemote))
Get-Item -LiteralPath $rxTrace | Select-Object Name, Length, LastWriteTime
Get-FileHash -LiteralPath $rxTrace -Algorithm SHA256
python test/t765/check_trace.py $rxTrace --compile-receipt test/reports/t763/compile.json
```

**回传最小集合。** 原 JSON、记录的文件路径/字节数/SHA、同票原日志及终态布局。以 `status.pid/processStartTick/configure.ticket/run/context/mappedBuildId`、`status.source.ownerTid/owner/nonce/plan/roleSetSHA` 联核。提取 `APPLIED/EXTENT_QUALIFIED/SET_ACTIVE/QUEUE/FIRST_PULSE/OWNER_WAKE/CLAIM/FREEZE/BEGIN_RESULT`，再对 `firstReject/recentPass` 的 phase、why、now、begin/deadline、实际/期望 scope、pulse seq/age、receipt age、cursor、dispatch 数与调用前后原边。保 12 dispatch、3 copy 的真实计数以及 fence create/wait、map/unmap、commit 和 retirement 的可用原返回。分别登记首拒时执行 fence 与晚到退役 fence。若首次文件过去未采、或溢出缺关键边，按层级保目前证据，保留原文件并附缺项。T765 的判定器覆盖已支持 schema 和边界，输出 PASS 不替代 WORK 1024 B、独立 oracle、FIFO 与有效 Swap。 [T765 判定器](../test/t765/check_trace.py)

## DM 改动可迁移的前置

| T768 路线 | RX 裁定 |
| --- | --- |
| 有界 owner 轮询 | RX 本来就在 owner 的 `wait_for(2ms)` 循环中等窗口/配置/pulse/超时唤醒。DM 的研究驱动用 `yield()` 忙轮询，T762 观察到 13,069 poll。先从 RX 记录本轮 `OWNER_WAKE`、各 Pump/执行 poll 的间隔、首个 timeout/最后 timeout、实际 pulse gap、真实 deadline 和支配耗时。若确为 fence 已提交且 owner 唤醒迟滞，才单独对照按最早下一 poll 与截止唤醒的方案。250 μs 只作假设参数，可能增加 RX 的负载。任何 wait 或 GL 操作仍在 controller/bridge 锁外，实际 PagePulse 只经原页。 |
| 末批先 fence 后单 flush | 两族目前均有批末 flush、建 fence、再 flush，RX 也保原 12 pass、12 个 dispatch barriers 和 3 copy。可评估迁移的结构前置是 RX 先证明 cursor=12、实际大耗时集中于末批首次 flush，且工程上能保所有原同步/GL 错误和撤权分支。独立对照仅改最后一批 flush，保中间批、copies、barriers、同 owner/context/fence 和时限，收益按整根 typed commit 量。T762 的 85.55/77.46 ms 不移填 RX。 |

跨族目前只共享「固定期限内未得到有效提交」的结果。按 T768 方式分解 RX 问题所需时，先用现有设备导出。若同票导出稳定复现仍缺最后一条 guard/调用或 fence 配对，由 developer 仅修订阶段诊断，保固定槽、首拒、前后调用与退役终态和新旧 schema 对齐，由 tester 离线验，再安排 effector 复跑。原事务保持 209,715 Q20、同根 typed、commit+1、quanta 不增和 FIFO 恰消费一次。任何迟到尾仅退役。根前编译、合格后的缓存持久化和暖新进程加载单列。仅 XComponent+shader 上屏，GPU 驻留、CPU 停投，禁止 Canvas 回退。 [DM T768 合同](architecture-T768-v1.md) · [RX owner 等待](../entry/src/main/cpp/gpu_resident/g9rx/bridge.cpp#L373)

离线复核 `python tools/architecture/analyze_t772.py`。本卡无真机、无产品实现改动。**G20 约 33%（4/12）**，RX 研究臂 SOURCE 冷编译与 live Pump 有证，暖新进程命中和三个优化臂首有效 Swap 仍待证。 [T741 计分](architecture-T741-v1.md#g20-完成度口径)
