# T222：明水 C 补篇与落水 V2 工程定版

2026-10-28，architect；原卡 `6ae1a825e4b0c1a6bd5e81d7`。算法唯一依据 `algorithm-T221-sand-water-demo-v1.md` / `CW_DEMO_FLIGHT_DESIGN_V1`（提交 `4725b23`）；消费 T219 部分工程 `95ac99b`、T209 A/B 与 T215 独立证据。逐槽布局、输入哈希、定容公式见 `analysis-T222-clear-water-flight-engineering-manifest-v1.json`。**ENGINEERING_READY_FOR_STAGED_DEVELOPMENT；C、V2、V4、页面均尚未实现或验证。** 不使用真机，不写业务代码或重跑产品测试。目标8/P3排目标1–7之后，GPU compute仍最高调度优先。

## 1. 版本、当前接口冲突与唯一 owner

目标7 V1 A/B 的 `SINGLE_SAND_CLEAR_WATER_V1`、源 row20/56/24、`B_EVENT40`、原 `audit()`、snapshot/status 0..22、原 ID 顺序 P、已通过的离线证据全部保持其原义。目标8是**新 run 冷建**的 `SINGLE_SAND_CLEAR_WATER_FLIGHT_V2`，显式绑定 `CW_SOURCE_FLIGHT_V2 / CW_FLIGHT_STATE_V1 / CW_B_FLIGHT_STEP_V2 / CW_FLIGHT_EVENT48_V1 / CW_REPLAY_TAPE_V1 / CW_FLIGHT_RESERVATION_V1 / CW_SCENE_FLIGHT_V4 / CW_WATER_OPTICS16_V2 / CW_GRAVITY_ACCEPT_V1`。任何 V1 request/snapshot/packet 进 V2 reader 都拒版本，不能“兼容”解释。V2 不能从已有 V1 活 run 迁移或热挂水包；新 run 首态空 W/沙/J，`restoreBInto` 仍 `UNSUPPORTED_FEATURE`。这不是 T207 DRY 数值 V2，W 真实非零，沙 P 继续原 ID 顺序 RN fold。

源码核对后的必要改造点：`ClearWaterState.rebuild/audit/waterTotal` 只知 W，`ClearWaterSource.commitNext` 当前水源立即写 W，`ClearWaterBStep.runInto` 自行审计、replay、提交并独占 event40；直接在页面加水点动画、或整步后另写 J，均不能满足同源账/同 root 事务。新 C 只读 capture 与源终态 reader 仍按 T219 实施；本稿仅覆写 T219 §7 两缺口、optics8 草案和 flight V4 增量，不废 T219 profile8/透射/完整帧 swap 的已定原则。

唯一 `ClearWaterEngine` owner 在冷配置中选 `V1` 或 `FLIGHT_V2`。建议 `ClearWaterEngine.createFlightV2Into(aConfig,bConfig,flightConfig,out): ClearWaterEngine | null` 为未暴露的构造事务：新建 A owner→冷 attach B→分配/绑定五 bank 的 water pool/J/header、source suffix、双 arena/tape、V2 exact 与计数→空 run V2 audit→成功才返回；失败丢 shadow owner，不能公开半配置。V1 现有 constructor/`attachBInto`/`advanceSubstepInto` 行为保持。V2 必须启 B 并使用专用 `advanceFlightSubstepInto(dt,gravityToken,out)`；旧 V1 advance 在 V2 返回既有 `S.UNSUPPORTED_FEATURE` 加独立 `FlightReason.INCOMPATIBLE_ROUTE`，反向亦然，不插改原 coarse status 枚举。`resetFlightV2Into()` 只允许明确新空 run，并一次清两池、J/账/源 suffix/arena/tape/时钟/角度，不用 initial 重设运行水基准。

V2 对外签名和 Out 所有权固定如下；所有 Out 在 cold 由调用方一次分配、入口只填已有字段/typed arrays，不在热路径 new；无全量返回 `snapshot()`：

