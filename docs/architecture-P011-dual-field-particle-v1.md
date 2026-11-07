# P-011 路线 C 双场架构工程设计 v1：真实活动粒子层 + 128 列高度场

> **M1生产热接口补充（2026-10-24，6adc91c3…d612）**：新增§17及独立热接口附件，保持严格零分配门槛；生产prepare/collect/apply使用定容Into接口与数值代际token，§13对象/字符串API保留冷诊断兼容。本文既有验收历史以manager账本为准；新热接口待manager验收，不代表D1 R3已通过。

> **当前有效修订：rev3（2026-10-24，6adc91c3…d612），已由manager 19:25验收。** rev2（6adc85fc…81cb）已于18:40验收，补§13.2的 **R9-REFUND_ADAPTER** 并更新endpoint字段对齐登记，接口质量/预算/guard语义不变；rev3 在此之上新增 **§17 与独立热接口附件**（生产热路径定容缓冲 + 数值代际patch句柄），并同步更新§13.6的HOT阶段追加验证要求。本次索引见§16。旧§14“R9”是rev1历史状态索引，与退款条目不是同一编号。**D1 R3 是否通过仍等developer实现与全链证据交manager验收**；本稿不代关卡、不解锁D2。

> **rev1历史状态记录（交付时点，后由16:12验收覆盖）**：rev1（2026-10-24，卡6adc5ed0…c94e）交付时待manager验收，D1 B段当时等待；现rev1验收事实以priority.md 16:12为准。§13为提交接口、双跑分母、路由归属与交付边界契约，新增退款接法以本次R9为准。D0 fdc6a5a已修复验收；原始D-C1发现/早期状态保留历史，不重新阻塞D0。rev1索引见§14，纯设计零代码、冻结预注册不变。

日期：2026-10-24。责任：architect。状态：**ENGINEERING_DESIGN / MANAGER_ACCEPTANCE_PENDING / NOT_AUTHORIZED_FOR_DEVELOPER**。依据：[三路线设计 §4/§6](design-P011-falling-sand-vertical-routes-v1.md)（用户 2026-10-24 选型回执：路线 C = 「手指落沙」与「摇晃扬沙」共同目标架构，`USER_SELECTED_TARGET_ROUTE_C / DESIGN_ONLY / NOT_IMPLEMENTED / NOT_VALIDATED`）、[手势沙源注入算法设计](algorithm-P011-gesture-sand-source-v1.md)（已验收）、[B 路线工程](architecture-P011-falling-sand-B-route-v1.md)（已实现 bbe8e33）、[手势注入物理层任务书](architecture-P011-gesture-sand-inject-phys-v1.md)（已实现 693a724）、[advection 工程](architecture-P011-advection-v1.md)、[S4 工程](architecture-P011-S4-deep-creep-v1.md)、[S3 工程](architecture-P011-S3-moisture-v1.md)、[priority.md 2026-10-24 排期](priority.md)、[tester 预注册稿](preregistration-P011-dual-field-particle-v1.md)（DUALFIELD-* 16 例 METHOD_FROZEN，遥测字段名以本稿为准复审）。tester 用例卡 6adc4394…2edc 已并行预注册。纯设计零代码；本稿不授权 developer 开工（manager 验收后才建实现卡，隔离原型先行）。

代码行号均对照 `entry/src/main/ets/physics/WaterSandField.ets`（6222 行，工作区 HEAD 附近）与同目录其他文件，核对时间 2026-10-24。

---

## 0. 前置：代码现状核对与一个阻塞级发现

### 0.1 现状核对表（C 的全部依赖面）

| 依赖项 | 现状（行号） | 对 C 的意义 |
|---|---|---|
| canonical ledger | `waterTop/bh/bl/mh/ml/suspH/suspL` 七数组 Float64Array(cols)（:741-748）；`S=sandTopScratch=bh+bl+mh+ml`（:6104-6108，`updateSandTop`） | 粒子池是第八份 canonical 库存，与七数组同级 |
| 外源注入事务 | `injectGestureSand` **已实现**（:1489-1601，commit 693a724）：λ 统一准入（≤8 次折半，λmin 1e-4）+ η 单调二分抬升（:1639-1690）+ 审计链（`gestureInjectionAudit` :1694+）+ IH/IL/sourceEpoch 回写；队列 `stageGestureSand`（:1435，≤8 溢丢最旧）/`consumePendingGestureSand`（:1449，≤1 笔/子步、Δt≤1/30s 钳制）/`clearGestureSandQueue`（:1430） | C 复用整套准入数学，只把汇从 `suspH/suspL` 改为粒子池（§2.1） |
| receipt 契约 | `P011InjectionReceipt` 九字段定义在 render 侧 `P012SandTracer.ets:19-39`，物理层 import 复用 | C 保留契约（B 过渡期仍消费）；C 启用时页面停止 feed tracer（§6） |
| B tracer | `P012SandTracer.ets`（388 行）已实现入库（bbe8e33）：SoA 256 点 + 64 receipt 环 + 三段遮蔽 + ≤2 draw；renderer 接入 `P012WaterSandRenderer.ets:469-471/808`，开关 `frameContext.sandTracerEnabled`（`P012RenderTypes.ets:121`） | 过渡外观；C 启用后与其互斥绘制（§6） |
| 摇晃扬沙 | `liftByShake`（:5806-5884）：shakeEnergy 动力学 + band + desired + 容量式（:5831-5833）+ `clipSubstepMotion(c,-1,…)` 本地/总额扣减（:5844）+ guard `ΔS=−q`（:5856）+ `splitTopV2`（:5886-5904，混合→轻→重暴露序）→ 写 `suspH/suspL`（:5880-5881） | C 只改最后一步汇：写粒子池（§2.2）；其余逐字保留 |
| 沉降 | `settleSandV2`（:5311-5356）：`desired=susp×min(1,settleSpeed·dt/depth)` + `clipSubstepMotion(c,-1,…)`（:5329）+ guard `ΔS=+q`（:5336）→ `mh/ml` | 粒子的落床事务与 settle 同构（§2.3） |
| §19.5 guard | `p011GuardAdmit` 闭式区间相交（`P011MaterialGuard.ets:51-115`）；场内封装 `materialGuardAdmitIncrement(amount, deltaColumns)`（:3880）；硬上限 `MATERIAL_GUARD_HARD_MAX=tan30°`（:3861）；退款 `materialGuardRefund`（:4092，§28 修复后只退实际扣额） | 粒子落床/扬起逐笔过同一 guard（§2.4） |
| 四层预算 | local 每列 `vSandTopMax=28`（:710）、bottom 专账 `vBottomCreepMax=10`（:715）、total `totalMotionBudget=300`（:711）、channel cap 仅 advection（`advectionChannelCapRemaining` :934）；扣减入口 `clipSubstepMotion`（:3548，**同时扣 local+total**）/`clipTotalMotion`（:3571）/`clipBottomCreepMotion`（:3583）；预算=速率×substepDt（:3464-3491） | 粒子三类事务的计费定义（§3.1） |
| correction 共享 | `LiveCorrectionLedger.debit` 同时扣 local×2+total+correction 四本账（P011RelaxCorrection.ets:129-138）；step 循环折叠：`ordinaryBudget=totalLimit−rLimit`，普通通道按 scale 缩放（:1852-1857） | 粒子请求进同一分母（§3.4） |
| 账本/快照 | 观测账本 schema **v6**（`P011NaturalObservation.ets:19`，advection 占用）；场内快照 `WaterSandSnapshot.schemaVersion=3`（:663）；`getTotals` heavy=Σ(bh+mh+suspH)（:1942-1943）；`getDrift` 期望=initial+sourceInjected（:3366-3367）；逐帧 `copyStateTo` 复用对象（页面 :2076-2078） | 粒子进 totals/drift/audit/快照/观测账本（§5） |
| 生命周期 | `reset/initialize`（:1222-1303，清 sourceInjected*/sourceEpoch/队列）；`invert180`（:1730-1753，沉积+悬沙全部转入镜像列 susp、waterTop 重构造）；dormant 快速路径（:1777-1785，shakeEnergy/gestureQueue 为空时跳子步）；dormant 判定（:6007-6015） | 粒子状态扩展（§5）；**在途粒子必须阻断 dormant**（§5.3/§5.5） |
| 页面接线 | Pan→`routeGestureSand`（SandCanvas.ets:2276-2338）→`stageGestureSandAndFeed`（:2344-2360，直调 `injectGestureSand` 拿 receipt feed tracer）；`clearFallingSand`（:2362-2367） | C 的入口与互斥（§6/§7） |

### 0.2 阻塞级发现 D-C1：`audit(true)` 漂移分母用错基线（C 的前置，也是掉沙链既有前置）

**事实**（Node 探针 `test/probe_p011_audit_drift_gate.cjs`，2026-10-24 实测，可复跑）：

- `audit(checkDrift=true)`（:6045-6052）的守恒漂移分母用**原始 `initialHeavy/Light/Water`**，不是期望值。而 `getDrift()`（:3366-3367）已按验收设计改用 `expectedHeavy=initialHeavy+sourceInjectedHeavy`——两处口径分叉。
- 后果：持续外源准入使 `totals.heavy−initialHeavy=Σadmitted` 超过 `DRIFT_LIMIT=0.001×max(initialHeavy,EPS)` 时，子步尾 `audit(true)`（:3614/:3650）返回 `UNKNOWN`（errorCode 5）→ step 循环置 `this.frozen=true`（:1889-1896）→ **此后每个 step 全部 `ok=false/stepped=false`，物理永久冻结，只能 reset**。
- 探针数值：128 列场景（initialHeavy=initialLight=12800）累计准入 48 units（ratio 0.00375>0.001）后 `step(1/60)` 即 `errorCode:5`，第二步同样失败；同期 `getDrift()` 三项全 0（真守恒）。
- 既有单测未暴露：`test/run_p011_gesture_inject_units.cjs` 10/10 PASS——守恒用例累计注入约 110 units（ratio≈0.0086，已越界）但**只调 `injectGestureSand` 从不调 `step()`**（断言走 `getDrift()`）；唯一调 `step()` 的队列 FIFO 用例注入量约 1.2 units（远低于 12.8 units 阈值），故 `audit(true)` 从未在越界态被执行。

**定性**：这是 693a724 的实现与已验收设计（[注入物理层任务书 §3](architecture-P011-gesture-sand-inject-phys-v1.md)「audit(true) 的守恒项分母换 expected 公式」）之间的**遗留缺口**，不是新设计引入的问题；但它是 C 的**硬前置**——C 的手势落沙走同一注入事务，摇晃扬沙在 C 下同样改变 `totals` 与 `initial` 的关系（扬起不改变 expected，但注入会）。

**修复方向（登记，不属本卡实现范围）**：`audit(true)` 的三个漂移分母改用与 `getDrift()` 同一期望公式（heavy/light：`initial+sourceInjected*`；water：`initialWater+waterTransactionNet`），`DRIFT_LIMIT` 与全部判定逻辑不动。这是**基线公式归正**（实现对齐已验收设计），不是放宽阈值。修复落地前：①C 的任何真机/Node 长时注入试验都会误冻结，试验无意义；②掉沙链（693a724 的验收）同样被卡。建议 manager 将 D-C1 修复单列前置卡（developer 级小修+单测：持续注入越 0.1% 后 step 不冻结、getDrift 与 audit 口径一致），先行于 C 实现卡。

### 0.3 范围与红线复述

- 纯设计零代码：本稿只定义接口、事务边界、检查点；不授权 developer 开工；不建实现卡（manager 验收后）。
- 不动冻结阈值：300/s 总额、128 列、守恒/库存/容量/有限值、26/20 双角、30°±2°、t90/R²、tan30 硬上限——全部原样；粒子不提高任何预算、不开任何快车道。
- C 未实现前不改 B tracer 现有行为（renderer/页面/tracer 三处零改动）。
- 不把可见飞沙当达标证据：守恒、落床时刻、扬起对账只认账本与 checker；主观效果归 effector，且明确不能代替守恒证据。
- 数值候选（m_i、终端速度律、阻尼、起沙初速、合并半径、Nmax 等）全部标「待定标非冻结」（§9）。
- 工作区开发，不合入 108-run 正式批（该批已封存 82/108，物理二进制/脚本/参考帧身份不动）。

---

## 1. 总体架构裁定

### 1.1 双场模型

- **下层（既有，不变）**：128 列 1D 高度场。每列 `bh/bl/mh/ml`（沉积，重在下轻在上）+ `suspH/suspL`（列均值悬沙）+ `waterTop`。这是「床」——连续、保守、受 guard 与四层预算约束的沉积域。
- **上层（本稿新增）**：**稀疏活动粒子池**——有真实质量、连续位置、连续速度的离散对象。这是「飞沙/在途」——携带 `id, phase, mass, x, y, vx, vy, sourceKind`，是 **canonical 物理库存**（不是 B 的 tracer：tracer 无体积零库存）。
- **共同架构**：手势落沙（外源准入→粒子）与摇晃扬沙（床面提取→粒子）共用同一粒子池、同一积分器、同一事务集；仅 `sourceKind=GESTURE_EXTERNAL|SHAKE_LIFT` 分账（§5.5）。
- **材料坐标**：`x` 为连续列坐标（0..cols，映射到 128 列，**不升分辨率**）；`y` 为沿重力方向、自重力底起算的材料高度（与 `S/waterTop` 同一坐标；容器沿重力 extent = `capacity`）。传感器/TEST 只经既有 `effectiveTilt` 入口影响重力方向，不建第二姿态源。

### 1.2 粒子池数据结构（SoA，定容 Nmax）

