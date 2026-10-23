# P-011 S3 湿度分级工程设计 v1：六模块工程方案 + 五接口预留 + 三维候选比较 + developer 任务书

日期：2026-10-23。责任：architect。状态：**ENGINEERING_DESIGN / READY_FOR_DEVELOPER（离线原型+真机定档二段）**。依据：[S3 算法设计](algorithm-P011-S3-moisture-grading-v1.md)（已验收）+ [工程任务书草稿](task-P011-S3-moisture-architect-v1.md)（六模块必答表+五接口预留+三维候选条件审核清单——**本卡照此范围不增不减**）。代码现状核对（2026-10-23）：`relaxSliding`/`relaxDecisionOrdinals` per-edge 数组已存在（:904）；状态机调用点在 relaxAngle collect 扫描 :4375（`p011RelaxHysteresisNextActive(activeBefore, deviation, hysteresis, ...)`——单全局 pair）；`P011ActiveThresholdConfig`（:92）含 relaxMode/configRevision/thresholdSource；`executedThresholdsForLedger:2063` 返回单 pair 结构；`shakeEnergy:732`（shakeDecay=2.2/shakeRampRate=8 既有动力学）；`initialWater:679`；ledger schema 已 v4；`WaterSandSnapshot` schemaVersion=2 含八数组无湿度场。

## 0. 总体结构裁定

**S3 是「阈值层插件」不是「新物理通道」**：湿度场 s[c,k] → 每边局部 pair → 替换 relaxAngle 状态机的阈值输入——材料通道（settle/percolate/bottomCreep/lift）、guard（§19.5）、预算（300/s+local+bottom）、planner（§20）全部**零接口变化**（湿度只改请求不改判据常数）。工程落点分三块：①新状态（湿度场+每边 pair 缓存）+ 刷新时序；②状态机局部化改造（全局 pair → per-edge pair，**不新增第二状态机**——改造既有 `relaxSliding` 数组的消费方式）；③兼容层（OFF 位精确 + ON/default 同轨迹）。

## 1. 模块一：逐列湿度场（必答 1）

### 1.1 字段落点（WaterSandField 私有，snapshot 扩展）

```ts
// WaterSandField 新增私有状态：
private moistureField: Float32Array | null = null;      // 128×K，null=moistureRoute OFF
private moistureOccupancy: Float32Array | null = null;  // 128×K 占据掩码 m[c,k]
private moistureRevision: number = 0;                   // 冻结版本号（模块三）
private moistureParams: P011MoistureParams | null = null; // 武装条件（route 双条件门）
private moistureK: number = 8;                          // 8|16 两档

// 每边阈值缓存（127 条边）：
private edgeThetaStart: Float64Array = new Float64Array(0);  // 派生缓存非持久状态
private edgeThetaStop: Float64Array = new Float64Array(0);
private edgeThresholdFrozen: boolean = false;                 // moistureRevision 冻结标记

// P011Moisture.ets（新文件，纯函数）：
export const P011_MOISTURE_MODEL_VERSION = 's3-moisture-profile-v1';
export interface P011MoistureParams {
  k: 8 | 16;
  sBaseSlope: number;   // s_base(q) 线性候选：sBase0+sBaseSlope·q（首版最简）
  sBase0: number;
  betaSlope: number;    // β(q)=betaSlope·q（线性候选）
  muShakeMax: number;   // μ_shake 上限
  muShakeTau: number;   // 摇晃停止回落时间常数
  qRef: number;         // 默认水量身份（冻结旧场景）
  aC: number; aB: number; // 角度增益
  cBreaks: [number, number, number];  // C(s) 分段点（s1,s2,s3，项目定标）
  bBreaks: [number, number, number];  // B(s) 分段点
}
```

**内存/快照/备份/重建归属**：

