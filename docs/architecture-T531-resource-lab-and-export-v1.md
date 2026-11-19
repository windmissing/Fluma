# T531 资源采样入口与冷压力独立可达工程修订 v1

> 2026-11-19 T635补充生效：[lab终态/排空/FilesV2修订](architecture-T635-v1.md)明确不可变终态、取消时间、test-only同锁有界等待实际接线与文件原件边界。T587两份job原生执行约0.35s，不能以延迟UI标签判运行中取消；历史分页窄PASS保留。生产Enqueue/Drain和物理证据门不变。

2026-11-16，architect，P5，不使用真机。读取T519/T526正式报告及本地全部原件、T501/T502/T503/T507实际生成链。结论：**生产冷接口已有，页面未接线是真工程缺口；压力设施也未提供，不应被CURRENT显示状态绑死。** 本卡冻结developer完整接线/隔离lab范围与tester接力，不以RSS回落证明逻辑释放/driver归还。机器证据 `analysis-T531-evidence-v1.json`。

## 1. 当前证据边界与身份更正

T519本地T502-m1签包收据及实际文件SHA为 `362a3cb66f452d1fef71e4ca9a57012e5d6d1f02ea8472ba5a100fa5ab79898b`；报告和timeline-manifest identity却填 `362a3cb66f452d1f4e35b7c29be4c0599f268aa30c66e20d55e7d33d0a5b0e4a`，只前缀相同，**不能宣全文MATCH**。后者来自页面TextInput声明，不是系统已安装hash测量。现存RSS/PSS仍是PID42442观察，须保留；但绑定该完整HAP/ELF的声明需tester用安装记录/包内ELF或对应实际签包收据澄清，不从输入框自证。

本地原件有mem冷启/reset/export三份及vm-after-reset，未见报告P1/P4两个VmRSS原文件；报告所述四相位/RSS值保观察，不把未存原件冒可逐项复算。mem标题为Pss Total，GL单项已有系统归类值，不能把全部PSS叫native heap；vm-after-reset中VmRSS370136kB另列，不能与GL PSS按字面相加成独占总量。缺原件不要求重跑已证全部，仅补同scope最低所需。

T526签包b702032f…、scope PID60647/m2/epoch4/run2/session1和9文件manifest在位；本地只有trace0/1/2三页及manifest，非9页全原件。三页可核实际UTF8/哈希，manifest九项byte总和24786、actualWords7680、state7424、events0、rootKind2、root nonce521。**这是成功QUANTUM的空场冷档，证明无SOURCE和非CURRENT标签不自动阻止cold采集。** 后来UI WAIT_RETIRED不推翻该先前scope原件；也不证明满载、两slot、TTL、2MiB已测。缺六页可向tester取回已有文件，不必先再跑物理。

manifest release=ENDED和RSS回落只是资格撤销及系统数值，T503允许FREE backing保留复用，不能由RSS推算capacity0、唯一析构或driver真正释放。ROOT521也不等于521次SOURCE或完整流水长稳。

## 2. 生产只读导出，不依赖CURRENT

在实际T502/T507 overlay两页面添加固定“资源/缓存快照”入口（独立面板或页面，不挤掉XComponent，参T530布局合同）。通过现有 `resourceState()`、`cacheStorageState()`消费真实helper；不另写一套手工bytes估算API。read-only入口在创建中/READY/busy/FAULT/WAIT_RETIRED均可达；状态只能说明本次观测scope，不能强置ready或发送SOURCE唤醒。

新增collector `GpuResourceSnapshotCold`，单export gate；一次记录sampleId/label/PID/route/实际模块source声明、外部package/ELF收据、native before/after scope及单调时刻/调用结果，原样保存两API JSON及schema/UTF8 bytes/SHA，最终manifest `T531_RESOURCE_SAMPLE_V1`。两API非原子，若两读之间epoch/session/context/permission改变，标 `scopeStable=false/INCOMPLETE_SCOPE_CHANGED`，保两原件，不拼同代差分。U64原字符串保真，不能Number丢精度。