```ts
static createFlightV2Into(a: ClearWaterRunConfig, b: ClearWaterBConfig,
  f: ClearWaterFlightConfig, out: ClearWaterFlightCreateResult): ClearWaterEngine | null
resetFlightV2Into(out: ClearWaterFlightResetResult): ClearWaterStatus
commitFlightQueuedSourcesInto(boundary: ClearWaterBoundaryIdentity,
  out: ClearWaterFlightBatchResult, receipts: ClearWaterFlightReceiptSlots): ClearWaterStatus
advanceFlightSubstepInto(dt: number, token: GravityAcceptanceToken,
  out: ClearWaterFlightStepResult): ClearWaterStatus
copyCurrentFlightSceneInto(expected: ClearWaterFlightBoundary,
  out: ClearWaterFlightPublicationSlot): ClearWaterStatus
copyLastFlightStepInto(expected: ClearWaterFlightSealedIdentity,
  out: ClearWaterFlightEvidenceBuffers): ClearWaterStatus
copyFlightFrameCountersInto(out: ClearWaterFlightFrameCounters): ClearWaterStatus
copySourceReceiptInto(request: ClearWaterSourceRequest,
  out: ClearWaterSourceReceipt): ClearWaterStatus                 // V1 only
copyFlightSourceReceiptInto(request: ClearWaterFlightSourceRequest,
  out: ClearWaterFlightReceipt): ClearWaterStatus                // V2 only
```

`FlightConfig` 冷绑 route/revision、Kw=64/ew=8、水 event/tape 容量、gFall/dragFlight/vEmit、source emissionPolicy、sort/flight/acc版本与 calibrationStatus；候选运动/光学参数有限合法并标 PENDING_CALIBRATION。`FlightCreateResult` 给 status、first reason、真实 allocation ledger revision/payload；失败 null。`FlightStepResult` 沿 B 原 step fields 增 first FlightReason/stage/globalOrdinal/waterEventCount/tapeCount/arrivalCount/simulationTime before/after/angle token revision，status沿 A coarse 0..22；`copyLastFlightStepInto` 必须同时输出同一 root 封存的 B40、水48、tape、双header与当前 canonical commit（可晚于 sealed），不外借可变 view。`copyFlightFrameCountersInto` 将现有 B48、T219 C32与 V2 arena/header 实际计数装入调用方预分配 Out，不另偷加 owner 热数组；scope须 `COMPLETE_EXPLICIT_FRAME` 或 `PARTIAL_SCOPE`，缺设备计时为 NOT_MEASURED。`copyCurrentFlightSceneInto` 在 owner busy guard 下核run/config/geometry、所有目的 buffer 非 alias，一次 O(N+K+Kw)复制当前 canonical；失败不切 publishedIndex。所有 public 入口拒跨route/token，保留 first stage，不把成功 source 或首子步回滚。

## 2. 五 bank、权威总账与审计

`canonical/sourceBefore/stepBefore/work/replay` 五份仍互不 alias；每份追加 `WaterPool(Kw)`、`J:F64[N]`、`waterHeader:F64[16]`，复用现有沙 `originSourceCommit/waitReason/bHeader`。水池每 slot 为 `mass,x,y,vx,vy,originSourceCommit,birthPhysicsCommit:F64[Kw]`、`id:U32[Kw]`、`state,wait:U8[Kw]`、`prev,next,freeSlots:I32[Kw]`。活链严格 waterId 递增，`nextWaterId` U32 正单调，退役 slot 清质量与状态、按确定 free 顺序回收；复用 slot 不复用 ID。state `AIR=1, INTERFACE_WAIT=2, SIDE_WAIT=3`；wait `NONE=0, SURFACE=1, SIDE_CAPACITY=2, NUMERIC_ARRIVAL_BLOCKED=3, HIGH_BED=4`。这两套枚举只给 V2，不能把沙 state/tag 重编号。

`waterHeader[0..15]=[head,tail,freeCount,liveCount,nextWaterId,arrivalSerial,simulationTime,lastArrivalPhysicsCommit,flightRevisionCode,waterArenaSerial,emissionPolicyRev,flightConfigRev,sortPolicyRev,0,0,0]`。空链 head/tail=-1；`simulationTime` 只在成功正 dt 以规定 RN 顺序增加，必须有限且确实前进。entry 与 replay 校验 count、free/prev/next、未用 slot、ID、origin、birthCommit、状态/坐标/速度。source 成功出生时 `originSourceCommit=本事务提交序号`，`birthPhysicsCommit=本源提交后的物理序号`；后续 move/arrival 不改来源。所有五份 `copyFrom/reset/storageValid/allocations` 必须纳入这些数组；A `toJSON()` 和 V1 snapshot 字节/字段不夹 V2。V2 暂无可恢复 snapshot，禁止把 V1/B snapshot 当 V2 保存/恢复包。