| 项 | 归属 | 说明 |
|---|---|---|
| 内存 | `moistureField`/`moistureOccupancy` 各 128×K×4B（K=8: 4KB×2；K=16: 8KB×2） | 设计稿 §2 数字核对一致 |
| 快照 | `WaterSandSnapshot` **schemaVersion 2→3**：追加 `moistureField?: Float32Array`、`moistureOccupancy?: Float32Array`、`moistureK?: number`、`moistureModelVersion?: string`（全部可选——OFF run 缺省 undefined，旧消费者零破坏） | 快照是渲染/诊断消费面；P-012 renderer 只读（若 E 档将来要湿度着色，读 snapshot 即可，不改物理） |
| 备份 | **不进 saveBackup/restoreBackup**（子步级八数组备份）——湿度场子步内不变（模块三的冻结时序保证），子步边界整体刷新；材料 writer 的回滚不触碰湿度 | 湿度不是材料，writer 回滚语义无关 |
| 重建 | run/reset：`moistureField=null` 后按新初态重算；mode 切换：保留 s（设计稿 §3「TEST/EXPERIENCE 切换保留合法材料/水场，清路线私有缓存」——s 是合法材料属性场的代理，保留；每边 pair 缓存清（方向翻转重算 donor 的要求） | |
| ledger | `P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION` 4→5：追加 `moisture: { modelVersion, q, qRef, k, sMin, sMean, sMax, edgeThetaStartMin/Max, edgeThetaStopMin/Max, thresholdCause }`（旧 run 缺省容忍——§24.2-4 向后兼容惯例） | 模块六详述 |

### 1.2 K=8/16 性能档选择依据

帧时预估（文档级）：每子步刷新成本 = 占据格扫描 O(128×K)（K=8: 1024 格；K=16: 2048 格）+ 代数赋值 O(1)/格 + 每边 s_e 计算 O(127×平均占据格数)——总计 **K=8 ≈0.02ms、K=16 ≈0.04ms/子步**（Node 通道量级，真机 ×2-3 保守系数）。60fps 子步 1-2 个 → 湿度总开销 <0.1ms/帧（K=16 最坏）。**裁定：默认 K=8 起步**；K=16 作为定档实验臂（want 参数 `p011s3k=16` 切换），真机配对块（OFF vs K=8 vs K=16 三臂）测帧时后定档——**K 不进产品 UI**（性能档非效果参数），定档后写死。

## 2. 模块二：水量输入事务（必答 2）

### 2.1 现有 inject(WATER) 与产品级事务的边界（裁定）

**现有 `inject(WATER)` 保持不动**（离散场景初始化入口：改 waterTop+重置 initialWater/initialHeavy/initialLight 基准——它服务场景搭建，语义是「定义新场景」不是「运行中操作」）。产品级加水/排水是**运行中事务**，独立设计：

```ts
// WaterSandField 新公开方法（独立于 inject）：
addWater(requested: number): P011WaterTransactionReceipt | null;
drainWater(requested: number): P011WaterTransactionReceipt | null;
// Receipt: { admitted: number, rejectedReason: 'CAPACITY_FULL'|'INSUFFICIENT_WATER'|null,
//            waterBefore: number, waterAfter: number, revision: number }
```

**事务语义（五条硬规则）**：

1. **来源/去向可追溯**：加水来源=外源（用户操作），去向=自由水账（waterTop 均匀抬升或注入列局部——**首版均匀抬升**：ΣΔwaterTop=admitted 按列容量比例分配，复用 η 求解的单调性思路但目标是加法不是排水；排水反向：从 waterTop 均匀降，admitted=实际移除量）；每笔回执进 ledger（模块六）。
2. **容量/余额**：加水受 `Σ(H−waterTop)` 全局余量界（不足→CAPACITY_FULL 拒绝或部分准入缩放——**裁定：部分准入**（照 injectGestureSand 的 λ 语义先例），admitted=余量与请求的 min）；排水受当前自由水余量（`Σmax(0,waterTop−S−susp)`，不足→部分准入）。
3. **基准不重置**：`initialWater` **不随事务变**——expectedWater=initialWater+累计净准入（新字段 `waterTransactionNet`，构造/reset 清零）；这是 §27.3-3（gesture inject 不重置基准）的同款裁决，修正旧 inject 的重置缺陷路线。
4. **拒绝零影响**：admitted=0 时 waterTop/s/阈值/全部状态逐位不变（单测硬断言）；部分准入时只按 admitted 执行。
5. **完成后刷新**：每笔成功事务后立即读 Vw 重算 q → 触发湿度刷新（模块三时序：事务在子步边界，刷新随后）。