resource包含LOGICAL_REQUEST/host实际capacity、live/retiring对象generation、退出首错与dropped；cache包含entries/pins/queue、tmp files/bytes、diskScanFailed/cleanup原错/预算拒；trace包含两slot state/capacity/reserved/reader/builder；snapshot保active和End后留存事实。不可用字段NOT_MEASURED，不补0。driver仍DRIVER_NOT_MEASURED，物理RSS/PSS由外部系统命令另采，API不假装能读driver总账。

先核JSON schema及实际UTF8限额：资源原件每份≤1MiB、单次总含manifest≤2MiB，UI不显示全JSON；沿T503 bounded writer模式在native构建前给信用，不能先无界ostringstream再在JS拒。T501 ledger本身256对象/256事件/64host/32类/16退出有界，但串行输出仍须实测最大escape，超限写完整INCOMPLETE保首退出错误/计数。新限额只约束此资源导出，不抬diagnostic2MiB/timeline4MiB/coldtrace55,410,688B。原endpoint若实际最大原件超过该新输出界，更新versioned resource导出adapter，原语义不冒成功截断。

文件逐个write/fsync/hash，最终manifest失败也保事实；finally释放自己的snapshot/trace/export gate，不擅End其它请求。Export按钮回显明确COMPLETE/INCOMPLETE/调用拒因和文件名，console WS_EVENT带bytes/hash。仅查资源不能reset_run、setSource/angle、额外Flush/fence/捕获粒子或热文件写。

## 3. 独立压力lab，不伪造CURRENT或物理证据

新增独立冷诊断构建profile `T531_RESOURCE_LAB_V1`及研究页/原生lab模块，启动不依赖物理owner创建、SOURCE或有效Swap。对状态机/序列化/文件预算使用**同一生产helper类**（TraceManager、Writer、DiagnosticGate、ArtifactStore、ledger），实例、目录、ID/clock/fixture身份隔离。不能把生产全局App root/publication/current指针改成合成状态；不把合成completion归档成生产TRACE_READY数值通过。

独立模块可用`libgrs_resource_lab.so`，仅在lab构建链接测试fault宏；生产libgrsm1不启故障注入。ArtifactStore用独立实例和 `filesDir/t531-resource-lab/<session>/`，所有清理只在已解析校验的该目录；真实生产cache不删不改故障开关。lab结果schema `T531_RESOURCE_LAB_RECORD_V1`，必有 `evidenceOrigin=FIXTURE_SYNTHETIC` 或 `PRODUCTION_READ_ONLY`，`physicsValidated=false,currentGranted=false,deviceGLMeasured=false`；若单项确实用实际GL对象，另列GL实际结果，仍不替代物理验证。

接口合同（全部有界异步任务，不在UI/NAPI同步等60秒）：

```text
resourceSampleBeginV1(label, expectedScope) -> sampleId/status
resourceLabStartV1(caseId, config) -> labSession/jobId/label
resourceLabPollV1(jobId) -> fixed progress/result + original cause
resourceLabReleaseBarrierV1(jobId, barrierId) -> released/status
resourceLabCancelV1(jobId) -> cancelled (deferred cleanup if owner busy)
resourceLabExportV1(jobId) -> bounded manifest/files
```

lab仅允许预注册case/config范围，不提供任意JS读写native内存。最多一个lab session、两worker（reader/builder）、8条固定命令，固定控制记录256条×256B=65,536B；页writer≤16KiB一个、diagnostic gate≤1及原副本≤1,404,072B/2MiB。实际fixture trace block完全由生产两slot持有，合计仍≤55,410,688B，不先造第三份full vector。大fixture在slot caller span中逐字填充，明确是合成内容。lab总已知hostpayload上界=55,410,688+3,501,224+16,384+65,536=58,993,832B，另列helper固定对象、cache pin up to128MiB及worker栈/allocator；不可宣全进程硬界。

生产Page acquire/release始终同call RAII，不新增能把产品reader permit逃出NAPI的接口。pin中End/replace用lab内部reader worker持住生产Read在测试barrier，控制worker调同helper Begin/End，再归还并导出固定记录；只lab对象有此barrier。gate、时钟与ID注入只在lab，主App持续用真时钟/真实许可。

