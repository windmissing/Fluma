# T498 M3 时间 gate 工程闭包 v2

2026-11-16，architect，不使用真机。消费 T508 正式政策和当前 T497 manifest，取代原工程稿 §1 的设计等待。结论 **ENGINEERING_READY / NOT_IMPLEMENTED**，允许完整 V4 实施，不代表产品或设备 PASS。物理 V4/timeRev1、SOURCE72、Q549/619/594/664、五银行、四 binding、250,000,000ns root 期限和双 GAP33,333,333ns 均保持。显示目标30fps，quota120Hz。

## 1. 独立控制模块与接口

新增 `gpu_resident/m3a/progress_clock_v3.{h,cpp}`、`planning_control.{h,cpp}`、`quota_snapshot_v3.{h,cpp}`；`root_owner/run`、V4 producer/controller/cold reader 接入。旧 m1/m2route 的 ProgressClockV2 不改、不别名为 V3。使用 `GRS_M3_PROGRESS_CLOCK_V3` + `GRS_M3_BLOCKED_QUOTA_V1`，控制 schema 与物理 wire revision 分开。

以下为必须实现的具名接口合同；具体 C++ 拆分可调整，职责不可省略：

```cpp
ControlResult PulseLocked(const PagePulseV3&, uint64_t nativeNow);
ControlResult RegisterSourceIntentLocked(const SourceIntentKeyV4&, SourcePacketV4);
ControlResult RegisterTargetLocked(const ControlIdentityV4&, int32_t effectiveIndex);
ControlResult RegisterModeLocked(const ControlIdentityV4&, GravityMode);
bool ClaimLocked(uint64_t now, PlanGrantV3& out);
ControlResult BlockPlanningControlLocked(const ValidatedBlockedTailV4&);
ControlResult AcceptQuantumLocked(const ValidatedSuccessTailV4&);
ControlResult RevokeLocked(ControlFault);
ControlResult BeginSessionLocked(const RecoveryHandoff&);
ControlResult ResetRunLocked(const RetiredRunHandoff&);
QuotaSnapshotV3 SnapshotLocked() const;
```

`Validated*Tail` 只能由 completion_gate 在核完整身份、seal、实际 pass count/审核位、GL/fence、原 deadline 和 permit 后构造，不能通过普通布尔参数跳过 gate。未实现时返回明确 UNSUPPORTED，不借旧 V2 Balanced 产出 V3 结论。

新 source producer 传独立 sidecar `SourceIntentKeyV4{run,owner,context,inputEpoch,pageSession,sourceControlGeneration,userIntentSequence}`，与已有命令/512word payload 原件一起校验。该 sidecar 属新 M3 入口，不覆盖既有物理 word、不改旧 M1/M2 endpoint。真实 DOWN/MOVE/显式脚本动作分配 userIntentSequence；同一意图重发/timer 保持 key，不能换 nonce 造新意图。native 校验当前代和序列、FIFO≤16，先 checked 计算并成功入队，再提交 meaningfulInputSeq++；失败/背压不得 arm。旧 producer 缺此 key 时拒 M3 unlock，不从显示文本猜测。target 按实际 bounded+quantized index 判变，mode 按实际状态判变，均在同一 owner 锁序登记。FIFO已有命令/代号不重复复制：新增每槽只保sourceControlGeneration/userIntentSequence共16B，16槽256B，纳入下述1024B新增状态上限；已有packet身份加session控制代核完整key。完整sidecar只作定长调用输入，不另留一套无预算身份队列。

## 2. 原子切点与锁序

所有 quota、input seq、mode、active grant、jobLive、plan retirement 在一把现有 App 控制锁下形成短事务；Run 队列仍仅 owner 线程操作。即使已有ValidatedTail，消费切点仍须锁内复核当前代、permit、Fresh、deadline和未退休身份，防止完成校验后Stop/pulse先行撤权的竞态。owner 在锁内调用只修改固定控制字段的 `RetireBlockedPlanLocked(planId, nonce)`，同时终止 Tick.active/ordinal/续提/source阶段，再一次发布 clock 和 Run 的退休状态。该方法禁止 GL、等待 fence、文件 I/O、回调或再次获取 App 锁。其它线程只登记输入/pulse，不读写 Run 局部对象。

合法 outcome5 要求 QUANTUM、X>=1、同 active plan/root/session/完整代、base=proposed commits、publication资格0、审核全齐。先 checked 计算 `Ba+=X; Bp+=P; BC+=C` 及 episode；再统一 X=P=C=0、退休 plan+nonce、清 grant/jobLive/compute与CURRENT Swap许可、mode=BLOCKED_WAIT，记录切点最新 meaningfulInputSeq/pulseSeq。保 session/open/ready/baseline/last pulse、resource permissionRevision、FIFO、CURRENT、source/physics commits、accepted和LastGood。不得调用 ResetRun/Revoke/AcceptQuantum。

重复同 retired key 返回 ALREADY_RETIRED，不再扣账；key包括 run/owner/context/input/session/plan/nonce，比较完整 tuple。其它旧/晚尾走 STALE_REJECTED，不因 nonce 数字碰巧相同恢复资格。真实 GAP、Stop/reset、deadline先于正常 blocked 处理。GPU 对象按既有 fence/quarantine 退休；依赖 workspace 未安全退休前禁止新 Claim。LastGood只允许独立 STALE显示资格，标 PLANNING_BLOCKED，不复活旧 CURRENT permit。

