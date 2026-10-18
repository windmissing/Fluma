# P-011 工程方案：bottom-creep-v2.7.3

> 日期：2026-10-18（v2.7.3 设计修订）  
> 问题：P-011 倾斜跟手性 / 底层运动响应  
> 技术路线：`bottom-creep-v2.7.3`  
> 状态：DESIGN_UPDATED_PENDING_RETEST  
> 依据：`docs/algorithm-P011.md` §3、`docs/test-cases.md` P-011、`docs/effect-cases.md` P-011  
> 历史版本：`bottom-creep-v2.6` 为 `REJECTED`（2026-10-15），不得继续实施、调参复用或作为验收基线

## 1. 目标与非目标

### 1.1 目标

1. 修复底层蠕变方向语义：方向只由 `effectiveTilt` 符号决定，不允许被列间高度差反转。
2. 新增独立的底层蠕变运动预算 `vBottomCreepMax = 10` 列/秒，避免与沉降、渗透、滑移、扬起互相挤占。
3. 保留所有运动共享的 `totalMotionBudget` 硬上限；bottom creep 可以被总上限二次截断，但不得突破。
4. 输出可归因的双预算观测指标，区分“独立预算不足”与“总运动硬上限截断”。
5. 降低“输入倾角变化 → 首个底层有效位移”的感知延迟，目标 `P95 T90 <= 100ms`。

### 1.2 非目标

1. 不实现整层重力对流；该能力属于 `gravity-advection-v1` 独立路线。
2. 不恢复或调参复用 `bottom-creep-v2.6`。
3. 不改变场景一液体模式、三相守恒校验与既有容量约束。
4. 不放宽 P-011 的硬门槛或分档阈值。
5. 不与其他技术路线合并数据、报告或验收结论。

## 2. 状态量与参数

### 2.1 既有状态

| 状态 | 含义 | 本方案处理 |
|---|---|---|
| `bh[]` | 底层重沙高度 | 作为 donor/receiver 迁移的权威状态 |
| `bl[]` | 底层轻沙高度 | 作为 donor/receiver 迁移的权威状态 |
| `mh[] / ml[]` | 混合层重/轻沙 | 不直接参与底层蠕变取样 |
| `suspH[] / suspL[]` | 悬浮重/轻沙 | 不参与底层蠕变取样 |
| `waterTop[]` | 水面高度 | 用于接收列容量约束 |
| `sandTopScratch[]` | 沙顶缓存 | 继续用于容量判断与运动预算统计 |

### 2.2 新增或调整参数

| 参数 | 建议默认值 | 说明 |
|---|---:|---|
| `routeId` | `bottom-creep-v2.7.3` | v2.7.3 运行时、测试钩子与报告必须输出；不与 v2.7/v2.7.1/v2.7.2 日志合并 |
| `bottomCreepRate` | `96` | 基础底层蠕变速率，单位：列/秒 |
| `bottomCreepVelocityGain` | `24~48` | 由有效倾角变化率额外驱动的蠕变增益 |
| `tiltResponseRate` | `720°/s` | 输入目标倾角到有效倾角的最大响应速率 |
| `inputSmoothingAlpha` | `0.6` | 传感器新样本权重；旧样本权重为 `1-alpha` |
| `bottomCreepDeadZone` | `1°` | 小于该角度不产生新的 donor/receiver 迁移 |
| `directionHoldTime` | `80ms` | 方向保持窗口，抑制快速反向时的抖动 |
| `vBottomCreepMax` | `10` | bottom creep 独立预算上限，单位：列/秒；首轮固定值 |
| `totalMotionBudget` | 沿用现有配置 | 所有运动共享的硬上限，单位：列/秒 |
| `kTarget` | `0.5` | v2.7.2 目标形状整形系数，单位：列/(°·秒)；20° 时对应 `vBottomCreepMax=10` |
| `tauH` | `100ms` | heavy 通道累计形状残差补偿时间常数；固定基线，不作为复测调参项 |
| `tauL` | `60ms` | light 通道累计形状残差补偿时间常数；固定基线，不作为复测调参项 |

> `vBottomCreepMax = 10` 是 v2.7 首轮基准配置；参数敏感性验证只允许在算法方案声明的 `8/10/12` 三档内做有限消融，不得改变基准报告口径。

## 3. 模块与接口设计

### 3.1 模块划分

| 模块/文件 | 职责 |
|---|---|
| `WaterSandField` | 计算 `effectiveTilt`、执行 v2.7 方向映射、申请独立预算、汇总总预算并更新底层状态 |
| `SandCanvas` | 接收传感器或测试注入倾角，记录命令时间戳并触发物理更新 |
| `P011TiltWant`（新增，必需模块） | 统一解析、校验与解码 `testmode=true && rendermode=4` 下的 Want 倾角注入命令；禁止在 `EntryAbility` / `SandCanvas` 内联复制该逻辑 |
| `P011Telemetry`（或现有日志辅助模块） | 仅在 `rendermode=4 && testmode=true` 时输出 `P011_*` 钩子；负责 `runId`、reset 边界、`sequence/frameId/substepId` 的生成与单调性 |
| `test/p011_check.py` | 解析日志、计算方向与预算指标、生成 v2.7 独立报告 |

### 3.2 核心接口建议

```ts
interface BottomCreepRequest {
  effectiveTilt: number;
  effectiveTiltRate: number;
  substepDt: number;
}

interface BottomCreepBudget {
  requestedAmount: number;
  independentGrant: number;
  independentClipped: number;
  effectiveAmountAfterTotalClip: number;
}

interface TotalMotionBudget {
  requestedAmount: number;
  grantedAmount: number;
  clippedAmount: number;
}
```

`WaterSandField` 内部建议暴露统一入口：

```ts
applyBottomCreepV27(request: BottomCreepRequest): {
  bottomCreep: BottomCreepBudget;
  totalMotion: TotalMotionBudget;
  directionCorrect: boolean;
}
```

实现方可根据现有类结构合并参数或拆分私有方法，但必须保持以下语义不变：

1. donor/receiver 只由 `effectiveTilt` 符号决定。
2. `vBottomCreepMax` 只约束 bottom creep，不与其他运动共享。
3. `totalMotionBudget` 统一约束所有运动。
4. 指标在总上限二次截断之后统计真实有效位移。

## 4. 底层蠕变 v2.7 计算流程

### 4.1 输入与有效倾角

1. `SandCanvas` 接收传感器或测试注入的目标倾角 `targetTilt`。
2. 传感器路径使用指数平滑：

```text
sensorG = (1 - inputSmoothingAlpha) * sensorG_old
        + inputSmoothingAlpha * sensorG_new
```

3. 计算目标倾角后，按最大角速度 `tiltResponseRate` 逼近：

```text
effectiveTilt(t+dt) =
    effectiveTilt(t)
    + clamp(targetTilt - effectiveTilt(t),
            -tiltResponseRate * dt,
             tiltResponseRate * dt)
```

4. 记录 `effectiveTiltRate = (effectiveTilt(t) - effectiveTilt(t-dt)) / dt`。

5. `SandCanvas` 在进入视觉倒立判定与 `WaterSandField` 消费前，必须把原始倾角规范为带符号角度：

```text
normalized = ((tiltX % 360) + 360) % 360
signedTilt = normalized >= 180 ? normalized - 360 : normalized
```

`signedTilt` 的合法区间为 `[-180, 180)`。视觉倒立判定使用 `abs(signedTilt) > 90`；倒立状态下的模型注入角为 `signedTilt - sign(signedTilt) * 180`，再统一 clamp 到 `[-89, 89]`。该口径只修正角度表示，不改变 `invert180()` 的视觉语义、`routeId`、双预算或第 4.2 节 donor/receiver 方向规则。由于 `wstilt` 在解析阶段已限定为 `[-89, 89]`，其 `signedTilt` 必须等于输入值，不得被判定为倒立，也不得改变符号。

### 4.2 donor/receiver 方向规则

对每个相邻列对 `(c, c+1)`，`c` 为左列，`c+1` 为右列：

1. 若 `effectiveTilt > 0`：左列 `c` 固定为 donor，右列 `c+1` 固定为 receiver，质量/位移从左向右迁移。
2. 若 `effectiveTilt < 0`：右列 `c+1` 固定为 donor，左列 `c` 固定为 receiver，质量/位移从右向左迁移。
3. 若 `effectiveTilt == 0`、处于死区或输入无效，不产生新的 donor/receiver 迁移。
4. donor/receiver 判定不得由列间当前高度差反转；高度差只允许影响迁移量的限幅或缓释。

### 4.3 底层蠕变申请

1. 计算强度因子：

```text
tiltFactor = min(1, |effectiveTilt| / 20°)
velocityFactor = min(1, |effectiveTiltRate| / 180°/s)
creepRate = bottomCreepRate
          + bottomCreepVelocityGain * velocityFactor
requested = creepRate * dt * tiltFactor
```

2. `requested` 继续受 donor 底层 `bh+bl` 余量、receiver 可用容量与 `bottomCreepMaxSubstep` 约束。
3. 从 donor 底层按 `bh:bl` 比例取出：

```text
qH = q * bh[donor] / (bh[donor] + bl[donor])
qL = q * bl[donor] / (bh[donor] + bl[donor])
```

4. 写入 receiver：

```text
bh[receiver] += qH
bl[receiver] += qL
```

### 4.4 独立预算与总硬上限

每个物理子步按以下顺序处理：

```text
bottomLimit = vBottomCreepMax * substepDt
bottomRequested = sum(abs(requested bottom creep column motion))
bottomGrant = min(bottomRequested, bottomLimit)
bottomIndependentClip = bottomRequested - bottomGrant
bottomGrant = distributeScaled(bottomRequests, bottomGrant)

otherRequested =
    settlementRequested
  + percolationRequested
  + slipRequested
  + riseRequested

totalLimit = totalMotionBudget * substepDt
totalRequested = bottomGrant + otherRequested
totalGrant = min(totalRequested, totalLimit)
totalClip = totalRequested - totalGrant
grants = distributeScaled(allRequests, totalGrant)
```

要求：

1. `vBottomCreepMax` 不复用 `vSandTopMax` 共享预算，也不被沉降、渗透、滑移、扬起消耗或借用预算。
2. 若总预算耗尽，bottom creep 必须与其他运动一起按申请比例归一化二次截断。
3. `bottomCreepBudgetUsed` 必须统计总上限二次截断后的实际有效位移。
4. `bottomCreepBudgetClipped` 只统计独立子通道截断，不包含总上限造成的二次截断。
5. 总上限造成的 bottom creep 二次截断量如需调试，另行输出到调试明细；正式报告必须写明统计口径。
6. 预算统计必须复用现有列更新循环累计，不得为统计新增二次全列扫描。
7. 所有通道请求必须在同一物理子步开始时的状态快照上生成，形成唯一的 `allRequests` 请求集合。
8. `totalMotionBudgetRequested` 必须按 `bottomGrant + otherRequested` 统计；不得仅统计其他运动请求。
9. 总预算耗尽时，必须对同一 `allRequests` 集合统一计算缩放比例并分配授权；禁止用状态变化后的重放结果替代原始请求集合。
10. 收集阶段允许把 `avalanchePasses` 的多轮滑移折叠为单列遍历请求生成，这是为了满足第 7 条的同一子步开始状态快照语义，避免后续轮次基于中间态生成新的总预算请求。