**UI 口径**：面板显示「实际水量」= 当前 Vw（物理真值），操作输入目标量/增量；q 不进 UI（内部派生量——设计稿 §3「不能把 q 滑杆直接改成水量而不入账」）。

### 2.2 与 s 的账目边界（红线落地）

`s` 是代理不是水：**不加 s·m 到任何水量统计**；`initialWater`/expectedWater/守恒审计全部只看自由水账；snapshot 的 moistureField 仅供消费（阈值/渲染），不进 audit 的水量项。单测断言：任意湿度参数下，同输入的 `ΣwaterTop−expectedWater` 漂移与 OFF run 逐位一致。

## 3. 模块三：代数刷新/搅拌时序（必答 3）

### 3.1 刷新算子实现点与两轮时序（核心裁定）

**刷新位置：子步边界、planner 之前**（设计稿 §3 的时序链照搬）：

```text
step() 子步循环内（每子步）：
  consumePendingGestureSand / inject 消费（既有）
  → 【S3 新增】water command commit（若本子步有待处理水事务——§2 队列）
  → 【S3 新增】refreshMoisture()：
      a. 读 Vw（getTotals 实时）、Vs、shakeEnergy
      b. q = Vw/max(Vw+Vs, ε)
      c. 对全部占据格：s[c,k] = (1−μ_shake)·s_eq(c,k;q) + μ_shake·s_mean(q)
      d. 占据掩码 m[c,k] 重算（沙顶变化后）
      e. 派生每边 s_e → C/B → Δ_e → edgeThetaStart/Stop
      f. moistureRevision++（版本冻结点）
  → saveBackup → updateSandTop → beginSubstepMotionBudget → ...（既有链）
  → planner（previewAt）读 edgeThetaStart/Stop（经 moistureRevision 冻结校验）
  → apply（applyMotionChannels → relaxAngle 读同版本 pair）
```

**两轮时序的关键实现**：`moistureRevision` 在子步首刷新后冻结到子步尾——planner 的 preview 通道与 apply 通道都读**同一份** edgeThetaStart/Stop 数组（数组本体子步内不写——只有 refreshMoisture 写，而它只在子步首跑）。**无需显式版本比对**（单线程+子步内无写者=天然冻结）；moistureRevision 仍是 ledger 遥测字段（跨子步审计用）。shadow 镜像：planner 预演的 relax 槽位读局部 pair 的镜像——**edgeThetaStart/Stop 是纯派生缓存（从 s 派生），shadow 管线不镜像 s 本身**（预演中 S 变化不反馈进 s——s 只在子步首按子步前状态刷新，preview 期间 S 的假设变化不影响阈值，这正是「preview/apply 同一湿度版本」的实现方式）；但 relax 槽位镜像的 naturalAmount 计算读 per-edge pair（`p011RelaxHysteresisRequested(deviation, edgePair, ...)`）——shadow 侧传 edge pair 的同一冻结数组即可（数组引用只读，零复制）。

### 3.2 μ_shake 来源（既有 shakeEnergy 接线）

`μ_shake = muShakeMax · clamp(shakeEnergy, 0, 1)`（shakeEnergy 是既有 [0,1] 标量：shakeRampRate=8 上升/shakeDecay=2.2 回落——liftByShake 的驱动量）；停止摇晃后 shakeEnergy 按 2.2/s 指数回落 → μ_shake 同步回落（s 剖面从均匀恢复分层）——**无需新时间常数字段**（复用 shakeDecay；muShakeTau 保留为候选参数但首版由 shakeEnergy 直接派生）。**正常 shake 不改水量**：μ_shake 只重排 s 读数（设计稿红线），单测断言 shake 全程 Vw 逐位不变。

### 3.3 s_eq 与 q_ref

首版剖面族（设计稿 §3 公式的参数化实例）：`s_base(q)=sBase0+sBaseSlope·q`、`β(q)=betaSlope·q`（两参数线性族——定标域见模块五；非线性族留 s3-local-seepage-v2 前的效果迭代）。`q_ref`：**出厂默认=默认场景构造时的水量比**（`defaultWaterSandParams` 场景的 Vw/(Vw+Vs) 实测值，developer 实现时算出写死为常量）；ON 且 q=q_ref 且 μ_shake=0 时 s_e≡s_ref,e → Δ_e≡0 → 每边 26/20（兼容层锚点，模块五）。

