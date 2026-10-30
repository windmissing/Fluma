# T312 V2 整体复杂度真机续证（真机，结束待验收）

2026-10-30，tester 真机（VYG-AL00 62T0225B10010020）。包：T311-signed.hap。**注意**：实际安装包实测 SHA `366d1ca6bdc24678ef6b5328dd17e9db03ae358ded6d4ccbbde648a4473ace94`，与 T312 卡面给出的 `9e7ac541…` **不一致**（T311 卡未明示此差异；本报告按实际安装包取证，SHA 差异如实登记交 architect 核对交付清单）。

## 一、T311 阶段计量真机产出（1Hz 有界冷 timer，20 stage 字典）

四 run（59315 满注入 / 61528 脉冲 / 63414 低剂量）采集 32+ 行 `T311_STAGE`，字段 R/N/B/live/retry/bytes/counts/totalMs/maxMs 全在（`stage_all.txt`）：

- **真实工作规模（非容量冒实测）**：B=399（实际绑定数）、N=512、live=512、R 随注入 0→261（满注入）/0→242（2s 低剂量）、**retry=0、rollback=0**、copiedBytes 峰值 39.6MB。
- **各阶段累计/最大 inclusive 耗时（Date.now wall ms，报告按交接要求不做简单求和冒端到端）**，末行（R=261）：

| stage | total ms | max ms |
|---|---|---|
| storage | 105 | 1 |
| fieldBinding | 8 | 1 |
| prepare | 91 | 1 |
| lease | 75 | 1 |
| readTransfer/restoreTransfer/snapshotCopy | 66 | 1 |
| **audit** | 1165 | 50 |
| sort | 777 | 63 |
| budgetAudit/derivedFlush/capacityRead | 37 | ≤2 |
| collect | 14 | 7 |
| **apply（含嵌套 audit/sink）** | 1449 | **1449** |
| sink | 1420 | 52 |

## 二、T285 缺陷修复确认 + 残留复杂度定量（本卡核心成果）

- **ownsBufferHot membership 扫描已消除**：三 run 全部阻塞栈中 ownsBufferHot 出现 0 次（T285 栈顶特征消失）——T311 冷注册 B 表修复真机生效。
- **阻塞点转移且线性定量**：新热点 `auditParticleHotBody`（O(R(N+C)) 逐行审计）+ `sortOrderHot`（O(L log L)）。**apply 单事件耗时随 R 严格线性**：R=58→335ms、82→460、109→599、137→816、166→898、196→1060、228→1282、261→**1449ms**；最小二乘拟合 **apply ≈ 5.45 ms/R → R≈1102 时单事件超 6s watchdog 必杀**（59315/61528 两 run 死于 R 260+ 持续注入后的延迟死亡，栈均为 audit→applyInto→applyParticlePlan，与注入模式无关）。
- 与 T311 交付声明一致：残留 `O(R*(B+N log N+C))`、whole-audit reads `Ω(R*(N+C))` 保留——**非次线性，不能离线保证 watchdog 消失**的声明被真机定量证实。

## 三、watchdog / 时钟 / 锚点

- watchdog：THREAD_BLOCK_6S 两 run 记录后 signal 9（R>~250 持续注入时）；低剂量 run 63414 单事件 ≤1.4s 未达阈值，**进程长期存活（>20 分钟）**。
- **sinceFirstBirth ≥8s 首次真机达成**：T267 缺陷 A（差值法）修复确证——sinceFirstBirth 随 step 真实累计（0→0.77→2.77→4.77→**8.77**），T266_CLOCK_V3 周期锚点 PERIODIC 连续触发（frame 38/60/120/180/300，physicsCommit 604）。
- 源勘误：admittedMass=23.33（非整）、rejectedCount=0、sourceDropped=0；stepDropped=44（当前实现丢弃计数显式）。
- **冷采锚点完整性**：FIRST_BIRTH 与 **SINCE_BIRTH_8S** 两锚点均触发，冷采状态如实 **INCOMPLETE / reason=BUDGET_CAPTURE_MS**（captureMs 19.3/46.5ms 超 capture 预算）——无冷分片产出，按卡**不冒冷采 PASS**；T284 INCOMPLETE 规则真机生效。
- 物理推进速率被 audit 负载拖慢（~2s 物理 / 5min 墙钟），LAND→床量→同 token 像素链 NOT_RUN（8s 达成后床仍为 0 前景，且冷采 INCOMPLETE 无分档可核；不伪称完成）。

## 四、结论与移交

- T311 修复真机确认 + 残留复杂度真机定量（5.45ms/R 线性）+ since≥8s 首次达成 + 冷采 INCOMPLETE 语义真机生效——**结束待验收**。
- 交 architect：①实际包 SHA 与卡面 SHA 差异核对；②据 5.45ms/R 线性与 audit Ω(R(N+C)) 决定工程/设计接力（T311 交接已预留分诊路径）；③冷采 BUDGET_CAPTURE_MS 预算口径复核。

证据：`test/reports/t312/`（stage_all.txt 32 行、anchors.txt 14 行、两份 THREAD_BLOCK faultlog、截屏）。