### 4.5 方向保持与防抖

1. 若新方向与上一有效方向相反，且间隔小于 `directionHoldTime`，保持旧方向。
2. 若输入回到 `0°` 并保持超过 `directionHoldTime`，允许方向清零。
3. 方向保持不得改变第 4.2 节的符号映射；它只影响切换时机。

## 5. 数据流、线程与时序

### 5.1 数据流

```text
sensor / script input
    -> SandCanvas: commandTime / acceptedTime / targetTilt
    -> WaterSandField: effectiveTilt / effectiveTiltRate
    -> bottom creep direction mapping
    -> independent budget clip
    -> total motion budget clip
    -> apply bh / bl and update sandTopScratch
    -> P011_EVENT input / P011_METRIC / P011_BOTTOM
    -> test/p011_check.py
    -> docs/test-report-P011-bottom-creep-v2.7.md
```

脚本注入链路固定为：

```text
hdc shell aa start --ps wstilt <deg>
    -> EntryAbility.onCreate / onNewWant: P011TiltWant.parse + commandTime
    -> AppStorage[p011TiltWantCommand / p011TiltWantCommandSeq]
    -> SandCanvas frame callback: consume command + acceptedTime
    -> existing targetTilt -> effectiveTilt pipeline
    -> first physics step after acceptance: physicsTime
    -> P011_EVENT input(source="script")
```

### 5.2 HarmonyOS 集成点

1. `SandCanvas` 保持现有 UIAbility / 页面绘制链路，只新增时间戳记录与事件透传。
2. `WaterSandField` 继续在现有物理更新任务中执行，不新增独立物理线程。
3. 钩子输出必须与底层状态更新同一线程或通过已有的单线程消息队列串行化，避免日志样本与状态错位。
4. UI 线程不执行全列物理计算与预算统计；若现有架构直接在 UI 线程同步执行物理更新，developer 需沿用当前任务的异步拆分策略，本方案不新增阻塞点。
5. Want 解析与 `AppStorage` 单槽写入在 Ability 主线程完成，仅做 O(1) 数值校验；`SandCanvas` 帧回调消费该单槽，不新增线程、锁或消息队列。
6. 物理更新仍走 `WaterSandField` 现有执行位置；脚本注入只替换目标倾角来源，不得直接写 `bh/bl`、预算状态或跳过 `effectiveTilt` 计算。

### 5.3 单步时序

1. 记录输入事件与时间戳。
2. 计算平滑后的 `effectiveTilt` 与 `effectiveTiltRate`。
3. 判断死区与方向保持。
4. 生成 bottom creep 申请，并按独立预算截断。
5. 汇总所有运动申请，并按总硬上限统一截断。
6. 写入 `bh/bl`，更新缓存。
7. 输出 `P011_METRIC` 与 `P011_BOTTOM`。
8. 若本帧消费了新的脚本倾角命令，在该命令后的第一个物理子步完成后输出 `P011_EVENT input`。

第 7 步的观测 `dt` 必须取 `WaterSandField.step()` 当前实际执行的 `substepDt`，或由该值写入的显式 `P011_METRIC.physicsDt` 字段；不得用渲染帧间隔、hilog 行时间戳或 `physicsTime` 差值反推物理 `dt`。

## 6. P-011 测试钩子设计

### 6.1 `P011_EVENT input` / `P011_EVENT reset`

每次目标倾角命令输出：

```json
{
  "routeId": "bottom-creep-v2.7",
  "runId": "p011-20261016-0001",
  "sequence": 128,
  "commandTime": 1234567890123,
  "acceptedTime": 1234567890150,
  "physicsTime": 1234567890167,
  "targetTilt": 25.0,
  "source": "script | sensor | gesture"
}
```

`P011_EVENT input` 的输出条件是脚本命令被 `SandCanvas` 消费且 `acceptedTime` 已确定，而不是产生状态变化。目标角相同、处于死区、`bottomCreepBudgetUsed=0` 或 dormant no-op 都必须输出该事件；不得用 `bh/bl` 是否变化作为是否输出事件的依据。

每次 P-011 运行开始或场景状态被重置时，必须先输出一条 reset 边界标记，再输出后续物理样本：

```json
{
  "routeId": "bottom-creep-v2.7",
  "runId": "p011-20261016-0001",
  "sequence": 0,
  "reason": "startup",
  "physicsTime": 1234567890123
}
```

`reason` 只允许 `startup`、`field-reset`、`scene-reset`、`preset-change` 四个值。`runId` 在同一 `WaterSandField` 生命周期内保持不变；`aa start` 造成的进程重启、`WaterSandField` 重新构造、`bh/bl` 数组重分配、`wspreset/wscols/wscapacity` 等场景参数变化都必须生成新的 `runId`，并输出 `P011_EVENT reset`。`sequence` 是同一 `runId` 内的严格递增整数；`P011_METRIC` 与 `P011_BOTTOM` 成对输出时共享同一个 `sequence`，input 与 reset 事件各自占用一个 `sequence`。checker 必须把 `sequence` 回退、重复或跨 reset 缺口判为数据完整性失败。

### 6.2 `P011_METRIC`

每个物理子步输出。预算字段必须使用同一时间窗口与同一样本集合，禁止混合帧与秒级口径：

```json
{
  "routeId": "bottom-creep-v2.7",
  "runId": "p011-20261016-0001",
  "sequence": 129,
  "frameId": 43,
  "substepId": 86,
  "physicsTime": 1234567890167,
  "physicsDt": 0.008333,
  "targetTilt": 25.0,
  "effectiveTilt": 24.8,
  "effectiveTiltRate": 180.0,
  "bottomCreepRate": 120.0,
  "bottomFlux": 1.8,
  "bottomCreepBudgetUsed": 1.6,
  "bottomCreepBudgetLimit": 1.6667,
  "bottomCreepBudgetClipped": 0.1,
  "totalMotionBudgetUsed": 4.8,
  "totalMotionBudgetLimit": 5.0,
  "totalMotionBudgetClipped": 0.3,
  "directionCorrect": true,
  "waterTotal": 4864,
  "heavyTotal": 9216,
  "lightTotal": 9216
}
```

字段口径：

| 字段 | 含义 | 单位 |
|---|---|---|
| `runId` | 当前 `WaterSandField` 运行标识；跨 run 禁止差分 | - |
| `sequence` | 同 run 内遥测样本单调序号；metric/bottom 成对共享 | - |
| `frameId` | 同 run 内渲染帧单调序号；同帧多个子步共享该值 | - |
| `substepId` | 同 run 内逻辑物理子步单调序号；dormant no-op 也必须占用 | - |
| `physicsDt` | 产生本样本的逻辑物理子步时长；dormant no-op 输出 `0` | 秒 |
| `bottomCreepBudgetUsed` | 独立预算内、且经总上限二次截断后的 bottom creep 实际有效位移 | 列 |
| `bottomCreepBudgetLimit` | `vBottomCreepMax * substepDt` | 列 |
| `bottomCreepBudgetClipped` | 仅因独立预算上限被截断的申请量 | 列 |
| `totalMotionBudgetUsed` | 所有运动经总上限截断后的实际用量 | 列 |
| `totalMotionBudgetLimit` | `totalMotionBudget * substepDt` | 列 |
| `totalMotionBudgetClipped` | 因总硬上限被截断的所有运动申请量 | 列 |

`P011_METRIC` 与 `P011_BOTTOM` 必须由同一个物理子步回调一次生成，并共享同一 `runId`、`sequence`、`frameId`、`substepId` 与 `physicsTime`；测试解析器必须校验两者样本数、`physicsTime` 集合和四元身份集合一一对应，缺失、重复或错位均判为数据完整性失败。`maxUncommandedTiltStep` 只允许使用当前样本的 `physicsDt`；dormant no-op 的 `physicsDt=0` 不参与该残差计算。

### 6.3 `P011_BOTTOM`

测试模式下必须逐物理子步输出，确保 `maxSubstepBh/Bl` 能由相邻子步差分直接计算；该输出仅在 `testmode=true` 时启用：

```json
{
  "routeId": "bottom-creep-v2.7",
  "runId": "p011-20261016-0001",
  "sequence": 129,
  "frameId": 43,
  "substepId": 86,
  "physicsTime": 1234567890167,
  "bh": [72, 72, 71.8, 71.6],
  "bl": [72, 72, 72.1, 72.2],
  "bottomCreepDonorEffectiveDisplacement": [-0.2, -0.1, 0, 0],
  "bottomCreepReceiverEffectiveDisplacement": [0, 0.2, 0.1, 0]
}
```

`bottomCreepDonorEffectiveDisplacement[c]` 与 `bottomCreepReceiverEffectiveDisplacement[c]` 只统计本子步 bottom creep 在该列上造成的有效位移，单位为重沙加轻沙的等效高度，且必须经独立预算与总上限二次截断后统计；其他运动不得计入。两个字段是**带符号的全局左右位移**，不是正数幅值：

```text
directionSign = +1    // left-to-right
directionSign = -1    // right-to-left

bottomCreepDonorEffectiveDisplacement[c]   += -directionSign * amount
bottomCreepReceiverEffectiveDisplacement[c] += directionSign * amount
```

因此左到右迁移时 donor 为负、receiver 为正；右到左迁移时 donor 为正、receiver 为负。若同一列同时作为相邻列对的 donor 与 receiver，两个字段分别累计对应方向的带符号位移，禁止用 `bh[c]` 或 `bl[c]` 的总净变化替代，也禁止先累计正幅值再在 checker 侧补符号。

> 正式测试脚本必须以 `P011_BOTTOM` 为权威数据源；渲染像素只做交叉验证。

### 6.4 兼容性

1. 保留 `WS_EVENT tilt`、`WS_METRIC`、`WS_SNAPSHOT`。
2. `P011_*` 仅在 `rendermode=4 && testmode=true` 时输出。
3. 不修改既有日志字段名与语义。

### 6.5 测试专用 Want 倾角注入

#### 6.5.1 Want 参数与接口

唯一用户可见参数名为 `wstilt`，取值为十进制角度字符串，单位为度，合法范围 `[-89, 89]`。不新增 `wstiltseq` 等脚本侧参数；命令序号由应用内部生成。