对每列按**waterId 升序**从活链折 `J[c]`；沙 `P[c]` 仍按 sandId 活链升序。`S=RN(RN(RN(bh+0)+mh)+0)`、`eta=RN(S+W)`、`R=RN(eta+P)`、`R8=RN(R+J)≤256`。`J` 是飞行保留库存，不抬 eta、不参与水头或光学深度；`wet=(W>0)`。`Wtotal=RN(fold_c(W)+fold_waterId(mass))`，对 `RN(initialWater+I_water)`沿原累计相对 drift `<.001` 核；沙总量/原阈值沿 B。正源 `I_water`/waterSuccessCount/sourceCommit/physicsCommit 各只推进一次；MOVE/CROSS/ARRIVE 是内部转移，不改源账或 initial。源/整步实际差沿原绝对 `1e-6`，不补尾质量或新容量 EPS。flight 正质量即使 `Wtotal` RN 不变仍是实际库存；DIRECT 正 q 若 W raw bits 无写则拒无效成功。

V2 新 `auditFlightInto(state,marksSand,marksWater,out)` 与 `rebuildFlightInto(state)` 在固定 bank 上执行：保留 V1 全部沙结构、stock、缓存、finite 和 P/R 检查，**仅将水总账判断改为 W+水包并补 J/R8/水池核**。不能先运行 V1 `audit()` 再吞水总账失败或暂改 `I_water`；V1 路径仍调用原 `audit()`。V2 `waterTotalFlightInto` 具有固定列→waterId fold 次序，不能只用 ΣW。仅在 source/step候选末尾及 replay末尾进行 whole fold/audit；每事件用局部 exact guard，禁止每行扫两池。

## 3. 容量与辅助精确质量

复用 B `ClearWaterExactMass` 70×U16编码和 `capacityBound70[N]`，在 V2 将 `pExact70` **仅作为私有工作数组升级为 `Gexact70[N]`**：每列精确加沙活动质量和当前列水包质量，按各自 stored mass 单独编码。入口 `Eentry=exact(S)+exact(W)+Gexact`、`Bcap=max(exact(256),Eentry)`，现有RN可容微小 exact 越界入口保留；每次 patch 不恶化 Bcap，最终无条件 `R≤256` 与 `R8≤256`。V1 仍只积沙 Pexact，不改行为、版本或比较器。水包跨列、ARRIVE、W 水边、床边、沙 CROSS/LAND、source birth 均需把 J 放入两端容量预检；水不扣固体 300dt/28dt 预算。

工程调用分 `previewMassPatchInto(c, oldS,newS,oldW,newW,oldSandMass,newSandMass,oldFlightMass,newFlightMass)` 与 `applyMassPatchInto`：旧/新 stored operands 分别编码后加减，先核全部涉及的1或2列、token、事件容量，才统一写 `Gexact` 和 work；不以 `RN(new-old)` 控制量冒实际差。移动水包整质量，不拆 ID；目标不足保持原列与 `SIDE_WAIT`，不穿墙/高沙。ARRIVE 同列先预检 `W'=RN(W+m)` 确有 raw 写、更新 J 后容量和水总差都合法；不合法保持 `INTERFACE_WAIT`、原因 `NUMERIC_ARRIVAL_BLOCKED`，不退 slot、不缩 q。过程中的 `work.J`/`replay.J` 为按事件顺序 RN 更新的**局部摘要**，供 event48 旧/新 bits 对照，最终必须按 waterId 全折重新写成权威 J；exact guard 不依赖该临时 RN 摘要。每次 source/step final 独立重算 J/R8 与总账。

## 4. V2 source suffix 与终态 reader

