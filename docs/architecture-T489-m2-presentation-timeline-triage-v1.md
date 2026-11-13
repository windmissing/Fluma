# T489 · M2呈现、session与时间线导出首次归因

2026-11-13，architect，P5，不使用真机。消费T477 v2/v2.1原件、9565704、最终签包及冻结生成源。工程稿先纠正报告边界，再确定已有修复的接续和仍需的最小证据；不重跑产品/设备测试。包/ELF、原件hash和解析数据见 [证据索引](analysis-T489-T490-evidence-v1.json)。M1首root另见 [T490](architecture-T490-m1-first-root-budget-v1.md)，不先定同因。

## 1. 来源必须分三代

| 代际 | 最终HAP / libgrsm1 SHA256 | 可支持结论 |
|---|---|---|
| T482-final，T477 v2/v2.1实际原件声明包 | `300bd52a…381e` / `783cb115…a5b2` | 旧T404_STAGE65字典、旧GpuRecoveryPulse、旧runnable门；真实compute冷暖缓存原件 |
| 9565704修复，T477-fix | `b27eb211…ac13a` / `5b1129f3…02cda` | 新T477_STAGE67、被拒页保存、撤权原因显示及runnable修复；仅已有非机/签包交付证据 |
| T467-experience-final，T448 v2 | `1a3d8d53…f6f2` / `b2dd4ba2…61ea7` | M1旧包，一个有效root的首错和呈现晚拒，不能与M2直接拼接 |

本轮重算三个最终包及包内ELF，与各T257签包收据一致。T482与T467 ELF含旧T404_STAGE65_V1、不含新T477_STAGE67_V1；T477-fix相反。故T477 v2.1再次INVALID_ROW不是9565704修复包的复测失败，不能把工作区新源码当旧设备包已含修复。T477-fix冻结目录为 `.codex/local-test/t477-fix-release`，T482为 `.codex/local-test/t482-final`。安装声明仍须用T483显式收据核HAP/ELF，界面SHA不单独证明安装身份。

## 2. 呈现结论纠偏

| 原件scope | manifest状态 | 实际数据 |
|---|---|---|
| PID48949，epoch3/run1/session1；两次导出同进程 | PAUSED_CLOCK / CLOCK_GAP | accepted=physicsCommit=2446、sourceCommit=0、**newPhysicalFrames=2446**；clockDeltaNs=34,329,218 |
| PID27279，epoch3/run1/session1；rerun3 | 同上 | accepted=physicsCommit=2441、sourceCommit=0、**newPhysicalFrames=2441**；clockDeltaNs=33,441,407 |

冻结bridge仅在`Scene::Draw`返回current、其Swap成功且事后PermitLocked仍通过时调用 `renderCadence.Presented`，该函数对新physical递增newPhysicalFrames。它是native接受+呈现检查账，不是只有compute进度。因此原报告“全部没有Draw/Swap”“卡的是呈现域而非计算域”均超出证据：存在大量native当前物理帧计数，缺少的是完整同窗逐root/系统像素证明，不能把导出在第120/88行中断当作后续Draw不存在。

这也不证明30fps或沙水可见。原件sourceRequestSeq=0、sourceReceiptStatus=0、lastOfferCode=NONE、sourceCommit=0，水/沙库存零；现有现场样本没有原生SOURCE接受，可能在空场推进。无触摸command/拒绝原因回执，不能把source0归给GPU准入算法，也不能确认页面尝试送源后被session拒绝。lastSwapGap分别82.975/109.186ms，maxSwapGap约205.98/203.52ms，不据较小截图区间或计数增长宣称持续30fps。

`STALE_PAGE_SESSION`的生产条件是token不符**或open=false**，不能仅凭此字符串判token失配。上述最终status明确pageFault=CLOCK_GAP；native观察间隔34,329,218和33,441,407都超33,333,333ns。pageNativeGapMax约27.26/24.38ms是已处理pulse历史统计，supervisor可在下一pulse交付前检测超时，所以较小max不能反证真实GAP。M2该原件不是已证明的250ms root超时，也不能套M1首root成本解释。