```ts
// P011ParticleField.ets（新文件，physics/，纯物理无渲染依赖）
export const P011_PARTICLE_NMAX_DEFAULT = 512;   // 候选非冻结（§9）
export const P011_PARTICLE_HEAVY = 0;
export const P011_PARTICLE_LIGHT = 1;
export const P011_SOURCE_GESTURE = 0;            // GESTURE_EXTERNAL 外源准入
export const P011_SOURCE_SHAKE = 1;              // SHAKE_LIFT 床面扬起

// 定容 SoA（Float64Array 保守恒精度；Uint8 存枚举）
private readonly pMass: Float64Array;   // 每粒质量（材料体积单位）
private readonly pX: Float64Array;      // 连续列坐标 [0, cols]
private readonly pY: Float64Array;      // 材料高度（重力底起算）
private readonly pVx: Float64Array;     // 列坐标/秒
private readonly pVy: Float64Array;     // 材料高度/秒
private readonly pPhase: Uint8Array;    // 0=H 1=L
private readonly pSource: Uint8Array;   // sourceKind
private readonly pSourceId: Uint32Array; // rev1：来源事件id，merge/回滚/快照均保留
private readonly pId: Uint32Array;      // 单调 id（可追溯）
private readonly pState: Uint8Array;    // 0=空槽 1=飞行 2=静置待准入
private readonly freeSlots: Int32Array; // 空槽栈（O(1) 分配，禁 O(N) 扫槽）
private liveCount = 0;
private nextId = 1;
// 每列在途质量（派生缓存，事务时增量维护，审计/容量 O(1) 读）
private readonly colParticleMassH: Float64Array;  // 长度 cols
private readonly colParticleMassL: Float64Array;
```

**为什么不存 age/seed**：积分确定性由「每子步固定种子 + 事务按 id 升序」保证（§4.2），不需要逐粒年龄；视觉抖动归渲染层（外观种子独立，不消耗物理随机源）。

**id 与可追溯**：`pId` 单调递增，跨 reset 不清零（run 内唯一）；事务流水逐笔记录 `particleId`，满足「保留逐笔可追溯 ID 与先后次序」。

### 1.3 与高度场的耦合面：六类事务

| # | 事务 | 方向 | ΔS | guard | 预算 | 汇/源 |
|---|---|---|---|---|---|---|
| T1 | 外源准入 spawn | 注入→粒子 | 0 | 不需要（ΔS=0） | 不扣（外源非场内搬运，注入设计 §3 既定） | `injectGestureSand` 汇改粒子池 |
| T2 | 摇晃扬起 lift | 床→粒子 | −q | **要**（ΔS_c=−q 逐笔） | local[c]+total | `liftByShake` 汇改粒子池 |
| T3 | 落床 deposit | 粒子→床 | +granted | **要**（ΔS_c=+granted 逐笔） | local[c]+total | 仅减粒子 granted、`mh/ml+=granted`；余量留原 id/state=2，质量归零才释放槽（§13.1） |
| T4 | 跨列 crossing | 粒子 c→c' | 0（S 不变） | 不需要（S-invariant 登记，advection 先例） | local[c]+local[c']+total | 粒子 x 跨列边界 |
| T5 | 满额降级 degrade | 粒子→susp | 0 | 不需要 | 不扣（同一笔已准入质量的汇变化） | `suspH/suspL+=q`，标 `PARTICLE_DEGRADED_TO_COLUMN` |
| T6 | 合并 merge | 粒子+粒子 | 0 | 不需要 | 不扣 | 质量/一阶矩守恒的有限包 |

**不变式（全部进 checker，§5.6）**：
- I-1 分相守恒：`VH_actual=Σ(bh+mh+suspH)+Σ_{p∈H}pMass`，`VL_actual` 同理；`VH_expected=initialHeavy+sourceInjectedHeavy`（扬起不增 expected——它是相内搬运）。
- I-2 互斥：同一质量任一时刻只在 {沉积, 悬沙, 粒子} 之一（事务原子性保证；checker 断言）。
- I-3 容量：逐列 `S+suspH+suspL+colParticleMass[c] ≤ waterTop[c] ≤ capacity`（audit 扩展，§5.6）。
- I-4 有限值：粒子六数组逐元素 finite（audit 扩展）。
- I-5 tan30：T2/T3 逐笔 guard；T1/T4/T5/T6 为 S-invariant 登记类（不新增拦截义务，也不豁免——它们本就不改 S）。
- I-6 预算：每笔事务 `requested=granted+Σclipped`（分段幂等，advection 先例）；四层 used+remaining 闭合。

---

## 2. 工作 1：床面↔粒子原子质量交换与 guard 可提交条件

### 2.1 T1 外源准入：injectGestureSand 汇切换（ΔS=0）

沿用 693a724 的六步事务（预检→saveBackup→湿列重查→λ 准入→η 求解+审计→提交回写），**只改三处**：

1. **试写与容量界**：`T[c]=S[c]+suspH[c]+suspL[c]+colParticleMass[c]`（现有 `T` 加在途粒子项）；全局必要界 `Σ(H−max(waterTop[c],T[c]))`、η 目标 `Σmax(0,min(H,max(T,waterTop+η))−T)=Vw`——公式逐字照搬，`T` 定义扩展。粒子和悬沙占据同一列空间，准入数学不变。
2. **提交汇**：准入量 `aH[c]/aL[c]` 不写 `suspH/suspL`，改为 spawn 粒子（§2.1.1）；`sourceInjectedHeavy/Light += Σa`、`sourceEpoch += 1`、`dormant=false`、t90 epoch 重启——全部照旧。
3. **审计链**：`gestureInjectionAudit` 增加在途粒子项（有限值/容量按 I-3/I-4）。

**2.1.1 spawn 细节**：对每个 `aH[c]>0` 的列，按质量包 `m_i`（候选，§9）拆分：`k=floor(aH[c]/m_i)` 个整包 + 余数 `r=aH[c]−k·m_i`；`r>0` 时**再生成一个质量为 r 的小粒子**（不四舍五入丢库存；尾数粒随后可被 T6 合并吸收）。出生位置：`x=c+抖动`（种子驱动，候选 ±0.5 列内）、`y=clamp(触点材料高度, S[c]+contactEps, waterTop[c]−eps)`（触点落在沙区时不得生成穿沙长轨迹——钳到沙面之上）；初速 = 候选终端速度沿重力方向（§9，**不直接采用 settleSpeed**，见 §9.2）。每事件固定种子（receipt.seed 同源）保证 tracer 停产后的可复现性。

**为什么保留 η 抬升**：在途粒子与悬沙一样排水——粒子占据水柱空间，注入后自由水总量须严格不变（`ΣwaterTop` 的变化只是位移不是漂移）。η 构造照旧。

### 2.2 T2 摇晃扬起：liftByShake 汇切换（ΔS=−q）

`liftByShake`（:5806-5884）除最后两步外逐字保留：shakeEnergy  ramp/衰减、`band=min(S,max(0.5,tan(angleDeg)))`、`desired=requestEnergy·band·dt·(heavyLiftRate·surfaceHeavyShare+lightLiftRate·(1−share))`、容量式 `capacity=waterTop−S+desired−suspH−suspL`（**该式天然覆盖粒子汇**：扬出 q 后 S 减 q，粒子占的空间正是释放的空间；只需把 `suspH+suspL` 换成 `suspH+suspL+colParticleMass`——即要求在床面有在途粒子时少扬，保证 I-3）、collect 记 `riseRequestAmounts`+总额请求、apply 侧 `clipSubstepMotion(c,-1,…)`（local+total）、guard `ΔS=−q`、`splitTopV2(c,amount)` 得 `{heavy,light}`。

**唯一改动**：`:5880-5881` 的 `suspH[c]+=split.heavy; suspL[c]+=split.light` 改为 spawn 两相粒子（质量=split.heavy/split.light，`sourceKind=SHAKE_LIFT`），出生 `x=c+抖动`、`y=S[c]+contactEps`（床面），初速=候选起沙速度（向上分量+横向散布，§9.3）。**IH/IL 不变**（扬起是相内搬运，expected 不增）；sourceKind 分账供 checker/遥测区分库存来源。

**路由门**：仅当 WaterSandField 的 `particleRouteActive()`（§13.4）为真时走粒子汇；否则走原 susp 汇——**非 C 路线位精确不变**（route 隔离红线）。

### 2.3 T3 落床：粒子→mh/ml 的原子事务（ΔS=+q）

每子步积分后，对每个飞行粒子以 `c=clamp(floor(x),0,cols−1)` 查询只读床面：

```
if (pY ≤ S[c]) → deposit 事务：
  0. 钳位：若上一子步已判定待准入（state=2），pY = max(pY, S[c]+contactEps)（静置于床面，不沉入沙体）
  1. q = pMass（整笔；相 = pPhase）
  2. allowed = clipSubstepMotion(c, -1, q)          // local[c]+total，与 settle 同型
  3. guard：deltaMap={c:+1} → materialGuardAdmitIncrement(allowed, deltaMap)
     rejected>0 → materialGuardRefund(rejected,[c],false)；allowed=accepted
  4. 原子提交：mh[c]+=allowed（或 ml[c]）；pMass-=allowed；同相 colParticleMass[c]-=allowed。
     remainder=requested-allowed；remainder>0 保留原 id/sourceKind 与槽，state=2，贴当前床面；
     只有质量归零才释放槽并 liveCount−1。allowed==0 不改库存，仍保留 state=2。
     遥测 LAND {particleId, c, phase, requested, granted, remainder,
       remainderDisposition='RETAIN_PARTICLE'（余量>0）或'NONE', guardRejected, budgetClipped}。
  5. state=2 每子步进入重试，不能先钳到 S+contactEps 后又只以 y<=S 判定而漏掉重试；
     EPS 不得作为删除正质量尾数的条件。提交、退款、回滚完整定义见 §13.1/§13.2。
```

**guard 可提交条件（与 §19.5 逐字同构）**：对受影响边集 `{c−1,c}`（`p011GuardConstraintsForDelta` 派生），闭式求解 `−L_e ≤ d_e+k_e·q ≤ L_e`：合法边 `L_e=tan30`（保持合法），已违约边 `L_e=|d_e|`（non-worsening）；区间交 `[0,qGranted]` 得上界。**这正是 settle（:5336）的同一函数**——粒子落床不新造判据。

**与 route A「接触队列」的本质区别**：A 需要在途队列是因为其包团无位置；C 的粒子**本身就是库存**，受阻时以 state=2 静置于床面即是「接触队列」——零新抽象。静置粒子的视觉：贴在沙面上（渲染按 S(c) 连续采样），随床面松弛（relax/settle 改变 S）后自然达成 deposit。

### 2.4 平沙大角度时单列 lift 被邻边 non-worsening 拦截的处理

**情形**：平沙（近水平床）大倾角下，单列 lift 的 `ΔS_c=−q` 使邻边残差越过 `|tan30|` 或加剧已违约边，guard 返回 `accepted=0`。

**裁定（与现行 lift 语义一致，不发明新路径）**：
1. 该列本子步**不扬沙**：splitTopV2 不执行，质量留在 `bh/bl/mh/ml`，`suspH/suspL` 与粒子池零变化；`riseRequestAmounts` 的请求照记，apply 侧 guard 拒绝量进 `guardClipped` 遥测（现 `recordLift` 通道已覆盖）。
2. **不允许改道 susp**：床面质量进 susp 必然伴随 S 下降，同一 guard 义务——改道只是换名字的越界。也不允许「先扬后合并」式旁路（合并只作用于已在池粒子）。
3. 下个子步 shakeEnergy 仍在（衰减中），该列可重试；床面经 relax/correction 松弛后边残差改善，lift 自然可行。持续被拦截时沙子**留在床上不动**——这是安全优先的正确行为，不是缺陷。
4. 遥测分报 `liftRequested/liftGranted/liftGuardRejected`（现制），checker 对 `guardRejected>0` 的列不判失败（non-worsening 语义内），但登记供归因。

---

## 3. 工作 2：跨列粒子输运的计费定义与 300/s 可达性

### 3.1 T4 计费定义（与 advection 逐字同构）

粒子在一个子步内因 `vx` 跨过列边界（`floor(x)` 改变）即构成一笔跨列输运：

| 账本 | 扣法 | 依据 |
|---|---|---|
| local | donor 列与 receiver 列**各扣 m**（pair 型双端，`clipSubstepMotion(c,c',m)` 直接复用） | pair 先例（relax/creep :124-125；advection §1.3） |
| total | 每笔扣 **m**（不是 2m——单相单向的真实位移量是 m；H/L 各笔独立） | advection §1.3 同款 |
| bottom 专账 | 不扣（粒子不是底部通道） | S4 §1.2 同款 |
| channel cap | 登记实际使用量（`particle` 字段，**首版记账不限流**，先观测后定标） | S4 §1.2 同款（deepCreep 先例） |

**容量预演（receiver 侧）**：跨界前检查 `colParticleMass[c']+m ≤ waterTop[c']−S[c']−suspH[c']−suspL[c']+EPS`；不足则该子步**水平位移在边界处等待**（x 钳在边界内），不写账、不瞬移；下子步容量恢复（悬沙沉降/水面变化）后自然过去。**禁止给上层开未经批准的免费传输通道**：位移本身不扣「画面移动」豁免，一律进预算。

**守卫性质**：T4 不改 S（粒子质量从 c 的粒子池进 c' 的粒子池，两列 S 均不变）→ S-invariant 登记类，不调 `materialGuardAdmit`；内部有效性自查（双端粒子质量非负、receiver 容量、有限性），失败该笔跳过+遥测 reason，不部分写。

### 3.2 子步步长限制（CFL 式）

- **每粒子每子步最多跨 1 条列边界**：积分后若 `|x−x0|` 越过边界，超出量钳在边界处（等下子步）。理由：①与列离散床面的碰撞检测一致（不会「跨过」一列深处的床面形状）；②预算笔数有界（每粒子每子步 ≤1 笔跨界+≤1 笔落床）；③确定性（无子迭代）。
- **垂直方向**：床面碰撞由 T3 事务处理（不钳 y，y≤S[c] 即触发 deposit 或静置）；水面之上为 airborne（无上界钳制——容器顶由 `y≤capacity` 与有限值审计兜底）。
- **vx 与 effectiveTilt 的关系**：候选 `vx=v_term·sin(effectiveTilt)`（符号随倾角，§9.2）。θ_eff 只读 planner 接受角（与 relax/advection 同源），不用命令角。

### 3.3 300/s 上限内可达性（定量，候选参数下）

以 60fps 帧（`step` 的 `clipped=min(dt,1/30)`、`substeps=clipped<=1/60?1:2`，:1789-1791：60fps 时 1 子步、30fps 掉帧时 2 子步，**子步 dt 均为 1/60**）计，每子步总额度 = `300×1/60=5 units`，每帧 5 units（60fps）或 10 units（30fps）：