## 4. 模块四：局部双角（必答 4）

### 4.1 有向边供体表层 s_e（占据权重平均）

边 e=(c, c+1)，方向由 deviation 符号定（:4366 既有 donor/receiver 判定）。**供体表层**定义（设计稿 §4「可移动供体表层所覆盖格」）：donor 列的沙顶向下 `tTop` 材料单位内的占据格（tTop=表层厚度候选，默认 2.0 材料单位——定标参数）：

```text
s_e = Σ_k m[donor,k]·s[donor,k]·w_k / Σ_k m[donor,k]·w_k
  其中 w_k = 1 若 z_k ∈ [S_donor−tTop, S_donor]，否则 0
  （z_k 是格中心绝对高度；m 是占据掩码——部分占据格按 m 加权）
```

**方向翻转重算**：active 方向反转 → donor 换列 → s_e 用新 donor 重算——**每子步刷新时两个方向都算**（edgeThetaStart/Stop 按双向各存一套？**裁定：不**——同一子步内方向由当前 deviation 唯一确定（relaxAngle 扫描时读），刷新时算「双向中当前 donor 侧」不可行（刷新在扫描前）——**正确做法：刷新时存两套**（edgeThetaStartFwd/StopFwd 按 donor=c、edgeThetaStartRev/StopRev 按 donor=c+1），relaxAngle 按 deviation 符号选用；数组 4×127×8B=4KB，可忽略）。**s_e 供体反转的语义正确性**：设计稿「不能用整列均值盖住干表层」由 tTop 表层窗保证（深格不参与）。

### 4.2 C(s)/B(s) 映射实现

照设计稿 §1 的 c257 同拓扑分段（项目参数，非论文数值）：

```ts
// P011Moisture.ets 纯函数：
export function p011MoistureCohesion(s: number, breaks: [number,number,number]): number;
// 三段：[0,s1) 线性升 0→c1；[s1,s2) 线性 c1→c2（近恒定）；[s2,s3] 线性降至 0；>s3 为 0
export function p011MoistureBuoyancy(s: number, breaks: [number,number,number]): number;
// B(s)：单调非降（高湿浮力/润滑），分段线性候选（三段同址或独立断点）
```

### 4.3 每边 start/stop 缓存与 active 生命周期（状态机局部化改造——最大工程点）

**改造面：`p011RelaxHysteresisNextActive` 的调用从全局 pair 换 per-edge pair，状态数组不变**：

```text
现状（:4375）：decision = p011RelaxHysteresisNextActive(activeBefore, deviation,
                  hysteresis /* 全局 P011RelaxHysteresisThresholds */, thetaStart, thetaStop)
S3 后（moistureRoute ON）：
        decision = p011RelaxHysteresisNextActive(activeBefore, deviation,
                  edgePair /* 从 edgeThetaStart/Stop[Fwd|Rev] 现场构造的
                     P011RelaxHysteresisThresholds */, edgeThetaStart, edgeThetaStop)
```

**关键工程裁定**：
1. **不每子步 configure() 重置**（任务书红线）：`relaxSliding[127]` 的 active 状态跨子步持久（既有语义），局部 pair 只改**判定阈值**不改**状态所有权**——状态机的三态转换（start: r≥slopeStart；stop: r≤slopeStop；hold）用 per-edge pair 逐边执行，`p011RelaxHysteresisNextActive` 本身零改动（它已接收 thresholds 参数——只是传参从全局换局部）；**派生缓存 edgeTheta* 不缓存 active**（active 在 relaxSliding，阈值在 edgeTheta*，两者分离）。
2. **executedThresholdsForLedger 的局部化**：单 pair 结构 → per-edge 时 ledger 记 min/max/分布（`edgeThetaStartMin/Max` 等，模块六）；全局 configRevision/thresholdSource 照记（基准 pair 身份）+ thresholdCause 标湿度来源。
3. **WETNESS_THRESHOLD_CROSSED 事件**（设计稿 §4）：湿度刷新后若某边 pair 越过当前 r_e 且导致 active 翻转，记该事件进 ledger（与倾斜转折分开——`transitionReason` 既有四值冻结词汇表**不扩**，新增独立字段 `wetnessCrossed: boolean`，向后兼容）。
4. **与观测③标定隔离**：湿度 run 的 routeId 独立（`s3-moisture-profile-v1`），旧 34run 协议数据不追认（设计稿 §5.1 重标定协议归 tester 另卡）。