单 FIFO≤64、dedup/event≤4096，保留 V1 key `(runGeneration,inputGeneration,gestureId,sourceId,requestSeq,kind)` 及原 payload raw bits；V2 revision、route、emissionPolicyRev/cold config 另绑定同 run，旧 request 不可跨 run/版本重放。V2 仍复用 queue20/dedup56/event24 的**公共 prefix 布局和索引**，另随同 rowIndex 分配 `dedupSuffix16:F64[D×16]`、`eventSuffix8:F64[D×8]`，不是第二张去重表。prefix 中全水前后/delta在 V2 revision 下明确为 `Wtotal`，V1 reader拒 V2。suffix16 槽0–15为 `[settledBefore,settledAfter,flightBefore,flightAfter,JcolBefore,JcolAfter,firstWaterId,waterBirthCount,deliveryMode,nextWaterIdBefore,nextWaterIdAfter,arrivalSerialAtBirth,emissionPolicyRev,birthSimulationTime,0,0]`；suffix8槽0–7为 `[waterId,deliveryMode,JcolBefore,JcolAfter,nextWaterIdBefore,nextWaterIdAfter,emissionPolicyRev,birthSimulationTime]`。`deliveryMode` NONE0/DIRECT1/FLIGHT2。未用槽包括沙请求水专用字段为+0；原始 request20 不偷塞参数。新增 `ClearWaterFlightReceipt` 保留原终态字段并给 settled/flight/J/firstWaterId/deliveryMode，预分配 out；T219 精确只读 `copySourceReceiptInto` 按当前 route 同步选择 prefix+suffix，PENDING不冒成功、不二次enqueue。

`commitFlightQueuedSourcesInto(boundary,batchOut,receiptSlots)` 每次显式边界最多处理当前 Q≤64，逐请求 root：核 run/input/geometry/clock/raw time/版本/queue/dedup/event/suffix容量→`sourceBefore←canonical`、`work←canonical`（含五份水元数据）→SAND沿旧 birth 但容量含J；WATER且 `S≤y≤eta` 为 DIRECT 加 W；`y>eta` 为 FLIGHT，在触点出生一个正质量包 `x/y, v=(0,-vEmit)`，W不变；`y<S`按原 `POSITION_IN_BED` 拒。先受 requested/maxSourceVolume/当前 `C-S-P-W-J` 和真实资源约束，再以真实存储候选、total差、V2 audit 裁决。`birthSimulationTime=当前simulationTime`；墙钟timestamp只记采样窗口，绝不从timestamp回填飞行。

每个候选准备完整 receipt56/suffix16/event24/suffix8/沙 origin 区间、water ID/slot 和 owner state，**在 publish 前校验所有 ArrayBuffer/offset/length/非 alias、计数/token 及写区**。receipt56复用现有 A 预建 scratch，suffix16用本稿列出的128B scratch；event24/suffix8在预检后写入**有效 count 以外**的目标尾行并保持不可见，不另申请未入预算的逐请求行。进入同步无回调提交区后复制 work→canonical，最后一起推进 dedup/event 有效 count/成功账可见代。若承诺的 copy/store 仍可抛异常，必须从 sourceBefore 及预留旧尾行/旧有效 count 恢复；不能留下 canonical 已成功而 suffix 不可读。各请求成功独立发布，后续请求失败不撤此前成功。终态拒绝沿既有去重规则留痕，并在同一 revision 的 suffix 写全零有效行；容量耗尽/身份冲突保留首次 status/stage，不伪造成功 transactionId。`ACK`只表示入队；UI以只读终态 receipt 的 admitted>0、transactionId/kindSuccessSeq/currentSourceCommit 做一次反馈。

## 5. 可组合 B 阶段、水事件与统一提交

当前 `ClearWaterBStep.runInto` 是完整 V1 闭环，**V2不得先调用它再补水事件**。同一 Step 集成 owner 将其内部阶段抽成受 busy/root 控制的 `beginCandidateInto`、`runWaterEdgesInto`、`runSandParticleInto`、`runBedEdgesInto`、`finishSandFoldInto`、`applyBReplayRowInto`、`precommitInto` 等有界 helper；V1 `runInto` 按原顺序调用并保持原 event40/replay/commit bits。V2 `ClearWaterFlightStep` 复用这些核，在同一个 candidate `work` 中交错水包阶段，拥有 root commit、双 paired arena 和 B sealedIndex 切换权。`applyBReplayRowInto` 从现有 `replayInto` 拆单行，不得在 V2 另运行一遍 B 全轨迹/replay；所有 B event40 仍以原 local ordinal/used-mask 核。

正 `dt≤1/60` 且同一有效 GravityAcceptanceToken：entry V2 audit、资源与 token 预检→`stepBefore/work`全量复制，冻结入口 Bcap/固体 guard→原水头边每对一次，接收 free 含J→冻结 `etaMedium/wetMask`（不含J）→按 waterId 一轮弦段运动→按 sandId 一轮原 B 运动→床边一次→沙末尾 CONTACT 与 ID fold刷新 S/eta/P→水包索引**一次**固定 scratch `(column,y,id)` 排序并按当前 eta 接触/ARRIVE→waterId fold J/R8→work V2 audit/水沙差/预算→从 stepBefore 统一 tape replay 并独立 final fold/audit→authority/两池/预算/事件逐位对比→无回调同步提交 canonical+paired sealed arena+angle+simulationTime。只有这个 root 可切成功 physicsCommit/stepSerial/arrivalSerial；源成功在子步失败时保留，第二子步失败保留第一成功，`dt=0`无物理/源/事件/角成功写。失败 candidate 可脏，canonical/旧 sealed pair/已成功计数不变，保留首次 stage/ordinal/attempt，不悄悄重试 λ。