| 量 | 数值（候选参数） | 占每子步总额（300×dt=5 units） |
|---|---|---|
| 手势源产生率 | `gestureSandRate=8 unit/s` → 0.133 unit/子步（1/60s） | 2.7% |
| 手势在途峰值 | 8 unit/s × 飞行 ~1.5s ≈ 12 units | — |
| 摇晃扬起聚合上限 | 已被总额度钳在 ~5 units/子步（lift 请求 collect 阶段即进分母） | ≤100% |
| 未合并等包质量示例（非质量硬上限） | `Nmax×m_i`：m_i=0.05→25.6；0.1→51.2；0.25→128 units；T6可生成更大质量包 | — |
| 跨界笔数上限 | 5 units/子步 ÷ m_i：m_i=0.05→100 笔；0.1→50 笔 | — |

**结论（三条，均可从账本直接复核）**：
1. **手势源率示例不等于输运可达性证明**：8 unit/s的新准入量每子步0.133，占5 units额度2.7%；已经在途的全部存量还可能重复跨界并最终落床，实际竞争必须按§13.3的完整分母计入。不得把新增率占比外推为“不挤占其它渠道”。
2. **摇晃可能持续占满槽位，合并/降级须按正常工况验证**：300 unit/s×候选飞行1.5s≈450 units是全额度均给lift时的量级上界，不是可同时实现的lift+cross+deposit吞吐。`Nmax×m_i`只描述未合并等包量级，不能写成允许质量加权T6后的池质量硬上限；真正的约束是槽数、逐列容量与共享预算。池满仍按既定合并/降级链处理，不静默提高Nmax/m_i，定标走§9。
3. **跨界与落床/扬起共享同一总额度**（§7.2 的事务执行序内串行扣减：粒子槽 deposit→crossing，链尾 lift），无第二额度、无快车道；预算耗尽时水平位移等待（§3.1），垂直 deposit 静置重试（§2.3）——等待与降级都不产生质量丢失。

### 3.4 与 advection/S3/S4/correction 的预算共享核对（完成条件）

| 通道 | ΔS | local | total | bottom | channel cap | 与粒子的关系 |
|---|---|---|---|---|---|---|
| settle | +q | c | q（经 clipSubstepMotion） | — | — | 与 deposit 同型；汇不同（susp→床 vs 粒子→床） |
| percolate | 0（列内） | — | q（clipTotalMotion） | — | — | 无共享面 |
| bottomCreep | ≠0 | 双端 | 经 clipBottomCreepMotion 独立比例 | 10/s 专账 | — | 无共享面 |
| relax/correction | ≠0 | 双端 | 共享 total；correction 另有预留折叠（`ordinaryBudget=totalLimit−rLimit`，:1852-1857） | — | — | **粒子请求进同一分母**（§4.2 collect 阶段），与 correction 预留竞争同一 total，无豁免 |
| deepCreep | 0 | 双端各 q | 2q | 不扣 | 记账不限流 | 无共享面（S-invariant 登记同类） |
| advection | 0 | 双端各 q | q | 不扣 | `advectionChannelCap` 限流 | 与跨界同为「悬浮/在途的列间搬运」，但对象不同（列均值悬沙 vs 离散粒子），**两机制并存不合并**，遥测分池 |
| **粒子（本稿）** | T2/T3 改 S | T2/T3 单端 c；T4 双端 | T2/T3/T4 各笔实际量 | 不扣 | `particle` 字段记账不限流（候选） | 全部进 total 分母（§4.2）；**correction 共享额度明确：粒子与 correction 预留共同折叠 total，粒子不享预留、也不给 correction 让额度**——竞争序由 collect 请求总量决定 scale，apply 按事务序串行扣减 |

**S3 湿度**：不是预算通道（阈值层插件，零接口变化）；对粒子的作用是**局部参数输入**（起沙门槛/内聚/阻力/终端速度的湿度调制，§9.4），不进预算账本。

---

## 4. 工作 6（部分）：模块、接口、时序与事务边界

### 4.1 模块清单

| # | 模块 | 位置 | 内容 | 依赖 |
|---|---|---|---|---|
| M1 | `P011ParticleField.ets` | physics/ | 粒子池（SoA+空槽栈+列质量缓存）+ 纯积分器 + 六类事务 + 事务流水（journal） | 无（不 import render） |
| M2 | WaterSandField 集成 | physics/WaterSandField.ets | §4.3 的 9 个接入点 + 审计/totals/dormant/快照扩展 | M1 |
| M3 | `P012ParticleLayer.ets` | render/ | 只读粒子快照的 Canvas2D 绘制（三段遮蔽复用+分组批绘） | M1 快照、sampler |
| M4 | adapter/帧槽扩展 | render/P012WaterSandAdapter.ets | FrameSlot 加粒子 SoA 拷贝（O(Nmax)/帧，复用对象） | M3 |
| M5 | 页面接线 | pages/SandCanvas.ets | route 门、tracer feed 互斥、生命周期钩子、want 参数 | M2/M3 |

### 4.2 时序：双跑积分与请求分母（本稿最核心的工程裁定）

**问题**：粒子运动是连续积分，不是「请求→scale→apply」形态；但若完全绕开 collect/apply，就开了预算快车道（违反 §3.4）。

**rev1 裁定：同一不可变输入生成确定性请求计划 + apply 重算核对计划 + 同型 sink 提交；见 §13.3。** collect 不调用真实 clip/guard/写入接口；apply 不在裁剪后的状态上重生成分母请求。下列是定位时序，完整签名与账本公式由 §13 定义。

```
substep(dt)（collect 模式，渠道尾部——liftByShake collect 之后、末级 updateSandTop/audit 之前）：
  saveParticleState()                       // SoA 快照（O(Nmax)）
  plan = collectParticlePlan(identity, frozenField, frozenPool, requestCollector)
      // collect只推进scratch，无真实budgets/commit写接口；不提交粒子或七数组；
      // 产出事务日志（每笔 {kind, particleId, columns, phase, mass, requested}）
      // 覆盖 T3 deposit / T4 crossing / T5 degrade / T6 merge（T2 扬起在 liftByShake
      // 自己的 collect 槽记 riseRequestAmounts，T1 准入在注入事务内，均不在此处）
  totalMotionBudgetRequested += Σ request.totalWeight
      // 只含 T3/T4 请求质量，各笔一次；T1/T5/T6 权重0，T2已在riseRequestAmounts记一次
  restoreParticleState()                    // 粒子状态回到子步初（apply 将重跑）

applyMotionChannels(dt)（apply 模式，渠道头部——advectionSlot 之前）：
  journalApply = applyParticlePlan(已核对第二跑一致的plan, commitSink)
      // apply只消费冻结请求计划；逐笔执行事务——
      //   T3 deposit：同型clip→guard→床增granted/粒子减granted；余量留原粒state=2，质量归零才释放
      //   T4 crossing：容量预演→clipSubstepMotion(c,c',m)→列质量缓存±
      //   T5 degrade：池满降级 susp（PARTICLE_DEGRADED_TO_COLUMN）
      //   T6 merge：池满确定性合并
      // （T2 摇晃扬起的 spawn 发生在链尾 liftByShake 槽——本子步新粒子不参与本子步
      //  积分，下子步才动：无瞬移，与 advection「起沙最快下一子步被搬运」同语义）
```

**rev1 输入同源性**：在 waterLevel/diffuse 后的 motion snapshot 建立处同时冻结七数组、派生S、粒子池/列质量、dt、planner接受角、种子/参数revision；collect尾部与apply头部都读这份快照，不能凭“尾部应未写数组”推断同源。两次纯积分必须给出同一请求key/质量/顺序；apply授予差异还包括local/total、容量、guard和池槽条件，不能笼统称“只差scale”。完整比较、分母以及失败回滚见 §13.3。

**rev1 preview/commit镜像义务**：粒子新增分母与链头T3会改变后续旧渠道的额度、床面和guard残差，不能用“粒子日志另对账”跳过planner镜像。D2必须在候选角preview、接受角verify及真实apply的同槽消费同一生成规则，并在影子field/池/预算上先重放粒子plan；详见§13.3的planner接入补充。D1不接真实planner，只出可供影子/真实sink复用的执行器。旧preview/commit等式与tan30门槛不放宽。

### 4.3 WaterSandField 接入点（9 处，精确到行）

| # | 位置 | 改动 |
|---|---|---|
| A1 | 类字段区（:741-748 旁） | `private particles: P011ParticleField \| null`（null=route 未武装）；`particleParams` |
| A2 | :1801 附近（consumePendingGestureSand 同区） | 无需新钩子——注入汇切换在 `injectGestureSand` 内部按 route 门分流（§2.1） |
| A3 | :1509-1519（注入事务备份区） | 注入事务增 `saveParticleState/restoreParticleState`（失败回滚含粒子态，与七数组备份同机制） |
| A4 | :5316-5349（settleSandV2 公式区） | 不改；粒子 deposit 在 M1 自己的事务里（同函数不同汇） |
| A5 | :5806-5884（liftByShake） | 汇按 route 门分流（§2.2）；容量式 `suspH+suspL`→`suspH+suspL+colParticleMass` |
| A6 | :3594-3615（substep 尾部） | 插入 §4.2 collect 跑 |
| A7 | :3617-3651（applyMotionChannels 头部） | 插入 §4.2 apply 跑 |
| A8 | :6023-6059（audit） | 容量项加 `colParticleMass[c]`；有限值项加粒子六数组；**漂移分母按 §0.2 修正（D-C1）** |
| A9 | :1936-1946（getTotals）/:3353-3371（getDrift）/ :1777-1785 与 :6007-6015（dormant）/ :3421-3455+6150-6173（snapshot）/ :1730-1753（invert180）/ :1222-1303（initialize） | 见 §5 |

### 4.4 复杂度与帧时风险

- **积分**：O(liveCount) ≈ O(Nmax)；每粒 ~20-30 次浮点运算 + 2 次 O(1) 列查表（`S[c]`/`waterTop[c]` 直索引）+ 分支事务测试。Node 估 ~50-100ns/粒 → 512 粒 ≈ 25-50µs/跑；双跑 ≈ 50-100µs/子步。
- **事务**：每笔 O(1)（guard ≤2 边闭式、clip O(1)）；每子步事务数 ≤ liveCount×2（跨界+落床）。
- **渲染**：O(Nmax) 遮蔽采样（3 次 sampler/粒，tracer 先例）+ ≤2 相群×≤3 alpha 档 = ≤6 次 fill 提交。
- **帧时风险（诚实登记）**：ArkTS 真机解释开销实测为桌面 Node 的数倍（P-012/E3 经验：桌面基准**不足**以裁决帧时）；上表全部是**候选预算**：物理 ≤0.3ms/帧、绘制 ≤0.2ms/帧、adapter 拷贝可忽略——**以真机 P50/P95 为准**（§4.6）。降级次序（帧时不达标时）：减可见粒子数/合并绘制 → 提高合并激进度 → Nmax 下探（§9 定标）→ **绝不减物理子步、绝不降 128 列、绝不跳物理 step**。
- **GC/分配**：粒子池定容预分配，运行期零分配（事务日志用定长环形缓冲，候选 4096 条/子步，溢出记 `journalOverflow` 遥测并冻结子步——防静默丢账）。

### 4.5 异常与降级路径汇总

| 异常 | 行为 | 守恒性 |
|---|---|---|
| 池满 + 新准入（T1） | T6 合并（确定性选粒）→ 仍满 → λ 再折半重解（注入事务内）→ λmin 以下 CAPACITY_BLOCKED 零写入 | 准入量本就没进场，无丢失 |
| 池满 + 扬起（T2，已扣预算已改 S） | T6 合并 → 仍满 → T5 降级 susp + `PARTICLE_DEGRADED_TO_COLUMN` 标志 | 质量进场（susp），不二次扣额 |
| deposit 被 guard 拒 | state=2 静置床面，下子步重试 | 质量在池 |
| 跨界容量不足 | 边界等待，下子步重试 | 质量在池 |
| 非有限值（任何数组） | audit 冻结子步 + `NON_FINITE_VALUE`（现制） | 回滚 |
| 日志溢出 | 冻结子步 + `journalOverflow`（新遥测） | 回滚 |
| D-C1 未修时的注入 | （现状）永久冻结——**必须先修 D-C1** | — |

### 4.6 性能采集方案（桌面不足裁决，双通道）

1. **Node 通道（算法成本，现在就可跑）**：`integrateParticles` 计时（每相/子步 µs）、事务密度直方图、合并频率、Nmax∈{128,256,512,1024}×场景{手势 only, 摇晃 heavy} 扫描；判据：积分 ≤50µs/跑（512 粒）量级且随 Nmax 线性。
2. **真机通道（帧时裁决，effector 配对）**：同一构建跑 §4.6-1 的同参数场景，采集 `physicsTimeMs`（现制，step 循环 :1925 附近）、fps P50/P95、draw 提交数、hilog 分段耗时；判据：物理段+绘制段合计不使 fps P95 跌破现行档位（对照 B tracer 臂）。**桌面数字只作预期，不作裁决**（红线上沿用 E3 结论）。
3. **Nmax 定标**：由真机 P95 帧时与 §3.3 的池容量关系联合决定；定标前一律按候选 512 行为「上限扫描值」而非默认值。

---

## 5. 工作 5：生命周期、快照与 checker

### 5.1 reset / initialize

`initialize`（:1222-1303）现清 sourceInjected*/sourceEpoch/gestureQueue——**增补**：粒子池全清（state 全 0、liveCount=0、nextId 不清零（run 内 id 唯一性）、列质量缓存清零、事务日志清空）。reset=initialize 同路径。**旧无源 run 的粒子恒空**——公式退化，无迁移假历史（§6 选型稿「旧无源数据不迁移成曾有粒子的假历史」照办）。

### 5.2 invert180（倒置）

现行（:1730-1753）把每列沉积+悬沙**全部**转入镜像列 susp 并重造 waterTop（列总量字段的无位置近似）。对粒子采用**刚体镜像**（C 的存在意义就是有位置，不做无位置坍缩；映射 O(N) 且精确）：

```
x' = cols − 1 − x ;  y' = capacity − y ;  vx' = −vx ;  vy' = −vy
phase/mass/sourceKind/id 不变
newWaterTop[mirrored] = getWaterVolume(c) + newSuspH + newSuspL + colParticleMass[mirrored]   // 在途粒子同占水柱空间
```