### 4.4 可行域证明（0≤stop<start≤30 与 deadzone）

**证明（构造性）**：`θstop,e = θstop_base + Δ_e`、`θstart,e = θstart_base + Δ_e`（同 Δ 平移——滞回宽度恒 6°）。可行域四条件：

1. `θstop,e ≥ 0`：需 `Δ_e ≥ −θstop_base`，即 `Δ_e ≥ −20°`；
2. `θstart,e ≤ 30`：需 `Δ_e ≤ 30 − θstart_base = +4°`；
3. `θstop,e > bottomCreepDeadZone`：需 `Δ_e > deadzone − θstop_base`（deadzone 现值 <4°，故 `Δ_e > −16°` 量级——被条件 1 覆盖）；
4. `θstop,e < θstart,e`：恒成立（6° 宽度）。

设计稿 §4 的 clip 域 `Δ_e ∈ [deadzone+ε−20°, +4°]` 正是条件 1-3 的交集——**clip 在派生层执行（refreshMoisture 的 e 步）**，clip 后四条件自动满足（数学上：clip 下界=max(条件1,条件3)上界=min(条件2)，clip 保证 Δ 落在交集内）。**拒绝语义**（设计稿「若 deadzone 使可行区间为空，拒绝配置」）：若用户基准 pair 改变（§27 用户配置）使 20° 换成用户 stop 值，clip 域按 `deadzone+ε−θstop_user` 重算——区间空（`θstop_user ≤ deadzone`）时 **configure 拒绝 moisture 武装**（fail-closed，零状态变化——照 §27.2 先例）。单测覆盖四条件各一例+区间空拒绝一例。

## 5. 模块五：五通道/安全/兼容层（必答 5）

### 5.1 relaxAngle 直读局部 pair 的接入点

:4375 一处（collect 扫描内的状态机调用）+ :4390 的 `p011RelaxHysteresisRequested`（requested 公式读 pair 的 slopeStop/maxFlux——per-edge pair 同样传入）。**两处签名不变**（函数已参数化），只换传参来源。

### 5.2 其余四通道的间接影响边界（登记，不改实现）

| 通道 | 影响 | 边界 |
|---|---|---|
| settleSandV2 | 无直接（沉降率不随阈值变——设计稿 §4） | 间接：湿度改变 relax 请求 → 预算竞争格局变 |
| percolateHeavyConservative | 无直接 | 同上 |
| creepBottom | 无直接（自身 deadzone/请求律不动） | 间接：relax 改变 S → creep 的目标形状残差对象变；共享预算竞争 |
| liftByShake | 无直接（shakeEnergy 是共享输入但 lift 用它驱动起沙，S3 用它驱动 μ_shake——**两个消费者读同一标量，互不干扰**） | 湿度不改变 heavyLiftRate |

### 5.3 guard/planner 接口不变性

- **§19.5 guard**：tan30 常数不动；湿度只改 relax 的请求量与资格（active 判定）——guard 拦截的是**材料写入**（S 变化），relax 的 grant 照常逐笔过 guard（局部 pair 改变的是「请求多少」不是「guard 判什么」）。单测：任意湿度参数下 guard 拦截行为与同请求量的 OFF run 逐笔一致。
- **§20 planner**：preview 的 relax 槽位镜像读 per-edge pair（§3.1）——可行域判定仍只由 tan30/动态可行域决定（θ_eff 不因湿度变——**湿度改变请求不改变 planner 的可行域计算**）；verify 一致性比对覆盖（deltaSandTop 逐列比较自动含局部 pair 的影响）。

### 5.4 兼容层实现（OFF 位精确 + ON/default 同轨迹）

**OFF 位精确**：`moistureRouteActive() = naturalObservationRouteId === 's3-moisture-profile-v1' && moistureParams !== null`（route 双条件门，S4/correction 同款先例）——OFF 时 refreshMoisture 零调用、relaxAngle 走全局 pair 旧路径、全部行为与 HEAD 逐位一致（**单测：锚点序列逐位比对，复用 D-1/U11 资产模式**）。