水点空气段按 T221 §5.2 的逐项 RN `exp/angle` 式，沿候选直线弦求最早 λ，按 `RN(h×λ)` 重算速度，事件 tie 为床/水界面→墙→列边；目标列高界面阻挡。点只撞容器/柱状床与水面，不做水点互撞/SPH；ew=8，每包最多7运动行+1末尾行，耗尽且仍有余时 `EVENT_LIMIT`。零时间拓扑以 plane+出射方向推进，不得用1e-9位置偏移；RN时间不进且拓扑不变 `TIME_NO_PROGRESS`。墙法向速度归零，跨列整包容量转移；接触先 WAIT，不在轨迹行直接改 W。末尾排序低y先到达，已到水面以下才申请整包ARRIVE，`W'=RN(W+m)`与J退役同一patch；可表示/容量不足等待并写原因。新到达 W **下一子步**才参与邻接水流，不补做同一子步的水头扫描。

水 arena 双份，每份 `event48 F64[ew×Kw×48]`、`header64 F64[64]`、`summary16 F64[Kw×16]`。event48槽0–35严格按 T221 §5.3：kind/localOrdinal/slot/id/origin/birthCommit/fromCol/toCol/m0/m1/x0/y0/vx0/vy0/x1/y1/vx1/vy1/W0/W1/Jfrom0/Jfrom1/Jto0/Jto1/state0/state1/wait0/wait1/remaining0/remaining1/hEvent/lambda/planeType/planeCoord/reason/flags；36–47 必须正零。kind MOVE1/CROSS2/CONTACT_WAIT3/ARRIVE4；ARRIVE m1=0且只此行允许W写/退役，MOVE/CROSS/WAIT m不变。`flags` U32低6位分别 oldLive/newLive/changedColumn/changedW/topologyWait/arrived，其余0，必须与各字段/类型一致；planeType `NONE0/BED1/WATER2/WALL3/TOP4/COLUMN5`，未知值拒绝。每种 kind 的允许非零槽位、强制零槽位和语义约束见 manifest；`Jto` 在非CROSS时为0，非ARRIVE的W0/W1为0，ARRIVE的remaining/hEvent/lambda为0，所有 reserved 为正零。不能把 reserved 当诊断空位。`Jfrom/to` 是顺序 RN 的局部摘要，不是全局 IDfold 或 exact guard；replay 必须逐位核旧摘要和实际patch。

水 arena header64 槽0–31依次为 `arenaSerial,eventCount,tapeCount,physicsCommit,sourceCommit,runGeneration,configRev,flightConfigRev,stepSerial,attemptSerial,acceptedAngleDeg,dt,clockWindowId,ownerId,waterLiveAtEntry,waterLiveAtExit,arrivalCount,arrivalSerial,simulationTimeBefore,simulationTimeAfter,ew,Kw,sandEventCount,sandLineageCount,sandSealedArenaSerial,tapeSerial,waterMotionVisits,contactSortRows,waterReplayRows,WtotalBefore,WtotalAfter,rootCommitCode`；32–63正零。`rootCommitCode=2`仅成功 V2 sealed。water summary16 每 slot 为 `waterId,slot,origin,birthCommit,state,wait,column,mass,x,y,vx,vy,firstEventOrdinal,eventCount,arrived,flags`；未使用slot全零，已退役slot保留本次 sealed 事件摘要但 canonical 池状态仍为退役，不能误画。numeric整数码全部安全整数，source-only 后 header所记 commit 为历史 sealed。V2 独立 `FlightReason` 0..17 及失败/合法WAIT/渲染/控制分类在manifest；例如 `NUMERIC_ARRIVAL_BLOCKED` 和 `SIDE_CAPACITY` 是合法WAIT行，整体 status仍OK，`EVENT_LIMIT/TAPE_FULL/REPLAY_OLD_BITS` 才是整步失败。不要把最近局部原因覆盖首次失败阶段。

