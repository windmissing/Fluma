# T345 · V2 粒子不沉降首次归因 v1

日期：2026-10-31。责任：architect。任务卡：滴答 `6ae5a6b6e4b0efba6955622c`。
状态：**CHAIN_TRACED_INTACT / TWO_MECHANISMS_NARROWED / DECISIVE_OFFLINE_REPRO_DESIGNED / ROUTE_TO_DEVELOPER_DIAGNOSTIC_FIRST**。
依据：`test-report-T343-v2-dualstate-device-v2.md`、`test/reports/t343/hilog_full.txt`（15729 行，T218DBG 15 笔注入实录）、T342 `8de02c9`、T218/T243/T274 历史链、V2 全链源码（P051DerivedV2Demo/P051DerivedDemoV2/P051DerivedFacadeV2/WaterSandField/P011ParticleHot/P011DerivedCommitV2/P013DualFieldAdapter）。纯归因零代码。

## 0. 裁定总览

| 项 | 结论 |
|---|---|
| 重力→沉降链静态核验 | **七环节逐行核验全部在位**（§1）：重力数学在 HOT collect 正确、collect 门在 step 循环正确武装、apply 路径完整、池恢复语义完整、publication 拷贝活池、渲染读 publication、dormant 排除含活粒子——**静态阅读未找到单点断链** |
| 机理判别 | 排除 (b)「LAND 恒假」的单纯形式（contact 判据 y≤sandHot 在空床容器 y=0 处必然可达）；**(a) 输入断链的设备侧变体**（步进被某运行时条件静默旁路）与 **(c) 未显断言的运行时行为**（异常吞咽/状态标志/收集空转）无法静态区分——**不冒唯一机理，按红线分列** |
| 判别手段 | **离线 Node 1:1 复现**（无需真机、确定性）：V2 facade 序列 create→inject→step(1/60)×300→逐 step 转储 pY 演化——**单一实验同时判别 (a)/(c) 与定位断点行号**（§3） |
| V1/V2 分歧点 | T95/T187 V1 线床增有证（applyMotionChannels 同路径）vs V2 悬浮——**分歧不在物理内核**（两线共用 WaterSandField.step/P011ParticleHot；V2 只多了 TupleBarrier 包裹）——**最小嫌疑收敛到 V2 包装层（derivedV2 startCommand/verifyPublicBoundary 与 preview×粒子互作）+ 页面层输入**，exact70/T216 实施未改动 HOT collect 的重力数学（:786-787 逐字保留） |
| 修复路由 | **唯一路由 → developer 诊断先行**（离线 1:1 复现 harness，产出二选一结论）→ 按结论分流工程修复或设备补证；**不两案并存** |

## 1. 七环节逐行核验（全部在位的证据链）

| # | 环节 | 源码位置 | 判定 |
|---|---|---|---|
| 1 | 重力数学（HOT collect） | P011ParticleHot.ets :786-787：`vy = wet ? … : pVy − 9.8×(dry?cos(tilt):1)×dt`；:791 `y=min(capacity, pY+vy×dt)` | **在位**——dry 模式重力垂直分量逐子步累计，tilt=0 时 vy=−9.8t，220 单位落差 ~6.7s（T343 2min 余量 17 倍） |
| 2 | collect 门武装 | WaterSandField.ets :2394 `beginSubstepMotionBudget(substepDt, true, 1)`（step 循环内、每子步）；:4703 `if(collectingTotalMotionRequests)` → saveMotionSnapshot → beginCollect | **在位**——权威 collect 每子步运行 |
| 3 | beginCollect 双收集 | P011ParticleM2Field.ets :277-293：planA/planB 双 collect + compare；sortActive（Hot :736-759）对 pState≠0 池全量排序 | **在位**——T343 渲染见 12-15 活粒子（liveCount>0 → sortActive 会枚举到） |
| 4 | apply 路径 | WaterSandField.ets :2451 applyMotionChannels → binding.apply → applyParticlePlanInto(planA)：motions 写回 live 池（Hot :1195-1200 patchScratch→publish）、deposits 经 M2Sink 写 mh/ml | **在位**——plan 消耗/回滚自洽（:624 beginStepHot 重置 consumed、:655 rollback 重置） |
| 5 | publication 拷贝活池 | P011DerivedCommitV2.ets prepareInactive → fill(state) → fillDerivedPublicationV2（WaterSandField :839-840）→ copyDerivedPayloadV2 拷贝 **live pool** pX/pY（P011DerivedSnapshotV2 :42-57 逐数组 .set） | **在位**——matchesAuthority 用 canonical.pool 校验（:120-135），commit 1157+ 证明 STEP publication 连续成功 |
| 6 | 渲染读 publication | P013DualFieldAdapter.ets :28-70：pool=field.particleState（publication 内拷贝），strict 域校验后投影 pX/pY | **在位**——D1 PASS（58+ 行 CURRENT）证明该链每帧流动 |
| 7 | dormant 排除 | WaterSandField.ets :2353（fast-path 条件含 `!(particleRouteActive && liveCount>0)`）+ :7313-7315（updateMetrics 中活粒子强制 dormant=false） | **在位**——12-15 活粒子使 fast-path 不生效 |