**ON/default 同轨迹**：ON 且 q=q_ref 且 μ_shake=0 时 Δ_e≡0（§3.3 的 s_ref 构造）——但**严格逐位一致需要时序也一致**：refreshMoisture 的额外计算不改变材料演化（纯读 Vw/Vs 写派生缓存）→ 材料轨迹只由 writer 决定 → writer 输入（请求量）在 Δ=0 时与全局 26/20 完全相同 → **轨迹等价成立**。单测：同 seed/初态/输入，ON(q=q_ref) vs OFF 的 sandTop 锚点序列逐位一致（这是兼容层的最强断言——比「参数字段相同」强得多，任务书明确要求）。

**旧锚点适用域缩窄登记**：26/20 冻结锚点判定只认 OFF 或 ON/default-reference（manager 裁决项，本设计登记不裁决）；三水量档效果是候选待冻结。

## 6. 模块六：验收/遥测（必答 6）

### 6.1 字段落点

| 字段 | 落点 | 类型/语义 |
|---|---|---|
| moistureModelVersion | ledger context.moisture.modelVersion = 's3-moisture-profile-v1' | 版本身份（route 隔离配套） |
| q / q_ref | ledger context.moisture.{q, qRef} | 全局水量驱动量+基准 |
| sRange | ledger context.moisture.{sMin, sMean, sMax}（占据格统计） | 剖面摘要 |
| edgeThresholdRange | ledger context.moisture.{edgeThetaStartMin/Max, edgeThetaStopMin/Max} | 局部 pair 分布 |
| thresholdCause | ledger context.moisture.thresholdCause: 'moisture' \| 'reference' | 每边 pair 的来源（Δ=0 全边时 reference；任一边 Δ≠0 时 moisture——子步级聚合标记） |
| 水事务回执 | ledger context.moisture.waterTransactions: Array<{op:'add'\|'drain', requested, admitted, reason, revision}> | §2 事务逐笔 |

### 6.2 三水量档分组验证设计（离线原型执行，真机档 effector）

干/中湿/高湿三档（档位数值采前冻结——离线用 q∈{q_dry, q_mid, q_ref, q_wet} 四点含基准）；每档：①剖面统计（s_bottom−s_top、高度相关系数）；②每边 θ 分布+真实 start/stop 转换残差；③±15° 稳定段/±45° 渐进段/翻转/长期停驻的响应曲线；④shake OFF/ON 臂。**帧时门槛**：OFF vs ON 配对块（K=8/K=16 两臂）沿现行测试流程（§7.3 配对块+bootstrap）——真机归 effector，离线先出 Node 计时。

## 7. 五接口预留（只登记类型与版本边界）

1. **(column,heightBin) 寻址+只读约定**：`moistureField[c*K+k]` 平铺寻址；`saturationAt(c, k): number` 与 `edgeMobileSaturation(e): number` 只读方法（消费者：阈值派生+将来渲染）——高级版换局部通量更新时**地址与签名不变**。
2. **P_pore/W_pore 账本插槽**：`P011MoistureParams` 预留可选字段位（首版不定义类型细节——版本边界：`s3-local-seepage-v2` 的 schema 独立设计，首版只保证 s[c,k] 地址可被 v2 直接接管）。
3. **s→真实孔隙水迁移事务签名**：`migrateMoistureToPore(): P011PoreMigrationReceipt | null`——从自由水账扣 ΣW_pore，不足拒绝或缩放回执（照 §2 事务语义）；首版只登记签名不实现。
4. **材料 writer 携水接口**：writer 改占据格时的孔隙水携带/挤出钩子签名（`onOccupancyChange(c, kFrom, kTo, carriedWater)` 候选形态）+悬沙携水独立裁决点——v2 设计域，首版登记。
5. **标定协议新版本入口**：`thresholdSource` 扩枚举 `'moisture-candidate'`（v2 标定时用）；旧观测③数据不追认（routeId 隔离天然保证）。

## 8. 三维候选比较（必答附加，单独一节）

### 8.1 分层高度场 S[x]+W[x,k] 方案分析（草稿要求先行给出）

