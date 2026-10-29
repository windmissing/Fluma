# T243 床沙不可见与低帧率：T249 到件后的工程交接 v2

2026-10-29，architect；T265 恢复已在 T243 原卡报告、但未留在当前工作树的 v2 工程结论。基线为 [v1](architecture-T243-v2-bed-visibility-attribution-v1.md)，本稿消费 `docs/test-report-T249-v2-bed-land-evidence-v1.md`、`test/reports/t249/{t249_tail400,t249_inject_all,t249_freeze}.txt`、`test/t248/T248Observation.ets`、`test/t248/generate.cjs` 与现有 V2 页面/物理源码。只恢复工程文档，不使用真机、不重跑测试、不改业务代码。此稿为 T266 的工程前置；T267 真机续证后再回 architect 判定产品根因。

## 1. 首次归因与证据边界

| 事实 | 原始证据及计算 | 当前结论 |
|---|---|---|
| 源准入 | `t249_inject_all.txt` 有 24 个请求；seq1–23 `ok=true`，seq24 `ok=false,adm=0,rej=CAPACITY_BLOCKED`。请求 `sec` 总和 8.2545，成功请求携带的 `sec` 总和 7.2545；成功准入 `adm` 总和 50.09851504516602，按本 route 标称 8 mass/s 折算 6.262314380645752 秒当量（seq23 部分准入）。 | T249 报告“24 次全部成功”和“源注入 8.255 秒已满足累计接受模拟≥8秒”均不能成立。请求秒、准入量当量与物理 step 时间是三个量；容量拒绝按原收据保留，不算 LAND 缺陷。 |
| 首次出生后的物理时间 | seq1 在 12:41:43.934；此前最近成功 `T248_STEP` 于 12:41:43.868，观察器累计 42.400000000000155；最后成功 step 于 12:42:36.998，累计 43.533333333333424。两者相差约 **1.133333333333269 秒**，对应 34 个成功的 1/30 秒 step。`t249_tail400.txt` 的 100 个成功 step 总计 3.333333 秒，其中前 66 个在首次出生前。 | T248 观察器累计 43.53 秒跨 run；首次出生后的已见成功物理时间约 1.13 秒，既定 ≥8 秒锚点没有到达。无 LAND/床量 0 与 512 活粒悬浮有证，但不能据此归 LAND 数值、publication 或床层 shader 缺陷。 |
| 同步冻结 | 注入前 66 个 step 的 `stepMs` 中位数约 75ms；注入后 34 个 step 从 79ms 增至末次 8360ms，末 10 个中位数约 3911ms。`stepMs` 与产品 `original.physicsTimeMs` 逐条基本相同。`WaterSandField.stepWithTiltTrace` 从入口到返回包围这段计时；末次调用与 `THREAD_BLOCK_6S` 事件重叠。 | 性能恶化边界已落在同步 `field.step` 调用内，足以造成低帧率/冻结风险；内部 planner、粒子、审核、事务历史、GC 等尚无分段证据，不能选唯一热点或改数值算法。绘制段 `renderInclusiveMs` 37–46ms 不能代表整帧。 |
| 停源顺序 | seq24 在 12:41:52.296 容量拒绝，`P051DerivedDemoV2.tick()` 对失败调用 `stop()`；随后仍有多次成功 step，直到 12:42:36.998，耗时持续增长。`t249_freeze.txt` 只有 watchdog/Faultlogger 行，无 JS/原生堆栈。 | 不能称“停源后首个 step 冻结”或已证明死循环。23.6MB 单次 heap 不证明没有临时分配/GC。 |
| 床量与绘制 | tail 中最近已提交 LAND 事件页 100 次均 `[]`；末次权威 `bh/bl/mh/ml`、同 token publication/packed 床量均为 0。T248 LAND 钩子只读最近事件页，并非全 run 完整归档。 | 本窗口权威床量为零，床层缺像素无可对对象；后续只有先见非零权威床量，才沿 publication→packed→shader/pixel 分流。 |

以上纠正 T249 报告的判据与表述，保留原报告和原始日志不改。DD12 床沙可见性及低帧率产品目标仍未通过；T249 执行卡关闭不等于产品 PASS。既有 CPU V2 仅用于定位现有失败，新增效果仍按 GPU 优先。

## 2. 已定位的诊断工程缺口