已确定的工程链为：旧page frame在native撤权关闭session后继续发旧token，UI长期显示STALE掩盖原始CLOCK_GAP；旧runnable没有AllowedLocked，残余active tick可在已经撤权时使owner空转、持续产生诊断。9565704已在页面同代FAULTED_REVOKED时停止pulse并显示原因，处理state-read→pulse间撤权竞态，且修runnable门。CPU忙与百万dropped的具体占比没有采样，不能断言全部丢行仅由这一个循环造成；但修门是已定位可复现工程修复，不应重复开同样实现卡。

## 3. INVALID_ROW的实际合同缺陷

旧冻结生成bridge把 `static_cast<TailStage>(141/142)` 的Flush enter/exit送入AddTailLocked并直接写timeline，native页却仍声明T404_STAGE65_V1，ArkTS严格拒stage>65。这是明确存在且足以触发INVALID_ROW的生产者/消费者不一致。坏页在旧原件未保存，不能唯一还原历史具体第一个违规字段/行；“恰120行固定坏cursor”与“88行后仍失败”均不是定位依据。

三份已存prefix分别120/120/88行，serial连续从1开始、stage均在1..65。两份120行属于同一PID且prefix相同，不算两次独立运行。第三份不同PID、首批路径长度不同。失败后缺页仍判INCOMPLETE；不得把未知141/142静默映射进旧字典、跳行继续宣称完整。

BridgeTimeline::Add实际为**固定4096行前缀**，容量满后只递增serial/dropped，不循环覆盖旧行。原报告“环形覆盖窗口/并发写坏行”没有源码依据。manifest的count=4096而serial/dropped分别4,687,609/4,683,513及2,216,758/2,212,662满足serial=count+dropped，正是满后弃新增的计数。部分重复导出时before的serial不同是仍有新增尝试，不是prefix旧行被覆盖。

9565704已把Flush正式登记66/67，新字典T477_STAGE67_V1与reader同步；新字典只对ENTER事件允许probeEnd=0未返回编码，旧字典仍严格拒。已交非机检查覆盖124行越过旧失败点、未知68拒、被拒原字节保存、dropped仍INCOMPLETE；生成M2 host还行使SOURCE/J→W及native Draw/Swap current=1。以上是已交开发证据，不能冒设备修复通过，也不增加开发关闭门。

## 4. 最小接续及稳定快照合同

先消费现有T477-fix签包做独立复证，**不重做已完成66/67/页面门实现**。若其它在途生成路线需承接，developer只补实际生成源与签包manifest的来源合并，保compute cache key、shader、恢复和T485场景准备；M1/M2分开签包和核身份。

现有timeline(cursor)为在线前缀读接口，collect前后还要求count/serial/dropped不增长；在活跃仿真中逐页读很容易GREW_DURING_EXPORT。它正确拒判，不应放宽为完整。若要稳定导出活跃时刻，新增**独立只读快照接口**，旧endpoint、格式及判据保留：

```text
timelineSnapshotBegin() -> snapshotId + immutable metadata
timelineSnapshotPage(snapshotId,cursor) -> original page envelope + snapshot identity
timelineSnapshotEnd(snapshotId) -> release receipt
```

快照在App同一短锁内复制现有固定前缀、token表及identity/serial/dropped/stageRevision，不reset/暂停业务、不撤permit、不清原timeline，也不唤醒owner。GL、文件写入、等待与JSON序列化在锁外；复制耗时用原native域记录。快照只允许一份，4096×sizeof(BridgeTimeRow)=688128B、32对1024B token=65536B，共753664B固定payload，另加有界metadata；禁止并行多份或无界历史。BUSY/复制失败不改业务状态、不偷偷复用旧snapshotId。申请/释放都不调用GL，不能用fake heartbeat避免复制引起的真实GAP。