新增 `entry/src/main/ets/physics/P011TiltWant.ets`：

```ts
export type P011TiltWantWarning =
  | 'INVALID_WSTILT'
  | 'P011_RENDER_MODE_REQUIRED'
  | 'P011_TEST_MODE_REQUIRED'
  | 'P011_FORCE_INVERT_CONFLICT';

export interface P011TiltWantCommand {
  sequence: number;
  targetTilt: number;
  commandTime: number;
}

export interface P011TiltWantParseResult {
  command: P011TiltWantCommand | null;
  warnings: P011TiltWantWarning[];
}

export function parseP011TiltWant(
  parameters: Record<string, Object> | undefined,
  renderMode: number,
  testMode: boolean,
  forceInvert: boolean,
  sequence: number,
  commandTime: number
): P011TiltWantParseResult;

export function decodeP011TiltWant(
  encoded: string | undefined,
  sequence: number,
  consumedSequence: number
): P011TiltWantCommand | null;
```

`parseP011TiltWant` 只在 `EntryAbility.onCreate/onNewWant` 使用；`decodeP011TiltWant` 只在 `SandCanvas` 消费单槽命令时使用。`P011TiltWantCommand`、解析规则、警告枚举与 JSON 编解码必须唯一归属 `P011TiltWant.ets`，`EntryAbility` 与 `SandCanvas` 不得各自声明同名接口或复制 `JSON.parse` / 数值校验逻辑。

`EntryAbility` 保留内部计数器 `p011TiltCommandSequence`。每次收到有效 Want 边沿时先递增该计数器，再把新序号与 `nextP011TimeMs()` 传入解析器；解析成功后写入两个 `AppStorage` 键：

```text
p011TiltWantCommand    = JSON.stringify(P011TiltWantCommand)
p011TiltWantCommandSeq = P011TiltWantCommand.sequence
```

`SandCanvas` 保存已消费的本地序号，只在全局序号大于本地序号且 `waterSandField !== null` 时读取 JSON 并消费命令。该单槽为 last-writer-wins：若两条 Want 命令在下一帧前重叠，后一条覆盖前一条，前一条不输出 `P011_EVENT input`。正式脚本必须等待至少一个显示帧（建议 `250ms`）再发送下一条，避免丢步。连续目标角测试不得用紧密 `aa start` 循环喷射整数目标角；测试驱动必须按时间表逐显示帧更新连续目标角，物理子步继续通过既有限速和平滑链路跟踪该目标，且相邻 Want 间隔不得小于一个显示帧，禁止因 last-writer-wins 把连续运动退化成 `1/2°` 脉冲。

#### 6.5.2 边沿与物理语义

1. `onCreate` 与 `onNewWant` 都构成一次命令边沿；初始 Want 允许携带 `wstilt`，即使 `SandCanvas` 尚未完成初始化，命令也停留在单槽中，待字段创建后由首个帧回调消费。
2. 每个被解析成功的 `wstilt` 命令只消费一次，即使目标角度与当前目标相同，也输出一条 `P011_EVENT input`；同角度命令不产生新的物理位移，只用于测试链路确认。
3. 脚本命令只设置目标倾角，后续仍经过既有限速、平滑、`effectiveTilt`、死区、方向保持、双预算与总预算链路；不得直接修改 `WaterSandField` 状态或绕过物理约束。
4. 在 `testmode=true` 下脚本命令优先于手势保持状态；`source` 必须输出 `script`。真实传感器路径仍只在 `testmode=false` 时生效。
5. `wstilt` 与 `forceinvert=true` 互斥；同时出现时拒绝 `wstilt` 并记录 `P011_FORCE_INVERT_CONFLICT`，避免两个测试输入源竞争同一主轴。
6. `wstilt` 消费必须复用第 4.1 节的带符号角度规范。以 `wstilt=-88` 为例：`normalized=272`、`signedTilt=-88`、`visualInverted=false`、`effectiveTilt=-88`；`P011_METRIC.effectiveTilt` 与 `WS_EVENT tilt.effectiveDeg` 都必须保持 `-88`，禁止 clamp 为 `89`。

#### 6.5.3 时间戳口径

| 字段 | 产生位置 | 口径 |
|---|---|---|
| `commandTime` | `EntryAbility.onCreate/onNewWant` | 有效 Want 边沿被 Ability 接收的时刻；统一调用 `nextP011TimeMs()`，保证单调 |
| `acceptedTime` | `SandCanvas` 帧回调 | 单槽命令被读取且水沙字段已创建、目标倾角被采纳的时刻；同样调用 `nextP011TimeMs()` |
| `physicsTime` | `WaterSandField` 第一个后续物理子步 | 该子步指标中的 `physicsTime`，不是渲染帧时间 |
| `targetTilt` | 解析后的 Want 值 | 命令目标角度；`P011_METRIC.effectiveTilt` 可因限速和平滑滞后 |

`P011_EVENT input` 在 accepted 命令后的第一个物理子步完成后输出，保证三个时间戳均已确定。时间戳必须满足 `commandTime <= acceptedTime <= physicsTime`。

#### 6.5.4 权限与安全约束

1. 不新增 `module.json5` 权限；`wstilt` 由 hdc/`aa start` 显式传入，不需要传感器、存储或网络权限。
2. 仅 `rendermode=4 && testmode=true && forceinvert=false` 时解析 `wstilt`；其他模式、生产传感器路径或倒立注入路径一律忽略。
3. `wstilt` 是不可信外部输入：只允许有限数值与 `[-89, 89]` 范围，不得拼接日志格式、路径、URL 或动态表达式。
4. 单槽 last-writer-wins 与固定 JSON 结构限制内存占用；未消费命令最多一条，不建立无界队列。
5. 该通道只能改变演示仿真的输入目标，不能访问用户数据或系统服务；若后续区分 debug/release 构建，release 构建必须保持该入口关闭或继续仅作为无敏感权限的视觉测试入口。
6. 解析失败必须输出一次性 warning 日志并丢弃命令，不得回退为 `0°`，也不得改变当前物理状态。

#### 6.5.5 真机脚本调用

初始启动必须携带完整水沙配置；后续命令仍需携带 `rendermode=4` 与 `testmode=true`：

```powershell
hdc -t <device> shell aa force-stop com.example.helloworld
hdc -t <device> shell hilog -r
hdc -t <device> shell aa start -b com.example.helloworld -a EntryAbility `
  --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wstilt 25
Start-Sleep -Milliseconds 500
hdc -t <device> shell aa start -b com.example.helloworld -a EntryAbility `
  --ps rendermode 4 --ps testmode true --ps wstilt -25
Start-Sleep -Milliseconds 500
hdc -t <device> shell aa start -b com.example.helloworld -a EntryAbility `
  --ps rendermode 4 --ps testmode true --ps wstilt 0
hdc -t <device> shell hilog -x
```

多步序列不把数组放进一个 Want。测试脚本每一步调用一次 `aa start` 改变 `wstilt`，等待至少一个显示帧或固定 `250~500ms` 后发送下一步；应用处于单实例运行态时，后续 `aa start` 走 `onNewWant` 边沿。TC-P011-02 必须先离线生成线性目标角时间表，再按该时间表发送 Want，并在报告中校验 `P011_EVENT input` 与物理样本的数量、时间与相位。`tools/run_p011_effect.ps1` 用 `Invoke-P011Tilt` 封装上述调用，并保留截图与 hilog 采集；不得使用无等待的紧凑 `aa start` 循环作为连续运动测试。

### 6.6 bottom-creep-v2.7.3 行为观测与相位窗口

本节是 `bottom-creep-v2.7.3` 的实现边界，本轮只登记设计，不修改业务代码。

1. **观测窗口**：TC02 behavior 统一使用 `100ms` 滚动窗口做物理响应整形与诊断聚合；正式 Pearson/FE 仍逐样本对齐，禁止在 checker 侧做平滑后验收。
2. **命令相位**：新增 `commandPhaseId` 与 `commandPhaseKind`，相位仅在 `runId/resetEpoch` 变化或目标符号翻转时递增；`0°` hold、dormant no-op 与零预算样本不重置相位。
3. **日志字段**：`P011_METRIC` 在 v2.7.2 下追加：
   - `commandPhaseId`
   - `commandPhaseKind`：`zero-hold`、`positive-ramp`、`positive-hold`、`negative-ramp`、`negative-hold`
   - `phaseElapsedMs`
   - `responseWindowMs`
   - `responseFeatureH`
   - `responseFeatureL`
   - `targetShapeResidualH`
   - `targetShapeResidualL`
   - `targetShapeRequestRate`
   - `targetShapeClipped`
   - `pearsonEligible`
4. **相位边界事件**：可选新增 `P011_BEHAVIOR_PHASE`，仅在相位切换时输出 `phaseStartPhysicsTime`、`phaseEndPhysicsTime`、`sampleCount`、`targetShapeResidualStart`、`targetShapeResidualEnd`，用于诊断，不参与替代逐样本验收。
5. **模块接口**：后续实现建议拆分为 `BehaviorPhaseTracker` 与 `TargetShapeShaper`；前者维护相位状态，后者维护 `C*H/C*L`、`C_H/C_L`、`eH/eL` 并生成 `clamp(kTarget*|targetTilt| + max(eH/tauH, eL/tauL), 0, vBottomCreepMax)` 的总请求速率；截断后按 light 优先、heavy 次后的固定顺序完成通道内分配。
6. **业务边界**：本轮不修改 `WaterSandField`、`SandCanvas`、`P011Telemetry` 或任何业务代码；上述字段与模块仅作为下一迭代的开发契约。
7. **落地顺序**：后续实现必须先冻结 v2.7.3 日志 schema 与 Pearson/FE 样本选择，再实现 `BehaviorPhaseTracker` 与 `TargetShapeShaper`，最后更新 checker 预检和分档报告；每个阶段不得先行放宽或改写 `7.10` 的验收阈值。

## 7. `test/p011_check.py` 设计

### 7.1 输入

1. hilog 文件或实时捕获文本
2. 输入事件清单
3. 固定 `routeId=bottom-creep-v2.7`
4. 可选：截图目录、帧时间戳 manifest

### 7.2 处理流程

1. 解析 `P011_EVENT input`、`P011_METRIC`、`P011_BOTTOM`。
2. 拒绝混入其他 `routeId` 的样本；v2.6 数据不得参与 v2.7 统计。
3. 按同一 `physicsTime` 对齐输入、有效倾角、预算指标与底层状态。
4. 校验 `P011_METRIC` 与 `P011_BOTTOM` 的样本数和 `physicsTime` 集合完全一致；同一 `physicsTime` 各一条，缺失、重复或错位均判为数据完整性失败。
5. 计算：
   - `T50 / T90 / P95 T90`
   - `directionError` / `directionAccuracy`
   - `Pearson(A(t), ΔB_h(t))`
   - `Pearson(A(t), ΔB_l(t))`
   - `lag_h / lag_l`
   - `FE_h / FE_l`
   - `maxSubstepBh / maxSubstepBl`
   - `maxUncommandedTiltStep`（限速爬坡残差口径）
   - `zeroResidual`
   - `reverseDirectionError`
   - `bottomCreepBudgetViolationRate`
   - `totalMotionBudgetViolationRate`
   - `bottomCreepIndependentClipRate`
   - `totalMotionClipRate`
   - 三相漂移

`maxUncommandedTiltStep` 不使用 `|ΔeffectiveTilt - ΔtargetTilt|`。每个样本先计算：

```text
expectedEffectiveTilt(t) =
    effectiveTilt(t-dt)
  + clamp(targetTilt(t) - effectiveTilt(t-dt),
          -tiltResponseRate * dt,
           tiltResponseRate * dt)