两个 arena 共同有 `tape:U32[2×(Ls+Lw)]`，每行 `[stream,rowIndex]`，stream沙0/水1，tape顺序为全局实际写序，位置+1是 global ordinal；各 stream row.localOrdinal 仍从1连续。`Ls=eK+2(N−1)`，`Lw=ewKw`，主档e=8/Kw64/ew8。reserve时同时核对应 stream、tape、per-water event cap、B lineage/summary及下一 commit token；不足在 work 候选失败，不丢行借下帧。replay在**同一** replay bank 按 tape 行顺序检查 stream、行号、old raw bits、来源/plane/topology、实际 mass patch、局部容量和预算后应用；不重新积分、排序或选到达。结束同样一次按 ID fold/audit，比较 canonical 所需数组/头、free链/nextId、J、预算、全行及 source identity；产品 replay 与独立 tester oracle 不是同一个证明。双 paired arena 的 sealed index/serial/有效 count 只在 canonical copy 完成后共同可见；热导出必须把 sealed B 与 sealed water/tape 的同一 root token 一起返回，并另报当前 canonical source/physics commit，不把 source-only 后历史 sealed 当 current。

## 6. T219 C 补篇：optics16、角接受与 V4

### 6.1 CW-C-D1 高光定版

T219 V3 原 optics8 是未解决高光的临时布局。本稿起**新 C parser 统一要求 optics16**：`[sigma,tintR,tintG,tintB,surfaceGain,opticsRev,profileRev,reserved0,widthPx,lightR,lightG,lightB,F0,specPower,lightX,lightY]`。sigma≥0；tint/light/gain/F0∈[0,1]；widthPx∈(0,8]；specPower∈[1,64]；light向量有限非零，冷规范化一次并将原向量/归一化revision绑定 token。候选 width1.5px/gain.35/白光/F0=.02/p32/L=(-.4,1)只标 `PENDING_CALIBRATION`，不是设备合格值。V3 profileRev=1、V4 profileRev=2，8槽旧 packet 显式拒绝，不重释 V1/V2 fog 配置。

水体仍用 T219 原 `T=exp(-sigma×clamp(eta-max(y,S),0,W))` 线性预乘透射，W0/y>eta alpha0。水面在实际轴对齐 box 的**屏幕像素**算解析面积：湿列 `Ys=S`、`Ye=eta` 经同 geometry/镜像投影；带 `[max(Ys,Ye-widthPx),Ye]`，像素单位方形与当前列屏幕矩形求交面积 `A`。W0无带；浅水真实厚度截带；box宽必须≥N，保证每像素最多涉及相邻两列，否则 `UNSUPPORTED_GEOMETRY`，不静默全列扫描。列内 nearest，法线只用**直接相连湿邻**eta差：双邻 `(etaR-etaL)/2`、单邻差、无邻0；`n=normalize((-s,1))`、`v=(0,1)`、`L=normalize(lightXY)`、`F=F0+(1-F0)(1-clamp(dot(n,v),0,1))^5`、`G=clamp(dot(n,L),0,1)^specPower`、`a=gain×A×(F+(1-F)G)`，预乘RGB=`a×lightLinear`。两列贡献分别含面积，**求和一次输出一个 fragment**，不可把两次局部覆盖当两层反复over。解析面积是 AA，不改物理eta/W，不跨干列桥接；gain0与 W→0 边界按公式。CPU oracle按同 footprint/镜像/DPI/线性 RGB 计算，独立 tester 另给 GPU实际容差，不能凭静态模型宣称效果PASS。

### 6.2 CW-C-D2 重力接受定版

`P051GravityInputAdapter` 的 raw `targetTiltDeg`、sampleTime/seq、shake/stale仅作观测；生产样本必须 source=sensor，synthetic仅离线显式模式。新 `ClearWaterGravityPort` 每 active tick **冻结一次**同一有效样本，先依原250ms候选新鲜度、单调时间和finite校验；`tBound=min(30,max(-30,target))`，`saturated=(tBound!==target)`明确记录支持域。每正子步 h 从最近**成功**a0取 `deltaFiltered=(tBound-a0)*(1-exp(-h/tau))`，`delta=clamp(deltaFiltered,-omega*h,+omega*h)`，`aTrial=a0+delta`；核 finite与[-30,30]，非法失败不尾端再夹角。候选 tau=.12s、omega=90°/s、状态 `PENDING_CALIBRATION`。初始 a0=0/angleValid=0；成功正 dt 后angleValid=1并angleRev++，同一 aTrial 进 B/flight、水头和 token。第二子步只用第一成功角继续；第二失败保留第一角、first commit、sealed scene；失败子步不更新acceptedAngle。accepted 状态跟 root 物理成功同提交，不能拿已更新 adapter target 直接转水面或提前更新C controller。source-only publication继承最后接受角和所属 physicsCommit，未成功则 angleValid=0。