**适用域明示**：镜像后粒子可能位于新 waterTop 之上（现行 invert180 把水坍缩到新底部，水面并非旧水面的镜像）——超出部分按 airborne 处理（§3.2 水面之上规则），随后 waterLevel 动力学重建水面。该近似的代价是倒置瞬间的粒子-水面关系可能短暂不连续，**属已知边界，登记不隐瞒**；倒置后的再沉降/守恒不受影响（质量守恒与镜像严格成立）。

### 5.3 TEST↔EXPERIENCE 切换 / renderMode 切换 / 页面离开

- **粒子是物理状态**：跨 TEST/EXPERIENCE、跨 renderMode、跨页面隐藏**持续保存**（不因外观切换清物理库存）。与 B tracer 的现行规则（模式切换清 tracer，B 设计 §4）**并存不冲突**：清的是 tracer（外观），不是粒子（物理）。
- 页面 aboutToDisappear：粒子随 field 保存（快照 §5.4）；renderer 的粒子帧槽随 renderer dispose，重建时从 field 快照恢复。
- 手势结束/取消：清页面侧未消费路径积分（现制 `clearFallingSand` 的队列部分）；**在途粒子继续飞完**（「抬手后余沙落完」是正确行为）。

### 5.4 snapshot / schema

- 场内快照 `WaterSandSnapshot.schemaVersion 3→4`：追加粒子块 `{ particleCount: number; pPhase: Uint8Array; pMass: Float64Array; pX/pY/pVx/pVy: Float64Array; pSource: Uint8Array }`（定容 Nmax，`particleCount=0` 时数组可空置）。`copyStateTo` 接受 3（粒子按 0 容忍，向后兼容惯例同 v6 ledger）或 4；逐帧路径沿用**复用对象 + `set()` 拷贝**（页面 :2076-2078 现制，禁每帧分配）。
- **hilog 证据路径（P-009 关联）**：`snapshotJson`（页面 :1030 一带，WS_SNAPSHOT 4KB 截断瓶颈）**默认不含粒子块**；粒子状态经观测账本（§5.6）的汇总字段进证据链。若未来要逐粒落盘，走分片导出（P-009 既定方向），不在本卡范围。

### 5.5 sourceKind 分账与 expected 语义

| 项 | GESTURE_EXTERNAL | SHAKE_LIFT |
|---|---|---|
| expected 增量 | `IH/IL += Σadmitted`（一次性） | **0**（相内搬运） |
| 账本字段 | `sourceInjectedHeavy/Light`（现制） | `shakeLiftInFlightH/L`（新，仅遥测/对账） |
| t90 | sourceEpoch 递增→窗口重启（现制） | 不重启（不起砂 epoch）——**注意**：现制 lift 也走 susp 汇但不碰 sourceEpoch，C 保持一致 |
| dormant | 入场即醒（现制 :1582） | 入场即醒（liveCount>0 阻断休眠快速路径，§5.3） |

### 5.6 checker / 观测账本扩展（schema v6→v7）

**契约权威来源**：tester 预注册稿 [preregistration-P011-dual-field-particle-v1.md](preregistration-P011-dual-field-particle-v1.md)（`METHOD_FROZEN / NOT_IMPLEMENTED / NO_CAPTURE_AUTHORIZATION`，protocolId `p011-dual-field-particle-v1`）已冻结 estimand 与遥测契约语义；**本稿是字段名的落地权威**（预注册稿 §2 明示「字段名以架构稿为准对齐复审」）。本稿 §4.2 的事务流水（journal）逐笔字段与预注册的七类事件一一对应：

| 本稿事务 | 预注册事件 kind | 关键字段（以预注册 §2.3 为准） |
|---|---|---|
| T1 spawn | `SPAWN` | particleId, phase, mass, sourceKind, x, y, sourceId, requested, admitted |
| T3 deposit | `LAND` | particleId, phase, mass, column, requested, granted, deltaS, guardPreview, remainder, remainderDisposition |
| T4 crossing | `CROSS_COLUMN` | particleId, phase, mass, fromColumn, toColumn, budget{local,bottom,total}, distance |
| T2 lift | `LIFT` | particleId, phase, mass, fromLayer(bh/bl/mh/ml), column, requested, granted |
| T6 merge | `MERGE` | particleIds[], phase, massBefore, massAfter, positionMomentPreserved |
| T5 degrade（整粒） | `DEGRADE` | particleIds[], phase, mass, toColumn, toSuspH/toSuspL, flag=`PARTICLE_DEGRADED_TO_COLUMN` |
| T3 余量转 susp（schema保留，本版不启用） | `TO_SUSP` | particleId, phase, mass, toColumn, suspDelta, reason；当前T3固定保留原粒 |

- **路由身份（context 级）**：`particleRouteActive` / `particleRouteId`（与 routeId 双条件门同源，:2231 `setNaturalObservationRouteId` 单值字段天然互斥）。
- **池摘要（每子步 `particles` 块，仅 route 武装时记录，OFF run 零开销——advection 遥测开关先例）**：`count/massH/massL`、`bySourceKind.{GESTURE_EXTERNAL,SHAKE_LIFT}.{count,massH,massL}`、`capacityUsed/capacityLimit`（候选 Nmax=512 非冻结）、`degraded.{count,massH,massL}`。
- **守恒判据（checker 新增 MUST-PASS，C route run）**（预注册 §3 estimand 原样）：
  - `VH_actual=Σ(bh+mh+suspH)+Σ_particle(H) mass` 对 `VH_expected=VH0+sourceInjectedH` 漂移 ≤1e-9（VL 同理）；
  - 互斥原子性：同一子步同一质量不得同记 susp 与粒子；每次转移经一笔事件原子完成；
  - 部分授予余量：`granted+remainder==requested`；rev1 固定余量留原接触粒子（state=2），同 id/sourceKind，不隐式转 susp；仅质量归零释放槽。`TO_SUSP` 是预注册允许的另一显式事件形式，当前 T3 不启用（§13.1），不得删除或无 guard 直写；
  - 逐列容量 `S+suspH+suspL+colParticleMass[c] ≤ waterTop[c]+EPS` 且 `≤capacity`；
  - 水量 `|ΣwaterTop'−ΣwaterTop|/Vw0 < 0.1%`。
- **终点悬沙口径（协议 revision 项，不单方面改）**：现 `suspensionFraction=Σsusp/totalSand`（getMetrics，:3373+）未含在途粒子。C 下分报三桶：`suspFraction`（列悬沙）/`airborneFraction`（在途粒子，=池摘要 massH+massL 口径）/`depositedFraction`（现制）。**「终点悬沙≤5%」门槛建议改判 `susp+airborne`**（都非沉积），分项仍披露——**须走 designer/manager 协议 revision，本稿只登记提案**；在 revision 落地前，C run 的终点验收同时报两个口径，不用旧口径单独放行。
- **26/20 双角状态机**：仍只读真实 `S` 与 `effectiveTilt`；粒子运动不伪造滑移 start/stop；T2/T3 改 S 时与 settle/lift 同受 guard 与 planner 约束。

---

## 6. 工作 4：与 B tracer 的切换/删除规则

### 6.1 现状冻结

C 实现前，`P012SandTracer.ets`、renderer 接入（:469-471/:808）、页面 feed（:2352-2356）、`sandTracerEnabled` 开关——**零改动**。B 继续作为过渡外观（manager 2026-10-23 裁决 B 首版 + 用户 10-24 选型回执均确认）。

### 6.2 C 启用时的互斥规则（同一库存不画两遍、不记两遍）

| 面 | 规则 |
|---|---|
| 绘制互斥 | D2 的 WaterSandField.particleRouteActive() 为唯一物理门；D4-D6 由快照传给 buildP012FrameContext，页面只消费，不重算。为真画粒子且不画tracer，为假走tracer、不建粒子层；D1只交纯谓词与契约（§13.4） |
| feed 互斥 | 页面 `stageGestureSandAndFeed` 在 C 下仍调 `injectGestureSand`（收 receipt 对账），但**不 feed tracer**（receipt 的 admitted 已由粒子层真实呈现；再喂 tracer 即同一库存画两遍） |
| 记账互斥 | tracer 永不写物理（现制红线不变）；粒子层的质量来自物理账本——**「粒子落地闪烁」不得另记库存**（B 的接触闪烁是纯视觉，C 的落床是 T3 真事务，两者语义不同、不得混串） |
| 消融开关 | `p012ablate=tracer` 枚举在 C 下**改义为「真粒子渲染层开关」**（消融视觉、物理库存不受影响——B 设计 §5 预留表同款） |
| 拒绝注入 | admitted=0 零粒子零 tracer（两路同一红线：路由 bug 单测断言） |

### 6.3 删除规则与保留件

- **保留**（C 启用后仍在用）：`InjectionReceipt` 九字段契约（物理账本输入，非 B 私有）、三段遮蔽逻辑（粒子层复用同一几何：waterTop 裁剪/雾带 smoothstep/沙 mask）、生命周期钩子触发面、ablate 机制、环形队列限界思路（粒子事务日志的定长缓冲同思路）。
- **删除判据**（三条同时满足才可删 `P012SandTracer.ets`）：①C route 通过隔离原型+真机双报告（守恒 checker + effector 主观）；②manager 裁决 B 退场；③粒子层渲染 Nmax 档位定标完成。删除时把 tracer 内联的三段遮蔽并入共享纯函数（C 层已是共享模块首消费者，无漂移风险）。
- **过渡期双轨纪律**：B 的 tracer **不得作为 C 的守恒、落床时刻或扬沙验收证据**（用户回执原文）；B 录像只作过渡期主观参考。

---

## 7. 工作 6（续）：接口、门控与参数

### 7.1 核心接口（精确到可实施）

```ts
// M1 P011ParticleField.ets（纯函数式积分器 + 有状态池）
export interface P011ParticleParams {
  massPackage: number;        // m_i 候选（§9.1）
  nmax: number;               // 候选 512（§9.6）
  terminalSpeedHeavy: number; // 候选（§9.2）—— NOT heavySettleSpeed
  terminalSpeedLight: number;
  dragHeavy: number;          // 水中阻尼速率 1/s 候选（§9.2）
  dragLight: number;
  liftSpeedHeavy: number;     // 起沙初速候选（§9.3）
  liftSpeedLight: number;
  liftSpread: number;         // 起沙横向散布（列/s）候选
  mergeColRadius: number;     // 合并列邻域候选（§9.5）
  mergeHeightRadius: number;  // 合并高度邻域候选
  contactEps: number;         // 静置间隙候选
  channelRateCap: number;     // 跨界 channel cap 候选（首版可 0=不限流仅记账）
}

export interface P011ParticleFieldView {          // 积分器只读视图（双跑同源）
  sandTopAt(c: number): number;
  waterTopAt(c: number): number;
  suspAt(c: number): number;                       // suspH+suspL
  freeSpaceAt(c: number): number;                  // waterTop−S−susp−colParticleMass
  cols: number; capacity: number;
  effectiveTiltRad: number;                        // planner 接受角（只读）
}

export interface P011ParticleTransaction {
  kind: 'spawn' | 'lift' | 'deposit' | 'crossing' | 'degrade' | 'merge';
  particleId: number; phase: number; sourceKind: number;
  mass: number; requested: number;
  column: number; targetColumn: number;            // crossing 用
  granted: number;
  budgetClipped: number; guardRejected: number;
  capacityClipped: number; poolClipped: number;
  requestKey: string; sourceId: number;
  remainder: number; massAfter: number; released: boolean;
  remainderDisposition: 'RETAIN_PARTICLE' | 'NONE'; // T3本版固定策略
  guardPreview: P011ParticleGuardDecision | null; // T2/T3实际guard证据；其余null
  mergedIds: number[];                             // merge 用
  substepId: number;
}

// rev1：collectParticlePlan / applyParticlePlan 完整接口见 §13.2/§13.3；
// 旧 integrate(..., budgets, collect:boolean) 已撤销，不保留运行时兼容入口。
```

```ts
// M2 WaterSandField 公开面（新增，其余公开 API 不变）
setParticleRoute(params: P011ParticleParams | null): P011ParticleRouteResult; // 返回拒绝/忙状态，见§13.4
particleRouteActive(): boolean;                             // routeId+params 双条件门（D-1 先例）
particleTotals(): { heavy: number; light: number; count: number; resting: number };
```

- rev1 `P011ParticleBudgetSink` 的完整签名、票据退款、collect能力隔离与真实提交sink见 §13.2；禁止把旧 `clipLocalPair/clipTotal` 分两次调用而对同一质量重复扣 total。D1 用同公式桩，D2接真实账本。
- **routeId**：候选 `dual-field-particle-v1`（`P011_PARTICLE_ROUTE_ID`）；门控 `naturalObservationRouteId===P011_PARTICLE_ROUTE_ID && particleParams!==null`。v1 要求**独占绑定**（不同时绑 advection/correction/deepCreep/moisture route）——组合路由留 manager 裁决，防多新通道混叠归因。
- **want 参数**（候选，命名随实现定）：`p011particle=on:<m_i>:<nmax>:<vscale>:<cap>` 五段式（`p011adv` 先例），非法整段拒绝；默认 off（C 未验收前产品档不走粒子汇）。

### 7.2 事务执行序（每子步，全序）

固定序（确定性 + collect 跑与 apply 跑同序）：

1. **T1 spawn**（注入事务内，子步最前，`consumePendingGestureSand` 同区——现有注入位不变）：准入量直接进粒子池。
2. **粒子积分槽**（apply 链头）：`deposit(T3, 按粒子 id 升序) → crossing(T4, 按粒子 id 升序) → degrade(T5) → merge(T6)`。
3. **T2 lift**（链尾 `liftByShake` 槽，按列升序）：guard 准入的 split 量 spawn 进池——**本子步新粒子不参与本子步积分**（下子步才动），与 advection「起沙最快下一子步被搬运」同语义，无瞬移。

---

## 8. 隔离原型验证方案（优先：守恒 + 起沙→飞行→落床）

**原型范围（P0，Node 通道，无 renderer/无页面）**：M1+M2 的 A3/A5/A6/A7/A8/A9 + D-C1 修复 + 观测账本 v7 汇总字段。目标是在任何视觉工作之前证明「床↔粒子交换守恒、起沙→飞行→落床闭环、预算/guard 身份、route 隔离」。

**Node 单测清单（13 项；tester 预注册卡 6adc4394…2edc 的 16 例 DUALFIELD-* 已冻结 estimand，本清单是其**实现侧单元边界**，映射关系见表下）**：