maxUncommandedTiltStep =
    max_t |effectiveTilt(t) - expectedEffectiveTilt(t)|
```

该口径只衡量实际有效倾角偏离合法限速爬坡的残差；合法爬坡本身不算非命令变化。
Activity 相关相关性窗口必须使用逐样本对齐的 `A(t)` 与 `ΔB_h(t)` / `ΔB_l(t)`。不得把一次非零输入 activity 扩展成前后 `±200ms` 之类的窗口，也不得把 `A(t)=0` 样本混入该 activity 窗口后再与持续 tilt 状态驱动的底层运动求相关。若需要评估命令后的持续状态响应，必须另立 `state-response` 窗口并明确标注，不得并入 `Pearson(A(t), ΔB_h(t))` 或 `Pearson(A(t), ΔB_l(t))` 的样本集合。报告必须输出 `activitySamples` 与进入相关性统计的实际样本数，二者必须一致。

`bottomDirection` 逐样本判定使用第 6.3 节的带符号 donor/receiver 字段做全局左右位移统计：

```text
donorSum = sum_c bottomCreepDonorEffectiveDisplacement[c]
receiverSum = sum_c bottomCreepReceiverEffectiveDisplacement[c]
expectedSign = sign(effectiveTilt)

directionCorrect =
    donorSum * expectedSign <= epsilon
    && receiverSum * expectedSign >= -epsilon