| 维度 | S[x]+W[x,k]（分层高度场） | 现状 1D s[c,k] 首版 | S[x,y]+W[x,y,k]（二维底座） |
|---|---|---|---|
| 水量守恒 | `Vw=Vfree+ΣW_pore`，格间同量异号通量——**账本可审计**（照 §28 分域核算先例） | s 是代理无水账（红线） | 同左+横向 y 通量 |
| 竖向通量 | 重力排水 J_down+毛细 J_cap（k↔k±1） | 无（代数刷新） | 同左 |
| 横向通量 | 列间 J_lat（c↔c±1） | 无 | y 向真实库存 |
| 每列层数 | K=8/16（与首版同址） | K | 同 |
| 帧时预估 | 通量更新 O(128×K×3 邻域)≈K=16 时 0.1ms/子步量级（Node 文档级） | 0.04ms | ×Ny（若 Ny=8：0.8ms——逼近预算） |
| 迁移路径 | **首版 s[c,k] 地址直接接管**（§7-1 接口） | —（起点） | 需 2D 底座先行（Ny=1 兼容域） |

### 8.2 分层高度场无法表示的冻结场景（明确清单）

- 前后向（y）真实水流/湿润前沿——**无 y 库存**（设计稿 §7 明示：须 S[x,y] 底座）；
- 一列多个不连通沙段/悬垂/洞穴——单值高度图极限（需局部固体体素，设计稿 §8「何时升级」条件）。

### 8.3 体素条件审核清单的处置（草稿四点）

按草稿「条件审核清单非实施授权」处理：**本卡不实施任何 3D**；四点（1D→2D 过渡/砖块唯一所有权/固体首批适用域/触发关闭+四级验证门）登记为**manager 若选体素路线时的 architect 另卡输入**——设计稿 §7.1/7.2 已给条件设计，本工程稿不重复展开，仅在体素立项时按四点逐项出独立工程方案。**比较结论（供 manager 排后续）**：分层高度场（S[x]+W[x,k]）是 2D 底座前唯一不破账本的升级路径，且与首版接口连续（同址接管）；局部体素仅在分层高度场出现可测缺陷（湿润前沿位置/速度持续失配或多值几何需求）后再议——与 designer §8 推荐一致。**建议排序**：首版 1D（本卡）→ s3-local-seepage-v2（1D 真实孔隙水+守恒通量）→ 视效果证据决定 S[x,y] 底座 → 局部体素仅条件立项。

## 9. developer 任务书（工作五）

### 9.1 实现拆解

| # | 模块 | 内容 | 依赖 |
|---|---|---|---|
| M-1 | P011Moisture.ets | 参数/版本常量+s_eq/C/B 纯函数+clip 可行域函数 | 无 |
| M-2 | 湿度场状态 | moistureField/Occupancy/edgeTheta×4 数组+快照扩展（schemaVersion 3）+run/reset/模式切换生命周期 | M-1 |
| M-3 | 刷新时序 | refreshMoisture()（§3.1 六步）+moistureRevision+双向 s_e 计算 | M-2 |
| M-4 | 状态机局部化 | :4375/:4390 传参换 per-edge pair（双向选路）+executedThresholds 局部化+relaxSliding 不重置验证 | M-3 |
| M-5 | 水量事务 | addWater/drainWater+回执+waterTransactionNet 基准+事务后刷新触发 | M-3 |
| M-6 | route 兼容层 | 双条件门+want 参数 p011s3（`moisture:<k>:<q_ref>` 三段式+水事务 want 独立参数 p011water） | M-2 |
| M-7 | 遥测 | ledger schema v5（moisture 块六字段+水事务回执）+wetnessCrossed 事件 | M-4/M-5 |
| M-8 | shadow 镜像 | relax 槽位镜像读 per-edge pair 冻结数组+verify 覆盖 | M-4 |
| M-9 | 单测+定档 | §9.2 全部+K=8/16 计时臂 | 全部 |

### 9.2 单测范围（Node 通道，断言级）