快照冻结的是**诊断观察值**，不是业务完成许可。页面每页同时核snapshotId、捕获的epoch/run/context/surface/PID/source包声明、字典、count/serial/dropped，旧快照跨新run仍可读但必须一直带旧scope，不能重新贴当前身份。cursor按同一不可变prefix前进；prefix含未知stage或原nativeDropped>0仍输出INCOMPLETE，不能因snapshot稳定就改PASS。快照新增独立revision，不把snapshot的serial稳定规则混入旧T306 live格式。

新reader沿原每页16KiB、4行/页、总4MiB/30s有界预算；拒页先保存≤16KiB原始字节与SHA，再写reason及具体field/row index，只作rejectedPage不计有效行。取消、切后台、超时finally释放快照，已写文件与manifest留存；native以捕获后60s单调时钟TTL兜底回收，不因读页无限续期。snapshotId用进程内不复用的64位序号，溢出明确失败；过期/已释放/错ID返回STALE_SNAPSHOT，不返回其它run页面。先落盘then报告文件，不在最后catch才编造坏页。旧已丢坏页不能补造。

4096前缀与recovery512前缀都不可能容纳无限pulse。追加有界**分阶段汇总+首错锁存**：保每stage累计数、最后一次/首次错误scope和callEnter/Exit，hot端定长计数不写字符串/文件；首个root的intent/accept/Draw/Swap/撤权固定槽与terminal不可覆写。unknown stage单列计数/原值，仍不当已知事件。汇总只补诊断，不代替缺失全trace；扩大容量、改环形覆盖或抽样规则须新版本，不能让百万dropped归零。现有旧timeline不自动清空，独立新capture使用单独session/预算，不能复用重命名旧档充完整。

## 5. 精确补证与工程检查prompt

developer，不使用真机。按第4节在新工程卡实现快照及最小汇总/源操作诊断；9565704已有项只合并来源、不能重复派工。UI源链优先复用既有recordSourceAttempt：有限记录pointerDown/source选择→READY判据→窗口/vp坐标转换→实际32U32上传hash→Offer回复→accepted SOURCE摘要，所有事件绑定同reset/session/run/permissionRevision，拒绝也保存；上限固定、无热文件/全量粒子读回。不改SOURCE数学或以模拟触摸绕过真实入口。

必要非机检查直接执行实际生成native exporter/ArkTS reader：新字典66/67和未返回ENTER、旧字典拒141、未知68拒与原页落盘；活跃写入中snapshot各页稳定、token/identity不串代、重复/越界cursor、取消/超时释放、第二快照BUSY、dropped>0拒完整；同代撤权不续旧pulse、旧session请求不能覆盖首因；新run/新frame/session后新scope。source尝试分别覆盖未READY拒、合法offer、晚代/错坐标拒，不对空场计数冒SOURCE通过。编译签包及必要非机检查完成即可交付，独立真机不作开发门。

tester真机接续，带“真机”标签同设备串行。先明确安装T477-fix或消费它的新签包，核HAP/ELF和stageRevision；保缓存数据分别核compute暖load、显式新run/旧token终止、首次SOURCE尝试及准入、接受/Draw/Swap同root和系统截图/录制。发生GAP保last accepted pulse及触发观察stamp/线程，不能只报中位数。timeline导出至少跨旧88/120位置并保存完整坏页/快照manifest；若dropped>0，允许诊断交付但仍INCOMPLETE。系统像素、30fps、SOURCE接入和缓存分别报告，补证先回architect再分流。T477旧冷暖成功证据保留，不重开旧执行卡、不要求manager重测。

各执行prompt共用：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用已有oracle。所有前景背景只XComponent+shader，禁止Canvas/软件GL产品回退；失败FAULT或标STALE完整旧有效帧。成功原卡回执并本人行结束待验收；失败/关键缺证写未完项、复现原错、原件、尝试、不能自行继续原因、前置与接手角色，本人行看评论。

## 6. 本轮结论

已完成来源核验、native计数/呈现边界、真实GAP、字典不一致和固定前缀解释，以及已交修复/新增快照/精确补证的分流。历史坏页与同窗系统像素缺证真实保留；本卡归因范围完成，不把未知项记产品PASS，也不从M1期限推M2同因。