| # | 用例 | 判据 |
|---|---|---|
| 1 | 守恒闭合（含在途） | 随机 spawn/lift/deposit/crossing 600s：`VH/VL−(initial+IH/IL)` <1e-9；逐列 `S+susp+particle+water=waterTop≤capacity` |
| 2 | 互斥不变 | 事务日志推导的「同一质量双记」计数恒 0 |
| 3 | guard 身份 | 完整requested分解为granted+budgetClipped+guardRejected+capacityClipped+poolClipped；guard输入单独满足budgetGranted=guardAccepted+guardRejected（无其它后置裁剪时）；违约边non-worsening，拒绝/部分授予余量留源（§13） |
| 4 | 预算身份 | 每笔 local/total 扣减；四层 used+remaining 闭合；跨界双端 local；correction 折叠下 scale 行为 |
| 5 | 容量预演 | 满列跨界被拒（边界等待）；满容注入 λ 降；spawn 后 I-3 逐列成立 |
| 6 | 合并守恒 | merge 保质量与一阶矩（x/y/v 质量加权）；同种子同幸存者（确定性） |
| 7 | 降级链 | 池满→merge→degrade 标志置位；无删除；无二次扣额（预算 used 不变） |
| 8 | route 隔离 | D1纯谓词/桩门；D2验证C OFF无库存变更时与固定配对基线逐位一致；有粒子时直接卸武返回BUSY保留状态，不丢库存或冒称历史逐位回退（§13.4） |
| 9 | dormant | 在途粒子（含静置待准入）阻断快速路径；粒子清空后恢复现制判定 |
| 10 | invert180 | 镜像公式保 `VH/VL` 与逐列容量；airborne 边界不产生非有限值 |
| 11 | snapshot | schema 4 往返；schema 3 读取粒子为 0 不报错；复用对象拷贝逐元素相等 |
| 12 | 水容积 | `getWaterVolume` 扣在途粒子后，注入 η 不变量（`ΣwaterTop` 变化=0）仍成立 |
| 13 | D-C1 回归 | 持续注入越 0.1% 后 step 不冻结；`audit(true)` 与 `getDrift()` 口径一致 |

**与预注册 16 例的映射**（预注册稿 §3-§6 estimand 为准，本清单不另立判据）：CONS-01..04 ← 1/2/12；BUD-01..03 ← 3/4/5；LIFE-01..04 ← 8/9/10/11（LIFE-04a 路由关闭逐位等价已由 tester 今日可执行）；SRC-01..03 ← 6/7/13；GES-01..02 ← P2 真机（BLOCKED 于 s134 闭环/污染声明）。两稿字段名冲突时以本稿 §5.6 为准（预注册稿 §2 已声明该复审机制）。

**P1（手势+扬起全链，Node）**：injectGestureSand 汇切换 + liftByShake 汇切换的端到端账本对账（准入量=在途+已落+已降；扬起量=在途+已落）。
**P2（真机，effector）**：渲染层+页面+主观录像——**只作效果证据，守恒以 checker 为准**；验收场景初拟：静置落沙（轨迹贴重力、落床时刻与 ledger grant 时差）、倾斜落沙、摇晃扬沙（起飞-飞行-落床闭环、终点悬沙两口径）、满容拒注（零粒子零 tracer）。

**原型出口条件**：1-13 全绿 + P1 对账闭合 + D-C1 修复合入 → manager 验收 → 才建 developer 实现卡（M3/M4/M5 渲染与接线）。

---

## 9. 数值候选登记表（全部待定标，非冻结）

| # | 参数 | 候选值/域 | 定标依据 | 备注 |
|---|---|---|---|---|
| 9.1 | 质量包 `m_i` | {0.02, 0.05, 0.1, 0.25} unit | 未合并等包量级 `Nmax×m_i` vs 在途峰值（§3.3）+可见密度，非硬质量上限 | 尾数不丢（§2.1.1） |
| 9.2 | 终端速度律 | `v_term(phase,环境)`：水中候选 heavy 64 / light 38 unit/s 量级（**与 settleSpeed 数值同量级但语义全新——settleSpeed 是列均值沉降比例律，不是已验证粒子速度律**）；airborne 候选自由落体+微阻力 | 真机录像落床时刻 vs ledger grant 时差 + Node 弹道 | **红线：不得直接把 settleSpeed 当已验证粒子速度**；`vx=v_term·sin(θ_eff)`、`vy=−v_term·cos(θ_eff)` 为候选形态 |
| 9.3 | 起沙初速 | 向上 `liftSpeed` 候选 {4, 8, 16} unit/s；横向散布 `liftSpread` 候选 {2, 4} 列/s | 扬沙观感 + 守恒无关（速度不定标也守恒） | 与 shakeEnergy  ramp 解耦 |
| 9.4 | 环境参数 | 干/湿/水下三档阻力与起沙门槛；S3 湿度场（`moistureField`，128×K）作局部输入；**首版静水阻力近似适用域：只读局部水域判定（y vs waterTop(x)），不读水速场（v1 无水速场）——有流动时近似失效，须明示** | S3 定档 + 效果试采 | 水下粒子读 `waterTop(x)` 连续采样 |
| 9.5 | 合并邻域 | `mergeColRadius` 候选 {1, 2} 列；`mergeHeightRadius` 候选 {2, 4} 材料单位 | 合并频率 vs 轨迹保真 | 确定性选粒（同相/近列/近高度） |
| 9.6 | `Nmax` | {128, 256, 512, 1024} | 真机 P95 帧时（§4.6） | 512 是扫描值非默认值 |
| 9.7 | 视觉参数 | 点径 {1, 2} px、alpha 档 ≤3、每 receipt 上界 | 真机观感 + E3/B 锚点配对 | 归 renderer 定标 |

---

## 10. 冻结不变量 → 检查点对照表（完成条件）

| 冻结项 | 数值/口径 | C 下的检查点 |
|---|---|---|
| 总额预算 | 300/s（`totalMotionBudget`） | §3.1/§4.2：粒子三类事务全经 `clipSubstepMotion`/`clipTotalMotion`，请求进分母；单测 4/13 |
| 列数 | 128，不升分辨率 | §1.1：粒子 x 连续映射到 128 列；池与列解耦；无新网格 |
| 守恒 | 分相 VH/VL，含在途 | §5.6 I-1 + 单测 1/12；`getTotals/getDrift/audit` 三处同口径（D-C1 修复后） |
| 库存/互斥 | 同质量不双记 | I-2 + 单测 2/7 |
| 容量 | 逐列 ≤waterTop≤capacity | I-3 + audit 扩展（A8）+ 单测 5/11 |
| 有限值 | 全数组 finite | I-4 + audit 扩展（A8） |
| tan30 | 提交前逐笔 guard | §2.4：T2/T3 闭式拦截；T1/T4/T5/T6 S-invariant 登记；单测 3 |
| 26/20、30°±2°、t90、R² | 双角状态机/终点包 | §5.6：粒子不喂状态机；t90 epoch 规则照旧；终点悬沙口径走协议 revision（§5.6 提案） |
| 四层预算结构 | local 28/s、bottom 10/s、total 300/s、channel cap | §3.4 全通道核对表；correction 共享额度明确（同 total 折叠，粒子不预留不让额度） |
| route 隔离 | 单值 routeId + 双条件门 | §7.1/§6.2；单测 8（OFF 逐位） |
| B tracer 现状 | 未验收 C 前零改动 | §6.1；tester 回归 B 既有 9 单测+配对录像 |
| 108-run 批 | 封存 82/108，物理身份不动 | §0.3：工作区开发；C 的 route/build/pilot 单开 |

---

## 11. developer 任务书预注册（**未授权**——manager 验收后才建卡）

| # | 模块 | 内容 | 前置 |
|---|---|---|---|
| D0 | D-C1 修复 | `audit(true)` 分母归正 + 单测 13 | **独立前置卡，先行** |
| D1 | M1 粒子池+积分器+六事务 | §4.1/§4.2/§7.1 全量（Node 可测） | D0 |
| D2 | M2 场集成 9 接入点 | §4.3 + totals/drift/audit/dormant/快照/invert | D1 |
| D3 | 观测账本 v7 + checker 字段 | §5.6 + 单测 1-13 | D2 |
| D4 | M3/M4 渲染层 + 遮蔽共享模块 | §6.2/§9.7；ablate 改义 | D3（可并行于 D2 末段） |
| D5 | M5 页面接线 + want 参数 | §6.2/§7.1；feed 互斥；生命周期 | D4 |
| D6 | 真机定标 + 双报告 | §4.6/§8-P2；Nmax 定标 | D5 |

实现次序建议：D0→D1→D2→D3（隔离原型出口）→ manager 验收 → D4/D5/D6。

---

## 12. 结论与路由

本稿把路线 C 落为第八份canonical库存、六事务与同源预算/guard契约。rev1明确：双跑对账之外，D2还须补planner影子镜像；新增源率不足以证明整池输运可达，Nmax×m_i不是合并后质量硬上限，最终吞吐由完整分母与真机数据核验。`sourceKind`分账统一两个入口，B继续保持过渡纪律。全部数值候选待定标，隔离原型与真实场守恒闭环先于视觉接线，当前可执行边界以§13.5和manager验收为准。

**路由**：本卡交 manager 验收。验收后建 D0（D-C1 修复，独立前置）与 D1-D6 实现卡（D1 起隔离原型）；tester 预注册稿（DUALFIELD-* 16 例，卡 6adc4394…2edc）的 estimand 已冻结，本稿 §5.6 是其遥测字段名的落地权威、§8 是其实现侧单元边界——tester 按预注册稿声明的复审机制对齐字段名即可，不构成方法变更；effector 真机队列待原型出口后接。**本稿不授权 developer 开工。**

**未决/需上游裁决项**（不阻塞本稿验收，阻塞后续实现）：①终点悬沙口径 revision（§5.6，designer/manager）；②粒子 route 与 advection/correction 的组合路由（§7.1，manager）；③Nmax 与 m_i 定标（§9，effector 真机数据后）。

---

## 13. rev1 提交、分母与路由契约（2026-10-24，当前规范）

依据：manager `priority.md` 15:35 六项补契约任务（6adc5ed0…c94e），直接裁定“deposit只扣granted、余量留原粒、质量归零才释放；D1只出路由契约，WaterSandField接门归D2”，以及冻结预注册 CONS-03/BUD-01/BUD-02/BUD-03。此节补工程可执行面，不改变冻结 estimand、物理阈值或候选参数。原 §7.1 不完整 integrate/sink 示意由本节替代；原 §11 未授权分卡表是历史预注册，最新实际边界见 §13.5。

### 13.1 T3 部分落床的唯一处理

设提交前粒子质量 `m>0`，请求 `r=m`，所有裁剪后最终床层授予 `g∈[0,r]`，`remainder=r-g`。一次提交同时完成：

```text
H粒：mh[c] += g；L粒：ml[c] += g
pMass[id] = remainder；colParticleMass{H|L}[c] -= g
updateSandTop；逐列容量/分相总量审计；LAND事件发布
remainder>0：原id、phase、sourceKind、sourceId不变，state=2，y=S_after[c]+contactEps，vy=0
remainder==0：清槽/列缓存归账，freeSlots.push(slot)，liveCount-=1（仅一次）
```

`g=0` 同样保留原质量并进入state=2；state=2每个后续子步显式重试，不依赖y是否再次穿过S。不可把 `mass<=EPS` 当清槽条件，不可把小尾数静默归零；只有真实全额授权时可用 `g=m,remainder=0` 的闭式分支消除舍入尾数。部分授权用保存的m做一次减法，事件与池使用同一remainder。拒绝后等待不是第二份库存，没有另建接触队列质量副本。

LAND字段：`mass=massBefore,requested=m,granted=g,deltaS=g,remainder=m-g,remainderDisposition=RETAIN_PARTICLE|NONE`；新增 `massAfter/released/sourceKind/sourceId/requestKey` 为审计辅助字段，不改既有字段含义。`released==(massAfter==0)`，`massAfter==remainder`。一次LAND只记录床层g，不能把remainder另计“已落床”。T3本版**不自动TO_SUSP**；预注册允许的TO_SUSP支路仍可保留schema，但不作为当前实现可选策略。若以后批准该支路，须明确一次独立T5式原子转移，不能删粒子后仅补日志。

T5（整粒降级）另定义：`susp{phase}[c]+=m；pool.mass-=m；colParticleMass{phase}[c]-=m`，三者同时发布；`S`不变，`susp+particle`逐列不变，`waterTop`不变；守恒/容量检查后才释放槽。事件DEGRADE含 `requested=m,granted=m,remainder=0,toSuspH/toSuspL`。它不是T3余量处理，也不冒充LAND。T2已从床扣出的已授予量若spawn失败，走同事务内T5式“床→susp汇”，见下节，不能先丢弃质量再补标志。

### 13.2 完整 sink、guard 与字段写入归属

所有下列类型在 `physics/P011ParticleField.ets` 导出（也可拆同目录纯类型文件）；不能import render。签名为设计契约，不是本次新增代码。

```ts
type P011ParticlePass = 'COLLECT' | 'APPLY';
type P011ParticleOp = 'spawn' | 'lift' | 'deposit' | 'crossing' | 'degrade' | 'merge';
interface P011ParticleRequest {
  requestKey: string; substepId: number; ordinal: number;
  kind: P011ParticleOp; particleId: number; sourceId: number;
  phase: number; sourceKind: number;
  column: number; targetColumn: number; // 无目标=-1
  requested: number; totalWeight: number;
  predictedX: number; predictedY: number; predictedVx: number; predictedVy: number;
  mergedIds: number[]; // 非merge为空，merge明确幸存id与被吸收id
}
interface P011ParticleBudgetTicket {
  ticketId: number; requestKey: string; substepId: number;
  source: number; target: number;
  requested: number; localAllowed: number; reserved: number;
  refunded: number; // 累计退款，≤reserved，票据对外只读
}
interface P011ParticleRequestCollector {
  record(request: P011ParticleRequest): void;
  finish(): P011ParticlePlan; // 不暴露clip、guard或canonical写接口
}
interface P011ParticleBudgetSink {
  reserve(request: P011ParticleRequest): P011ParticleBudgetTicket;
  refund(ticketId: number, amount: number): void;
  close(ticketId: number, committed: number): void;
}
interface P011ParticleGuardDecision {
  accepted: number; rejected: number; allLegalBefore: boolean;
  limitingEdge: number | null; affectedEdgeIds: number[];
}
interface P011ParticleCommitSink {
  apply(request: P011ParticleRequest,
    pool: P011ParticleAtomicPool): P011ParticleApplyResult;
}
interface P011ParticleGrant {
  requestKey: string; grantedHeavy: number; grantedLight: number;
  disposition: 'PARTICLE' | 'BED' | 'SUSP' | 'MOVE' | 'MERGE' | 'WAIT';
}
interface P011ParticlePoolPatch { patchId: number; requestKey: string; }
interface P011ParticleAtomicPool {
  prepare(request: P011ParticleRequest,
    grant: P011ParticleGrant): P011ParticlePoolPatch | null;
  publish(patch: P011ParticlePoolPatch): void;
  restore(patch: P011ParticlePoolPatch): void;
  discard(patch: P011ParticlePoolPatch): void;
}
interface P011ParticleApplyResult {
  requestKey: string; committed: boolean; errorCode: string | null;
  requested: number; granted: number; remainder: number;
  budgetClipped: number; guardRejected: number;
  capacityClipped: number; poolClipped: number;
  guard: P011ParticleGuardDecision | null;
  eventBegin: number; eventCount: number; // 本次成功发布的journal区间
}
```