**结论**：静态链完整——注入（28 笔 adm>0 实证）→ 池有粒子 → collect 应运动 → apply 应写回 → publication 应拷贝 → 渲染应显示。T343 观测（位置静止+床零+LAND 无事件）与该链**静态相斥**，断点必在运行时行为中（静态不可见的分支/吞咽/标志）。

## 2. 机理分列（不冒唯一，按红线）

**(b) LAND 恒假的单纯形式——排除**：contact 判据（Hot :794 `pState===2 || y≤sandHot(work,c)`）在空容器（sandHot=0）下 y 降至 0 必触发；「阈值/坐标系不一致」无静态支点（同公式在 V1 线 T95/T187 时代产床有证）。保留其**变体形式**（粒子根本没降 → contact 永无机会评估），归入 (a)/(c'')。

**(a) 输入断链（设备侧变体）——无法静态排除**：重力输入本体（collect 用 `work.effectiveTiltRad`，来自 beginCollect 的 field.effectiveTilt）在位；但「步进被静默旁路」的设备侧形态（页面生命周期使 tick 序列与预期不符、传感器时钟路径副作用）静态不可见。**反线索**：commit 1157+ 连续推进说明 STEP publication 链每步成功（recordEndpointCommit 只在子步成功后调用 :2487-2488）——若子步被旁路，commit 不应推进——**此反线索同样压向 (c'') 的特定形态：子步执行了、collect 执行了、但池位置的演化在某个环节被回退或不落地**。