### 2.1 run 身份和计时语义

`test/t248/T248Observation.ets` 的 `acceptedSeconds/stepCount` 存于页面模型实例；产品 `P051DerivedDemoV2.reset()` 只重建 facade/field 和序号，隔离生成器没有清空观察器。故跨 run 的 43.53 秒被误当单 run。`test/t248/generate.cjs` 在每帧为 publication 另建 `new T248Observation().capture(...)`，该对象时长恒 0；这不是 publication 床量漏拷的证据。现有 `recordStep(dt, ok, stepped)` 还用整 root 布尔值累计，缺少已提交子步前缀身份。

T266 在隔离包中实现如下只读合同；不得改 `dt=min(1/30,wallDelta)`、源准入、物理事务或阈值：

```text
T248RunClockV2 { runId, stateEpoch, firstBirthCommit?, acceptedPhysicsSeconds,
  acceptedPhysicsSecondsSinceFirstBirth, committedSubstepCount,
  sourceRequestedSeconds, sourceAdmittedMass, sourceRejectedCount }
T248StepV2 { runId, stateEpoch, frameSerial, commitBefore, commitAfter,
  requestedDt, attemptedSubsteps, committedSubsteps, substepDt,
  committedDt, rootStatus, firstCode, clockBefore, clockAfter }
T248SourceV2 { runId, stateEpoch, sourceSequence, requestedSeconds,
  admittedMass, rejectedReason, commitBefore, commitAfter,
  admittedEquivalentSeconds? }
```

- 创建、reset 或 `runId/stateEpoch` 更换时清零本 run 计时，记录不可变身份；跨 run 合计如仍保留，必须单独命名。首次**成功出生**冻结 `firstBirthCommit`，不得用首次请求或仅源成功但 birth=0 替代。`admittedEquivalentSeconds=admittedMass/8` 只表示本 route 固定源率当量，绝不命名为模拟秒。
- `committedDt` 只由已提交的 `committedSubsteps*substepDt` 增加；未执行、拒绝、rollback 不累计。若 root 部分提交再失败，明确成功前缀及对应 commit，不用 `r.derivedFault===null` 或 `r.original.stepped` 单布尔推全 root 成功。若当前 public result 不足以区分局部前缀，须加默认关闭的只读提交钩子，不改成功/失败语义。
- STEP、SOURCE、LAND/权威、publication 和渲染锚点均带 `runId/stateEpoch/physicsCommit/frameSerial`，源事件另带 sourceSequence、子步另带 substepId。字段无证据时为 `NOT_CAPTURED`，不能填 0 冒测量。不得把完整全局 `acceptedSeconds` 拷到新 publication 观察器来伪造同源身份；用同一 run clock 的只读快照。

### 2.2 有界阶段计量与成本隔离

`WaterSandField.stepWithTiltTrace()` 当前仅给整 root 的 `physicsTimeMs`；它在 `P051DerivedV2Demo.tick()` 的 UI frame 回调同步运行。T248 生成器每帧把 authority/publication 大对象序列化，并对整块 pack `Array.from` 写 hilog；这会额外分配、序列化并扰动帧率，当前 0.19fps 不能直接当无诊断版本的产品帧率。

T266 生成**仅隔离包启用**的预分配固定容量计量：

1. 外层分 `source`、`field.step`、cold publication、pack/upload、shader draw/swap 和诊断序列化/日志自身时长。`renderInclusiveMs` 保留原含义，不重命名为整帧；记录单调时钟与 wall 对齐点、每段入口/退出及未归属开销。
2. 在 `WaterSandField.stepWithTiltTrace` 的每个尝试子步，分别标记 `consumePendingGestureSand`、`saveBackup`、`planAndCommitTilt`（必要时细分 preview）、`substep` collect、`restoreMotionSnapshot/applyMotionChannels`、`recordEndpointCommit/finishNaturalObservation`、`changedSinceBackup/updateMetrics`。每条记 root/substep 身份、pool live/capacity、阶段时长/状态、firstCode；入口无出口时标 `INCOMPLETE`，需系统堆栈或可见阶段标记才可定位卡点。不得在业务临界段写大 JSON 或文件，不能改原 firstCode、receipt、事件、lineage、rollback。
3. 正常帧只写固定大小标量/哈希摘要并限流，完整床列/粒子/packed 冷快照仅在预注册锚点采；记录采样字节与开销。环形缓冲满按明确丢弃计数，不阻塞 UI；无法安全在冻结前导出时注明 `NOT_CAPTURED`，不伪称已捕获最后阶段。保持 T248 原始完整日志方案仅作为可开关对照臂，T267 用相同 fixture/包身份分别采“有/无重日志”并报告额外成本，不能把两臂混算。
4. 非机回归比较计量开关前后的权威 state/receipt、失败首码、源收据、LAND 事件、rollback 与 frame token；验证 reset/新 run 清零、局部失败前缀和固定容量。编译与非机自测即 developer 交付，设备 p50/p95/收益由 T267 执行。