## 4. 必须可触达的case矩阵

| case | 实际调用/观察 | 不能冒的结论 |
|---|---|---|
| RESOURCE_PHASE_SAMPLE | READY前/创建后/reset/resize/释放/导出前后读生产两API；同scope系统采样原命令另存 | PSS回落不等逻辑free/driver页退 |
| TRACE_MAX_SOURCE/Q | 四profile最大合法counts/stride，由span直接填已标合成fixture，经真实Layout/Page/Writer核限额 | 合成库存不代表SOURCE/物理replay通过 |
| TWO_SLOT_PIN_REPLACE | 旧READY被reader持有、新候选成功/失败、第二替换WOULD_BLOCK、End RETIRING、归还后FREE，跟踪owner局部/retained/detached | published指针归零不代表容量归零 |
| TTL_WALL | 真实native clock、request创建起60s、≤1s异步poll、读不续期；过期后拒Page/发布 | 实际过期发现延迟另记，不能靠mock冒真60s |
| TTL_EDGE_INJECTED | 注入59,999,999,999/60,000,000,000/倒序、ID上限，label INJECTED_CLOCK | 不作设备调度时延证据 |
| DIAGNOSTIC_CREDIT | 真Capture结构/生产writer生成完整档，limit恰好actualBytes及少1；传入>2MiB应clamp；最坏escape触fallback | 不要求合法1024U32页必须恰占16KiB；测试实际UTF8而非UTF16 |
| DIAGNOSTIC_BUSY/FAIL | gate前置、同一副本计数、分配/NAPI/I/O/取消/最后manifest故障及四scope首因/faultRoot逐字段保真 | 注入分配失败不叫真实设备allocator耗尽 |
| CACHE_FAULT | 独立store remove/ENOSPC/scan/queue/pin边界、tmp实际files/bytes及恢复后收敛，文件边界沿T50232key/32MiB/128MiB | 不修改生产cache，不以artifact收据证明context句柄可用 |

writer边界可用同生产Capture填充测试，若完整档actualBytes小于2MiB就用actualBytes/actualBytes−1证边界，并另核capacity≤2MiB；不能靠增加业务可变字段去凑精确2MiB。fixture原始bytes/生成recipe/hash在档，diagnostic fallback完整JSON≤16KiB，原首因与导出故障分开。每case开始/每阶段/终态保实际object容量和scope，不只保总PASS数字。

按物理root采集满非空trace仍需T530源链/真实root；此域保NOT_REACHED，不因lab提供就宣已测。反过来，逻辑账、两slot/TTL/writer/cache这些case的真实前置是可触达的lab实现，**不必等CURRENT修复**。T519 M2包未测单列；不能复制M1数字给M2。

## 5. 完整接力

developer一次交实际T502/T507生成链两页面的生产只读导出、scope/来源双读manifest、独立lab/故障宏隔离、上表全部case入口/释放和有界导出、signedHAP/ELF/源hash。沿T507实际最终来源，不能拿早期handoff helper换掉增强字段。编译/必要非机自测即可交开发，不把独立真机重复设开发门；没有CURRENT的测试构建也必须可启动lab/read-only页。

tester先纠正T519完整SHA声明并保原系统观察，补取T526已有六页原件核hash（取不到明确INCOMPLETE_LOCAL_EVIDENCE，不能拿manifest自证）。接新实现串行真机先跑上述不依赖物理case，再在真实源/有效呈现前置齐时做同代长序列。报告分别列生产只读/合成native逻辑/注入故障/实际GL/系统物理/driver未知，失败首次回architect，不新增manager验收门。

本卡工程范围完成，T501/T502逻辑真机账及压力仍NOT_MEASURED，不把本工程稿冒设备PASS。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投；前景背景仅XComponent+shader，禁止Canvas/软件GL产品回退；FAULT或标STALE完整上一有效帧。GAP/250ms/120Hz quota/30fps/原资源预算不放宽。