**值域/身份**：所有质量/额度有限非负；phase=H|L，sourceKind沿 §1.2，列合法且crossing两端不同相邻。`requestKey=(runId,substepId,kind,particleId或sourceId,column,targetColumn,ordinal)` 序列化，runId由场提供；同key只能record一次、apply一次。T1/T2新生粒子用sourceId+ordinal定位，particleId尚未分配时为0，不把0当真实活粒id。字符串可在run/子步预构造并复用；事务数组定容，数量上界照 §4.4，溢出回滚，不无限增长。票据和patch按substepId绑定，不可跨子步退款、重复publish或重复close。

**M1只拥有粒子池写权限**：prepare在scratch验证质量/空槽/列缓存/目标位置并构造待发布patch，不写live池；publish将patch作用于live SoA，restore以patch保存的before-image撤回已发布修改，discard仅丢未发布patch。publish/restore不得分配、不得回调用户代码，且预检通过后不得逻辑失败。patch必须含质量、位置/速度、id/nextId、state、freeSlots/liveCount和列质量缓存的全部变更；不能只回滚pMass。多相T2的H/L整包与尾包均在一个patch内预留槽，不能H成功/L失败后留下半笔。

**M2拥有床/悬沙/水面/真实预算/guard/事件发布权限**：WaterSandField内部实现真实CommitSink，M1不能直接调用private函数或持有七数组可写引用。sink.apply在同一同步调用中协调字段stage和pool patch，D1 Node桩提供相同语义。真实apply顺序：

1. 验证路由、requestKey、substep/version与源库存；保存受影响床/悬沙/水面、预算状态、派生S、事件尾指针及相关遥测before-image。尚未提交前不对外emit事件、receipt或checker可见的半状态。
2. 对T2/T3/T4用同一个BudgetSink.reserve；T1/T5/T6不扣额度。T3 guard调用场内 `materialGuardAdmitIncrement(budgetGranted,new Map([[c,+1]]))`，T2用`[[c,-1]]`，返回现有闭式结果并附受影响物理edgeIds；T4/T5/T6是S-invariant，不调床面guard。guard不是额度扣减，禁止把accepted再次送clipTotal。
3. 容量/库存/池槽预检进一步截断时按**实际已扣票据余额**退款；T3被guard拒绝的金额等量退local+total，bottom=false。T2先获得guard许可，再在stage按当前床层splitTopV2同式求heavy/light，不能先改床再预览guard。没有正grant时不写床或缓存，允许保留WAIT状态。
4. 建立field stage（T3只增mh/ml；T2按splitTopV2的混合→轻→重顺序扣bh/bl/mh/ml；T5只增对应susp；T4不改S）。准备pool patch并验证列容量。T2池满先按 §4.5 同列安全合并，仍无槽则已准入H/L写susp并标降级；T1池满仍按原λ折半/拒注入，不先增IH/IL后丢粒。T1完整λ/η/sourceEpoch事务归D2，不由粒子sink重新发明。
5. 同步发布pool和field stage，刷新S，对受影响列与分相守恒审计；无外部观察插入这两次内存写之间。成功后close票据（净扣=实际g），再发布journal/receipt。失败则restore所有已发布池patch、恢复field/预算/事件尾；返回committed=false，并让外层子步回滚。禁止“catch后只退预算”保留床写入。

**退款精确语义（rev2修正接法）**：amount≤reserved-refunded，各local端按实际已扣额退，total整笔只退一次、bottom恒0；没有先扣就不能退款，重放退款拒绝。旧 `materialGuardRefund` 内部按columns.length平分amount，**不能**仅“包一层票据校验后传双端列表”来满足本契约。粒子统一走下方R9新增adapter，旧通道保留原路径。只退实际扣额，不退完整requested。`close`要求 `reserved-refunded==committed`（数值审计按冻结1e-9），close后票据不可修改；退款不是新的授权，不重新分配给本请求。若异常整笔中止，直接恢复before-image，禁止又restore又refund导致双退。

**T4整粒跨界**：先容量预演，再reserve双端；若reserved<m，整笔等待并全退reserved，净扣0、budgetClipped=m。不允许用部分额度移动整粒，也不在此卡私造分裂子粒。若足额且容量仍满足，移动整粒并对donor/receiver缓存各减/加m，total扣m、local各扣m。预算拒绝时x留原列内，不能让floor(x)先变到邻列但缓存仍在原列。容量拒绝计capacityClipped=m；拒绝原因按最先限制阶段分配，不能重复算同一m。

**T5两种提交源**：已有粒子降级先prepare扣原粒m，再stage悬沙+m，净总额/local均0；T2新提取质量的降级在同一lift事务内完成床−g/susp+g，不先生成假粒再销毁，不新增第二次额度扣减。两者事件注明sourceKind/sourceId和实际质量来源；T2记录LIFT的g，再以关联requestKey的降级disposition事件说明汇，不把这两条事件都算成新增质量。T3余量不进入该自动分支。

**合并边界**：同列同phase合并可免费（列质量不变），sourceKind/sourceId不同的粒子本版不合并，避免来源分账被抹平；同source多id保留mergedIds。原 §9.5 跨列邻域候选只可用作寻找候选，不得免费跨列合并改变列缓存：若合并需跨列质量移动，必须有独立T4计划与预算/容量许可，否则本子步不合并，走既定降级/拒注入。合并不会令质量必须≤massPackage，故旧 `Nmax×m_i` 仅为不合并时等包量级示例，不是已冻结的池总质量硬上限。

#### R9-REFUND_ADAPTER：粒子票据逐端退款窄接口（rev2）

依据：[manager双端退款裁定](decision-P011-dual-end-refund-adapter-v1.md) §2.1–§3、[D1反馈](feedback-P011-D1-refund-adapter-v1.md)及当前源码 `clipSubstepMotion/materialGuardRefund/P011GuardLedgerRefund.refundLocal`。核查确认reserve对source/target各扣a、total扣a；旧refundLocal对两列只各退amount/2，且未退部分不重分配。这是接口不兼容，不是D1票据公式错，不改冻结额度或旧通道行为。

**接口/存储**：保留M1公开 `P011ParticleBudgetSink.reserve/refund/close` 签名与scalar ticket；新增D2内部adapter，以票据sidecar保存逐端实际扣额，避免要求M1直接持有场数组。sidecar生命周期同substep，预分配复用，不以array push/对象新建实现生产热路径。

```ts
interface P011ParticleRefundReceipt {
  ticketId: number; requestKey: string; substepId: number;
  requested: number; amount: number; totalReserved: number; totalRefunded: number;
  localCount: number; // 1或2；以下数组定长2，仅前localCount有效
  localColumns: Int32Array;
  localReserved: Float64Array; // reserve时各端真实扣额，累计不变
  localRefunded: Float64Array; // 截至本次各端累计已退款
  netLocalUsed: Float64Array;  // 各端reserved-refunded
  netLocalDelta: Float64Array; // close时净扣额减该端committed；必须0
  netTotalUsed: number;       // totalReserved-totalRefunded
  netTotalDelta: number;      // close时netTotalUsed-committed；必须0
  netDeltaValid: boolean;     // REFUNDED=false，CLOSED=true；未close不能当最终净差证据
  bottomReserved: number; bottomRefunded: number; // 均恒0
  status: 'REFUNDED' | 'CLOSED';
}
interface P011ParticleTicketRefundAdapter {
  refundParticleTicket(ticketId: number, amount: number,
    out: P011ParticleRefundReceipt): void;
  closeParticleTicket(ticketId: number, committed: number,
    out: P011ParticleRefundReceipt): void;
}
```

`out`由调用方预分配，数组固定长度2；snapshot/journal如需保留记录须拷入已分配槽，不保留被下次重写的out引用。公开sink.refund映射到refundParticleTicket，close映射到closeParticleTicket；reserve仍只调用一次真实clipSubstepMotion，不改local先截断再scale的公式。D1提供可替换存储的adapter桩，D2绑定真实localUsed/localRemaining与totalUsed/totalRemaining；shadow allocator使用同一adapter算法、独立shadow存储，不能走旧平分路径。

reserve时记录 `localColumns=[source]` 或 `[source,target]`（source!=target，不重复列）、每端 `localReserved[i]=本票据实际local扣额`、`totalReserved=本票据实际total扣额`。当前粒子reserve保证每端localReserved=totalReserved=reserved；requested可大于reserved，但requested从未成为退款上界。sidecar另保存累计refund、closed状态与所属run/substep，ticketId不可串代；两个端点扣a不意味着total扣2a。

**一次refund的原子动作**：

1. 校验本substep有效、票据未close、amount有限且非负、amount≤totalReserved-totalRefunded，且不超过每端reserved-refunded。无票据/过期/已关/重复导致超额或非有限值全部拒绝，抛明确契约错误由上层回滚；拒绝前后全部账本与票据逐值不变。amount=0为无账本变化的幂等空操作，不改变closed票据可用性（已close仍拒绝）。
2. 预检所有实际localUsed与totalUsed足以退回本次amount，ledger或票据不一致应报错回滚，**不使用min(amount,used)静默吞掉欠退**，不借其它票据额度补本票据。冻结1e-9仅用于审计残差，不授权多退；不足不通过clamp修饰。
3. 对每个有效端i，`localUsed[c_i]-=amount; localRemaining[c_i]+=amount; localRefunded[i]+=amount`。两端各退amount，不能amount/2。
4. **循环外一次**：`totalUsed-=amount; totalRemaining+=amount; totalRefunded+=amount`；bottom不读写，不改limit、scale或requested分母，不触碰correction/channel其它子账。
5. 填out并核查本次每端 `ΔlocalUsed=-amount/ΔlocalRemaining=+amount`、total同量一次，used+remaining维持原limit。任一阶段失败恢复所有已动端/total/sidecar/out发布位置，不出现单端成功的半退款。

这里的“重放退款拒绝”指超额/重复已完成操作不能再退；合法的**多次部分退款**允许，累计不超过各端与total实际扣额。调用由原子提交器串行控制，不把网络重试或日志重放当作第二次退款。因异常已restore整笔before-image时禁止再调refund，避免双退。REFUNDED收据设置netDeltaValid=false并将净差数组/标量初始化为0占位；只有CLOSED且netDeltaValid=true的值才是最终净差断言，禁止把占位0报为已通过对账。

**close与净差定义**：令a为实际reserved，f为累计已退，g为最终committed。每端 `netLocalUsed=a-f`；total `netTotalUsed=a-f`。close要求两者均等于g，`netLocalDelta[i]=(localReserved[i]-localRefunded[i])-g=0`、`netTotalDelta=netTotalUsed-g=0`（按既有1e-9审计）；底账全0。netLocalDelta是**对账残差**，不是“部分成功也必须净扣0”；完整WAIT取消时g=0，两端netLocalUsed也各为0。REFUNDED但未close时不伪称最终g已知，netLocalDelta/netTotalDelta留待close验证，收据同名字段只在status=CLOSED有最终审计效力。close后不得继续refund或再次close，不隐式退回仍未解释的余额。

**禁止的两种接法**：① `materialGuardRefund(q,[source,target],false)` 会平分而欠退；②为每端各调一次 `materialGuardRefund(q,[c],false)` 虽local能退足，却会total退两次。D2本版采用新增adapter直绑票据存储，不修改旧materialGuardRefund/refundLocal。manager裁定允许的“新增接受逐端实际扣额入口”只作为等价实现选择；若developer确有必要改旧函数内部，必须保持全部旧调用逐值不变并有回归守卫，不能顺手修旧通道。

**数值断言/负对照（非实测报告）**：

| 场景 | 粒子adapter预期 | 旧双端调用已知差异 |
|---|---|---|
| crossing请求m=1，仅reserve a=0.4→整笔WAIT | source/target各退0.4，total退0.4，g=0，local净扣均0，bottom0，close残差0 | 旧函数两端各退0.2，各留0.2；total已退0.4。须记录0.2/0.2差异，不冒称等价 |
| 单端deposit reserve0.4，guard拒0.1 | 本端退0.1，total退0.1；g=0.3，netLocalUsed=netTotalUsed=0.3，close残差0 | 不因单端此例可用旧函数就给双端开例外；粒子统一adapter |
| 双端票据reserve0.4，合法分两次各退0.2（桩接口边界例） | 每端累计退0.4、total累计退0.4；再退正数拒绝且零副作用 | 不能靠旧函数调用次数凑齐local而把total多退 |

**D1/D2验收归属**：真实reserve与clip参数网格逐值相等断言保留；粒子rev1语义桩对照新增adapter桩，逐端reserved/refunded/total/netLocalDelta全检查。唯一涉及旧materialGuardRefund双端平分的对照按manager裁定变成**已知不兼容负对照**，保留输入/差异数值和旧函数原行为，不删除断言或改冻结阈值；其它§13.6验证组必须全绿。D1只补桩与证据，生产adapter/WaterSandField接线和planner shadow归D2。旧函数回归守卫、热路径零分配仍为developer/manager尚需验收的独立项，本文不豁免、不关D1。

### 13.3 collect/apply 分母同源与双跑可核验性