样本缺失/非法/stale/倒退：先 stop 当前输入代、暂停本 tick 物理、保留旧 angle/W/J/沙，页面报 `GRAVITY_STALE`；恢复先首tick0、需新DOWN，不补飞行/源 backlog。shakeInput/magnitude原样诊断入 GravityAcceptanceToken，**不改 g、drag、床流或固体预算**；纯平移摇晃扬沙/浪未实现，不能在产品回执宣称通过。token显式含 owner/run/input/config/adapterRev/tick/sampleTime/sampleSeq/raw/bounded/saturated/shake/h/attempt/substepIndex/aBefore/aTrial/angleRevCandidate/committedPhysicsCommit或failure。V1 C 和 V2 route均可用此端口，V2 root还绑定 water flight revision；不用旧 WaterSandField planner。

### 6.3 V4 packet、层与故障

V3 依 T219 仍用 profile8、沙 proof8、当前 canonical capture，只把 optics 换为新严格16。V4 独立 `drawClearWaterFlightSceneV4(frame,profile64,sandProof64,waterProof64,sandXY32,sandMeta32,waterXY32,waterMeta32,optics16)`；`profile64[10c+0..9]=[S,W,eta,P,bh,mh,R,J,R8,wet]`，`waterProof64[10i+0..9]=[id,m,x64,y64,vx64,vy64,origin,birthPhysicsCommit,state,wait]` 按 waterId升序。water meta两槽 `[state|(wait<<8),id]`；xy按 T219 既有列保护投影，native由**原 x64**按waterId核 J、沙 proof核P、R8/总账/来源/状态和token；GL V4 profile三行 RGBA32F：row0 `[S,W,eta,bh]`、row1 `[mh,P,R,wet]`、row2 `[J,R8,0,0]`。前两行保持 V3 含义。water包`J>0,W=0`时只画真实在途 glyph，不画水面/水体。

`FrameTokenV4`在T219 current owner/run/physics/source/publication/session/context/surface/geometry/config/optics/profile/projection基础加 flightPolicy/config/state、arrivalSerial、emissionPolicy、waterProof原bits hash与 GravityAcceptanceToken 所属成功角/commit。每层同 packet/token，native strict typed array/offset/alias/finite/版本校验后复制至本地双 packet，不能用 V3/V1 parser 或 LastGood。层序 `BACKGROUND→SAND_BED→SAND_PARTICLES→WATER_BODY→WATER_SURFACE→FLIGHT_WATER→FOREGROUND`；背景/前景同样只用新 XComponent+shader，无 Canvas 上屏/失败回退。水包实例圆片按 T221 §7.1：`rPx=clamp(radiusScalePx×sqrt(m/mRef),rMinPx,rMaxPx)`，`coverage=clamp(rPx+.5-distance(pixel,center),0,1)`，`h=clamp(1-distance(pixel,center+(-.3rPx,.3rPx))/(.4rPx),0,1)^4`，预乘色 `alpha×((1-h)tint+h lightRGB)`、`alpha=opacity×coverage`；实际屏幕坐标/box裁剪与同token geometry一致。`FlightVisualConfig` 版本化并冷绑 mRef∈[2^-20,256]、0<rMin≤rMax≤8px、scale∈(0,8]、opacity∈[0,1]，候选 .125/2/.5/4/.6 标 PENDING_CALIBRATION；这是非体积glyph，不改库存或碰撞。被 ARRIVE 退役的包同 token 不可再画。完整 candidate 真实present/swap成功后才共同提交 renderedToken/LastGood；失败 FAULT 或带旧 token 的**完整兼容 STALE**，不可新沙+旧水、水包+已到达水混拼。V4 LastGood不能跨V3、run、context、geometry、尺寸/镜像/config；context loss从仍存 owner重新capture/reupload，绝不假restore。

## 7. 资源与完整 frame 成本