```

其中 `epsilon=1e-9` 仅用于消除浮点求和误差，不是新的验收阈值。
仅对 `bottomCreepBudgetUsed > 0` 且非 dormant no-op 的样本计数；零位移样本只跳过，不改变上一有效方向。checker 不得把 donor 与 receiver 分别在左、右空间半区聚合后再要求“右半区为正、左半区为负”随方向翻转，该口径会引入列边界偏差。
6. 输出 JSON 与 Markdown 报告：
   - `test/reports/p011_check.json`
   - `docs/test-report-P011-bottom-creep-v2.7.md`

### 7.3 判定顺序

1. 数据完整性
2. MUST-PASS
3. TIERED
4. 总体结论

任一 MUST-PASS 失败时，整体不得因 TIERED 高分而通过。

### 7.4 第二层工程归因（2026-10-16）

designer 已确认 `bottom-creep-v2.7` 算法方案无需变更。当前验收失败中的工程侧归因如下，修复不得改变算法语义、参数含义或验收阈值：

1. **模块收敛问题**：`P011TiltWant.ets` 是必需交付物，不能停留在文档设计。`EntryAbility` 与 `SandCanvas` 中的内联 Want 解析、重复 `P011TiltWantCommand` 定义和重复 JSON 校验必须收敛到该模块，避免生产端与消费端对同一命令格式产生解释漂移。
2. **物理 `dt` 观测问题**：`maxUncommandedTiltStep` 的残差计算必须使用逻辑物理子步 `substepDt`，或使用由 `substepDt` 写入的显式 `P011_METRIC.physicsDt`。`physicsTime` 差值、hilog 输出时间差、渲染调度间隔都不是物理积分时长，禁止作为 `dt` 代入限速公式。
3. **成对输出契约确认**：`P011_BOTTOM.bottomCreepDonorEffectiveDisplacement` 与 `bottomCreepReceiverEffectiveDisplacement` 必须只统计本逻辑物理子步内、经独立预算与总预算二次截断后的 bottom creep 有效位移；`P011_METRIC` 与 `P011_BOTTOM` 必须同回调生成、共享同一 `physicsTime`，并由测试器做样本数与时间集合的强一致性校验。

**developer 精确修复清单**：

1. 新增 `entry/src/main/ets/physics/P011TiltWant.ets`，实现 `P011TiltWantCommand`、`P011TiltWantWarning`、`parseP011TiltWant()` 与 `decodeP011TiltWant()`。
2. 修改 `EntryAbility.ets`：删除本地 `P011TiltWantCommand` 与 `applyP011TiltWant()` 内联解析，改为调用 `parseP011TiltWant()` 后写单槽 AppStorage。
3. 修改 `SandCanvas.ets`：删除本地 `P011TiltWantCommand` 与 `consumeP011TiltWant()` 内联 JSON 解析，改为调用 `decodeP011TiltWant()`；保留单槽 last-writer-wins 与本地已消费序号。
4. 修改 `WaterSandField.ets`：为 `WaterSandP011Metric` 增加 `physicsDt: number`；每个真实物理子步输出前写入 `substepDt`，`emitP011NoopTelemetry()` 输出 `physicsDt=0`。
5. 修改 `test/p011_check.py`：把 `physicsDt` 加入必需字段校验；`maxUncommandedTiltStep` 只用 `current.physicsDt`，并跳过 `physicsDt<=0` 的 dormant no-op；删除用 `physicsTime` 差值计算 `dt` 的逻辑。
6. 保持 `logWaterSandP011Pair()` 的单次成对输出与 `P011_BOTTOM` 有效位移数组口径不变；修复后重新执行 P-011 v2.7 全量验收，不得调整阈值或把失败样本剔除为通过。

### 7.5 第三层观测契约与测试执行归因（2026-10-16）

designer 已再次确认：不启动 `bottom-creep-v2.8`，不修改 v2.7 的算法语义、donor/receiver 方向、预算参数或验收阈值。当前失败按以下工程契约闭环：

1. **带符号观测字段**：`P011_BOTTOM` 的 donor/receiver 有效位移必须按第 6.3 节的 `directionSign` 公式在物理更新处直接累计为带符号全局左右位移。禁止输出正幅值后由 checker 推断方向；禁止改变 `bh/bl` 状态更新本身。
2. **符号切换计数**：checker 不得在 `targetTilt=0`、`effectiveTilt=0`、`bottomCreepBudgetUsed<=0` 或 dormant no-op 样本上清空上一有效方向。可按命令相位统计：仅以已接受的 `P011_EVENT input.targetTilt` 中非零相位为候选，维持 `lastNonZeroDirection`；相邻两个非零相位符号相反时 `bottomDirectionSignSwitches += 1`，零相位与零预算样本只跳过，不重置状态。
3. **TC-P011-02 原始序列**：正式执行不得替换为 `+88/0/-88` 或其他快捷相位。每轮必须为 `0° -> +20°` 用 `2s` 匀速到达、保持 `1s`、`+20° -> -20°` 用 `4s` 匀速到达、保持 `1s`、`-20° -> 0°` 用 `2s` 匀速到达，共重复 `5` 轮，并全程记录每个物理子步。测试脚本必须按线性目标角时间表驱动命令，避免 Want 重叠；报告中以 `P011_EVENT input.acceptedTime` 证明相位时长、保持时长与轮数。
4. **synthetic self-test 数据源**：self-test 的合成 `bh/bl` 只允许包含由期望 bottom creep 位移推导的状态变化；不得注入 settlement、suspension、随机数、抖动或任何与 bottom creep 无关的 activity 噪声。self-test 用于验证 checker 计算链路，不得通过噪声掩盖相关性实现问题。

#### 7.5.1 developer 分工

1. 修改 `WaterSandField.ets`：在同一物理子步内按 `donor += -directionSign * amount`、`receiver += directionSign * amount` 累计并输出 `P011_BOTTOM` 两个有效位移字段；状态迁移与预算截断逻辑不变。
2. 修改 `test/p011_check.py`：按命令相位重写 bottom direction sign-switch 计数，保留零样本但不重置上一有效方向。
3. 修改 synthetic self-test fixture：移除与 bottom creep 无关的 `bh/bl` activity 噪声，保留纯合成信号与解析/统计路径验证。
4. 增加最小回归：左到右与右到左各一段样本，断言 donor/receiver 数组符号相反、相位切换计数正确、零相位不清空方向。
5. 禁止修改 `routeId`、donor/receiver 判定、预算、倾角响应参数、`r >= 0.70`、`lag <= 66.7ms`、`FE <= 0.15` 或其他验收阈值。

#### 7.5.2 tester 分工

1. 先执行 `test/p011_check.py --self-check`，确认 checker 自检通过且合成数据无无关噪声。
2. 在固定种子与 `rendermode=4 && testmode=true` 下执行 TC-P011-02 原始 `0/+20/-20/0` 五轮序列，核对相位时间表、1s 保持、五轮完整性与 `P011_METRIC/P011_BOTTOM` 对齐。
3. 重新执行 P-011 v2.7 全量验收，报告 TC-P011-02 的 `pearsonH/L`、`lagH/L`、`feH/L` 与 sign-switch 结果；不得用替代序列的旧结果合并判定。
4. 若新结果仍失败，只报告观测事实并回传归因，不得删除失败样本、调整阈值或要求 developer 调整物理算法。

### 7.6 第四层测试驱动与观察口径归因（2026-10-16）

designer 已确认：`bottom-creep-v2.7` 算法、阈值与验收标准均不变；当前失败来自测试驱动与 checker 观察口径，不得据此调参或启动 `bottom-creep-v2.8`。本节为后续执行的有效契约，并覆盖 7.5 中已被 rerun 证明完成的实现项。

1. **TC-P011-02 连续驱动**：测试脚本必须使用连续目标角时间表逐显示帧驱动，相邻 Want 间隔不得小于一个显示帧；禁止无等待的紧凑 `aa start` 循环，禁止让单槽 last-writer-wins 吞掉中间目标角并产生 `1/2°` 脉冲。报告用 `P011_EVENT input.acceptedTime` 与 `P011_METRIC.physicsTime` 证明相位时长、保持时长、五轮完整性与无 Want 覆盖。
2. **activity 窗口隔离**：`A(t)=0` 的样本不得混入 activity 相关性窗口；持续 tilt 状态驱动的底层运动只能进入独立标注的 state-response 统计。checker 不得用 `±200ms` 脉冲扩展把 264 个非零 activity 样本扩成数千个混合样本。
3. **bottomDirection 逐样本方向**：使用带符号 donor/receiver 全局左右位移逐样本判定，禁止跨左/右空间半区聚合造成边界偏差。带符号字段实现已由 rerun 确认符合第 6.3 节语义，不再要求修改 `WaterSandField.ets` 的方向公式。
4. **objective suite 场景补齐**：客观套件必须包含满载并发场景，使 settlement、percolation、slip、suspension 在同一物理子步内同时产生非零申请，并证明 `fullLoadSamples > 0`；同时至少构造 3 个独立高压 total-clip 段，每段均出现 `totalMotionBudgetClipped > 0`，段间用 `totalMotionBudgetClipped=0`、独立 log 或分段标记隔离。
5. **失败归因边界**：若上述测试口径未满足，TC-P011-02 相关性、bottomDirection、full-load 与 total-clip 结果均不得判定为算法缺陷；测试方必须先修正驱动与 checker，再重新执行全量验收。

#### 7.6.1 developer 分工

1. 修改 `test/p011_check.py`：activity 相关性改为逐样本对齐，禁止零 activity 样本与 state-response 混算；输出 `activitySamples` 与相关性实际样本数。
2. 修改 `test/p011_check.py`：`bottomDirection` 改为第 7.2 节的逐样本带符号 donor/receiver 全局位移判定，删除左/右空间半区聚合方向要求。
3. 修改 objective fixture 或测试脚本：加入满载并发场景与至少 3 个独立高压 total-clip 段，并输出 `fullLoadSamples`、`totalMotionClipRuns` 与分段标识。
4. 保持 `WaterSandField.ets` 的 donor/receiver 公式、预算截断、`routeId`、`r >= 0.70`、`lag <= 66.7ms`、`FE <= 0.15` 及其他验收阈值不变。
5. 为 checker 增加最小回归：正/负倾角各一段样本、一段零 activity 持续 tilt 样本、一个左/右半区列分布不对称样本，断言相关性窗口与方向统计均不受空间半区边界影响。

#### 7.6.2 tester 分工

1. 生成并审查 TC-P011-02 线性目标角时间表，确认 Want 发送间隔至少一个显示帧且不存在被 last-writer-wins 吞掉的中间命令。
2. 执行 `test/p011_check.py --self-check`，确认 activity 窗口、逐样本方向、零样本跳过与空间不对称回归全部通过。
3. 在固定种子、`rendermode=4 && testmode=true` 下执行 TC-P011-02 原始 `0/+20/-20/0` 五轮序列，并核对 `P011_METRIC/P011_BOTTOM` 的 `physicsTime` 与样本数一一对齐。
4. 执行客观全量套件，报告满载并发样本数、3 个独立 total-clip 分段、预算违例率、独立截断率与总截断率；不得用缺失场景的运行补判通过。
5. 若新结果仍失败，只报告观测事实、原始样本与失败段，不得删除样本、调整阈值或要求 developer 修改物理算法。

### 7.7 第五层运行分段与套件覆盖契约（2026-10-16）

designer 已确认：`bottom-creep-v2.7` 的算法语义、参数与验收阈值均不变；当前 FAIL 归因于测试夹具与观测分段，不得据此调参或启动 `bottom-creep-v2.8`。本节为后续 checker 与测试夹具的有效契约。

1. **run/reset 分段**：checker 必须先按 `runId` 切分，再在每个 `P011_EVENT reset` 处切开。`runId` 变化而没有 reset 标记、`sequence` 回退或重复、`frameId/substepId` 非单调，均判为数据完整性失败。metric/bottom 必须共享同一 `runId/sequence/frameId/substepId`；任何跨 run、跨 reset 的样本不得进入相邻差分。
2. **target 相位分段**：每个已接受的 `P011_EVENT input` 定义一个目标相位，相位键为 `runId + ':' + inputEvent.sequence`。样本按 `acceptedTime <= physicsTime < nextAcceptedTime` 归属相位；首个 input 之前的样本不参与相位敏感指标。`maxSubstepBh/Bl` 与 `maxUncommandedTiltStep` 只允许在“同一 run、同一 reset 段、同一 target 相位”内，用当前样本的 `physicsDt` 计算；`physicsDt<=0` 或缺少同相位前序样本时跳过。
3. **场景重启隔离**：进程重启、`WaterSandField` 重建、preset/cols/capacity 变化等场景重启边界必须生成新 `runId` 并输出 reset 标记。checker 不得把重启前后的 `bh/bl`、`effectiveTilt` 或目标相位连接成相邻样本；跨边界差分结果必须丢弃并作为观测边界记录，不得计入 `maxSubstep` 或 `maxUncommandedTiltStep`。
4. **符号切换套件级覆盖**：`bottomDirection` 的正向到负向、负向到正向切换覆盖必须在 objective suite 聚合结果中判定，至少各出现 1 次。单符号压力日志只需如实输出本 run 的方向样本与切换计数，不得因自身没有符号切换而独立失败；若整个 suite 聚合后仍缺少任一方向切换，则判定场景覆盖不足，不得判定为算法缺陷。
5. **报告口径**：每份压力日志报告 `runId`、reset 次数、`sequence` 完整性、`frameId/substepId` 范围、target 相位数、相位内样本数与符号切换计数；suite 报告再输出跨 run 聚合的正负双向切换覆盖。缺失任一身份字段时，该 log 只能判定“观测数据不完整”，不得进入算法结论。

#### 7.7.1 developer 分工

1. 修改 `WaterSandField.ets` / `SandCanvas.ets`：为 `P011_METRIC`、`P011_BOTTOM`、`P011_EVENT input` 增加 `runId` 与 `sequence`，为物理样本增加 `frameId` 与 `substepId`，并输出 `P011_EVENT reset`。
2. 修改 `test/p011_check.py`：按 run/reset/target phase 建立分段索引，重写 `maxSubstepBh/Bl` 与 `maxUncommandedTiltStep` 的相邻样本选择逻辑，禁止跨 run、跨 reset、跨 phase 差分。
3. 修改 `test/p011_check.py`：把符号切换覆盖改为 suite 级聚合门禁，输出正向到负向、负向到正向两个计数；单符号压力日志不得独立判定 sign-switch 失败。
4. 增加 checker 回归：同 run 连续样本、run 切换、reset 切换、target phase 切换、dormant no-op、metric/bottom 身份错位与 sequence 回退各至少 1 例。
5. 不修改 donor/receiver 公式、预算截断、倾角响应参数、`r >= 0.70`、`lag <= 66.7ms`、`FE <= 0.15`、`maxSubstep < 5`、`maxUncommandedTiltStep <= 2°` 或其他算法阈值。

#### 7.7.2 tester 分工

1. 在固定种子下执行包含正/负倾角往返的 suite，确认 suite 聚合后正向到负向与负向到正向切换各至少 1 次。
2. 对每个压力场景记录独立 log 或确保同一 log 内 run/reset 标记完整，并核对 `P011_METRIC/P011_BOTTOM` 的身份四元组一一对齐。
3. 场景重启后必须等待新的 `P011_EVENT reset` 与首个新 `runId` 样本，再开始下一段采集；不得把重启边界前后的日志拼成一个连续相位。
4. 报告每个 run 的 `runId`、reset 次数、`sequence/frameId/substepId` 范围、target 相位数、相位内样本数与符号切换计数，并单独输出 suite 聚合覆盖。
5. 若身份字段、reset 标记或相位分段缺失，只判定“测试观测不完整”并回传原始日志；不得删除样本、调整阈值或要求 developer 修改物理算法。

### 7.8 第六层正式套件输入与判定层级契约（2026-10-17）

designer 已确认：`bottom-creep-v2.7` 的算法语义、参数与验收阈值均不变；当前 FAIL 归因于正式套件执行方式与 checker 判定层级，不得据此调参或启动 `bottom-creep-v2.8`。本节为正式客观验收的有效契约。

1. **正式套件输入**：正式验收必须一次性执行：

```powershell
powershell -ExecutionPolicy Bypass -File test/p011_tc02_formal_suite.ps1 -Mode All
```

`-Mode All` 必须先生成并校验 TC-P011-02 时间表，再依次采集 TC02 与四个 pressure 场景日志，最后把以下 5 份日志按该顺序一次性传给 suite checker：

```text
test/artifacts/p011-formal-prep-20261016/p011-tc02-formal.log
test/artifacts/p011-formal-prep-20261016/p011-full-load.log
test/artifacts/p011-formal-prep-20261016/p011-clip-segment-a-positive.log
test/artifacts/p011-formal-prep-20261016/p011-clip-segment-b-negative.log
test/artifacts/p011-formal-prep-20261016/p011-clip-segment-c-positive-repeat.log
```

2. **suite checker 调用**：`test/p011_tc02_formal_suite.ps1` 必须用 5 个 `--suite-input` 参数调用 `test/p011_check.py`，并输出唯一一份 suite JSON/Markdown 结论。不得先对 TC02 单独跑 suite 判定，再对 pressure logs 单独跑 suite 判定后人工合并结论；`-Mode Tc02` 与 `-Mode PressureSegments` 只能作为分段采集或排障手段，不是正式套件判定。

3. **high pressure 判定层级**：`highPressureEvidenceCount` 从 per-log `mustPass` 中移除，改为 suite 级布尔门禁；`highPressureCoverage` 是它的结构化 suite 级结果，两者必须同值。per-log 结果只保留原始 `totalMotionClipRuns` 诊断值；suite 级按四个 pressure logs 聚合：

```text
highPressureEvidenceCount = highPressureCoverage.passed =
    sum(totalMotionClipRuns(fullLoad, CLIP-A, CLIP-B, CLIP-C)) >= 3