1. **OFF 逐步位精确**：moistureRoute OFF（默认）任意场景 600s → 锚点序列与 HEAD 逐位一致（D-1 资产模式）。
2. **ON/default 同轨迹**：ON+q=q_ref+μ_shake=0 → sandTop 锚点序列与 OFF 逐位一致（兼容层最强断言）。
3. **占据掩码边界**：无沙列（S=0）无有效格；满沙列（S=H）全部格有效；沙顶跨格（S 落在格边界内）部分占据 m∈(0,1)——三构型无 NaN、无旧格幽灵值（沙顶降后升，旧格值不复用——刷新覆盖）。
4. **水量事务拒绝零影响**：CAPACITY_FULL/INSUFFICIENT_WATER 拒绝 → waterTop/s/edgeTheta/全部状态逐位不变。
5. **部分准入**：加水余量不足 → admitted=余量；waterTransactionNet 累计正确；expectedWater 守恒闭合。
6. **局部 pair 可行域**：四条件（θstop≥0/θstart≤30/θstop>deadzone/stop<start）各构造边界例+clip 域正确性+deadzone 区间空拒绝（fail-closed 零状态变化）。
7. **q 单调性**：加水后任意有效格 s 非降（设计稿 §3「加水反使某层变干」违反检查——s_base+β 剖面族的单调性断言）。
8. **shake 不改水量**：μ_shake>0 全程 Vw 逐位不变；停止后剖面按 shakeDecay 恢复分层。
9. **状态机生命周期**：局部 pair 下 relaxSliding 跨子步持久（不因 configure/刷新重置）；湿度阈值越过 r_e 时 wetnessCrossed=true 记录。
10. **guard 不变性**：任意湿度参数+同请求量注入 → guard 拦截行为与 OFF 逐笔一致。
11. **shadow 一致**：ON run 的 preview/apply plans 逐位一致（verifyPreviewCommitConsistency 全程 PASS）。
12. **性能**：K=8/K=16 每 substep refreshMoisture+per-edge 派生开销 Node 计时（<0.02/<0.04ms 量级确认；真机配对块归 effector）。
13. **WETNESS 与倾斜事件隔离**：湿度翻转事件不混入 transitionReason 旧四值（新字段独立）。
14. **schema 兼容**：v5 ledger 新块缺失时旧 run 读取不报错；OFF run 无 moisture 块。

### 9.3 离线验收口径

1. 三水量档（q_dry/q_mid/q_ref/q_wet 四点）×剖面/角度分布/响应曲线报告自动产出（§6.2）。
2. OFF 位精确+ON/default 同轨迹两条等价断言全绿（验收门槛非候选）。
3. 局部 pair 可行域证明的测试覆盖完整（§4.4 四条件+空区间拒绝）。
4. K 档计时数据落盘（真机定档输入）。
5. ledger v5 样例 run 产物供 checker 兼容性烟测。

## 10. 红线复述

不改冻结锚点（26/20 锚点适用域缩窄为 OFF/ON-default 是登记非修改；tan30/300/s/guard/预算零触碰——湿度只改请求）；首版不实现渗透求解器（v2 独立设计）；**s 不是第二份水**（不加账/不入守恒/不扣自由水）；108run 工作区 commit 习惯保持（合入等 manager 确认）；对称性红线（均匀水量变化不假造局部湿斑——首版 s 只随全局 q 变，天然满足）；观测③旧数据不追认（routeId 隔离）。

## 11. 结论与路由

六模块必答全部落盘：湿度场（128×K Float32+快照 v3+K 档裁定默认 8）/水量事务（独立 addWater/drainWater+五硬规则+基准不重置）/刷新时序（子步首六步链+revision 冻结+shadow 读冻结数组）/局部双角（双向 s_e+tTop 表层窗+状态机传参局部化零改造+可行域构造性证明+clip 派生层）/安全兼容（四通道间接影响登记+guard/planner 不变性论证+OFF 位精确+ON/default 同轨迹双等价断言）/遥测（schema v5 六字段+水事务回执+wetnessCrossed）。五接口预留只登记边界。三维比较：分层高度场为唯一不破账本升级路径、体素条件立项（与 designer §8 一致）。**developer 任务 M-1..M-9 + 单测 14 项 + 离线验收五条。真机定档（K 档+帧时配对块）归 effector；效果三水量档验收候选待冻结归 tester/manager。**

变更记录：2026-10-23，依据 manager【P-011·S3工程设计】任务产出。六模块+五接口+三维比较+任务书全量落盘；关键裁定：状态机传参局部化（零函数改造）、双向 pair 缓存、水事务基准不重置（§27 同款）、K=8 默认、shadow 读冻结数组不镜像 s。