**能力隔离而非可变布尔标记**：collect函数只拿冻结FieldView、冻结PoolSnapshot、参数和RequestCollector，没有真实BudgetSink/CommitSink/guard写口。apply才拿CommitSink。旧 `integrate(...,budgets,collect:boolean)` 不作为rev1接口：

```ts
interface P011ParticleStepIdentity {
  runId: string; substepId: number; dt: number; effectiveTiltRad: number;
  seed: number; paramsRevision: number; inputRevision: number;
}
interface P011ParticlePlan {
  identity: P011ParticleStepIdentity;
  requestCount: number; requests: P011ParticleRequest[];
  totalRequested: number; requestHash: string; // 只含冻结请求身份/量，不hashgranted
}
collectParticlePlan(identity: P011ParticleStepIdentity,
  frozenField: P011ParticleFieldView, frozenPool: P011ParticleSnapshot,
  collector: P011ParticleRequestCollector): P011ParticlePlan;
applyParticlePlan(plan: P011ParticlePlan, sink: P011ParticleCommitSink): P011ParticleApplyResult[];
```

`P011ParticleSnapshot` 是M1完整SoA快照，不是仅 §5.4 为渲染准备的压缩块。字段精确定义如下；全部TypedArray只读借给collect（语言接口不能阻止底层数组写入时，用scratch副本和before/after校验约束），长度=nmax或cols，freeCount限定freeSlots有效前缀：

```ts
interface P011ParticleSnapshot {
  nmax: number; cols: number; liveCount: number; freeCount: number; nextId: number;
  pMass: Float64Array; pX: Float64Array; pY: Float64Array;
  pVx: Float64Array; pVy: Float64Array;
  pPhase: Uint8Array; pSource: Uint8Array; pState: Uint8Array;
  pId: Uint32Array; pSourceId: Uint32Array; freeSlots: Int32Array;
  colParticleMassH: Float64Array; colParticleMassL: Float64Array;
}
```

frozenField的查询函数必须绑定冻结数组，不能闭包读取实时WaterSandField。dt/接受角/随机输入只能来自identity。固定容量拷贝，不省略会影响双跑的自由槽顺序。sourceId继承手势receipt来源id或摇晃子步/列事件id，按run内单调Uint32分配；id溢出时拒绝新分配并回滚该事务，不能wrap重用。ID耗尽是异常，不截断现有质量。

时序明确为：

```text
已处理T1/注入 → planner接受角 → waterLevel/diffuse → saveMotionSnapshot
  同处冻结ParticleStepInput（七数组+S+池+参数+接受角+dt+seed）
  旧渠道collect（含lift的riseRequestAmounts）
  collectParticlePlan(frozenInput) → record T3/T4请求，冻结plan
  验证collect零canonical写入；restoreMotionSnapshot与池输入校验
  第二次collectParticlePlan(同一frozenInput, 独立scratch)
    requestKey/kind/columns/phase/source/requested/totalWeight/顺序/预测运动全同
    不同 → PARTICLE_PLAN_MISMATCH，整子步回滚，不进入apply
  scale由“旧渠道请求+粒子plan.totalRequested”算一次
  beginMotionApplication(dt,scale) → applyParticlePlan(plan,真实sink)（链头）
  旧渠道apply → 链尾T2 lift消费已缓存rise请求并spawn（下一子步才积分）
  audit → 一次发布子步事件；失败回滚字段+池+预算+源账本+日志
```

第二跑是核对相同输入的**候选计划**，不是在已受前笔裁剪影响的live状态上生成另一套请求。apply只消费已冻结plan，按当前实际状态再次作guard/容量/库存检查；这会改变grant，不能改变requested或偷偷新增没有进入分母的cross/deposit。已落床的粒子不能在同子步再跨界；计划生成阶段对每粒先判T3，接触候选不再生成T4，受阻即WAIT。没有候选deposit的粒子若跨界后碰到更高床面，先限制位置等待，下子步生成T3，不在apply临时加免费落床请求。合并/降级是已授权质量的表示处理，不增加totalWeight。

纯运动预测（含位置速度）保存在同一预分配计划scratch，第二跑逐值对照；不能因budget不足把已提交质量带入第二次积分再得不同请求。collect结束对七数组、live池、id/freeSlots、真实budget和shakeEnergy进行before/after核验；只允许request缓冲与请求统计变化。requestEnergy沿旧collect只读、apply推进一次的规则，collect不得额外推进shakeEnergy或消耗随机源。restoreMotionSnapshot不含池/id/随机态，因此M2必须显式保存/恢复它们，不能仅调用旧函数就声称全量回滚。

**分母定义**（明确排除表示事务与重复lift）：

```text
R_particle = Σ(plan中T3.requested) + Σ(plan中T4.requested)
R_all = R_existing + R_particle
R_existing已含riseRequestAmounts(T2)以及既有渠道按其现制计费的请求
T1/T5/T6.totalWeight=0；T2在粒子journal可出现但不二次加分母
B=totalMotionBudgetLimit=300*dt
C独占路由：R_correction=0，scale=(R_all<=B ? 1 : B/R_all)
未来若组合路由另获批：按原correction流程折叠B-rLimit，不新增particle预留
```

`record`只对T3/T4要求totalWeight=requested；其它事件weight=0；重复requestKey是错误。R_all只写入真实场分母一次，第二次重算计划不再record到真实场统计。T2聚合H/L的床总提取量q计一次，拆成H/L粒子包时不得再次累计total请求。两个端点local各m不意味着分母2m。旧预算渠道的独立计价（如deepCreep）不因粒子接入改动。

**apply的桩与真实clip必须逐式同型**（当前WaterSandField实际函数，不凭函数名猜测）：

```text
r = request.requested
l = min(r, localRemaining[source], target>=0 ? localRemaining[target] : +∞)
t = min(l*scale, totalRemaining)  // clipSubstepMotion内部把l传入clipTotalMotion
reserved = min(l,t)
localUsed[source]+=reserved；若pair则target同加；totalUsed+=reserved
相应remaining同减；bottom/correction/channel cap本版不扣
```

不额外先调用clipLocalPair再clipTotal重复计费，不对r预乘scale后再进入现有clip二次缩放，不用 `min(r*scale,l)` 替换 `l*scale`。collect记录r不受local/guard/实时剩余额度裁剪；live apply的reserve调用场内 `clipSubstepMotion(source,target,r)` 一次，并与D1桩公式逐值核对reserved/退款后余额。guard后净使用量=最终实际转移量g，不等于原请求。所有拒绝原因顺序分解为 `r=granted+budgetClipped+guardRejected+capacityClipped+poolClipped`；remainder是未转移的库存，不在右侧再加一次。T4整粒足额规则可能使预算部分许可全退，budgetClipped应记整粒m而不是只记m-reserved。

对已在collect计划中的操作，即使apply时guard/容量条件改变，只能减少grant或WAIT，不能释放其分母请求再让自己重获比例。无reissue、无carry-over。计划不匹配、非有限值、日志溢出、原子提交审计失败都走同一子步回滚；观察账本保留失败attempt，不能伪造成功grant。

**planner接入补充（分母同源的必要组成，D2负责）**：当前 `allocatePreviewPlans` 会读取totalMotionBudgetRequested并在shadow里演化七数组；只给真实apply加粒子会让旧渠道preview与commit不一致。因此：

- `evaluateTiltCandidate` 的候选角collect与权威接受角collect均在本身的motion snapshot处生成粒子冻结输入与plan；每个候选独立scratch，不把一个角的plan拿给另一角。候选preview的plan/request量随planning scalars保存恢复，不能把最后一次候选残留带到权威collect。
- `allocatePreviewPlans` 在advection镜像前，用同一个 `applyParticlePlan(plan, shadowCommitSink)` 更新shadow池、七数组、local/total和S；shadowCommitSink与live sink共用预算公式、guard约束推导、床/susp写入公式与退款语义，只换存储。不得复制一份会漂移的“近似粒子费用”。T2仍在既有lift镜像槽执行同一分流规则。
- C ON时在内部 `P011ChannelPlan` 的channelId枚举增加 `particle` 项，内容为 `requested=R_particle`、真实净grant、逐列deltaS、双端local总用量、bottom=0、total净用量和实际admissibility；比较器/可行性汇总消费这一项。C OFF不加项，原plan顺序/数值保持不变。粒子完整SoA仍由WaterSandField私有context提供，不塞进外部命令/姿态输入；内部类型枚举变更属D2本卡集成面。
- 候选preview与接受角verify比较particle projection和旧渠道全部plans，真实apply再对照同角镜像的逐笔grant/退款/deltaS；失败沿既有PREVIEW_COMMIT_MISMATCH路径回滚。不能因粒子不在旧枚举中就忽略其床变化或直接跳过旧校验。
- D1提供上述执行器与shadow/live桩可替换性证明；D2同时扩展planning备份/restore、内部枚举/比较与字段副作用覆盖。该新增接入义务由当前源码核查发现，随rev1交manager明确验收，不以“原稿未要求planner”将真实接线交给developer临场设计。

### 13.4 唯一路由门归属与卸武行为

**M1 / D1** 只交：`P011_PARTICLE_ROUTE_ID='dual-field-particle-v1'`、只读输入类型及纯谓词：

```ts
particleRouteEnabled(routeId: string | null,
  params: P011ParticleParams | null): boolean;
// 精确返回 routeId===P011_PARTICLE_ROUTE_ID && params!==null；无分配、无field访问或写入。
```

参数合法域先由setParticleRoute校验，不能让非法params对象绕过该门。M1不导入WaterSandField，不读naturalObservationRouteId，不注册want参数，不改页面，不自行初始化粒子route或其它通道。

**M2 / D2** 实现 `WaterSandField.setParticleRoute(params)`、`particleRouteActive()`，后者唯一读取 `naturalObservationRouteId` 与validated `particleParams`并调上述纯谓词；注入汇、lift汇、collect/apply、totals/audit/dormant/快照均调用该物理门或消费同一次冻结的route身份，不能各写第二套开关。setter设置params不擅自修改naturalObservationRouteId，路由绑定仍经既有单值入口；C v1不同时武装advection/correction/deepCreep/moisture。冲突返回 `ROUTE_CONFLICT`且保持原状态，不覆盖别的已武装route；以manager既有独占裁定为准，组合能力未授权。

setter返回类型 `P011ParticleRouteResult={ok:boolean,code:'OK'|'INVALID_PARAMS'|'ROUTE_CONFLICT'|'BUSY_PARTICLES',active:boolean}`，取代早期void返回的“无法拒绝卸武”歧义。rev1仅支持channelRateCap=0（不限流、记实际量）；非0返回INVALID_PARAMS，不能配置后静默忽略限流。`params=null`或切换route时若仍有canonical粒子，返回BUSY_PARTICLES且不改route/params/库存；待粒子清空，或调用明确的现有reset（重启整个物理run）后才能卸武。不能把有库存池丢成null以满足OFF逐位对照，也不能隐藏库存不计totals。D1仅用桩测试这一状态契约，真实setter/单值route变更入口的一致性检查归D2。

**M3/M4/M5 / D4-D6** 从field快照取唯一active身份并在 `buildP012FrameContext` 传给renderer；这里只选显示互斥，不是物理路由门的归属。页面不能自行以want字符串重算active。TEST/EXPERIENCE或渲染模式切换不等同物理卸武，已准入库存照 §5.3 保留。C OFF空池/未曾武装run的旧路径仍逐位回归；ON运行后的真实物理状态不可能凭关开关恢复为“从未ON”的历史，不作此承诺。

### 13.5 D1 / D2 / D4-D6 交付边界与解锁

| 交付段 | 本段负责 | 明确不在本段 | 门序 |
|---|---|---|---|
| D1 A（M1，已允许） | SoA/空槽栈/列缓存/ID与来源、journal、纯route谓词+接口、冻结输入/双跑骨架、桩sink、CONS-03部分授予余量断言 | 不写WaterSandField，不接真实guard/床/susp/route/页面 | 按manager 15:35可继续，不等待本稿验收 |
| D1 B（M1接口闭合） | 本节完整Budget/Commit/AtomicPool接口、M1调用协议及prepare/publish/restore、同公式桩guard/床/susp、分母与双跑对照单元；验证真实接法所需契约完整 | private真实guard/clip/splitTop调用与真实七数组写入仍归D2 | 本rev1经manager验收后；不把“B段解阻”误解成越权改M2 |
| D2（M2，含原D3） | 在WaterSandField提供真实sink适配、guard/clip/退款/床层/susp/λ/η原子提交、snapshot与collect隔离、唯一物理route门、九接入点、schema v7/checker字段、13单元与16预注册映射 | 不接renderer/页面、不做真机效果定标 | D1完成；manager按现有卡6adc52e4…2a86调度 |
| 原型出口 | CONS/BUD/LIFE/SRC客观对账和手势+扬起全链闭合；D-C1实际修复复核 | 不以桩PASS代替真实场集成，不以B录像代替C闭环 | manager验收后才建后续实现卡 |
| D4 | M3/M4只读粒子绘制、adapter快照、tracer互斥、遮蔽共享 | 不反写质量/route，不删B资产 | 原型出口后 |
| D5 | 页面/want/输入、统一route身份消费、生命周期接线 | 不造第二物理门，不改预算/guard | D4及manager调度 |
| D6 | 真机参数定标与双报告，真实呈现/帧时/观感证据 | 不私改冻结阈值、estimand或上限 | D5；设备串行队列仍有效 |

这里的D1 B“真实guard/床/susp接法”交付是**接口与桩证明可接**；生产WaterSandField接线归D2，解决原卡措辞混杂。D0 fdc6a5a按priority 15:15已验收，本稿不重新打开D0。D1 B是否放行由manager对rev1验收，本稿只提供所需契约。

### 13.6 实现侧验证要求（不改冻结用例）

- CONS-03：m=1、g=0/0.4/1三臂，分别验证池质量1/0.6/0、床增0/0.4/1、列缓存同步、仅全额释放；多次部分授予与小正尾数均不丢、不同source保留身份、失败回滚恢复id/freeSlots。
- BUD-01/02：同r/local/scale/total输入，D1桩与D2真实clip的reserve逐值相等；退款按R9对照粒子adapter桩，覆盖local先截断后scale、guard部分退款、T4部分许可整笔退、重复/超额退款拒绝、bottom不变。旧平分函数双端退款唯一对照按manager裁定作为保留差异数值的已知不兼容负对照，不能将其差异掩成生产adapter等价。
- 分母：T3/T4各记一次、T2仅rise一次、T1/T5/T6为0；第二跑不累加；全拒绝grant=0时requested仍保持；事件原因分解闭合。request顺序/seed/snapshot被故意扰动必须PLAN_MISMATCH且零成功提交。
- 原子性：分别在field stage、pool prepare、publish后审计与journal容量处触发失败，床/susp/池/列缓存/预算/源账本/journal一致回滚；T5旧粒与T2直接降级不双计、不二扣。
- 路由：D1纯函数无field依赖；D2 route+params双门真实消费，冲突与有粒卸武均保持原状态；空池C OFF同配对基线回归。D4-D6不先行接线。