```

单个 pressure log 只需至少出现 1 个 `totalMotionClipClipped > 0` 的 run，即可计入聚合；不得要求每个 pressure log 自身达到 3 个 clip runs，也不得把 TC02 用作 high pressure 证据。

4. **per-log gate**：每份日志独立判定且仅判定以下门禁：解析完整性、身份一致性、`sequence` 单调性、方向正确性、预算约束、三相守恒与稳定性。per-log 不得把 high pressure 覆盖或符号切换覆盖列为失败项；但这些日志仍必须输出原始 `totalMotionClipRuns`、方向样本与符号切换计数，供 suite 聚合。

5. **suite gate**：suite 级额外判定 `highPressureCoverage` 与 `suiteSignSwitchCoverage`。`highPressureCoverage` 按第 3 条聚合；`suiteSignSwitchCoverage` 要求 `positive -> negative` 与 `negative -> positive` 各至少 1 次。

6. **符号切换分段**：符号切换统计必须维护 `(logIndex, runId, resetSegmentId)` 级的 `lastNonZeroDirection` 状态。仅当当前样本与上一个非零方向样本属于同一 log、同一 `runId`、同一 reset 段，且方向相反时计数一次。log 结束、`runId` 变化、reset 边界都必须清空 `lastNonZeroDirection`；零方向或 dormant no-op 样本跳过但不清空该状态。禁止把上一个 log 的末方向接到下一个 log 的首方向。

7. **结果结构**：suite JSON 必须包含 `logs[].mustPass`、`logs[].totalMotionClipRuns`、suite 级 `highPressureEvidenceCount`、`highPressureCoverage`、`suiteSignSwitchCoverage` 与 suite 级 `mustPass`。Markdown 必须逐 log 列出 per-log gate，并单独列出 suite 级两个聚合门禁；不得把 suite 门禁打印成 per-log PASS/FAIL。

8. **阈值冻结**：不得放宽 `totalMotionClipRuns >= 3`、双向符号切换各 `>= 1`、`r >= 0.70`、`lag <= 66.7ms`、`FE <= 0.15`、`maxSubstep < 5`、`maxUncommandedTiltStep <= 2°` 或其他既有算法与验收阈值。若套件输入缺失或不足 5 份，只能判定“测试执行不完整”，不得判定算法 FAIL。

#### 7.8.1 developer 分工

1. 修改 `test/p011_tc02_formal_suite.ps1`：`-Mode All` 依次执行 TC02 与四个 pressure 场景后，调用一次 suite checker，并把 TC02 与四份 pressure 日志全部加入 `--suite-input`；不得在 `-Mode All` 中只传入 pressure logs。
2. 修改 `test/p011_check.py`：suite 模式建立 per-log 与 suite 两级结果结构；从 per-log `mustPass` 中移除 `highPressureEvidenceCount` 与符号切换覆盖，仅保留原始诊断计数，并把 `highPressureEvidenceCount` 迁移到 suite 级。
3. 修改 `test/p011_check.py`：新增 suite 级 `highPressureCoverage` 结构，并令 `highPressureEvidenceCount = highPressureCoverage.passed`；按四个 pressure logs 聚合 `totalMotionClipRuns` 并判定 `>= 3`，TC02 不得参与该聚合。
4. 修改 `test/p011_check.py`：按 `(logIndex, runId, resetSegmentId)` 重写符号切换状态机，双向各 `>= 1` 仅在 suite 级判定，并禁止跨 log、跨 `runId`、跨 reset 接续方向。
5. 增加 checker 回归：5 输入正常套件、单 pressure log 只有 1 个 clip run、pressure logs 聚合恰好 3 个 clip run、跨 log 同方向不计数、reset 边界不计数、双向切换各 1 次通过、缺任一输入判测试不完整。
6. 不修改 `WaterSandField.ets` / `SandCanvas.ets` 的算法语义，不调整任何阈值；仅当发现遥测身份字段缺失时按第 7.7 节修复观测契约。

#### 7.8.2 tester 分工

1. 执行 `python test/p011_check.py --self-check`，确认新增 per-log/suite 两级回归全部通过。
2. 正式客观验收只执行 `test/p011_tc02_formal_suite.ps1 -Mode All`，并核对 suite JSON 中恰好包含 TC02 与四个 pressure logs 共 5 份输入。
3. 逐 log 报告解析、身份、序列、方向、预算、守恒与稳定性结果；high pressure 覆盖与正负双向符号切换只在 suite 汇总中报告。
4. 核对 suite 级 `highPressureCoverage.totalMotionClipRuns >= 3`、`suiteSignSwitchCoverage.positiveToNegativeSwitches >= 1`、`suiteSignSwitchCoverage.negativeToPositiveSwitches >= 1`，且所有既有算法阈值未变化。
5. 若 5 份日志未全部生成或 suite checker 未一次性接收全部输入，判定“测试执行不完整”并重跑 `-Mode All`；不得用分段结果、旧报告或其他运行补判，不得删除样本或放宽阈值。

### 7.9 2026-10-17 TC02 suite applicability and state-machine correction

依据 `docs/test-report-P011-bottom-creep-v2.7-20261017.md` 第二段正式复测结果，本轮不修改物理算法、`routeId`、参数或任何阈值；仅修正 suite checker 的证据适用范围与观测状态机。修订后的测试口径称为 **bottom-creep-v2.7.1 test-caliber**，仍归属 `bottom-creep-v2.7` 技术路线，不构成新算法版本。

#### 7.9.1 日志角色与适用矩阵

suite 输入必须携带显式角色清单，禁止用文件顺序或事后猜测决定语义：

| 日志 | suiteRole | 必须提供 | 不得承担 |
|---|---|---|---|
| `p011-tc02-formal.log` | `behavior` | TC01/TC02 分档、T90、正负双向 in-run sign-switch、逐样本方向正确性 | full-load 隔离证明、高压 total-clip 计数 |
| `p011-full-load.log` | `budgetIsolation + pressureClip` | `fullLoadSamples > 0`、独立预算隔离违例 0、bottom/total 预算违例 0、独立 total-clip run | TC02 行为相关性评分、符号切换覆盖 |
| `p011-clip-segment-a-positive.log` 等 pressure logs | `pressureClip` | 独立 run/reset 下的 total-clip run、预算与稳定性 | TC01/TC02 分档、双向符号切换 |

per-log gate 只判定“该角色适用的证据”，输出 `PASS` / `FAIL` / `NOT_APPLICABLE`。`NOT_APPLICABLE` 不得计入失败，也不得当作 PASS 证据；suite 总判定必须分别检查角色齐全、per-log 适用项全 PASS、suite 级覆盖全 PASS。

#### 7.9.2 独立预算隔离聚合口径

`independentBudgetIsolation` 从 per-log MUST-PASS 迁移到 suite 级 `independentBudgetCoverage`，且只读取 `budgetIsolation` 角色：

```text
expectedBottomLimit(t) = vBottomCreepMax * physicsDt(t)

for sample in budgetIsolationLog:
    if all four fullLoadRequestFields(sample) > 0:
        fullLoadSamples += 1
        if bottomCreepBudgetUsed > bottomCreepBudgetLimit + eps:
            violation += 1
        if bottomCreepBudgetLimit < expectedBottomLimit * 0.98:
            violation += 1

independentBudgetCoverage.passed =
    budgetIsolationLogCount == 1
    and fullLoadSamples > 0
    and violation == 0
```

TC02 的 `fullLoadSamples=0` 必须输出 `NOT_APPLICABLE`；full-load 日志缺失、full-load 请求字段不完整或 `fullLoadSamples=0` 时 suite 判定 `INCOMPLETE(budgetIsolationCoverage)`，不得判为算法缺陷。四个请求字段仍为 `settlementMotionRequest`、`percolationMotionRequest`、`slipMotionRequest`、`suspensionMotionRequest`。

#### 7.9.3 in-run sign-switch 状态机

方向切换状态的作用域是 `(logIndex, runId, resetEpoch)`，不是 `input sequence`、`target phase` 或文件顺序。当前 TC02 的正负相位存在，但 checker 在每个 input sequence 边界重置方向状态，导致计数为 0，这是观测设计缺陷。

```text
for bottomSample in one log, ordered by physicsTime:
    if (logIndex, runId, resetEpoch) changed:
        lastNonZeroBottomDirection = 0
    effectiveSign = sign(effectiveTilt outside dead zone)
    if effectiveSign == 0 or bottomCreepBudgetUsed <= eps:
        continue                         // 不重置 last direction
    observedDirection = sign(sum(receiverEffectiveDisplacement))
    if observedDirection == 0:
        continue                         // 不重置 last direction
    if lastNonZeroBottomDirection != 0
       and observedDirection != lastNonZeroBottomDirection:
        count the corresponding + -> - or - -> + switch
    lastNonZeroBottomDirection = observedDirection