不要只清 clock.X：旧 `Run.Tick.active` 若仍true，会永久挡住新 plan。也不要在资源仍READY时反复执行 tryReady 清 baseline，导致等待时长漏账。planning gate 要叠加在 Allowed/submit/Pump/Swap资格上，resourceReady 本身不代表物理可继续。

## 3. Pulse、输入、Claim 与生命周期

RUNNABLE 沿原 pending≤4政策。WAIT/REARM保原单调/双GAP/Fresh gate，`fresh=(C+120*gap)/1e9`，G和Bwait同时加fresh，carry取余，creditedElapsedNs和blockedWaitNs加gap，blockedPulses加1；X/P保持0，O/R不变。首次READY pulse只建基线，不填造缺失区间。

真实新输入只有在 BLOCKED_WAIT 时 arm 为 REARM_PENDING，记最新 seq和当时pulse序；后续新输入可更新 rearmInputSeq但不后移 rearmAfterPulseSeq。下个严格更新、身份有效的pulse仍全段按等待记账，然后把余数加BC、C=0、mode=RUNNABLE。该pulse之后的时间才生成pending。新SOURCE可在此pulse后Claim SOURCE-only；纯target需再等正pending。等待期不消费FIFO、不Pump、不试运行source，旧头按原顺序/身份拒绝规则处理。

Claim完整谓词：RUNNABLE/open/ready/baseline/Fresh、同run/session/owner/input/context/window/config、UI/gravity新鲜、无active plan（含X=0的SOURCE-only）且无未退休依赖root、P>0或合法FIFO非空、planSerial可增。一次冻结新plan及inputSeq/target/sourceBoundary，P移X。每个物理root启动才冻自己的原250ms deadline，续提不重置。成功Q仅在同plan完整permit且X>0时A++/X--；SOURCE成功不加A。

真Revoke使P/X入R、carry入Z、close open/ready/baseline，B系列保留。BeginSession先真Revoke结算、同run累计G/A/O/R/B/credited/Z/BC保留，新pageSession清arm/input序列并等资源READY/首pulse基线。ResetRun必须先退休旧root，再将旧新计数与时间分子一起清0，历史快照按旧run封存。任意U64增量溢出先固定fault并保事务前账，不得先部分清X/P；验证乘120和求和用checked U128/limbs。

## 4. 快照、reader 与资源

工程机器稿逐字嵌入 T508 `snapshot`，256U32/1024B，magic860963661、revision3、seal0xC0DE0003、U64低位在前。word4 mode0..3、5 flags bit0..7、6/7 P/X、8/9 root kinds、10 reason、16..99具名字段；11..15及100..255为0。signed target为I32符号扩展64位，禁止把负index当无符号Number。flags之外位0，word2=256；整快照在控制锁内一致复制，冷序列化在锁外。

reader按 `GRS_M3_QUOTA_SNAPSHOT_V3` 独立分派，未知schema/missingB/不合法保留位或flag拒绝；U64冷JSON全部十进制字符串，离线BigInt/limbs核：

`B=Ba+Bp+Bw; G=A+X+P+O+R+B; 120*creditedElapsedNs=1e9*G+C+Z+BC; 0<=C<1e9`。

还核mode/plan/flags交叉：WAIT/REARM不得X/P/jobLive/current物理许可；canClaim必须满足完整谓词（不能从G或ready猜）；retired身份不等新active。snapshot.session=pageSession属本run一致代。不能只见恒等式通过就宣布生命周期正确。

新增host状态保守1024B（包含256B FIFO意图补充）、一份快照1024B，共2048B/实例，developer以sizeof/static_assert核实际布局，不加GPU bank/dispatch或每根map。原每根completion1024B仍原账。冷quota副本每保留一份另计1024B及实际JSON bytes；T503/T507新schema适配必须在启动前给bounded writer/timeline预留credit，边界不够WOULD_BLOCK/INCOMPLETE，不扩旧4MiB预算、不将M2 payload版本误当M3。等待pulse只进既有定容ring/累计量，不能无限存快照。

## 5. 检查与完整接力

`analysis-T498-control-gate-v2.py`独立读取正式原件，不调用会重写designer资产的生成器；检查20条完整expected快照两条恒等式、3条必要轨迹、局部不可达carry迁移、256word/flags、T497↔T508政策一致、四档stage/资源不变及输入SHA。它是工程合同静态检查，无产品clock或GPU执行。

developer必须一次交完整V4各stage/solver/replay/所有writer/scene/reader及本控制模块；执行BQ01–BQ21、重复尾/旧成功尾、SOURCE-only忙、溢出、GAP/超时优先、FIFO/同target/heartbeat负控、READY交接及旧reader拒绝。独立tester与effector另卡，不把真机执行变成开发关卡。V5复用此控制合同仍需完整V4实现和冷协议接线等真实前置，设计闭包不代表这些已实现。

不恢复CPU投入。前景背景全部XComponent+shader，禁止Canvas或软件GL产品回退；故障FAULT或标STALE的上一完整有效新管线帧。不得降低250ms/GAP或以120fps为目标。本卡工程范围及必要检查完成，原2026-11-14未闭合记录保留为历史，由本v2替代。
