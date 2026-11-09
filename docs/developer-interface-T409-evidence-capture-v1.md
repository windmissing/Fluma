# T409 有界采集落地接口（T411/T412 消费）

2026-11-09，developer。细化 T407 §4.3 的落地序列化，未更改其物理/时限/必需证据约束。

- 三入口签名沿 T407；配置字段恰为9项，SHA64hex，expectedRevision 为当前实际 V1/V2，epoch/pageSession/durationNs 为严格十进制U64字符串，runLo/runHi/maxRows/maxBytes 为整数。duration2–5s；上限4096行/1048576字节。预算小于关键事件封套（16行/98304字节）明确 REJECTED/CAPTURE_BUDGET_TOO_SMALL。已有armed、busy/root/accept/present、身份不符拒绝，不等待/续pulse。新storage在控制锁外冷分配；旧storage锁外释放。
- begin/end 返回 `revision:T409_EVIDENCE_CAPTURE_V1,state:ARMED/FROZEN/REJECTED,reason,captureId`及scope；重复end同id不变，旧id拒绝。自动2–5s/首错/显式end冻结，reset/换代先封存旧账。frozen之前不能page。
- page `revision:T409_EVIDENCE_CAPTURE_PAGE_V1,state:FROZEN`；`cursor`是**页游标**，0始、next=cursor+1、pageCount≤1033、complete仅最后为true。每页重复captureId/actual activeRevision/packageSha256/epoch/runLo/runHi/pageSession/startNs/endNs，及rowCount/tokenCount/incomplete/reason。clock=STEADY_NS，uiClock=FRAME_CALLBACK_NS，跨域不相减。
- 第0页 type=SUMMARY，summary含initialLedger/terminalLedger（generatedQuanta/acceptedQuantumCount/pendingQuanta/activePlanRemaining/overloadDroppedQuanta/revokedDroppedQuanta/quotaCarry/clearedCarry均十进制字符串），maxRows/maxBytes/durationNs/dropped/suppressedDetailedRows/detailedRoot，pulseCount/rejectedPulses/窗口双域maxGapNs/8桶直方图（8ms桶，末桶≥56ms）；stageAggregates为固定阶段count/totalNs/maxNs。窗口统计从begin开始，未重置物理账。显示统计独立renderTargetHz/renderSkippedByCadence/newPhysicalFrames/sourceOnlyFrames/Swap间隔/STALE时长，systemPresentVerified=false。
- 后ceil(rowCount/4)页 type=EVENTS，每页1–4行。行serial严格1..rowCount，captureId；kind及reason；ns/tid/root/stage/code/probeStart/End/A/B/C；seq/uiNs/receiveNs及observedSeq/observedUiNs/observedReceiveNs、deadlineNs/permitRevision/planId/sourceBoundary、完整permit身份和ledger原子快照。observedSeq=0表示没有新UI回调（例如监督发现过期），不可捏造UI间隔。SOURCE_ATTEMPT额外down为UI sampleSerial/epoch/run/session、局部x/y/width/height、nativeViewport、acceptedByUi、nativeReady/nativePermission；不把UI接受当SOURCE成功。
- 关键预留：BEGIN、READY（Ready实际存在时）、每相别第一次SOURCE_ATTEMPT、FIRST_ACCEPT、FIRST_FAILURE、REVOKE、END。required由summary的readySeen/sourceAttemptSeen/acceptedSeen/faultSeen/revokeSeen推导；BEGIN/END始终必需，最后END.ledger须等于terminalLedger。未行使与丢失区分；incomplete或dropped>0一律不可判。GL_DETAIL仅第一个实际job意图root（detailedRoot）的DispatchExit/root开始取消/Batch/Flush/inflight/Poll/接受/隔离cut；重复相同结果Poll/Wait合并。detailPolicy=FIRST_ROOT_SELECTED_CALLS_V1，其它明细只固定统计，各阶段同时保最大区间原start/end/root/TID，suppressedDetailedRows不是dropped。行/预算不足立即INCOMPLETE冻结，关键区仍保留终态。
- 最后tokenCount页 type=TOKEN，各一个token：root/physicsCommit/sourceCommit/publicationSlot/generation/kind/phase，rootToken为`GRS_ACCEPTED_ROOT_Q20_M1_V1|completion[4..50]|shader/angle/layout哈希24词|inputHi|inputLo|session|planId|permitRevision`的完整接受身份，**不是scene token**。最多8组，保首空场Q、沙SOURCE/后继Q、水SOURCE/后继Q；实际尝试呈现才附原requestedToken/renderedToken/current/swapSucceeded。被cadence跳过的root没有scene token，显式`presentationEvidence:NOT_ATTEMPTED_NO_SCENE_TOKEN`，绝不伪造呈现。完整呈现链必须用实际ATTEMPTED且同root完整原token，不能将rootToken当显示证明。
- app冷导出 manifest `revision:T409_EVIDENCE_ARCHIVE_V1`，state COMPLETE/INCOMPLETE；重复scope、summary、requiredEvents、rowCount/tokenCount/pageCount、files（每页独立JSON原字节name/bytes/sha256）、dataBytes及totalBytes（全部分片+manifest实际UTF8 bytes）、maxBytes，exportStartMs/exportEndMs及operationClock=WALL_MS。全部文件同目录；manifest自身SHA由`GRS_EVIDENCE_CAPTURE`导出回执给出，不制造自引用hash。导出不再读取或拼入实时status。