**(c') mediumMode 字符串失配——静态排除**：`P051_DRY_MEDIUM: string = 'DRY_AIR_V1'`（P051DryParticleAdmission.ets :3）与 Hot :788 的比较字面量**完全一致**——dry 重力分支必然执行；且即便走 wet fallback（终端速度 64）粒子也会快速下落，与「冻结」矛盾。**(c'') 其余运行时行为（异常吞咽已由 model 未停排除、PARTICLE_BEGIN 失败已由 step 未停排除）**：剩余形态收敛为「collect 成功执行但池写入被 V2 包装层某状态静默回退/跳过」或「设备层（页面生命周期/传感器时钟）旁路」——静态不可见。

**(c') mediumMode 字符串失配——静态排除**：`P051_DRY_MEDIUM: string = 'DRY_AIR_V1'`（P051DryParticleAdmission.ets :3）与 Hot :788 的比较字面量**完全一致**——dry 重力分支必然执行；且即便走 wet fallback（终端速度 64）粒子也会快速下落，与「冻结」矛盾。**(c'') 其余运行时行为（异常吞咽已由 model 未停排除、PARTICLE_BEGIN 失败已由 step 未停排除）**：剩余形态收敛为「collect 成功执行但池写入被 V2 包装层某状态静默回退/跳过」或「设备层（页面生命周期/传感器时钟）旁路」——静态不可见。

**候选清单收敛（静态排除后）**：(a) 设备侧步进旁路（页面生命周期/传感器时钟路径使权威子步未如预期执行）/(c'') collect 执行但池写入被 V2 包装层某状态静默跳过/回退。两者**只有离线复现可分**（§3）。已静态排除：异常吞咽（model 未停反证）、PARTICLE_BEGIN collect 失败（step 未停反证）、mediumMode 字符串失配（:3 与 :788 字面一致）、wet-fallback 冻结（终端速度 64 会下落）。

## 3. 决定性判别实验（离线 Node 1:1 复现，无需真机）

**为什么离线复现是唯一正确手段**：V2 链全部是确定性 ArkTS（无传感器依赖——demo 页传感器只驱动 setTilt，复现可固定 tilt=0）；T343 现象跨 run 可复现且逐位稳定（s1≈s4）→ 若断点在代码，离线必然复现；若离线不复现，断点唯一可能在设备层（传感器/时钟/页面生命周期）——二分干净。

**Harness 规格（developer 诊断卡输入）**：
```
1:1 移植链（沿 compile_hot_modules.py 先例）：WaterSandField + P011ParticleHot/M2Field/M2Sink
+ derived/ 九文件（V2 facade/barrier/commit）→ Node 直接执行；
序列：new P051DerivedFacadeV2(config) → inject(×3, 同 T343 参数 requestedH=8×sec, y=220)
     → step(1/60) × 600（10s 物理）→ 每 60 步转储：
  a) binding.liveCount、pool pY 的 min/mean/max（活粒子）
  b) work.planA.motionCount/requestCount（collect 是否产生运动/请求行）
  c) work.mediumMode（§2 c' 检查点：==='DRY_AIR_V1'?）
  d) derivedV2.status/counters（barrier 是否 fault）
  e) mh/bl 列和（床是否增长）
判据：pY 逐窗下降 → 离线不复现 → 断点在设备层 → 真机补证接点（§4）；
      pY 恒 220 → 离线复现 → §2 三候选逐个激活判别（改一行重跑）→ 断点行号。
```

**真机补证接点（离线不复现时用，最小化）**：T218DBG 增一条 step 行（每 60 帧一条：`pYmin/pYmean/live/motionCount`），同批真机卡采集——区分「设备层输入旁路」。

## 4. V1/V2 分歧点定位（git -S 范围核验）

- **物理内核零分歧**：T95/T187 V1 线与 V2 线共用 `WaterSandField.step → substep → beginCollect → applyMotionChannels` 与 `P011ParticleHot` 全部函数；git -S 沿 derived/ 九文件与 Hot 接入查证：T216/T214 exact70 实施只增了 derivedFieldWriteV2 包裹与 TupleBarrier 校验层，**HOT collect 的重力/运动/接触数学逐字未动**（:786-794 与 T342 主链核验版一致）。
- **分歧面 = V2 包装层**：enableDerivedV2Cold（barrier 生命周期）+ startCommand/finishCommand 包裹 inject/step + DerivedCommitV2 publication（matchesAuthority canonical 校验）+ P051DerivedDemoV2（demo 模型层）。**设计意图核验**：T207/T214 设计稿的 V2 语义是「数值策略 exact70 + 元组屏障校验」，**不含「沉降延迟到 publication」或任何冻结粒子的设计条款**——若复现证实冻结，属实施遗漏/缺陷，非设计如此。**不预判 designer**：路由仍以复现结果为据。

## 5. 修复路由（唯一）

**→ developer 诊断卡（离线 1:1 复现 harness，§3 规格）**：
- 产出二选一结论（离线复现行号 / 设备层排除）；
- 复现 → 同卡内最小修复（按激活的候选：mediumMode 失配→一处字符串/传参；barrier 状态→startCommand 时序；其余→对应行）+ 非机回归（T216 套件 7/7 + strict pack 6 份 + 本 harness 固化为回归用例）；
- 不复现 → 交 tester 真机补证卡（§3 真机接点），按结果回流。

**不采**：tester 直接修（红线）；designer 重估（无设计矛盾证据，T207/T214 无冻结条款）；manager 用例裁决（非判据问题）。

## 6. 红线核对

- 未改任何物理 dt/阈值/守恒；未推翻 T342（code100 维度维持有效）；未采 tester 修法；与 T207/T214 设计的对照如实（§4：设计无冻结条款）；未把单页现象外推 V2 全局（EmptySandDemo V1 链 code100 已澄清维持）；不冒唯一机理（§2 分列+反线索标注）。

## 7. 变更记录

- 2026-10-31 v1：初版。七环节逐行核验表（链静态完整）；机理分列 (a)/(c')(c'')（含 mediumMode 字符串检查点与 wet-fallback 反推）；决定性离线 1:1 复现设计（5 项转储判据+二分出口）；V1/V2 分歧定位（物理内核零分歧、分歧面=V2 包装层、设计无冻结条款）；唯一路由 developer 诊断先行。