主档N128/C256/K192或512、e8、Kw64、ew8、D4096。新增模板在T209 A+B真实 payload与T219 C模板**之外**：五bank水池/J/header `5×(74Kw+8N+128)`；双water event48/header64/summary16 `2×(48×8×ewKw+64×8+16×8×Kw)`；双global tape `2×2×4×(eK+2(N−1)+ewKw)`；固定排序/marks `9Kw+256`；dedup/event suffix与临时行 `16×8D+8×8D+16×8`；四份ETS/native water proof+xy/meta `384Kw`；V4 profile与双GPU第三行 `64N+32N`；双水VBO/额外header/optics `32Kw+1024+256`。合计 K192 **1,304,480B**、K512 **1,345,440B**。其中 optics8→16 四份增量256B在**单做C基础**时就分配：C-only列明载荷变成207,368/331,528B；V4总式只加它一次。T209 owner 4,230,656/6,312,256B，加T219 C 207,112/331,272B与全T222增量，设计已列基础合计 **5,742,248/7,988,968B**。模板的假定与实际数组注册/冷失败资源回收见manifest；这不是RSS、driver占用或性能通过。另计双全帧 `8×widthPx×heightPx` 若不能复用，旧/新context及尺寸切换重叠、program/VAO、对象/view/alignment、NAPI临时、诊断/截图/IO队列、cold配置与可选导出；V2无可恢复 snapshot。不得为此自动提高预算或抢GPU路径资源。

完整 tick计 `G`实际源边界、Q≤64已排请求（每条候选copy/audit与失败也计）、F≤2正dt实际尝试、Ls≤eK+254、Lw≤ewKw。enqueue含实际 D/Q 扫描；sourceDrain逐项copy/guard/receipt/suffix；每step入口70limb×(N+K+Kw)、水/床边、70limb×事件数、**一次**Kw log Kw排序、work与replay独立fold/audit/比较及全bank copy；backend当前canonical capture/hash/native F64 proof/F32/上传、七层实际像素、submit/fence/present/swap和诊断IO全部进分母。失败attempt/提前退出记真实行数，cold attach/reset/context重传单列；GPU timer没有设备证据标 `NOT_MEASURED`。无逐水事件全池audit、无逐帧 `snapshotB`、无逐包热new/slice/Map。设备与资源真实ledger/峰值须由后续开发/测试填，PENDING_CALIBRATION不充达标证据。

## 8. 分阶段 developer 所有权与独立前置

| 阶段 | 唯一代码owner与交付 | 真实前置/边界 |
| --- | --- | --- |
| C基础（目标7） | T219既定 Engine只读capture、Source终态reader、V3 packet/profile/透射、XComponent页/触摸源；本稿补 optics16/面积高光与 GravityPort，统一 native/CMake/UI 集成owner | T219与T221/T222工程；可用显式accepted角先做离线夹具。真实C开发编译/非真机自测后另交独立/设备，不以B12/12当可视PASS |
| Flight V2 物理 | State/WaterPool/Source suffix/ExactMass Gexact、Step可组合阶段、water48/tape/统一replay/五bank root、V1隔离；`FlightTypes/Events/Particles/Guard`新模块 | T222工程与T209 B真实基线；单独新run、无页面，先真实ArkTS编译/离线自测。公共Engine/State/Source/Step由一个集成owner合并，不能并行覆盖同hunk |
| V4 packet与页面 | C基础既有新管线增profile10/waterProof/glyph/V4 parser+shader、单owner空容器页与source切换/完整FrameToken | C基础及V2接口实际交付，光学/角策略本稿；native bridge/CMake/scene compositor/UI由**同一个**集成owner串行处理，GPU compute优先，不改生产默认 |
| 独立tester/effector | SW01–14独立数值/事务/packet/oracle与 E-SW1–4连续同run真机效果 | 对相应开发阶段做NOT_IMPLEMENTED基线；设备卡须真机标签、串行且让位GPU。目标5 T216→T217→T218必要修复与目标7 C实际交付是完整DEMO验证真前置，不用manager验收门 |

此文可供 manager 创建有明确真实前置的实施卡；architect不改其它卡/清单队列、不自行派工。开发负责代码、编译和非真机自测；独立tester/effector负责设备与产品结论，失败先architect分流。未实现的预测试RED登记 `NOT_IMPLEMENTED`；源/设计模型/资源模板不记产品PASS。参数 sigma/tint/gain/落水速度/tau/omega/可见半径与效果门槛均 `PENDING_CALIBRATION`。