T411应独立核实际文件/UTF8 bytes/SHA/所有游标、scope、逐行serial、required与首末账、预算、token标记。旧T405不能按此格式追认。T412仍可直接对ProgressClockV2序列做独立oracle；捕获的首末/逐事件ledger是原子样本，直方图不是可重建每笔pulse序列，不能从聚合反推未归档精确序列。

## 自验收口补充（同一V1协议的附加诊断字段）

- scope另有evidenceSource/clock/uiClock/probeClock。正式包为DEVICE_NATIVE、STEADY_NS、FRAME_CALLBACK_NS、STEADY_NS。host App自验明确为HOST_SCRIPTED_OFFLINE、SCRIPTED_OFFLINE_STAMP_NS、SCRIPTED_UI_CALLBACK_NS、HOST_STEADY_NS；合成协议夹具标SYNTHETIC_PROTOCOL。host各域不做绝对时刻相减，更不能当真机性能档。
- 关键事件stage=0是合法kind事件；GL_DETAIL保存原stage与ordinal（例如58的RootCut）、deltaNs。summary源计数sourceOfferCount/sourceEnqueuedCount按沙水分列；sourceOfferSeen=true时另要求FIRST_OFFER，按相别保首次原生入队/拒绝结果及原IEEE64 x/y/mass词。SOURCE_ATTEMPT是UI DOWN，FIRST_OFFER是原生实际offer，二者不能互替。
- token另有sourceId/requestSeq/admittedQ/rejectedQ/receiptStatus/outcome；若首个后继Q被cadence跳过，至多增存该相别首个真实呈现Q的完整接受及scene token，仍≤8组。不以零准入SOURCE终态冒正准入。
- 原scene TokenReady钩子在lease核对后、Draw/Swap前复制真实requested token。首次fault若在Swap中/后发生，冻结时保swapCallStarted/swapReturned/nativeSwapSucceeded及原始起止；晚返回不篡改冻结档。scene token来自原scene函数，不伪造。
- Native status附计数自身最后更新的countersRunLo/Hi、countersEpoch/PageSession/ProgressRevision；失效/新session期间页面标STALE并显示计数旧身份，不能把旧prefix归给新session。页面每callback仍单snapshot；文字最多30Hz、展开诊断4Hz。采集/导出按钮常驻诊断面板之外，不因开始或关闭诊断改变XComponent几何。先收起面板、必要时新run，保持viewport稳定后采集。
- renderTargetHz=30仅V2；V1为0，含义不节流、每accepted root尝试（不是保证0fps）。所有结果仍需区分nativeSwap与系统呈现。captureStorageBytes是显式arm后固定native内存，maxBytes是冷档总字节，不能混为GPU或RSS预算。
- pulseApiCalls 是窗口内全部 API 调用；pulseCount 为当前身份的新序列样本（包括导致 GAP 拒绝的样本），acceptedPulses 为接受数；rejectedPulses 分为 rejectedNewSequencePulses 与 rejectedOtherPulses，重复/旧身份不能伪装健康新序列。序号及 U64 字符串先查完整 UTF8 长度后解析，不允许截断接受。
- 冷档保留证据与最终构建交接见 `docs/developer-report-T409-capture-cadence-v1.md`；T411 独立检查时按该 schema 及 T407 原要求判断，不能将 host 档当真机窗口。