```

`suiteSignSwitchCoverage.passed` 要求且仅要求 `behavior` 角色日志内 `positiveToNegativeSwitches >= 1` 且 `negativeToPositiveSwitches >= 1`。pressure logs 单符号运行中的 0 次切换是预期诊断值，不得触发失败，也不得跨 log、`runId`、reset 边界拼接出切换。

#### 7.9.4 TC01/TC02 分档口径

1. **TC01 T90**：只在 `behavior` 角色日志内统计。死区穿越检测与首个有效 bottom response 搜索必须在同一 `(runId, resetEpoch)` 内进行，禁止在每个 input sequence 边界重置；五轮 TC02 预期至少 10 次有效响应，P95 T90 阈值仍为 `<= 100ms`。响应不足时判 `INCOMPLETE(t90Coverage)`。
2. **TC02 state-following**：只在 `behavior` 角色日志内统计。候选样本为 `abs(targetTilt) > deadZone` 且 `bottomCreepBudgetUsed > eps`；`A(t)=targetTilt`，`R(t)=sum(receiverEffectiveDisplacement)-sum(donorEffectiveDisplacement)`，使用同一 run/reset 内的 `physicsTime` 做 lag。保留 `r >= 0.70`、`|lag| <= 66.7ms`、`FE <= 0.15` 与 score 6 及格线。若输入或响应方差为 0，输出 `INCOMPLETE(degenerateMeasurement)`，不得直接给算法 0 分。
3. **suite minimum**：`min(TC01Score, TC02Score, TC03Score) >= 6` 只由 `behavior` 角色日志计算；full-load 与 pressure logs 的 TC01/TC02 输出 `NOT_APPLICABLE`。这是适用范围修正，不是降低 6 分及格线或任何数值阈值。

#### 7.9.5 负责模块与阶段边界

| 模块/阶段 | 责任 | 禁止事项 |
|---|---|---|
| manager | 批准本测试口径修订与复测排期 | 不得把 2026-10-17 FAIL 直接归因为物理算法失败 |
| checker owner / `test/p011_check.py` | 实现 suiteRole、`NOT_APPLICABLE`、suite 级预算覆盖、run/reset 级方向状态机、behavior-only TC01/TC02 评分与回归 | 不得修改算法语义、参数、数值阈值或跨 log 拼接证据 |
| suite-script owner / `test/p011_tc02_formal_suite.ps1` | 保留 `-Mode All`，一次传入 5 份日志并显式声明 7.9.1 角色 | 不得用 `PressureSegments` 分段结果补判正式结论 |
| app developer / `WaterSandField`、`SandCanvas`、`P011Telemetry` | 本轮无授权修改；仅当复测暴露遥测字段缺失时另行走设计归因 | 不得调参、改 donor/receiver、改预算或为通过测试改日志 |
| tester | 先跑 checker 自检与角色/状态机回归，再完整 rerun `-Mode All` 并生成新报告 | 不得删除样本、放宽阈值或合并旧报告 |

#### 7.9.6 复测验收输出

1. suite JSON 必须包含 `suiteRoles`、`independentBudgetCoverage`、`suiteSignSwitchCoverage`、`behaviorTierScores`、每个 per-log gate 的 `PASS/FAIL/NOT_APPLICABLE`。
2. `independentBudgetCoverage` 必须显示 full-load 日志 `fullLoadSamples=2033` 或更新复测中的等量有效样本、isolation violations 0、bottom/total budget violations 0。
3. TC02 行为日志必须显示 positive-to-negative 与 negative-to-positive 各至少 1 次，且两者均来自同一 `runId/resetEpoch`；pressure logs 的 0 次切换只作为诊断输出。
4. TC01 `P95 T90 <= 100ms`，TC02 `r >= 0.70`、`|lag| <= 66.7ms`、`FE <= 0.15`，suite minimum score `>= 6`。
5. high-pressure coverage 仍要求四个 `pressureClip` 角色日志（full-load 与三个 clip-segment logs）聚合 `totalMotionClipRuns >= 3`，且单个 pressure log 至少 1 个 clip run；TC02 不得计入该聚合。full-load 日志可同时服务 `budgetIsolation` 与 `pressureClip`，但两种证据必须分别输出，不得互相替代。

### 7.10 bottom-creep-v2.7.3 TC02 behavior preflight and rerun caliber

1. **route 隔离**：checker 只接受 `routeId=bottom-creep-v2.7.3` 的 behavior 日志；v2.7、v2.7.1、v2.7.2 或其他 route 的样本不得混入统计。
2. **样本选择**：沿用 TC02 的 400 活动样本口径；H 与 L 必须使用同一样本集合，且来自同一 `runId/resetEpoch`。
3. **响应整形**：`WaterSandField` 先生成 total bottom creep 请求速率 `requestRate=clamp(kTarget*|targetTilt| + max(eH/tauH, eL/tauL), 0, vBottomCreepMax)`。`C*H=C*L=∫ kTarget*|targetTilt| dt`；`C_H/C_L` 分别累计本子步 heavy/light bottom creep 实际位移；`eH=C*H-C_H`，`eL=C*L-C_L`；固定 `kTarget=0.5`、`tauH=100ms`、`tauL=60ms`、`vBottomCreepMax=10`。死区内请求为 0，理想量、实际量与残差只在 run/reset 或目标符号翻转边界重置。总截断后先满足 light 残差、再满足 heavy 残差，但每个通道都受 donor 对应组成存量与 receiver 容量约束；不新增 light-only 预算，不改 donor/receiver、独立预算或总预算语义。
4. **特征定义**：`A_corr(t)=signed(targetTilt)`，`R_corr,H/L(t)=当前命令相位内以相位起点为零的 signed cumulative net bottom displacement`；不得继续使用已证明退化的 `|ΔeffectiveTilt|` 作为 TC02 输入特征。
5. **方差预检**：计算 `var(A_corr)`、`var(R_corr,H)`、`var(R_corr,L)`；三者均必须大于 `1e-12`，且所有值必须为有限数。
6. **样本覆盖**：正相位与负相位各至少 30 个有效样本，H/L 每通道至少 30 个非零响应样本；不满足时输出 `INCOMPLETE(degenerateMeasurement)`，不得继续计算 Pearson，也不得给 TC02 计 0 分。
7. **FE 口径**：`FE_H` 与 `FE_L` 分别使用 `|A_corr|` 与 `|R_corr,H/L|` 的归一化累计序列；不得在 checker 侧平滑、重采样或删除样本来降低 FE。
8. **复测命令**：保持 `powershell -ExecutionPolicy Bypass -File .\test\p011_tc02_formal_suite.ps1 -Mode All`；报告中必须显式记录 `routeId=bottom-creep-v2.7.3`。
9. **报告隔离**：客观与主观报告分别独立成文为 `docs/test-report-P011-bottom-creep-v2.7.3.md` 与 `docs/effect-report-P011-bottom-creep-v2.7.3.md`，不得与 v2.7、v2.7.1、v2.7.2 或其他路线合并。
10. **阈值冻结**：`pearsonH/L >= 0.70`、`|lagH/L| <= 66.7ms`、`FE_H/L <= 0.15`、`TC01/TC02/TC03 >= 6` 全部不变；本轮不通过修改报告口径转 PASS。
11. **证据输出**：复测报告必须输出 `activitySamples=400`、`var(A_corr)`、`var(R_corr,H/L)`、正负相位样本数、`pearsonH/L`、`lagH/LMs`、`FE_H/L`、边界排除计数、H/L 通道截断计数与两方向 ramp 完整性。若任一前置不满足，结论为 `INCOMPLETE(degenerateMeasurement)`。

## 8. 开发步骤

1. **参数与 routeId**
   - 在 `WaterSandField` 中加入 `routeId`、`vBottomCreepMax`、`bottomCreepVelocityGain`、`bottomCreepDeadZone`、`directionHoldTime`。
   - 确认 `totalMotionBudget` 沿用现有配置并保持硬上限语义。

2. **输入时间戳**
   - `SandCanvas` 记录 `commandTime`、`acceptedTime`、`physicsTime`。
   - 输出 `P011_EVENT input`。
   - 新增 `P011TiltWant` 解析器，`EntryAbility` 在 `onCreate/onNewWant` 发布单槽命令，`SandCanvas` 帧回调消费并输出 `source="script"`。
   - 增加角度规范回归：`-88 -> effectiveDeg=-88`、`88 -> effectiveDeg=88`、`180 -> visualInverted=true && effectiveDeg=0`、`135 -> visualInverted=true && effectiveDeg=-45`、`225 -> signedTilt=-135 && visualInverted=true && effectiveDeg=45`。

3. **方向语义**
   - 实现 `effectiveTilt > 0` 左 donor / 右 receiver。
   - 实现 `effectiveTilt < 0` 右 donor / 左 receiver。
   - 增加方向回归断言，禁止高度差反转 donor/receiver。

4. **双预算通道**
   - 先应用 `vBottomCreepMax` 独立截断。
   - 再与其他运动申请合并，由 `totalMotionBudget` 统一截断。
   - 在现有列循环内累计用量，不新增二次全列扫描。

5. **测试钩子**
   - 输出扩展后的 `P011_METRIC` 与 `P011_BOTTOM`。
   - `P011_BOTTOM` 必须包含 bottom creep 专用 donor/receiver 有效位移字段，并与 `P011_METRIC` 按 `physicsTime` 一一对齐。
   - 确保仅测试模式输出，避免影响正式包性能。

6. **脚本**
   - 实现 `test/p011_check.py`。
   - 支持 `--self-check`、预算指标校验、`maxUncommandedTiltStep` 限速爬坡残差口径与正式报告输出。
   - 修改 `tools/run_p011_effect.ps1`：以 `Invoke-P011Tilt` 调用 `aa start --ps wstilt <deg>` 驱动多步序列，替代不稳定的 `uinput` 倾角驱动。

7. **文件清单**
   - 新增：`entry/src/main/ets/physics/P011TiltWant.ets`。
   - 修改：`entry/src/main/ets/entryability/EntryAbility.ets`、`entry/src/main/ets/pages/SandCanvas.ets`、`tools/run_p011_effect.ps1`。
   - 不修改：`entry/src/main/module.json5`、`WaterSandField` 物理算法；新增 `P011_BOTTOM` 观测字段不改变既有字段语义。

8. **报告**
   - 客观与主观报告均按 `bottom-creep-v2.7` 独立成文。
   - 不与 `bottom-creep-v2.6` 或 `gravity-advection-v1` 合并结论。

9. **回归**
   - 运行既有水沙单测。
   - 运行 `water_sand_check.py --self-check-rate`。
   - 运行 P-011 客观测试与固定种子回放。

## 9. 验收口径

### 9.1 硬门槛

1. **方向正确率 = 100%**：`effectiveTilt > 0` 时左列净输出为 donor、右列净输出为 receiver；`effectiveTilt < 0` 时反向。覆盖至少 200 个非死区采样，并包含倾角符号切换。
2. **独立预算隔离 = 100%**：仅触发 bottom creep 时，`bottomCreepBudgetUsed <= bottomCreepBudgetLimit`；满载注入沉降/渗透/滑移/扬起时，bottom creep 独立可用预算下降不超过 1 帧统计误差或 2%。
3. **总运动硬上限不可突破**：`totalMotionBudgetUsed <= totalMotionBudgetLimit`，违例率为 0；suite 聚合四个 pressure logs 后 `totalMotionClipRuns >= 3`，系统不崩溃、不超时。单个 pressure log 只需至少 1 个 clip run；段间必须等待 `totalMotionBudgetClipped = 0` 或使用独立 log 分段，禁止把同一连续高压窗口重复计数。
4. **P95 T90 <= 100ms**：从有效倾角越过死区到首个 bottom creep 有效位移的 T90 分布。
5. **基础硬门槛**：三相漂移 `< 0.1%`、无 `model_error`、无非有限数、无越界、无 crash / ANR。

### 9.2 软门槛

1. 主观跟手性评分 `>= 6/10`；若 v2.6 存在有效同路线主观数据，则 v2.7 相对 v2.6 不下降。
2. 底层视觉突变率不高于 v2.6；若 v2.6 无有效客观数据，则帧间列位移突变率不超过 `5%`。
3. 高压场景下 `bottomCreepBudgetClipped / bottomCreepBudgetLimit <= 30%`；若超过，必须在同一测试中证明 `vBottomCreepMax` 从 10 调整到 12 后仍无法降低截断率，才可判定为预算参数上限问题。

### 9.3 测试方法

1. 客观测试使用固定种子回放，覆盖小/中/大倾角、左右往复、死区边缘、满载多运动并发、低帧率与高帧率。
2. 每个用例输出逐帧 `P011_METRIC`，统计 P95 T90、预算违例率、独立截断率与总截断率。
3. 主观测试在不知道技术路线版本的条件下评估跟手性、方向一致性、突变感与稳定性，采用 10 分制。
4. v2.7 的同路线历史参考只能是 v2.6 报告；`gravity-advection-v1` 只能作为定性选型参考，禁止作为 v2.7 验收依据。
5. 报告独立成文：
   - `docs/test-report-P011-bottom-creep-v2.7.md`
   - `docs/effect-report-P011-bottom-creep-v2.7.md`
6. 正式 objective suite 必须按第 7.8 节执行 `test/p011_tc02_formal_suite.ps1 -Mode All`，并把 TC02 与四个 pressure logs 一次性传入 suite checker；分段模式结果不得合并为正式结论。
7. `fullLoadSamples > 0` 只由 `budgetIsolation` 角色的 full-load 日志提供；四个 pressure logs 聚合后的 `totalMotionClipRuns >= 3` 只由 `pressureClip` 角色提供。任一条件不满足时只能判定“场景覆盖不足 / 测试未完成”，不得将对应项判定为算法缺陷，也不得让 TC02 承担 full-load 证明。
8. `bottomDirection` 正负双向符号切换覆盖必须按第 7.9 节由 `behavior` 角色的 TC02 日志提供，且只在同一 `runId/resetEpoch` 内计数；单符号压力日志不承担该覆盖要求，也不得跨 log 拼接。
9. TC01/TC02 分档只按第 7.9.4 节在 `behavior` 角色日志上计算；full-load 与 pressure logs 输出 `NOT_APPLICABLE`。suite minimum `TC01/TC02/TC03 >= 6` 与所有数值阈值不得放宽。

## 10. 异常处理与降级

1. **输入无效或非有限数**：跳过本子步 bottom creep，输出 `directionCorrect=false`，并保留既有错误日志。
2. **死区或零倾角**：不产生新的 donor/receiver 迁移，预算字段按 0 输出。
3. **donor 余量不足**：只截断迁移量，不反转 donor/receiver。
4. **receiver 容量不足**：只截断迁移量，并计入独立预算截断或容量截断调试口径。
5. **独立预算耗尽**：按申请比例缩放 bottom creep，记录 `bottomCreepBudgetClipped`。
6. **总预算耗尽**：所有运动按申请比例统一缩放，记录 `totalMotionBudgetClipped`，不抛出运行时异常。
7. **测试钩子输出失败**：物理更新继续执行，记录一次性降级日志；不得因测试日志导致 crash / ANR。
8. **`wstilt` 非法或模式不匹配**：丢弃命令，输出 warning 日志，保留当前目标倾角与物理状态，不得隐式置零。
9. **未消费命令被覆盖**：按 last-writer-wins 丢弃旧命令；测试脚本必须等待事件或至少一个显示帧，避免把丢步判定为实现缺陷。
10. **命令到达时字段未初始化**：命令停留在单槽，不生成 `acceptedTime`；待 `waterSandField` 创建后由下一帧消费，禁止提前输出没有 `physicsTime` 的输入事件。

## 11. 风险与分流

| 风险 | 缓解 |
|---|---|
| 方向实现仍依赖左右索引解释错误 | 使用固定相邻列对 `(c, c+1)` 映射，并增加 200 采样以上方向回归 |
| 独立预算与总预算统计口径混淆 | 所有预算字段在同一物理子步内统计，`bottomCreepBudgetUsed` 在总截断后取值 |
| 总硬上限在高压时截断 bottom creep | 记录总截断率与调试明细；不得通过突破 `totalMotionBudget` 解决 |
| 速度增益导致底层抖动 | 限制 `bottomCreepVelocityGain`，保留 `directionHoldTime` |
| 预算统计造成性能回退 | 复用列更新循环累计，仅测试模式输出全量钩子 |
| 日志量过大 | `P011_BOTTOM` 按 `100ms` 或事件边界输出，`P011_METRIC` 保持逐子步必需字段 |
| 守恒破坏 | 保留 `clipSubstepMotion()` 与既有 audit，三相漂移作为硬门槛 |
| 多条 Want 命令在同一帧前重叠 | 单槽 last-writer-wins，脚本每步等待至少一个显示帧；重叠不是实现缺陷 |
| `wstilt` 被误用于生产路径 | 仅 `rendermode=4 && testmode=true && forceinvert=false` 接受；其他路径忽略并输出 warning |
| Want 时间戳与物理时间戳错位 | 三个时间均来自单调 `nextP011TimeMs()` 或同子步 `metric.physicsTime`，事件在首个物理子步后输出 |

失败分流：

1. 若方向正确率未达 100%，优先判定为实现未按 `effectiveTilt` 固定 donor/receiver 语义执行，反馈 architect 修订实现。
2. 若 P95 T90 > 100ms 且方向、预算与指标实现均正确，仅在 `vBottomCreepMax = 8/10/12` 三档内做有限消融；若仍无法达标，立即停止 bottom-creep 路线调参，转入 `gravity-advection-v1`。
3. 若验收失败但测试用例存在口径错误，上报 manager 决策，不得私自修改用例。
4. 禁止回退到 `bottom-creep-v2.6`；该版本状态为 `REJECTED`。

## 12. 变更记录

| 日期 | 版本 | 变更 |
|---|---|---|
| 2026-10-14 | bottom-creep-v2.6 | 初版工程方案；该算法版本已于 2026-10-15 判定为 `REJECTED` |
| 2026-10-15 | bottom-creep-v2.7 | 修正 donor/receiver 方向语义；新增 `vBottomCreepMax=10` 独立预算并保留 `totalMotionBudget` 硬上限；扩展预算指标；更新实施步骤、测试钩子、验收口径、风险与报告独立性要求 |
| 2026-10-16 | bottom-creep-v2.7 | 主观评分口径由 5 分制阈值 `3.5/5` 修正为 10 分制阈值 `>= 6/10` |
| 2026-10-16 | bottom-creep-v2.7 | 补充 signed-HAP rerun 支撑口径：`P011_EVENT` 对 dormant no-op 也必须输出；`P011_BOTTOM` 增加 bottom creep 专用 donor/receiver 有效位移字段；明确 `maxUncommandedTiltStep` 限速爬坡残差口径；高压 total-clip 至少 3 个独立分段；要求 metric/bottom 按 `physicsTime` 与样本数一一对齐 |
| 2026-10-16 | bottom-creep-v2.7 | 补充总预算请求快照语义：所有通道请求基于同一子步开始状态生成，`totalMotionBudgetRequested` 必须包含 `bottomGrant + otherRequested`，总预算耗尽时仅对原始 `allRequests` 集合统一缩放，禁止用状态变化后的重放请求替代 |
| 2026-10-16 | bottom-creep-v2.7 | 新增测试专用 `wstilt` Want 注入设计：单槽 AppStorage 命令、`onCreate/onNewWant` 边沿、单调时间戳、last-writer-wins 多步驱动、安全约束与真机脚本调用方式；不改变 v2.7 物理语义 |
| 2026-10-16 | bottom-creep-v2.7 | 修正倾角工程口径：`normalized>=180` 时映射为 `normalized-360` 的带符号角，`abs(signedTilt)>90` 才判视觉倒立；`[-89,89]` 的 `wstilt` 保持原符号且不得强制倒立。视觉倒立、routeId、双预算与 bottom creep 方向语义不变 |
| 2026-10-16 | bottom-creep-v2.7 | 第二层工程归因：确认 `P011TiltWant.ets` 为必需模块并收敛 EntryAbility/SandCanvas 内联解析与重复命令定义；固化 `maxUncommandedTiltStep` 的逻辑物理子步 `dt` / `physicsDt` 观测契约，禁止用 hilog 墙钟间隔反推物理 `dt`；确认 `P011_BOTTOM` 有效位移与 metric/bottom `physicsTime` 成对输出契约，并输出 developer 精确修复清单。不改变算法语义，不放宽验收 |
| 2026-10-16 | bottom-creep-v2.7 | 第三层工程归因：固化 donor/receiver 有效位移为带符号全局左右位移；checker 符号切换改按非零命令相位统计且零度/零预算样本不清空方向；TC-P011-02 必须执行原始 `0/+20/-20/0` 五轮匀速序列；synthetic self-test 禁止向 `bh/bl` 注入无关 activity 噪声；输出 developer/tester 分工清单。不修改算法语义、参数与阈值 |
| 2026-10-16 | bottom-creep-v2.7 | 第四层工程归因：TC-P011-02 改为逐显示帧连续目标角驱动且相邻 Want 间隔不小于一帧，禁止紧凑 `aa start` 循环、last-writer-wins 丢步与 `1/2°` 脉冲；activity 相关性禁止混入 `A(t)=0` 样本，state-response 独立统计；bottomDirection 改为逐样本带符号 donor/receiver 全局位移判定，禁止左右空间半区聚合；objective suite 补满载并发与至少 3 个独立高压 total-clip 段，并输出 developer/tester 分工。不修改算法、参数与验收标准 |
| 2026-10-16 | bottom-creep-v2.7 | 第五层工程归因：P-011 遥测新增 `runId`、reset 边界标记与单调 `sequence/frameId/substepId`；checker 按 run/reset/target phase 分段，`maxSubstep` 与 `maxUncommandedTiltStep` 仅在同一 run、同一 reset 段、同一 target 相位内用 `physicsDt` 计算，场景重启不得参与；`bottomDirection` 符号切换覆盖提升为 suite 级要求，单符号压力日志不再独立承担；输出 developer/tester 分工。不修改算法语义、参数与阈值 |
| 2026-10-17 | bottom-creep-v2.7 | 第六层工程归因：正式 objective suite 固化为 `-Mode All`，TC02 与四个 pressure logs 必须一次性传入 suite checker；`highPressureEvidenceCount` 从 per-log 移除并改为 suite 级 `highPressureCoverage`，按 pressure logs 聚合 `totalMotionClipRuns >= 3` 且单 log 至少 1 个 clip run；per-log gate 只判解析、身份、序列、方向、预算、守恒与稳定性；suite 另设 `highPressureCoverage` 与 `suiteSignSwitchCoverage`，符号切换仅在同一 log/runId/reset 段内双向各至少 1 次；输出 developer/tester 分工。不修改算法语义、参数与阈值 |
| 2026-10-17 | bottom-creep-v2.7.1 test-caliber | 依据 TC02 正式复测完成设计归因：`independentBudgetIsolation` 改为 suite 级且只由 full-load 角色日志证明，TC02 输出 `NOT_APPLICABLE`；sign-switch 状态机作用域改为 log/runId/resetEpoch，禁止在每个 input sequence 边界重置；TC01/TC02 分档只在 behavior 角色日志计算。不修改物理算法、参数、routeId 或任何验收阈值 |
| 2026-10-18 | bottom-creep-v2.7.2 | 依据 checker correction rerun 完成 v2.7.2 可执行设计：新增行为观测窗口、命令相位、方差预检、H/L 相关性有效样本、目标累计形状整形与总预算内残差补偿；明确业务代码本轮不动，报告按 v2.7.2 route 独立成文，所有阈值不变 |
| 2026-10-18 | bottom-creep-v2.7.3 | 依据 v2.7.2 复测（TC01=10、TC02=0、TC03=10，`FE_L=0.20549138771581632`，Pearson H/L `null`）完成设计修订：route 升级为 v2.7.3；checker 固化 `signed(targetTilt)` 与相位内带符号累计净位移特征；业务侧设计 H/L 残差整形、light 优先的通道内分配与 `tauH=100ms`/`tauL=60ms`。硬门槛、数值阈值、及格线、方向语义与双预算上限不变 |