这些是实现侧边界断言及冻结用例的落地映射，**不是本次已执行的测试结果**，未修改预注册16例及任何阈值。若现有checker字段不足，按§5.6字段复审机制补实现，不自行改tester estimand。

**HOT 阶段追加验证要求（§17 及独立附件 §9 为准，不豁免上述五条）**：D1 的生产热接口收口除上述五条外，另须满足附件 §9 的五组——①静态完整性（HOT 根与所有可达方法的调用边与显式分配点清单，原 60 处逐项可追溯为「已迁冷/已替代/仍残留」，仍有可达显式分配即 NOT_ZERO_ALLOCATION，不得把 exit=2 改成 PASS）；②动态固定身份（覆盖 600s/36,000 子步全事务沙盒与双 collect/完整 apply，构造/容器/序列化 trap + 数组 buffer 引用与长度身份前后快照，trap 捕不到对象字面量须与 AST 及堆证据合用）；③句柄反例（旧 token 数值保留、discard/finish/restore 后再 prepare、rollback 同 substep 重试、reset/reconfigure、跨 owner、重复 publish/finish、busy prepare、MAX_SAFE_INTEGER 耗尽，每项拒绝后状态逐值不变，且显式保留旧 out 引用并复用 out 证明旧 primitive token 不会被后来字段改写）；④物理/日志（热冷同输入逐笔等价，现 33+11 例与既有回归全保留，CONS-03 正尾数与质量零才释放、预算/guard/来源/容量/一阶矩与 R9 负对照不变，Q/E/lineage/ticket 各容量边界与发布后审计失败均全量回滚且不伪造成功 journal）；⑤性能诚实性（只在上述功能与分配条件满足后报告 CPU P50/P95/GC/内存，Node 证据不代真机 ArkTS 帧时，零分配不得仅由 RSS 平稳推断）。

## 14. rev1 逐条修订索引与一致性自检

| # | 位置 | 原文/旧缺口（历史记录） | 新文/当前契约 | 依据 |
|---|---|---|---|---|
| R1 | §1.3 T3、§2.3第4/5步 | “allowed>0…粒子消失（释放槽）” | 仅扣granted，余量原id/state=2，质量归零才释放；§13.1展开 | manager直接裁定、CONS-03 |
| R2 | §4.2 apply伪代码 | “mh/ml+=q→粒子消失” | 床增/粒减均为granted；余量重试，禁止EPS删除 | 同上 |
| R3 | §5.6 LAND/余量、§8 guard/route条目 | 余量可任意TO_SUSP；requested=granted+guardRejected漏预算层 | T3只保留；其它裁剪互斥分解；T5另原子提交 | 冻结CONS-02/03、BUD-01 |
| R4 | §7.1旧sink、§13.2 | sink未定义、床/susp谁写不明、guard无调用面 | 完整签名/票据/patch/退款；M1池、M2字段与真实sink；原子回滚 | 本卡第②项、§19.5现实现 |
| R5 | §4.2分母/同源论证、§13.3 | Σ\|journal\|混计T5/T6；只说“差scale”；collect clip机制不明；声称不需planner镜像 | 冻结输入→纯候选双跑核对→不可变plan apply；T3/T4权重，T2仅rise；同型local后scale；D2补候选preview/verify/live同槽镜像 | manager第四缺口、现clipSubstepMotion/clipTotalMotion及allocatePreviewPlans |
| R6 | §6.2、§7.1、§13.4 | buildP012FrameContext被称唯一门；D1误承担field接线 | D1纯契约谓词，D2唯一物理门，D4-D6只消费；卸武不删库存 | manager归属裁定、LIFE-04 |
| R7 | §11历史表→§13.5 | 旧D1/D2/D3与已建卡界线不明，旧D4可早于出口 | A/B两段、D2含账本、原型出口后D4-D6；B真实接法=接口证明，生产接线归D2 | priority 15:35及14:50 |
| R8 | §3.3/§9.5的等包与跨列合并候选→§13.2 | Nmax×m_i写成硬质量封顶；免费邻列合并可能旁路T4 | 仅等包示例；免费合并只同列同相同来源，跨列须独立T4授权 | 分相/来源守恒、BUD-02/03，不增加额度 |
| R9 | 原稿状态/§0.2/§12→页首及§13.5 | 原稿待验收、D-C1未修描述过时 | 保留发现历史，注明原稿manager验收/D0已修，rev1待验收不自行放行B段 | priority 15:15/15:35 |

全稿检查口径：①两处释放整粒的规范表述已替换，原错句仅在本索引引用；②§4.2/§5.6/§8/§7.1/§6.2均指向当前契约；③未定义sink类型已在§13补齐，新增类型的所有权/值域/副作用有说明；④分母不含T1/T5/T6、不重复T2，桩与真实clip公式一致；⑤只有M2写真实field，只有M1写池，CommitSink协调原子发布；⑥路由/交付阶段与manager分卡一致；⑦CONS-03仍MUST-PASS、质量零才释放，无放宽；⑧不修改tester预注册、manager账本或产品代码。

## 15. endpoint字段revision登记（2026-10-24）

独立附件：[endpoint字段revision v1](architecture-P011-endpoint-fields-revision-v1.md)（6adc64ff…cb44）。用户已按[裁决记录](decision-P011-terminal-nondeposited-fraction-v1.md)批准终点U+P、历史分母不变、5%及固定终点末1s质量窗；本稿§5.6早期“建议改判/待裁”的状态作为历史提案保留，当前以该批准记录为准，**协议仍未正式生效、实现未验收**。

附件登记 `context.endpointSampleElapsedMs/context.integratedPhysicsSeconds`、post-commit身份与采样时序、`particles.byState.{FLYING,CONTACT_WAIT}`、三分项定名、历史R与E区别及报告字段清单；字段revision为`p011-endpoint-fields-v1`。三分项共用累计期望E，旧WS_METRIC.suspensionFraction保留旧公式，不作为新名别名。正式endpoint从原始v7同子步质量重算，不从250ms metric或渲染点数推断。

**rev2对齐更新**：16:24首次登记时tester稿尚未落盘（原状态PENDING_TESTER_PROTOCOL_ALIGNMENT，protocolId/revision当时留空）；tester16:26落盘后，manager18:05已出[对齐裁定](decision-P011-endpoint-field-alignment-v1.md)。现已核对tester T1–T3记录，附件R1–R3全部执行，状态 **ALIGNED**：endpointProtocolId=`p011-endpoint-terminal-nondeposited`、endpointProtocolRevision=`endpoint-terminal-nondeposited-v1`，证据=裁定+tester首次commit `69e27b9`+当前§11/§13。唯一名字为context.endpointPhaseOccurrenceId、legacyEndpointFraction（SLIP U/R_SLIP，WS U/(D+U)）、legacyMetricSuspensionFraction（U/max(A,EPS)）。developer终点指标卡的字段名一致性前置已在文档侧收齐，交manager复核，不再写“等待tester稿发布”；正式协议仍须实现+双报告+designer验证+manager确认适用范围，不改DUALFIELD/peakSusp/t90另案。

## 16. rev2修订索引（6adc85fc…81cb）

| 本卡项 | 位置/原缺口 | 当前修订 | 依据/验证范围 |
|---|---|---|---|
| HOT-1 | §4.4「运行期零分配」硬性措辞与§13对象/字符串API无法同时满足；缺生产热路径的定容缓冲与句柄契约 | 新增§17 + 独立附件 `architecture-P011-M1-hot-buffer-contract-v1.md`：HOT/COLD边界、八类定容缓冲与容量符号表、数值request身份、全局不回卷代际token、expectedStepEpoch授权、§13.6 HOT阶段追加验证要求 | manager 19:25 验收；纯设计零代码，阈值/guard/质量/R9语义/预注册estimand不变 |
| R9-REFUND_ADAPTER | §13.2原“包一层票据校验”误以为旧双端退款可直接复用 | 新增窄接口/逐端sidecar/原子退款/total仅一次/bottom0/净差定义；旧通道零修改；§13.6同步负对照 | 双端退款裁定§2.1–§3、源码逐行核对；设计未冒称实现PASS |
| R1 | endpoint附件§1/§7 protocol身份留空、alignment待对齐 | 复制tester §0两版本身份、ALIGNED及可核证据 | manager字段对齐裁定§3-R1，tester §11/§13 |
| R2 | 16:24“未落盘”被写成当前阻塞 | 保留16:24诚实声明，补16:26落盘/18:05裁定事实链；本稿§15同步 | 同裁定§3-R2，不删除历史痕迹 |
| R3 | 报告字段名与历史WS分母语义需明确 | 两个legacy字段/各自分母/原PASS-FAIL及身份分列；不互别名/不单独放行 | 同裁定§1 A2/A3、§2、§3-R3，附件§5/§6 |

本次只修改两份architecture文档，未修改manager裁定、tester协议/用例或任何产品/测试代码。旧退款负对照是manager指定的唯一对照口径修订，不豁免adapter桩、其它§13.6验证或生产零分配；D1/D2完成状态由后续developer证据与manager验收决定。

## 17. M1热接口契约登记（D1 R3，2026-10-24）

独立附件：[M1生产热路径定容缓冲与数值代际patch句柄契约 v1](architecture-P011-M1-hot-buffer-contract-v1.md)（6adc91c3…d612，P5）。该附件补全严格零分配收口所需签名与生命周期，不采用developer提案中的有界分配降级B、不放宽§4.4或任何冻结预算/guard/质量/estimand。

生产新增caller-owned定容plan A/B、request/motion/result/field/budget/guard/event/lineage缓冲，count/index有效前缀；完整HOT可达图禁止临时对象/数组/闭包/Map/Set/字符串/JSON/异常对象。prepare返回process内唯一、永不回卷或随rollback恢复的安全整数token，publish/restore/discard/finish验证当前owner/stepEpoch和状态；plan/sink调用显式携带调用方保存的expectedStepEpoch，避免复用out别名让旧操作获新授权。旧§13对象API保持冷诊断/兼容的独立副本语义，生产HOT不反调冷wrapper。

R9逐端实际退款/total一次/bottom0及负对照保持；双collect全字段逐值核对、分母T3/T4各一次/T2仅rise、部分LAND余量、全量checkpoint/错误回滚与对外v7字段保持。热内数字身份只在冷导出边界物化原requestKey/事件，不省略逐笔证据。容量溢出仍明确失败+整步回滚，不扩容、不丢质量或日志。

交接：developer按附件实现D1热接口与桩，完成全可达显式分配审计、长时固定buffer身份、ABA/跨步/跨owner/回滚/容量反例与原全部功能回归后交manager验收；adapter局部无分配或现319/0不替代R3通过。真实WaterSandField/λ/η/route/preview接线仍归D2，本文不自行解锁D2、不占真机。

## 变更记录

- 2026-10-24，rev3（6adc91c3…d612）：新增§17与独立热接口附件 `architecture-P011-M1-hot-buffer-contract-v1.md`，明确HOT/COLD边界、完整调用图零分配、数值句柄拒旧、rollback与冷导出契约；§13.6 增HOT阶段追加验证要求五组；§16 索引增 HOT-1 条目；页首有效修订更新为rev3。纯设计零代码，阈值/guard/质量/R9语义/预注册estimand不变，**D1 R3仍待实现与manager验收**。

- 2026-10-24，M1热接口补充（6adc91c3…d612）：新增§17与独立定容缓冲/数值代际patch附件，明确HOT/COLD边界、完整调用图零分配、数值句柄拒旧、rollback与冷导出契约。纯设计零代码，D1 R3仍待实现与manager验收。

- 2026-10-24，rev2（6adc85fc…81cb）：按两份manager裁定补§13.2 R9粒子票据退款adapter契约，明确旧平分函数不适用/双端各退实际扣额/total只一次/bottom0、退款与close净差、已知不兼容负对照及D1/D2归属；§15/§16关联endpoint附件R1–R3对齐。纯设计零代码，不改冻结预算、质量、guard或预注册estimand。

- 2026-10-24，endpoint字段登记（6adc64ff…cb44）：新增§15关联独立字段附件，记录用户口径已批准、协议未正式生效及tester字段对齐前置；原设计/裁决历史不覆盖。纯文书零代码。

- 2026-10-24，rev1（6adc5ed0…c94e）：按manager 15:35六项补契约修订。两处落床余量丢失表述与关联条目已纠正；新增§13完整提交/guard/退款/床与susp写入/collect隔离/分母同源/路由归属/交付边界，§14逐条索引保留原文与依据。D1 A可按既有授权继续，B仍等manager验收。纯文书零代码，预注册estimand及冻结阈值不变。

- 2026-10-24：依据 manager【P-011·路线C·工程设计】任务（6adc4394…0f7f）产出。覆盖 §6 后续五项+模块细化：床面↔粒子原子交换与 guard 可提交条件（§2，含平沙大角度 lift 被 non-worsening 拦截的「留床重试、不改道」裁定）/跨列计费与 300/s 可达性（§3，定量三条结论+全通道预算核对表）/susp 互斥与降级（§4 事务表+§5.6 三桶口径提案）/B tracer 切换删除（§6）/生命周期与快照 checker（§5）/模块接口时序复杂度帧时（§4）+性能采集双通道（§4.6）+数值候选登记（§9）+隔离原型 13 项（§8）+不变量检查点对照（§10）。新发现阻塞级缺陷 D-C1（`audit(true)` 漂移分母用错基线→持续注入永久冻结，Node 探针 `test/probe_p011_audit_drift_gate.cjs` 实证），登记为 C 的硬前置并给修复方向（基线公式归正，不动 DRIFT_LIMIT）。代码现状核对：`injectGestureSand`（693a724）与 B tracer（bbe8e33）均已实现——本稿以其为依赖面而非从零定义。