若阶段计量显示某个具体工程热点，再由 architect 修相应工程稿后交 developer 修复；若根因需要改变墙钟追赶或物理 dt 语义，转 designer，不能由计量任务暗改算法。

### 2.3 隔离包冷启动的合法页面

`test/t248/generate.cjs` 把克隆包 `main_pages.json` 收窄到 `pages/P051DerivedV2Demo`，却以无命中断言的 `replaceAll("'pages/Index'", ...)` 改 `EntryAbility`。当前 `EntryAbility.onWindowStageCreate()` 默认分支实际选择 `pages/SandCanvas`，不在克隆包页面白名单，因此冷启动 errorCode 13；T249 通过 want 的 `p051v2` 参数成功进入 V2，已取得的 V2 帧证据仍有效。此处只证明旧路径**尝试加载并失败**，没有证据证明 Canvas 曾上屏。

T266 生成器应对克隆包 `EntryAbility` 的**唯一默认分支**做命中数量断言并改为 V2 页，同时验证该页在克隆 `main_pages.json` 中；对任意未识别生成器源码版本 fail closed，而非静默 `replaceAll` 失败。保留显式 want 路由；加载失败按新管线合同报 FAULT，不加 Canvas 页面或回退。非机校验生成源码、页面清单、PackageHap；T267 真机只做一次冷启动 V2 route 取证，不重跑已过矩阵。

## 3. T266/T267 交付顺序与错误边界

**T266 developer，不使用真机。** 以 `test/t248/T248Observation.ets`、`test/t248/generate.cjs` 为主要改点，生成包内 `physics/derived/P051DerivedDemoV2.ets`、`pages/P051DerivedV2Demo.ets`、`physics/WaterSandField.ets`、`entryability/EntryAbility.ets` 为明确接缝。若必要修改共享生产 `WaterSandField`，只能增加默认关闭的只读有界钩子，并与在途 T236 平台 owner 协调共享 hunk。保持新 XComponent+shader 全背景/前景，禁止 Canvas 上屏/失败回退；故障为 FAULT 或显式 STALE 上一有效新管线帧。输出源码/生成器/包 SHA、固定 revision、原始非机等价日志、构建及可部署签名 HAP；签名失败按原卡如实交接，不增设备为开发门。

**T267 tester，真机串行。** 在 T266 可部署包与 SHA 到件后，先纠正 T249 源请求 23/24 和两类时间用语；同一新 run/epoch 从首成功出生累计成功物理 step `≥8s`，采首次 LAND→权威床量→同 token publication→packed→实际像素及后 3 完整 CURRENT 帧。若性能使锚点不可达，如实 `BLOCKED`，归档有界阶段 trace、未归属成本、系统 watchdog/堆栈（若权限不足明确 `NOT_CAPTURED`）、重日志对照。另证一次冷启动只进合法 V2 页。不要用 SOURCE 秒、跨 run clock、签包或旧窗口截图冒产品 PASS。结果回 architect 后决定工程热点修订、设计问题或下一轮精确补证；权威床量仍 0 时不派床层 shader 缺陷。

## 4. 版本变更记录

- v1（T218 到件）：20 秒墙钟无法证明真实模拟足以落床，要求同 run 诊断。
- v2（T249 到件，T265 恢复）：直接按原日志定量确认首次出生后仅约 1.133333 秒成功物理 step；修正 23/24 源成功与源时长语义；定位同步 `field.step` 整 root 耗时恶化，但不指认内部唯一热点；明确 T248 run 身份、重日志成本和冷启动路由工程修订，再交 T266/T267。T243 原执行已交付，T265 仅恢复工程交接，不新增 manager 验收门。
