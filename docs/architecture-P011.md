# P-011 工程方案：bottom-creep-v2.8-heavy-interface

> 日期：2026-10-18  
> 问题：P-011 倾斜跟手性 / 底层运动响应  
> 技术路线：`bottom-creep-v2.8-heavy-interface`  
> 状态：`READY_FOR_DEVELOPER`  
> 依据：`docs/algorithm-P011.md` §5、`docs/test-report-P011-bottom-creep-v2.7.3-20261018.md`、`docs/effect-report-P011-bottom-creep-v2.7.3.md`  
> 工程结论：v2.8 可在现有 `WaterSandField` 架构内实现；不需要新增物理线程、排序 pass 或预算上限调整。

---

## 1. 审批结论与前提

### 1.1 工程可行性判定

| 设计项 | 可行性 | 工程判定 |
|---|---|---|
| phase-balanced split | 可行 | 每个 donor→receiver 授予量仍是 `O(1)` 拆分，总复杂度保持 `O(cols)` |
| heavy share floor | 可行 | `0.35` 下限可与 donor 存量回退共存，不提高任何预算 |
| compact phase telemetry | 可行 | `P011_METRIC` 新增约 20 个标量；`P011_BOTTOM` 数组仅保留诊断用途 |
| routeId upgrade | 可行 | 只改默认 route 常量、参数校验与测试/checker 期望，不混用历史日志 |
| checker/report isolation | 可行 | checker 升级 schema v2，严格拒绝 route 或字段不一致，不修改历史报告 |
| green-ROI preflight | 可行 | 复用 `wstilt`、`snapshot_display`、hilog 与现有图像分析依赖，输出可复核标量 |
| TC02 capture lifecycle | 可行 | formal suite 与独立 precheck 共用同一注入/采集 runner，先冻结文件再校验 |

### 1.2 输入结论

1. v2.7.3 objective suite 为 PASS：方向、预算、守恒与稳定性门槛未失败。
2. v2.7.3 subjective review 为 FAIL：HG-TT-04 触发，绿色 heavy 层在全部有效场景中静止。
3. 两者不矛盾：客观指标观测的是聚合底层位移；主观验收要求用户可见的 `bh` 绿色边界移动。
4. 本方案不重新解释、不覆盖、不软化 v2.7.3 主观 FAIL。
5. v2.7.3 的 source commit 记录差异不作为结论修改依据；v2.8 复测必须同时记录 source commit、HAP SHA256 与每个原始 log SHA256。

## 2. 目标与非目标

### 2.1 目标

1. 将每个相邻 donor→receiver 的 bottom creep 授予量拆分为 heavy/light 双通道。
2. 保证 active flux 的 heavy 份额下限为 `0.35`，上限为 `0.65`； donor 存量不足时允许余量转入另一相。
3. 保持 `vBottomCreepMax=10`、`totalMotionBudget`、dead zone、方向语义与全部既有阈值不变。
4. 输出 heavy/light 独立累计位移、相位形状变化与子步跳变标量。
5. 让 checker 不再依赖聚合位移数组，也不再把同一聚合响应复制为 H/L 两个特征。
6. 增加最小 green-ROI preflight，用于验证观测链路，不替代完整 objective/subjective suite。

### 2.2 非目标

1. 不提高或放宽任何物理预算、客观分档阈值、主观硬门槛或及格线。
2. 不修改 `docs/algorithm-P011.md`、manager 文档、历史测试报告或 v2.7.3 证据。
3. 不引入整层重力对流；该机制属于 `gravity-advection-v1`。
4. 不把 `P011_BOTTOM` 长数组作为 heavy/light 唯一证据。
5. 不做跨路线 dominance 比较。

## 3. 参数与状态设计

### 3.1 固定常量

| 常量 | 值 | 说明 |
|---|---:|---|
| `P011_BOTTOM_ROUTE_ID` | `bottom-creep-v2.8-heavy-interface` | route、日志与报告唯一标识 |
| `P011_HEAVY_SHARE_MIN` | `0.35` | heavy flux 下限份额 |
| `P011_HEAVY_SHARE_MAX` | `0.65` | heavy flux 上限份额 |
| `P011_HEAVY_SHARE_BIAS` | `0.10` | `kBias` 固化值，单位 `1/model unit`；禁止运行时调参 |
| `P011_TARGET_SHAPE_K` | `0.5` | 保持不变 |
| `P011_TARGET_SHAPE_TAU_H_SECONDS` | `0.100` | `100ms` 的秒制表示 |
| `P011_TARGET_SHAPE_TAU_L_SECONDS` | `0.060` | `60ms` 的秒制表示 |
| `P011_CENTRAL_ROI_FRACTION` | `0.80` | 形状与 green-ROI 预检均使用中央 80% |

### 3.2 新增内部状态

`WaterSandField` 新增以下私有状态：

```ts
private phaseHeavyBaseline: Float64Array;
private phaseLightBaseline: Float64Array;
private commandPhaseId = 0;
private commandPhaseSign = 0;
private commandPhaseStartPhysicsTime = 0;
private phaseHeavyIdeal = 0;
private phaseLightIdeal = 0;
private phaseHeavyActual = 0;
private phaseLightActual = 0;
private lastHeavyFlux = 0;
private lastLightFlux = 0;
private lastHeavyShapeRms = 0;
private lastLightShapeRms = 0;
private lastHeavyShapeMaxDelta = 0;
private lastLightShapeMaxDelta = 0;
private lastMaxSubstepBh = 0;
private lastMaxSubstepBl = 0;
```

状态语义：

1. `phaseHeavyActual` / `phaseLightActual` 即算法中的 `C_H` / `C_L`，按 `directionSign * movedAmount` 累计。
2. `phaseHeavyIdeal` / `phaseLightIdeal` 即 `Cstar_H` / `Cstar_L`，只在有效倾斜相位内累计。
3. `lastHeavyFlux` / `lastLightFlux` 是当前子步实际迁移量，每个子步开始清零。
4. `phaseHeavyBaseline` / `phaseLightBaseline` 是相位起点的 `bh` / `bl` 快照。
5. `lastMaxSubstepBh` / `lastMaxSubstepBl` 是当前子步内列级最大绝对变化，每个子步开始清零。

### 3.3 相位边界

1. `commandPhaseId` 在 run/reset 初始化时重置为 `0`。
2. 首次从 `targetTilt=0` 进入非零目标时，`commandPhaseId += 1`，记录相位符号并复制 baseline。
3. 目标从 `+` 变为 `-` 或从 `-` 变为 `+` 时，`commandPhaseId += 1`，复制新 baseline。
4. `0° hold`、dormant no-op、零预算或方向 debounce 不关闭当前相位，也不清零 `C_H/C_L`。
5. 只有 run/reset 或目标符号翻转才重置相位累计与 baseline。

## 4. 核心算法与接口

### 4.1 拆分函数

替换当前 `splitBottomLightFirst` 调用点，新增内部方法：

```ts
private splitBottomPhaseBalanced(
  donor: number,
  amount: number,
  heavyResidual: number,
  lightResidual: number
): WaterSandSplitResult
```

精确实现语义：

```text
deposited = max(bh[donor] + bl[donor], EPS)
baseShare = clamp(bh[donor] / deposited, 0.35, 0.65)
heavyShare = clamp(baseShare + 0.10 * (heavyResidual - lightResidual), 0.35, 0.65)

heavy = min(amount * heavyShare, bh[donor], amount)
light = min(amount - heavy, bl[donor])
remainder = amount - heavy - light

extraHeavy = min(remainder, bh[donor] - heavy)
heavy += extraHeavy
remainder -= extraHeavy
extraLight = min(remainder, bl[donor] - light)
light += extraLight
remainder -= extraLight

if remainder > EPS:
    reject as model/capacity error; do not silently exceed donor stock

bh[donor] -= heavy
bl[donor] -= light
bh[receiver] += heavy
bl[receiver] += light
```

工程要求：

1. 拆分发生在 independent bottom budget 与 total motion budget clip 之后。
2. heavy/light 均受 donor 存量与 receiver 容量约束。
3. 不引入排序、卷积或额外物理 pass。
4. `kBias=0.10` 固化为常量；不得通过配置、环境变量或测试脚本调参。

### 4.2 残差整形

有效倾斜相位内：

```text
idealStep = 0.5 * abs(targetTilt) * dt
Cstar_H += idealStep
Cstar_L += idealStep
eH = Cstar_H - C_H
eL = Cstar_L - C_L

requestRate = clamp(
  0.5 * abs(targetTilt) + max(eH / 0.100, eL / 0.060),
  0,
  10
)
```

实现必须使用秒制时间常数，避免把 `100` / `60` 当作秒。dead zone 内 `requestRate=0`，但不清空当前相位累计。

### 4.3 遥测接口

扩展 `WaterSandP011Metric`，保持 hook 签名不变：

```ts
export interface WaterSandP011Metric {
  // existing identity, tilt, budget, totals fields remain unchanged
  commandPhaseId: number;
  commandPhaseSign: number;
  commandPhaseStartPhysicsTime: number;
  heavyFlux: number;
  lightFlux: number;
  heavyActualDisplacement: number;
  lightActualDisplacement: number;
  heavyIdealDisplacement: number;
  lightIdealDisplacement: number;
  heavyResidual: number;
  lightResidual: number;
  heavyShapeRms: number;
  lightShapeRms: number;
  heavyShapeMaxDelta: number;
  lightShapeMaxDelta: number;
  maxSubstepBh: number;
  maxSubstepBl: number;
  heavyShare: number | null;
  heavyShareDeficit: number;
}
```

字段定义：

| 字段 | 定义 |
|---|---|
| `heavyFlux` / `lightFlux` | 当前子步 heavy/light 实际迁移量 |
| `heavyActualDisplacement` / `lightActualDisplacement` | 相位内 signed `C_H` / `C_L` |
| `heavyIdealDisplacement` / `lightIdealDisplacement` | 相位内 `Cstar_H` / `Cstar_L` |
| `heavyResidual` / `lightResidual` | `eH` / `eL` |
| `heavyShare` | `bottomFlux > EPS ? heavyFlux / bottomFlux : null` |
| `heavyShareDeficit` | `heavyShare === null ? 0 : max(0, 0.35 - heavyShare)` |

形状指标在 central 80% 列上计算：

```text
start = floor(cols * 0.10)
endExclusive = cols - floor(cols * 0.10)

heavyShapeRms =
  sqrt(mean((bh[c] - phaseHeavyBaseline[c])^2))
heavyShapeMaxDelta =
  max(abs(bh[c] - phaseHeavyBaseline[c]))
lightShapeRms =
  sqrt(mean((bl[c] - phaseLightBaseline[c])^2))
lightShapeMaxDelta =
  max(abs(bl[c] - phaseLightBaseline[c]))
```

`P011_BOTTOM` 保持现有数组字段，但仅作为诊断数据；checker 不得把它作为 H/L 响应或 `maxSubstep` 的必要证据。

## 5. 模块与文件变更

| 文件 | 变更 |
|---|---|
| `entry/src/main/ets/physics/WaterSandField.ets` | 升级 route；新增相位状态与标量遥测；替换 light-first 拆分；修正 tau 单位；保持预算与守恒逻辑 |
| `entry/src/main/ets/pages/SandCanvas.ets` | 不改渲染与线程模型；`P011_METRIC` 因接口扩展自动携带新字段；绿色 `bh` 渲染逻辑不变 |
| `entry/src/main/ets/physics/P011TiltWant.ets` | 无语义变更；仅如注释需要，将版本说明更新为 v2.8 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | 无接口变更；继续负责 Want 解析与 AppStorage 单槽命令 |
| `entry/src/test/WaterSandFieldTest.ets` | 更新 route 期望；新增 phase split、share floor、相位累计、形状遥测与守恒测试 |
| `entry/src/ohosTest/ets/test/WaterSandFieldTest.ets` | 同步 route 与关键端侧行为断言 |
| `test/p011_check.py` | 升级为 schema v2；默认 route v2.8；解析 compact H/L 标量；拒绝 route/字段混用 |
| `test/p011_tc02_formal_suite.ps1` | 输出目录与报告路径改为 v2.8 独立命名；承载 canonical TC02 注入/采集/precheck 生命周期，提供 schedule-precheck-only 入口 |
| `tools/run_p011_v28_preflight.ps1`（新增） | 执行 ±15° 各 2 秒的观测链路预检 |
| `test/p011_v28_preflight_check.py`（新增） | 解析 hilog 与截图，输出 green ROI centroid 与 phase telemetry 摘要 |
| `test/p011_check.py`（TC02 schedule precheck） | 只接受冻结文件；校验 captureFrozen 与 size/mtime/SHA256 前后审计；拒绝活动句柄、管道流或缓存事件 |

## 6. 数据流与时序

### 6.1 输入到物理

1. `EntryAbility` 解析 `wstilt` Want，校验 `rendermode=4 && testmode=true`。
2. 合法命令写入 `p011TiltWantCommand` 与单调 `p011TiltWantCommandSeq`。
3. `SandCanvas` frame callback 消费命令，设置 `targetTilt`。
4. `WaterSandField.step()` 在现有子步内执行：tilt 更新 → motion request 收集 → total budget 缩放 → bottom split → state update → telemetry。
5. 物理仍在现有 UI frame callback 中执行；新增成本为常数标量和每子步一次 `O(cols)` 形状统计，不新增 worker。

### 6.2 遥测到报告

1. `WaterSandField` 每个有效子步输出 compact `P011_METRIC`。
2. `P011_BOTTOM` 数组继续输出，但允许 hilog 截断；截断不构成 H/L 判定失败。
3. objective checker 只使用 metric 标量计算 H/L 响应、形状与 `maxSubstep`。
4. checker 输出独立 JSON/Markdown，不覆盖 v2.7.3 报告。

### 6.3 green-ROI preflight

执行序列：

```text
reset / 0° baseline -> +15° hold 2s -> 0° return
                   -> -15° hold 2s -> 0° return
```

每段至少捕获一帧 `snapshot_display`，并连续捕获 hilog。帧与最近 `P011_METRIC.sequence` 对齐。

green ROI 指标定义为中央 80% 区域内 heavy-green 像素的面积质心横坐标。该实现把 designer 的“绿色 `bh` 边界质心”落为可直接测量的 heavy 带分布质心，避免平坦边界点的横坐标在左右列均有覆盖时发生抵消；这不改变阈值或主观语义：

```text
greenCentroidXPx = sum(pixelX) / greenPixelCount
positiveDeltaPx = centroidXPx(+15° hold) - centroidXPx(0° baseline)
negativeDeltaPx = centroidXPx(-15° hold) - centroidXPx(0° baseline)
```

图像分类使用固定 RGB mask：

```text
20 <= R <= 80
65 <= G <= 115
15 <= B <= 65
G > R + 15
G > B + 15
```

该 mask 针对 `#24571F` heavy deposited 带；不得把水/黄沙 ROI 纳入 green centroid。预检输出：

```json
{
  "schemaVersion": 1,
  "routeId": "bottom-creep-v2.8-heavy-interface",
  "status": "OBSERVATION_READY",
  "positive": {
    "greenCentroidDeltaPx": 0.0,
    "heavyFluxNonZeroSamples": 0,
    "heavyShapeRms": 0.0,
    "heavyShapeMaxDelta": 0.0
  },
  "negative": {
    "greenCentroidDeltaPx": 0.0,
    "heavyFluxNonZeroSamples": 0,
    "heavyShapeRms": 0.0,
    "heavyShapeMaxDelta": 0.0
  }
}
```

预检只判定观测链路是否 ready；不得替代 objective suite 或 subjective EC-TT-01~EC-TT-05。

## 7. Checker 与报告隔离

### 7.1 Checker schema

`test/p011_check.py` 输出 `schemaVersion=2`，默认：

```text
routeId = bottom-creep-v2.8-heavy-interface
```

required metric fields 增加：

```text
commandPhaseId, commandPhaseSign, commandPhaseStartPhysicsTime,
heavyFlux, lightFlux,
heavyActualDisplacement, lightActualDisplacement,
heavyIdealDisplacement, lightIdealDisplacement,
heavyResidual, lightResidual,
heavyShapeRms, lightShapeRms,
heavyShapeMaxDelta, lightShapeMaxDelta,
maxSubstepBh, maxSubstepBl,
heavyShare, heavyShareDeficit
```

### 7.2 H/L 特征隔离

1. TC02 的 heavy response 使用 `directionSign * heavyFlux`。
2. TC02 的 light response 使用 `directionSign * lightFlux`。
3. 禁止读取 `bottomCreepDonorEffectiveDisplacement` 或 receiver 聚合数组后复制为 H/L。
4. `variance(C_H) <= 1e-12` 或 `variance(C_L) <= 1e-12` 时输出 `INCOMPLETE(degenerateMeasurement)`，不给物理行为计分。
5. `maxSubstepBh/maxSubstepBl` 优先使用 compact metric 标量；数组仅用于一致性诊断。

### 7.3 v2.8 新增客观观测

1. 每个 ≥2 秒有效正/负相位：`heavyFlux > 0` 样本数 ≥30。
2. `sign(C_H)` 与相位目标符号一致，方向错误为 0。
3. central 80% 列：`heavyShapeRms >= 0.5` 且 `heavyShapeMaxDelta >= 1.0`。
4. `maxSubstepBh` 继续满足现行 TC03 平滑阈值。
5. 现有 Pearson、lag、FE、方向、预算、守恒、非有限值、out-of-range、model error、crash/ANR 与 TC01/TC02/TC03 分档阈值不变。

### 7.4 报告命名

| 类型 | 文件 |
|---|---|
| objective report | `docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md` |
| subjective report | `docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md` |
| preflight evidence | `demo/effect/P011-v28-heavy-interface/preflight/` |

报告必须记录 source commit、signed HAP SHA256、device、OS/API、每个原始 log SHA256、routeId、runId 与 resetEpoch。不同 route 的日志与结论不得合并。

### 7.5 TC02 观测协议闸门与 `input_state` 隔离

依据 `docs/algorithm-P011.md` §5.7/§5.7.3，TC02 schedule precheck 与物理评分必须分开执行；precheck FAIL 时不得产出或展示 TC02 物理分数。

#### 7.5.1 命令唯一来源与语义保持

1. 只有同一 run 内新消费到的 `wstilt` Want 才能产生 `P011_EVENT input` 且 `source=script`；该事件只代表命令注入，不代表帧循环派生状态。
2. 输入恢复、保持、平滑、force-invert 退场与零复位帧必须保持现有输入恢复/衰减语义，但不得写 `input/source=script`，也不得修改 `tiltX` 后复用该事件语义解释为命令。
3. 禁止通过改变事件名、清空字段或把非命令帧合并进 script 队列来掩盖 546/531 差异；修复边界必须在 test-mode 的 Want 消费与帧循环状态恢复之间隔离。

#### 7.5.2 遥测隔离

1. 新鲜 Want 命令仍输出 `P011_EVENT input/source=script`，并携带 run 内唯一递增 `sequence`、schedule 目标角与注入时间。
2. 帧循环状态恢复、保持或衰减帧如需诊断，输出 `P011_EVENT input_state`；checker 不得将其计入 script 事件、Pearson、lag、FE、direction、accepted-gap 或任何命令计数。
3. `input_state` 是补充诊断字段，不是 `script` 的别名；checker 必须按事件类型显式解析，未知或混写类型输出 `INCOMPLETE(telemetryProtocol)`。

#### 7.5.3 Schedule precheck 1:1 校验

在同一份原始 log、同一 `runId` 上执行 schedule precheck，全部满足后才进入物理评分：

1. `source=script` 事件数等于 531 条 schedule 行数。
2. script 事件与 schedule 行按 `sequence`/注入顺序 1:1 对齐；每个事件携带的 target 与 schedule target 完全相等。
3. 相邻 script 事件间隔与 schedule 间隔的误差不超过现行 accepted-gap 阈值；不允许用 `input_state` 填补缺失。
4. run 内 script `sequence` 连续、无重复、无回退。
5. 若出现第二个 `runId`、进程重启、reset marker 或任一事件缺失，判 `FAIL(schedulePrecheck)`；禁止跨 run 拼接物理解或用总事件数平均掩盖差异。

#### 7.5.4 复测闸门顺序

1. Tester 先运行 TC02 schedule precheck，并在报告输出 identity/count/gap/target 与 `input_state` 排除统计。
2. precheck 全部 PASS 后，才允许进入 TC02 物理评分、压力段统计或主观复测。
3. TC02 物理评分继续使用 §7.2/§7.3 现行 H/L 特征与阈值；压力段与 suite 聚合不因 precheck 修复而调整。
4. green-ROI preflight 仍先于完整 objective suite，但不替代本节 schedule precheck。

#### 7.5.5 最小代码边界

1. `WaterSandField` 物理算法、heavy/light split、预算、守恒、方向语义与现有阈值不改。
2. 只允许在 test-mode 观测路径隔离 Want 命令事件与帧循环 `input_state` 事件；生产 UI/输入路径不因该修复改变语义。
3. checker 新增 schedule precheck 与事件类型隔离逻辑，不修改历史 checker 结论或历史报告。

#### 7.5.6 采集冻结与全量/precheck 等价性

第二次 formal 复测的 `984/2-runId/23ms` 是在可增长 hilog 捕获上校验得到的非终态结果；等待同一采集生命周期结束后，冻结 log 复验为 `531/1-runId/116ms/0 violations`。因此采集生命周期属于 schedule precheck 的一部分，必须显式建模：

1. formal suite 与独立 schedule precheck 共用同一 schedule CSV、注入命令模板、shell 间隔和 hilog 参数；不得用轻量 precheck 验证另一条采集路径后，再让 formal suite 隐式使用不同路径。
2. TC02 注入脚本是同步任务：必须等待 `hdc shell sh` 返回，且 531 条命令执行完毕后才能停止 hilog。
3. 停止 hilog 后将原 log 冻结为唯一证据；校验前至少 1 秒复查 size/mtime，校验读取前后比较 size/mtime/SHA256。不一致时判 `FAIL(captureNotFrozen)`，不允许输出物理解。
4. runner 必须检测同一 target/output 的残留 hilog 或旧采集句柄；发现残留时先终止并重置目标输出，再开始新 run。
5. validator 只接受冻结后的完整文件输入，不接受运行中文件句柄、管道流或缓存事件；validation JSON 必须输出 captureFrozen、preParseSize、postParseSize、preParseMtime、postParseMtime、preParseSha256、postParseSha256。
6. 该闸门不新增物理阈值，也不改变 531、sequence、100ms 或 target 一致性要求。

#### 7.5.7 Canonical runner 边界

1. `test/p011_tc02_formal_suite.ps1` 是 TC02 的唯一 lifecycle runner：残留采集清理、hilog 启动、同步 Want 注入、注入完成等待、hilog 停止、文件冻结、冻结审计和 schedule precheck 必须在同一实现路径中顺序执行。
2. 全量 suite 的 TC02 与独立 schedule precheck 都必须调用该 runner；独立模式只允许改变报告/证据输出目录或 `schedule-precheck-only` 开关，不允许替换注入脚本、shell 间隔、采集参数或冻结校验实现。
3. runner 在开始前必须记录并校验 canonical 输入身份：schedule CSV SHA256、注入脚本 SHA256、注入命令模板、shell 间隔和 hilog 参数。formal suite 证据与 precheck 证据必须携带同一组身份；任一不同即判 `FAIL(capturePathMismatch)`。
4. validation JSON 必须把 `captureFrozen`、pre/post 的 size、mtime 与 SHA256 输出在 `captureAudit` 中；这些字段与 531/sequence/gap/target 结果共同构成 schedule precheck PASS 的必要条件。
4. 修复不得引入额外物理遥测语义、放宽阈值，也不得把 `input_state` 数据复制为 script 事件。

## 8. 线程模型与性能

1. 不新增 worker；`WaterSandField` 继续由 `SandCanvas` frame callback 驱动。
2. phase split 是每个相邻对的常数操作，整体仍为 `O(cols)`。
3. 形状指标每个子步一次 `O(cols)` 遍历，与现有 state update 同量级。
4. compact metric 预计小于 1.5KB，先于长数组输出，避免 hilog 截断影响判定。
5. 若 `step()` 返回 model error，UI 保持现有错误路径；不得用静默重放或预算提升掩盖失败。

## 9. 异常处理与降级

| 异常 | 处理 |
|---|---|
| donor heavy 不足 | remainder 转入 light，`heavyShareDeficit` 记录，不提高预算 |
| donor light 不足 | remainder 转入 heavy，仍受存量与容量约束 |
| heavy/light 均不足 | 返回 model/capacity error，不伪造 flux |
| total budget clip | 只缩小实际授予量，不调整 `kBias` 或份额上下限 |
| telemetry 字段缺失 | checker 输出 `INCOMPLETE`，不得按 0 分或 PASS 处理 |
| `P011_BOTTOM` 截断 | 不影响 H/L 判定；数组仅诊断 |
| green ROI 分类失败 | preflight 输出 `BLOCKED(roiObservation)`，不得推断算法失败或成功 |

## 10. 实施步骤

1. 将 route 常量与默认参数升级为 `bottom-creep-v2.8-heavy-interface`。
2. 扩展 `WaterSandP011Metric` 与内部相位状态。
3. 实现相位 baseline 与 signed ideal/actual 累计。
4. 替换 light-first split 为 `splitBottomPhaseBalanced`。
5. 每个子步累计 heavy/light flux 与 `maxSubstepBh/bl`，每个 metric 输出形状标量。
6. 更新 unit 与 ohos tests，先验证守恒、预算、方向与 share floor。
7. 升级 checker schema v2 与 H/L 独立特征。
8. 新增 preflight runner/analyzer，并执行一次 ±15° 观测链路预检。
9. 将 TC02 formal suite 重构为 canonical 注入/采集/precheck runner，并为独立 precheck 暴露同一 lifecycle 的 `schedule-precheck-only` 入口。
10. 校验冻结审计字段、残留采集检测、formal/precheck 输入身份一致性后，再执行完整 suite。
11. 生成 v2.8 objective 与 subjective 独立报告；不得复用或覆盖 v2.7.3 文件。

## 11. 验证清单

### 11.1 Developer 本地验证

1. `python test/p011_check.py --self-check`
2. WaterSandField unit tests：route、守恒、独立/总预算、方向、phase split、share floor、形状遥测。
3. Ohos tests：route 与端侧行为断言。
4. 构建 signed HAP 后记录 source commit 与 SHA256。
5. 验证 test-mode 中新鲜 Want 才输出 `input/source=script`；恢复、保持、平滑与零复位帧只输出 `input_state` 且不被 checker 消费。
6. 验证 TC02 formal suite 与 `schedule-precheck-only` 输出同一 capturePath 身份、先等待注入完成、冻结前后 size/mtime/SHA256 一致，并能拒绝残留 hilog 或活动文件句柄。

### 11.2 Tester 验证

1. 执行 v2.8 green-ROI preflight。
2. 先执行 TC02 schedule precheck：校验同一 run 内 531/531、sequence identity、gap 与 target 一致，并确认 `input_state` 被排除。
3. 独立 precheck 必须调用 §7.5.7 canonical runner；确认 schedule/injection/hilog 身份与 formal suite 相同，且 validation JSON 包含 `captureFrozen` 与完整前后 size/mtime/SHA256 审计。
4. precheck PASS 后执行完整 objective suite，所有 log 一次性传入 checker；不得重新采集或修改已冻结的 TC02 证据。
5. 执行 EC-TT-01~EC-TT-05 subjective review。
6. 独立生成 v2.8 objective/subjective 报告，并记录全部身份与哈希信息。

## 12. 风险与回滚

### 12.1 风险

| 风险 | 影响 | 缓解 |
|---|---|---|
| 35% heavy share 在 full-load 下幅度不足 | heavy shape 不达标 | 不调预算；如实 FAIL 并交 designer 重新校准 |
| residual bias 使 share 长期贴上限 | light 响应被压制 | 输出 `heavyShare` / `lightFlux`，由 checker 判定退化 |
| tau 单位实现错误 | request rate 错误 | 固定秒制常量并加单测 |
| JPEG 颜色分类噪声 | green ROI 误判 | 固定分辨率、固定 ROI、同帧 baseline，并输出 mask 统计 |
| hilog 长数组截断 | 诊断不完整 | checker 只依赖 compact metric |
| 采集未冻结或存在残留 hilog 句柄 | 事件数、sequence 与 gap 是非终态样本 | canonical runner 先检测残留、同步等待注入完成并强制冻结前后审计 |
| route/report 混用 | 历史结论污染 | schema v2 强制 route 与字段一致性 |

### 12.2 回滚

1. 回滚 `WaterSandField.ets`、tests、checker 与 v2.8 preflight 脚本。
2. 不回滚或改写 v2.7.3 objective/subjective 报告。
3. 若 v2.8 行为失败，保留 routeId 与报告，下一版必须再次升级 routeId。
4. 禁止在 v2.7.3 routeId 下发布 v2.8 行为变更。

## 13. 跨角色交接

### Developer

1. 按 §4–§6 实现，不调整阈值、预算或算法语义。
2. 固化 `kBias=0.10` 并在代码常量中记录。
3. 保持 hook 签名与 UI 线程模型不变。
4. 提交前完成 §11.1。
5. 按 §7.5.5 只在 test-mode 观测路径实现命令/状态遥测隔离与 schedule precheck；不得修改物理行为或放宽阈值。
6. 按 §7.5.7 实现 canonical TC02 lifecycle runner；全量 suite 与独立 precheck 必须调用同一注入/采集路径，不得复制第二套实现。

### Tester

1. 先执行 green-ROI preflight，再执行完整 objective suite 与 subjective review。
2. TC02 必须先执行 §7.5.3/§7.5.6 schedule precheck；只有 capture lifecycle、identity/count/gap/target 全部 PASS 才进入物理评分、压力段或 subjective review。
3. 分别生成 §7.4 的两个独立报告。
4. checker 使用 schema v2 与 v2.8 route，不混入历史日志。
5. 保持 HG-TT-04 原语义；不得因水/黄沙改善而改判。

### Effector / Review

1. 评审证据必须同时标出 water、yellow light、green heavy 三个 ROI。
2. green ROI 必须给出时间序列或逐帧 diff。
3. 若只提交视频，至少提供 0°、正倾、0°、负倾、0° 五个关键帧及 metric sequence 对齐表。
4. 不得跨 route 合并结论。

## 14. 变更记录

| 日期 | Route | 记录 |
|---|---|---|
| 2026-10-15 | `bottom-creep-v2.6` | REJECTED；方向语义、预算隔离与观测不足，不得调参复用 |
| 2026-10-16~17 | `bottom-creep-v2.7` 系列 | 逐步修复方向、预算、Want 注入、遥测身份、suite 聚合与 checker 口径 |
| 2026-10-18 | `bottom-creep-v2.7.3` | objective PASS，subjective FAIL（HG-TT-04）；确认绿色层绑定 `bh` 而 light-first 拆分导致 `bh` 静止 |
| 2026-10-18 | `bottom-creep-v2.8-heavy-interface` | 工程审批通过：新增 phase-balanced split、35% heavy floor、compact phase telemetry、schema v2 checker isolation 与 green-ROI preflight；状态 `READY_FOR_DEVELOPER` |
| 2026-10-18 | `bottom-creep-v2.8-heavy-interface` | 依据算法 §5.7 落地 TC02 观测协议闸门：新增 §7.5，隔离 `source=script` 与 `input_state`，要求 531 行 1:1 schedule precheck 与先闸后评分顺序；状态保持 `READY_FOR_DEVELOPER` |
| 2026-10-18 | `bottom-creep-v2.8-heavy-interface` | 依据第二次 formal FAIL 归因新增 §7.5.6：采集冻结、全量/precheck 等价注入与冻结前后 hash 审计；禁止在活动日志上判定 schedule precheck；状态保持 `READY_FOR_DEVELOPER` |
| 2026-10-18 | `bottom-creep-v2.8-heavy-interface` | 新增 §7.5.7：formal suite 与独立 precheck 必须共用 canonical TC02 注入/采集/precheck runner，携带同一 capturePath 身份与 captureAudit；状态保持 `READY_FOR_DEVELOPER` |
| 2026-10-19 | `bottom-creep-v2.9-central-dune-heavy-interface` | 工程审批 `BLOCKED`：bottom-first 预算预约与相邻输运可工程化，但 `A=2.7,K=2` 与现行 30° 崩塌不变量冲突，且 residual 清零不代表实际 heavy 形状仍达标；详见 §16 |

## 15. dual-input-v1 工程补充（2026-10-18）

状态：`READY_FOR_DEVELOPER`。依据：`algorithm-P011.md` §5.8、`retro-P011-v2.8-dual-input-v1.md`、`test-cases.md` §10 MODE-01~07 及客观报告新增 RED 基线。本节覆盖输入路径的旧说明；物理路线仍为 `bottom-creep-v2.8-heavy-interface`。manager 已确认测试先行资产就位，新增运行验收尚未通过。

### 15.1 文件与职责边界

| 文件 | 最小职责 |
|---|---|
| `entry/src/main/ets/physics/P011InputMode.ets`（新增） | 不依赖 ArkUI/AppStorage/sensor 的纯状态 helper；模式转换、sensor 接受、script 消费及手势所有权判断，可直接行为单测 |
| `entry/src/main/ets/physics/LiquidCAWant.ets` | `LiquidCAWantCommand` 增加可选 `p011TiltWantSequence?: number`，只传递当前 Want 命令身份，不改 P001 算法或历史协议 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | 先解析本次模式与合法倾角，再携带身份发布模式命令，最后发布该倾角；冷启动同样保存身份 |
| `entry/src/main/ets/pages/SandCanvas.ets` | 所有模式入口统一；接入 helper；回调、消费端和 Pan 双向 gate；显示两模式与真实输入来源 |
| `entry/src/main/ets/physics/WaterSandField.ets` | 仅增加下述输入缓存清理方法，不改物理计算、材料数组、预算或阈值 |
| `entry/src/test/P011InputModeTest.ets`（新增）及 `List.test.ets` | helper 状态机行为测试与注册；现有 WaterSandField 测试增加输入清理不改变材料与身份的断言 |

canonical host schedule、真实 `aa start --ps wstilt`、capture lifecycle 继续复用 §7.5.7；本子项不得内置场景引擎、生成伪 script 事件，或另建一条采集路径。不要掺入 P001 历史重构。

### 15.2 最小可测试接口

以下为开发契约；可沿用现有字段命名，通过组件中的读写适配器调用纯函数，不要求搬迁整页状态。`testMode` 仍为唯一模式真值，`useSensor` 必须由其推导。helper 的返回状态必须写回真实组件，不能只用于测试。

```ts
export type P011ModeRequestSource = 'startup' | 'want' | 'panel' | 'longPress';

export interface P011InputModeState {
  testMode: boolean;
  useSensor: boolean;
  scriptTargetTilt: number | null;
  consumedSequence: number;
  tiltX: number;
  lastTiltX: number;
  forceInvert: boolean;
  wasForceInvert: boolean;
  gestureHoldUntil: number;
  sensorGx: number;
  sensorGy: number;
  hasFreshSensorSample: boolean;
  lastSensorCommandTime: number;
  lastTiltCommandTime: number;
}

export interface P011ModeTransitionResult {
  state: P011InputModeState;
  changed: boolean;
}

export function transitionP011InputMode(
  state: P011InputModeState, nextTestMode: boolean,
  pendingSequence: number, currentWantSequence: number
): P011ModeTransitionResult;

export function acceptP011SensorSample(
  state: P011InputModeState, x: number, y: number,
  sampleTime: number, smoothingAlpha: number
): P011InputModeState;

export interface P011InputSelection {
  state: P011InputModeState;
  source: 'sensor' | 'script' | 'gesture' | 'idle';
  targetTilt: number | null;
  freshScriptCommand: P011TiltWantCommand | null;
}

export function selectP011Input(
  state: P011InputModeState, pendingSequence: number,
  command: P011TiltWantCommand | null
): P011InputSelection;

export function canP011GestureOwnInput(state: P011InputModeState): boolean;
```

`P011TiltWantCommand` 直接 import 既有类型；函数不调用时钟，时间由真实调用者传入。`pendingSequence` 是当前存储 watermark；`currentWantSequence=0` 表示本次入口没有合法新倾角，不能用时间戳猜测身份。返回新状态是纯函数语义，不改变输入对象；数据结构均固定大小，事件级 `O(1)`。无需额外每帧数组或 worker。

函数语义：

1. `transitionP011InputMode` 在 `nextTestMode===state.testMode` 时返回未清理状态、`changed=false`；调用方仍可处理本次合法新命令。模式变化时，清 `scriptTargetTilt=null`、`forceInvert/wasForceInvert=false`、`gestureHoldUntil=0`、`tiltX/lastTiltX=0`、`sensorGx/sensorGy=0`、`hasFreshSensorSample=false`、两个输入时间为 0；设置 `testMode=nextTestMode,useSensor=!nextTestMode`。消费 watermark 不回退、不重置为 0。
2. 切换进入 TEST 且当前合法新命令身份 `q>consumedSequence` 时，只废弃 `<q` 的旧命令：watermark 推进至 `max(consumedSequence,q-1)`，保留 q。前置条件：q 是本次 Want 解析结果且与本次发布命令一致。其他切换（含面板/长按、进入 EXPERIENCE）推进至 `max(consumedSequence,pendingSequence)`，废弃所有旧 pending。
3. `acceptP011SensorSample` 在 TEST 直接返回原状态，不能更新时间或滤波。EXPERIENCE 仅接受有限 x/y、有效真实 callback 时间；按既有 `oldWeight=1-alpha,newWeight=alpha` 更新滤波，记录该 sampleTime，并置 fresh 标志。无效数据忽略，不生成替代时间。
4. `selectP011Input` 在 EXPERIENCE 每帧废弃 pending、强制 `forceInvert/wasForceInvert=false,useSensor=true`，不消费 script；有 fresh 样本选择 sensor（`targetTilt=null`，由既有映射求角度），否则选择 idle、`targetTilt=0`。不能用缓存 script 或当前时钟伪造 sensor。
5. TEST 中仅在非 forceInvert 且 command 身份等于 pendingSequence、严格大于 consumedSequence、角度与时间有效时消费一次：更新 watermark 与 held target，输出 `freshScriptCommand=command,source=script,targetTilt=command.targetTilt`。重复/非法命令不得发事件，非法 pending 推进 watermark 以免反复重试。无新命令但 held target 非空时继续 script target，`freshScriptCommand=null`；不自动衰减。
6. TEST forceInvert 使用既有测试倒立路径，不能消费冲突 script。手势拥有权仅在 `testMode && scriptTargetTilt===null && !forceInvert` 时为 true；其他情况 Pan update/end 都无状态副作用。held script 没有新增“结束”按钮，本轮需要手势调试时明确切换模式再进入 TEST，不能由 Pan 偷夺控制。

### 15.3 当前 Want 身份与发布时序（选定显式 sequence 方案）

不要只调换两个 AppStorage 写入顺序后假定原子性。身份必须随模式命令传递：

1. `EntryAbility.applyWant` 先完成 `parseLiquidCAWant` 和可选 `parseP011TiltWant`。仅合法倾角得到本次 `q`；无合法命令身份为 0。沿用现有倾角 sequence 分配和校验，不重置 sequence，不改变有效角度范围。
2. 设置初始存储 `testmode` 等，并写 `p011TiltWantModeSequence=q`。热 Want 的 `LiquidCAWantCommand` 携带 `p011TiltWantSequence=q`，先发布该 mode command 与其 sequence。
3. 最后发布本次合法 `p011TiltWantCommand` 与其 sequence；不存在合法命令时不覆盖旧倾角对象，由消费端清理旧 pending。
4. `SandCanvas.aboutToAppear` 从初始 `testmode` 和 `p011TiltWantModeSequence` 调用统一转换；`processLiquidWantCommand` 使用该 command 自带的 `p011TiltWantSequence` 调用转换，不能读取“最近时刻”来猜当前 Want。本地 `LiquidWantCommand` 类型同步该可选字段，缺字段按 0 处理。
5. frame 顺序固定为：消费 mode command → 完成输入清理/输入缓存清理/UI 同步 → 读取倾角 pending → helper 选择输入 → 既有物理/渲染。当前热 Want 处理和 sensor callback 在既有 UI 事件循环串行，不在存储字段的 watcher 内提前消费半份数据；本轮不增加跨线程写入。
6. 冷启动首个 wstilt 同样以 q 身份保留，页初始化/建场不得随后把 watermark 无条件清到 q。相同模式连续 Want 不清 held target、runId、输入序列或物理累计；每条合法命令恰好产生一个原协议 script 事件。

### 15.4 组件集成与内部输入缓存

`SandCanvas` 增加私有 `applyInputMode(nextTestMode: boolean, source: P011ModeRequestSource, currentWantSequence: number = 0): void`。启动、面板 `toggleTestMode`、长按和热 Want 仅经此函数改变模式；按钮与长按不能复制状态赋值分支。变化后写回 helper 状态，`lastWaterSandTilt=null`，同步 `AppStorage.testmode/forceinvert`、`liquidTelemetry.setTestMode`、`syncSheet()`，请求重绘。

只有 `changed=true` 才清内部输入缓存，并输出一条独立边界诊断，例如 `P011_MODE boundary=true inputMode=TEST source=want pendingDiscardedThrough=<watermark> retainedWantSequence=<q>`。它不是 `input/source=script`、不是 reset marker、不增 resetEpoch，不进入 schedule 计数。边界前后不拼接连续输入延迟/跳变统计；正式 TC02 run 不应中途切模式。

`WaterSandField` 增加精确接口：

```ts
resetInputState(): void;
```

该方法只置 `targetTilt=effectiveTilt=effectiveTiltRate=0`、`bottomCreepDirection=0`，输入方向/保持时间缓存以当前 `simulationTime` 为新基点（`lastBottomDirectionChange`、`lastNonZeroTargetTime`、`zeroTargetSince`）。唤醒 `dormant=false` 以便下次输入正常处理；不调用 `reset/initialize/invert180/beginCommandPhase`，不遍历材料。`bh/bl/waterTop/mh/ml/suspH/suspL`、shape 累计、预算、simulationTime、runId、resetEpoch 与 frame sequence 保持原值。清理本身不执行 step、不生成材料位移。材料场快照与身份单测必须逐字段相等；既有 step 内相位管理随下一条真实输入正常执行。

sensor 保持单订阅；callback 第一条判断 TEST 并早退，再调用接受 helper。模式切换后 fresh 标志为 false，直到真正新 callback 被接受；等待期间 0°，不要用 `nextP011TimeMs()` 填补 `lastSensorCommandTime`。有新 callback 后下一次 frame 立即选择 sensor，不等待旧 2.5 秒/24 小时 hold。映射继续 `sx=-sensorGx,sy=sensorGy`，模长 >0.3 才求 `atan2`，既有 alpha、角速度限制和 physical effectiveTilt 参数不变。

水沙消费端必须再次执行模式 gate；不能只依赖 `P011TiltWant` 生产端 parser。体验中 Pan update/end 在任何 `useSensor`、hold、tilt 或时间赋值前返回。测试中 held script 优先，Pan end 不得开启 sensor；测试 forceInvert 的面板/Want 入口在 EXPERIENCE 均忽略并保持 false。其他模型的物理实现不变。

UI 面板文本固定为“测试模式（模拟姿态）”/“体验模式（传感器）”，与真实 `testMode` 同步；自动程序测试仍从 host 构造姿态。文案不声称内置一键场景，也不把模拟输入证据写成真实传感器端到端结论。

### 15.5 异常、验证与交接

1. 无 sensor/订阅失败/无效样本：体验安全姿态 0°，fresh=false；记录可定位设备限制，不切到 TEST 或旧 script。sensor 时间只来自有效 callback。
2. 无效 command/损坏 JSON/旧 sequence：沿用既有 parser/decode 拒绝，推进相应废弃 watermark；不得重放或发 script 事件。无效 forceInvert 不影响合法体验 sensor 状态。
3. developer 行为测试必须调用产品实际 helper：对比 0 次与 100 次 sensor 干扰、EXPERIENCE 中 script/Pan/forceInvert 对照、双向各 10 次切换、冷/热首命令、连续同模式三条 Want、fresh sensor 恢复，覆盖 MODE-01~07。固定 callback 序列、输入命令与 dt 比较数值轨迹精确相等；不能仅源码匹配。
4. 真实 `SandCanvas` 接线需检查每个入口均使用 helper、模式先于倾角消费、callback gate 在滤波/时间赋值前、Pan 无提前写入。无传感器设备时如实标记真实体验链路待验证，不能用 helper 测试伪证。
5. host 开发既有 canonical/compact 文件独立维护；业务 developer 按本节实施，不复制 host 工具。构建/相关单测完成后 reviewer 审核，tester/effector 按设备串行纪律复测。本节不修改任何 PASS 阈值，不宣布 P-011 闭环。

变更记录：2026-10-18，依据用户双模式授权与算法 §5.8 新增 §15；选定显式当前 Want sequence 保留首命令，统一输入转换与双向隔离，补充纯 helper 和 O(1) 物理输入缓存接口。状态 `READY_FOR_DEVELOPER`。

## 16. v2.9 工程审批与落地方案（2026-10-19）

> **routeId**：`bottom-creep-v2.9-central-dune-heavy-interface`  
> **状态**：`BLOCKED`  
> **依据**：`docs/algorithm-P011.md` §6、`docs/retro-P011-v2.9.md`、v2.8 正式证据与本文 §4~§15。  
> **结论**：bottom-first 预算预约、相邻界面输运、守恒与容量约束均可工程化；但 designer 给定的中央 4 列 dipole 与现行 `relaxAngle` 崩塌不变量存在直接冲突，不能进入 `READY_FOR_DEVELOPER`。

### 16.1 审批结论与冲突审计

| 审计项 | 结论 | 工程判定 |
|---|---|---|
| heavy/light/water 守恒 | 通过 | 只允许相邻 donor→receiver 输运；同一接口按 phase 拆分，`bh/bl` 分相总量不变，不引入跨列 teleport。 |
| donor 存量与 receiver 容量 | 通过 | 每个中央接口独立受限；不足时输出 `duneBlockedInterface` 与剩余 residual，不允许提高预算或静默改目标。 |
| 独立 bottom 预算 | 通过 | `bottomCreepBudgetUsed <= vBottomCreepMax*dt`；预约只改变 total budget 内的应用顺序，不改变上限。 |
| total motion 预算 | 通过 | bottom 先扣 total 预算，其余通道按剩余预算缩放；必须避免 bottom 同时进入 requested 缩放和预留扣减的双重衰减。 |
| 安息角/崩塌角 | **阻断** | `angleDeg=30` 时 `relaxAngle` 阈值为 `tan(30°)≈0.577`；`A=2.7` 的 heavy-only dipole 在中央界面产生 `2A=5.4` 高差，约为阈值的 9.4 倍，必然触发崩塌。 |
| 预算释放条件 | **阻断** | `duneResidual=sum(abs(F-X))` 可测，但只表示计划 flux 是否输送完；它不校验实际 `bh-phaseHeavyBaseline` 是否仍等于 `targetDelta`，清零后会把预算交还崩塌等通道。 |
| green ROI 可见性 | 待实测 | 算法 ASSUMPTION-7 未被模型换算证明；只能由冻结截图/连续录像判定，不能作为解除上述阻断的依据。 |
| 子步平滑 | 待实测 | 单次输运受 `vBottomCreepMax*dt` 限制，预计不违反 `maxSubstepBh`；但崩塌通道介入后的最终形状必须用现行 TC03 复测。 |

#### 16.1.1 阻断项 1：`BLOCKED-V29-ANGLE`

现行实现在每个 application substep 中执行 `relaxAngle`：

```text
limit = tan(angleDeg)
targetSlope = tan(effectiveTilt)
deviation = sandTop[c+1] - sandTop[c] - targetSlope
if abs(deviation) > limit:
    request avalanche from higher column to lower column
```

默认 `angleDeg=30`，正式输入为 ±15°。以 +15° 为例：

```text
tan(15°) ≈ 0.268
tan(30°) ≈ 0.577
触发高差上界 ≈ 0.268 + 0.577 = 0.845
```

v2.9 正方向 heavy 目标为：

```text
bh delta = {-2.7, -2.7, +2.7, +2.7}
```

即使不考虑 light 同 profile 输运，中央两个半 dune 之间的相邻高差也达到 `5.4`，远超 `0.845`；dune 与非目标列的边界同样产生 `2.7` 高差并触发崩塌。按 v2.9 的 bottom-first 顺序，同一 application pass 随后的 `relaxAngle` 即可看到该形状，并以 `maxAvalancheFlux*dt` 持续削平；`totalMotionBudget=300` 足以让该请求获得预算。因此在约 5 秒保持段结束时，中央 heavy RMS/max 大概率不满足 `RMS>=0.5/max>=1.0`。

解除条件必须由 designer 给出，可选方向包括但不限于：

1. 重新设计满足现行角度不变量的目标 profile，并同时保持 RMS/max、green ROI 与预算成本。
2. 在算法层显式定义允许的崩塌语义、幅度与保护窗口，并升级 route；工程侧不得私自豁免 `relaxAngle` 或调高 `angleDeg`。
3. 给出可证明的相位平衡方案，使崩塌后的稳态形状仍满足全部硬门槛。

#### 16.1.2 阻断项 2：`BLOCKED-V29-RELEASE`

算法的释放条件是 `duneResidual <= 1e-4`，其中：

```text
duneResidual = sum(abs(F - X))
```

该条件可以由 telemetry 精确测试，但它只跟踪“计划 cumulative heavy flux 是否输送完”。一旦 `X=F`，后续 `relaxAngle`、settle、percolation 或 rise 通道改变 `bh` 时，`X` 不会回退，`duneResidual` 仍为 0。于是：

1. bottom 停止新增 dune 输运；
2. 未用完的 bottom 预算与剩余 total 预算交还其他通道；
3. 崩塌通道获得预算并继续削平实际形状；
4. checker 仍可能看到 `duneResidual=0`，从而误判“目标已达成”。

因此释放条件可测但不充分。解除条件必须由 designer 明确以下之一：

1. release 改为校验实际 heavy shape residual，例如与 `targetDelta` 的列级误差，且同时定义崩塌后的允许误差；
2. dune 完成后维持某种明确的状态保护或预算预约，直到相位结束，并说明其他通道的公平性与总预算语义；
3. 接受崩塌后的稳态形状作为新目标，并给出对应数学模型与验收阈值。

在 designer 更新前，developer 不得通过关闭 `relaxAngle`、延长 reservation、修改 `F/X` 或调高阈值来绕过阻断。

### 16.2 条件化模块设计（未授权实施）

以下设计保留给 designer 解除 §16.1 阻断后的下一版工程审批使用；本轮不得据此修改业务代码。

#### 16.2.1 常量与内部状态

```ts
const P011_BOTTOM_ROUTE_ID = 'bottom-creep-v2.9-central-dune-heavy-interface';
const P011_DUNE_HALF_SUPPORT_COLUMNS = 2;
const P011_DUNE_TARGET_AMPLITUDE = 2.7;
const P011_DUNE_RESIDUAL_EPS = 1e-4;
```

`WaterSandField` 新增相位状态：

```ts
private duneCenterColumn = -1;
private duneTargetDelta: Float64Array;
private duneTargetFlux: Float64Array;      // length 3, signed left-to-right target
private duneCumulativeFlux: Float64Array;  // length 3, signed actual heavy flux
private duneResidual = 0;
private duneBlockedInterface: number | null = null;
private bottomBudgetReserved = 0;
private bottomBudgetReleased = 0;
private otherMotionBudgetRequested = 0;
```

`duneTargetDelta` 只保存中央 4 列非零目标；`duneTargetFlux` 与 `duneCumulativeFlux` 保存三个中央接口：

```text
interface[j] means column j -> j+1
j ∈ {center-2, center-1, center}
```

正方向 signed flux 为正，负方向为负。`X[j]` 只能由该接口实际 heavy 输运累加，不得由形状反推。

#### 16.2.2 内部方法

```ts
private initializeDuneProfile(sign: number): void;
private collectBottomDuneRequest(substepDt: number): number;
private applyBottomDune(substepDt: number): WaterSandErrorCode | null;
private updateDuneResidual(): void;
private computeOtherMotionScale(): number;
```

语义：

1. `beginCommandPhase(sign)` 调用 `initializeDuneProfile(sign)`，按算法 §6.3 重建 `targetDelta/F/X/residual`。
2. `collectBottomDuneRequest` 在 collect pass 计算本子步 dune 请求，并返回 `bottomBudgetReserved`。
3. `applyBottomDune` 在 application pass 中先于 settle/percolation/relax/rise 执行，只使用三个中央相邻接口。
4. `updateDuneResidual` 只由实际 `X` 更新；若 donor/capacity 使接口无法达到 `F[j]`，记录接口索引并返回观测失败，不回退 v2.8。
5. `computeOtherMotionScale` 在 bottom 实际应用后计算，使用剩余 total 预算，而不是 collect 时的预估剩余。

#### 16.2.3 Bottom-first 预算方程

Collect pass：

```text
otherRequested = sum(non-bottom motion requests)
duneRequested = min(vBottomCreepMax*dt, donor/receiver/capacity limits)

if duneResidual > EPS and abs(targetTilt) > bottomCreepDeadZone:
    bottomBudgetReserved =
        min(duneRequested, bottomCreepBudgetLimit, totalMotionBudgetLimit)
else:
    bottomBudgetReserved = 0
```

Application pass：

```text
bottomUsed = applyBottomDune()
bottomBudgetReleased = max(0, bottomBudgetReserved - bottomUsed)

if otherRequested <= EPS:
    otherScale = 1
else:
    otherScale = min(1, totalMotionBudgetRemaining / otherRequested)
```

不变量：

1. `bottomCreepBudgetUsed <= bottomCreepBudgetLimit`。
2. `totalMotionBudgetUsed <= totalMotionBudgetLimit`。
3. bottom 不先按 `totalMotionBudgetScale` 缩放，也不重复计入 other requested。
4. `totalMotionBudgetClipped` 只表示请求被裁剪，不表示预算 violation；violation 必须为 0。
5. 同一子步内 bottom 未用完的 reservation 留在 `totalMotionBudgetRemaining`，供 other channels 按 `otherScale` 使用。

### 16.3 遥测接口与观测钩子

解除阻断后，扩展 `WaterSandP011Metric`，hook 签名保持不变：

```ts
export interface WaterSandP011Metric {
  // all v2.8 fields remain required
  duneCenterColumn: number;
  duneTargetAmplitude: number;
  duneHalfSupport: number;
  duneTargetFlux: number[];
  duneCumulativeFlux: number[];
  duneResidual: number;
  duneBlockedInterface: number | null;
  bottomBudgetReserved: number;
  bottomBudgetReleased: number;
  otherMotionBudgetRequested: number;
  otherMotionBudgetScale: number | null;
}
```

字段定义：

| 字段 | 定义 |
|---|---|
| `duneTargetFlux` | 三个中央接口的 signed 目标 cumulative heavy flux。 |
| `duneCumulativeFlux` | 三个中央接口的 signed 实际 cumulative heavy flux。 |
| `duneResidual` | `sum(abs(duneTargetFlux-duneCumulativeFlux))`。 |
| `duneBlockedInterface` | 无法达到目标的接口索引；无阻断为 `null`。 |
| `bottomBudgetReserved` | collect pass 预留的 bottom total-budget 份额。 |
| `bottomBudgetReleased` | 同一 application pass 中未使用的 reservation。 |
| `otherMotionBudgetScale` | bottom 应用后按剩余 total 预算计算的 other-channel scale。 |

额外要求：

1. `P011_METRIC` 必须先输出 compact 标量与三个元素的数组，再输出 `P011_BOTTOM` 长数组。
2. checker 必须用 `P011_BOTTOM` 独立重算 central 80% RMS/max，并与 compact metric 差值小于浮点容差。
3. 若 designer 修改 release 语义，必须新增实际 shape residual 字段；不得继续只用 `F-X` 判定完成。

### 16.4 文件与集成点（解除阻断后）

| 文件 | 变更 |
|---|---|
| `entry/src/main/ets/physics/WaterSandField.ets` | 升级 route；新增 dune 状态、预算预约、中央三接口输运与 compact telemetry；不改输入协议。 |
| `entry/src/main/ets/pages/SandCanvas.ets` | 无渲染/线程语义变更；仅随 metric 接口携带新字段。 |
| `entry/src/main/ets/physics/P011InputMode.ets` | 不改；§15 `dual-input-v1` 语义保持。 |
| `entry/src/main/ets/physics/P011TiltWant.ets` | 不改协议。 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | 不改接口；继续 Want 解析与单槽命令发布。 |
| `entry/src/test/WaterSandFieldTest.ets` | 新增 route、守恒、角度不变量、预算预约、release、blocked-interface 与 telemetry 测试。 |
| `entry/src/ohosTest/ets/test/WaterSandFieldTest.ets` | 同步端侧行为与 route 断言。 |
| `test/p011_check.py` | 升级 schema v3，默认 v2.9 route，强制 dune 字段与 route 一致。 |
| `test/p011_tc02_formal_suite.ps1` | 输出 v2.9 独立目录与报告路径；继续复用 canonical 注入/采集/precheck lifecycle。 |
| `tools/run_p011_v29_preflight.ps1`（新增） | v2.9 green-ROI preflight runner；继承已修复的硬闸门逻辑。 |
| `test/p011_v29_preflight_check.py`（新增） | 解析 v2.9 hilog 与截图；不得复制出与 v2.8 不同的 gate 口径。 |

报告路径：

```text
docs/test-report-P011-bottom-creep-v2.9-central-dune-heavy-interface.md
docs/effect-report-P011-bottom-creep-v2.9-central-dune-heavy-interface.md
demo/effect/P011-v29-central-dune-heavy-interface/preflight/
```

### 16.5 线程模型与时序

1. 不新增 worker；`WaterSandField` 仍由 `SandCanvas` frame callback 驱动。
2. dune 请求与应用均为 `O(1)`，shape 遥测沿用既有 `O(cols)`。
3. 每个子步顺序必须为：

```text
collect other motion requests
collect dune request and reserve bottom budget
restore motion snapshot
apply bottom dune first
recompute other scale from remaining total budget
apply settle/percolation/relax/rise with remaining budget
audit conservation/capacity/finite/range
emit compact telemetry
```

4. 任何 model/capacity error 均恢复本子步 backup 并冻结字段，不得静默重放或降低阈值。
5. UI 不等待额外 pass；若现有 frame 时间超限，按性能 FAIL 处理，不得减少遥测或改用非 canonical 输入。

### 16.6 Tester 验证链（先证明阻断，再等复批）

本轮可立即执行的验证：

1. 使用同 route RED 基线证明角度冲突：构造默认 128 列正式几何与 ±15° 相位，断言同时满足现行 `relaxAngle` 不变量和 `heavyShapeRms>=0.5/max>=1.0`。当前算法预期 FAIL，输出用于 designer 归因。
2. 断言 release 语义缺陷：在 `X=F` 后注入一帧现行崩塌请求，验证 `duneResidual` 仍为 0 而 `bh-targetDelta` 增大。
3. 保留 tester 已修复的 preflight 硬闸门行为：ROI 存在只是必要条件，`heavyFluxNonZeroSamples>=30`、`heavyShapeRms>=0.5`、`heavyShapeMaxDelta>=1.0` 缺一不可。

解除阻断后的正式链：

1. 先生成 v2.9 RED 基线，不使用 v2.8 FAIL 冒充。
2. 执行 v2.9 green-ROI preflight；route、schema、capturePath 与冻结审计必须一致。
3. 先执行 TC02 schedule precheck：531/531、sequence、target、gap、`input_state` 隔离、capture frozen/canonical audit 全部 PASS。
4. precheck PASS 后执行完整 objective suite；所有 log 一次性传入 schema v3 checker。
5. 断言两方向 heavy 非零样本、RMS/max、green centroid、方向、守恒、预算、`maxSubstepBh`、dune release/blocked 语义全部满足。
6. 保留 10 列/capacity1000 压力回归，但其 PASS 不替代默认 128 列判定。

### 16.7 Effector 验证链

1. 使用原生连续录像执行 EC-TT-01~04 两轮与 EC-TT-05 回放。
2. 关键帧 timestamp 与最近 `P011_METRIC.sequence` 对齐。
3. 评审图同时标出 water、yellow light、green heavy ROI，并输出 green 质心时间序列或逐帧 diff。
4. HG-TT-01~06 全部按现行硬门槛判定；不得把水/黄沙运动或 light shape 替代 green heavy。
5. 绿色中央 dipole 不得出现 teleport、撕裂、持续冻结或 persistent jitter；若崩塌导致视觉抖动，如实 FAIL。
6. 生成 §16.4 的两份独立报告，记录 source commit、HAP SHA256、每个原始 log SHA256、runId/resetEpoch，并由 designer 复核后才可通知 manager。

### 16.8 实施步骤与回滚

当前阻断期：

1. developer 不修改业务代码、checker 或测试期望。
2. tester 可按 §16.6 增加“阻断证明”测试或独立分析产物，但不得把结果写成 v2.9 行为 FAIL 后又调参。
3. designer 需针对 §16.1.1/§16.1.2 升级算法版本；如继续使用 v2.9 route，必须先撤销本 BLOCKED 结论并追加变更记录。

解除阻断后：

1. architect 复审新算法并更新本节状态。
2. developer 先补 RED 测试，再实现 route、状态、预算预约、三接口输运与 telemetry。
3. 更新 schema v3 checker、formal suite 与 v2.9 preflight。
4. 按 §16.6/§16.7 串行执行 preflight、objective、subjective。
5. 任一环节失败即保留 routeId、日志与报告；下一版必须再次升级 routeId，禁止在 v2.8/v2.9 route 下混发行为。

回滚边界：

1. 只回滚 v2.9 实现文件与测试期望，不改写 v2.8/v2.7.3 历史报告。
2. 保留 BLOCKED 归因与阻断证明，作为下一版设计输入。
3. 不得通过提高 `angleDeg`、`vBottomCreepMax`、`totalMotionBudget`、capacity 或放宽 RMS/max/green 阈值解除失败。

变更记录：2026-10-19，依据 algorithm §6 与 retro v2.9 新增 §16；审批结论 `BLOCKED`。bottom-first 预算预约与相邻输运的工程方案已条件化记录，但 `A=2.7,K=2` 与 30° 崩塌不变量冲突，且 `F-X` residual 清零不能证明实际形状达标，未授权 developer 实施。

---

## 17. v2.10 工程方案：natural-observation-v2.10（2026-10-19）

状态：`READY_FOR_DEVELOPER`，仅授权观测实验实现与运行；不授权修改物理规则、参数或效果门槛。依据 `algorithm-P011.md` §6.16、§7.1~§7.10。本节 supersede §16 的实施指引；§16 保留为 v2.9 `BLOCKED/REJECTED` 历史归因，developer 不得再实现 dune/target/bottom-first reservation。

工程结论：

1. 不升级 `WaterSandParams.routeId`，不修改 `P011_BOTTOM_ROUTE_ID`。物理继续使用现有 `bottom-creep-v2.8-heavy-interface` 自然规则；v2.10 身份只存在于新增观测记录 `observationRouteId='natural-observation-v2.10'`，避免污染既有 route 断言和历史报告。
2. 观测使用独立 `P011NaturalObservationProbe`。它只复制状态、记录实际 grant 与审计，不写任何 `WaterSandField` 状态，不参与预算、容量、阈值或通道优先级决策。
3. 完整逐子步账本不进 hilog 长数组；由后台 worker 写应用 cache 文件，hilog 只输出 compact heartbeat 与文件路径，避免 UI 线程序列化/IO 阻塞。
4. 默认实验为 `128/192/28`、`wspreset=thick`、`dual-input-v1`、真实 Want 序列 `0/+15/0/-15/0`。runner 只复用现有 `wstilt` 输入，不新增输入语义。

### 17.1 模块划分与文件清单

| 文件 | 变更 | 职责 |
|---|---|---|
| `entry/src/main/ets/physics/P011NaturalObservation.ets` | 新增 | 数据契约、纯 phase 映射、Probe 与逐子步审计；不 import ArkUI/AppStorage/sensor/file |
| `entry/src/main/ets/physics/P011NaturalObservationStore.ets` | 新增 | 创建 worker sink，接收不可变 record，后台写 `ledger.ndjson` |
| `entry/src/main/ets/physics/P011NaturalObservationWorker.ets` | 新增 | worker 入口；只 `JSON.stringify`、追加写、flush，不接触物理对象 |
| `entry/src/main/ets/physics/WaterSandField.ets` | 修改 | 新增可选 probe setter 与只读 record 调用点；公式、常量、预算、错误处理逐字不变 |
| `entry/src/main/ets/pages/SandCanvas.ets` | 修改 | `wssnapshot=true` 时接线 store/probe 并输出 raster layout；渲染与 frame 驱动不变 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | 不改 | 继续既有 Want 解析与 `dual-input-v1` 发布时序 |
| `entry/src/main/ets/physics/P011InputMode.ets`、`P011TiltWant.ets` | 不改 | 输入模式、watermark、模式隔离与倾角命令语义保持 |
| `tools/run_p011_v210_natural_observation.ps1` | 新增 | 默认场景 phase runner 与 hilog/file/raster 冻结采集 |
| `test/p011_v210_natural_observation_check.py` | 新增 | 解析 ledger/raster/manifest，输出 summary 与分类；为观测分析工具，不是效果门槛 |
| 既有 `entry/src/test/**`、`entry/src/ohosTest/**`、`test/p011_check.py`、`test/p011_tc02_formal_suite.ps1` | 不改 | 不改任何既有测试用例、schema 或期望 |

证据路径：

```text
test/artifacts/p011-v210-natural-observation-<timestamp>/
  hilog.txt
  ledger.ndjson
  schedule.csv
  raster/<phase>/t<capture-time-ms>.jpeg
  raster-manifest.json
  summary.json
  source-manifest.json
docs/observation-report-P011-natural-observation-v2.10.md
```

`ledger.ndjson` 每行一个 record。`raster-manifest.json` 每项记录 capture wall time、相对 phase time、SHA256、原图尺寸、最近 `sequence/physicsTime` 和 runId。

### 17.2 核心接口

列向量长度为 `128`，相邻接口向量长度为 `127`；设备内用 `Float64Array` 复制，序列化为 JSON number array。

```ts
export const P011_NATURAL_OBSERVATION_ROUTE_ID = 'natural-observation-v2.10';
export const P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION = 3;

export type P011NaturalObservationPhaseName =
  'baseline' | 'positive' | 'positiveReturn' | 'negative' | 'negativeReturn' | 'unknown';

export interface P011NaturalObservationState {
  bh: number[]; bl: number[]; mh: number[]; ml: number[];
  suspH: number[]; suspL: number[]; S: number[]; waterTop: number[];
}

export interface P011NaturalObservationSubstepContext {
  observationRouteId: string;
  ledgerSchemaVersion: number;
  runId: string;
  frameId: number;
  substepId: number;
  sequence: number;
  physicsTime: number;
  physicsDt: number;
  commandSequence: number;
  commandPhaseId: number;
  commandPhaseSign: number;
  phaseD: P011NaturalObservationPhaseName;
  targetTilt: number;
  effectiveTilt: number;
  effectiveTiltRate: number;
  shakeEnergy: number;
}

export interface P011NaturalObservationColumnGrant {
  requested: number[]; allowed: number[]; heavy: number[]; light: number[];
}

export interface P011NaturalObservationPercolationGrant {
  requested: number[]; allowed: number[]; q: number[]; r: number[];
  heavyDown: number[]; lightDown: number[];
}

export interface P011NaturalObservationEdgeGrant {
  interfaceIndex: number; donor: number; receiver: number; directionSign: number;
  requested: number; allowed: number; heavy: number; light: number;
}

export interface P011RelaxStateTransitionRecord {
  runId: string;
  substepId: number;
  interfaceIndex: number; // c, edge between columns c and c+1
  phaseD: P011NaturalObservationPhaseName;
  thresholdCalibrationId: string;
  thetaStartDeg: number;
  thetaStopDeg: number;
  transition: 'start' | 'stop';
  effectiveTilt: number; // committed reference angle used for d/r in this substep
  d: number;             // signed residual at the transition decision, model height
  r: number;             // abs(d)/dx, model slope
  activeBefore: boolean;
  activeAfter: boolean;
  requested: number;
  granted: number;
  budgetScale: number | null; // granted/requested; null when requested == 0
  previousSubstepId: number | null;
  previousD: number | null;
  previousR: number | null;
  previousActive: boolean | null;
  previousRequested: number | null;
  previousGranted: number | null;
  previousBudgetScale: number | null;
  dAfter: number;         // post-application residual, same effectiveTilt
  rAfter: number;
  transitionPoint: 'collect' | 'postApply';
}

export interface P011NaturalObservationSplitRecord {
  channel: 'top' | 'bottom';
  interfaceIndex: number | null;
  column: number | null;
  amount: number;
  donorBhBefore: number; donorBlBefore: number;
  donorMhBefore: number; donorMlBefore: number;
  baseHeavyShare: number | null; biasedHeavyShare: number | null;
  initialHeavy: number; initialLight: number;
  fromMixedHeavy: number; fromMixedLight: number;
  fromLight: number; fromHeavy: number;
  fallbackHeavy: number; fallbackLight: number;
  finalHeavy: number; finalLight: number;
}

export interface P011NaturalObservationBudgets {
  localLimit: number[]; localUsed: number[]; localRemaining: number[];
  bottomRequested: number; bottomUsed: number; bottomLimit: number; bottomClipped: number;
  totalRequested: number; totalUsed: number; totalLimit: number;
  totalRemaining: number; totalClipped: number;
}

export interface P011NaturalObservationAudit {
  finite: boolean; range: boolean; capacity: boolean; stock: boolean; conservation: boolean;
  waterTotalBefore: number; waterTotalAfter: number;
  heavyTotalBefore: number; heavyTotalAfter: number;
  lightTotalBefore: number; lightTotalAfter: number;
  maxSubstepBh: number; maxSubstepBl: number;
  maxSandTopSubstepDelta: number; maxSandTopInvariantError: number;
  bottomBudgetExcess: number; totalBudgetExcess: number; maxLocalBudgetExcess: number;
  relaxCandidateCount: number; relaxTriggered: boolean;
  relaxRequestedTotal: number; relaxAppliedTotal: number;
  errorCode: string | null;
}

export interface P011NaturalObservationRecord {
  context: P011NaturalObservationSubstepContext;
  stateBefore: P011NaturalObservationState;
  stateAfter: P011NaturalObservationState;
  settle: P011NaturalObservationColumnGrant;
  percolate: P011NaturalObservationPercolationGrant;
  lift: P011NaturalObservationColumnGrant;
  suspensionDiffusion: P011NaturalObservationEdgeGrant[];
  slip: P011NaturalObservationEdgeGrant[];
  relaxStateTransitions: P011RelaxStateTransitionRecord[];
  bottomCreep: P011NaturalObservationEdgeGrant[];
  splits: P011NaturalObservationSplitRecord[];
  budgets: P011NaturalObservationBudgets;
  audit: P011NaturalObservationAudit;
}

export type P011NaturalObservationSink = (record: P011NaturalObservationRecord) => void;

export class P011NaturalObservationProbe {
  constructor(cols: number, sink: P011NaturalObservationSink);
  beginSubstep(context: P011NaturalObservationSubstepContext,
    stateBefore: P011NaturalObservationState): void;
  recordSuspensionDiffusion(grant: P011NaturalObservationEdgeGrant): void;
  recordSettle(column: number, requested: number, allowed: number,
    heavy: number, light: number): void;
  recordPercolate(column: number, requested: number, allowed: number,
    q: number, r: number): void;
  recordLift(column: number, requested: number, allowed: number,
    heavy: number, light: number): void;
  recordSlip(grant: P011NaturalObservationEdgeGrant): void;
  recordRelaxStateTransition(transition: P011RelaxStateTransitionRecord): void;
  recordBottomCreep(grant: P011NaturalObservationEdgeGrant): void;
  recordTopSplit(split: P011NaturalObservationSplitRecord): void;
  recordBottomSplit(split: P011NaturalObservationSplitRecord): void;
  recordRelaxAngle(candidateCount: number, triggered: boolean,
    requestedTotal: number, appliedTotal: number): void;
  finishSubstep(stateAfter: P011NaturalObservationState,
    budgets: P011NaturalObservationBudgets,
    audit: P011NaturalObservationAudit): void;
}

export function p011NaturalObservationPhaseForCommand(
  commandSequence: number, targetTilt: number
): P011NaturalObservationPhaseName;
```

Probe 不持有 `WaterSandField` 引用；`record*` 为 append-only，request 非零而 `allowed=0` 也必须记录。`directionSign=+1` 为左→右，`-1` 为右→左；`heavy/light` 是非负实际量。phase mapper 只识别 sequence `1..5` 与 `0/+15/0/-15/0`，其他输入返回 `unknown`。

`WaterSandField` 仅新增 `setNaturalObservationProbe(probe: P011NaturalObservationProbe | null): void`，不改变 `step(dt, p011Hook?)`、`WaterSandP011Metric`、`WaterSandP011Bottom`。文件 sink：

```ts
export interface P011NaturalObservationStore {
  readonly filePath: string;
  append(record: P011NaturalObservationRecord): void;
  close(): Promise<void>;
}

export function createP011NaturalObservationStore(
  context: common.UIAbilityContext, runId: string
): Promise<P011NaturalObservationStore>;
```

`append` 在 UI 线程只 postMessage 已复制 record；worker 对每条 record `JSON.stringify` 后追加一行并 flush。路径固定 `${context.cacheDir}/p011-v210-${runId}.ndjson`，成功后页面输出一次 `P011_OBS_STORE {"observationRouteId":"natural-observation-v2.10","runId":"...","filePath":"..."}`。

### 17.3 模拟循环插入点与时序

`WaterSandField.step` 的既有两阶段 collect/apply 语义不变：

```text
frame callback
  └─ WaterSandField.step(dt, existing P011 hook)
       ├─ saveBackup
       ├─ beginSubstepMotionBudget(dt, collect=true, 1)
       ├─ updateTilt(dt)
       ├─ probe.beginSubstep(context, stateBefore)          // effectiveTilt is the value used by this substep
       ├─ substep(dt)                                      // existing collect pass
       │    ├─ waterLevel / diffuseSuspension
       │    │    └─ recordSuspensionDiffusion(actual grant) // diffusion is not rolled back
       │    └─ settle/percolate/creep/relax/lift request collection
       ├─ restoreMotionSnapshot
       ├─ beginMotionApplication(dt, unchanged scale)
       ├─ applyMotionChannels(dt)
       │    ├─ settle: record requested/allowed/H/L
       │    ├─ percolate: record requested/allowed/q/r and H/L composition
       │    ├─ creepBottom: record edge requested/allowed/H/L + donor split/fallback
       │    ├─ relaxAngle: record request summary, triggered, edge grants + top split
       │    └─ lift: record requested/allowed/H/L + top split
       ├─ existing audit(true)
       ├─ probe.finishSubstep(stateAfter, budgets, independent audit)
       └─ existing getP011Metric/getP011Bottom hook
```

插入规则：

1. `settle/percolate/creep/relax/lift` 的 record 只放在 `collectingTotalMotionRequests===false` 的 application 分支；collect pass 不产生假 grant。
2. `diffuseSuspension` 只在 collect 阶段执行一次，实际 grant 在该处记录；它不进入 `300dt`，不得在 apply 后重复统计。
3. `effectiveTilt` 在本子步 `updateTilt(dt)` 后冻结；`stateBefore` 在任何改变材料的 settle/percolate/creep/relax/lift apply 前复制。水面/悬沙扩散按既有 collect 时序执行并单独记录；`stateAfter` 在 `applyMotionChannels` 与 `audit(true)` 后复制。context、逐边残差、grants 与 audit 必须引用同一 `substepId` 和同一冻结倾角，避免旧版 context 落后一子步。
4. Probe 调用不能出现在赋值表达式、预算计算、容量判断或 `if` 条件中；只允许作为语句后的观测记录。
5. 若 field 自身 audit 失败，沿用现有 restore/freeze/error 行为；Probe 记录 `errorCode`，不得替 field 重放、修复或放宽。
6. `P011_OBS_METRIC` heartbeat 每有效子步一条，仅含 identity、phase、`effectiveTilt/shakeEnergy`、totals、budget 汇总和 audit booleans；完整数组只进文件。

### 17.4 逐子步数据契约

| 字段组 | 契约 |
|---|---|
| `context` | `phaseD` 为 `baseline/positive/positiveReturn/negative/negativeReturn`；包含 command sequence、field phase id/sign、`targetTilt/effectiveTilt/effectiveTiltRate/shakeEnergy`、`physicsDt` 与身份 |
| `stateBefore/stateAfter` | 每列完整 `(bh,bl,mh,ml,suspH,suspL,S,waterTop)`；`S=sandTop=bh+bl+mh+ml`，两份状态用于差分与守恒 |
| `settle` | 每列 request、实际 allowed、`suspH→mh` H grant、`suspL→ml` L grant；`allowed=0` 也保留 request |
| `percolate` | 每列 request、allowed、`q`（heavy 穿透 light）、`r`（heavy/light 成对下移）；`heavyDown=q+r`，`lightDown=r-q` |
| `lift` | 每列 request、allowed、top→suspension 实际 H/L grant，含 local/shared budget 截断 |
| `suspensionDiffusion` | 127 个相邻接口实际 H/L 通量、donor/receiver、方向；不计 material motion 共享账本 |
| `slip` | `relaxAngle` 实际相邻输运 request、allowed、H/L、donor/receiver、方向；触发摘要另在 `audit` |
| `relaxStateTransitions` | 仅状态翻转时追加逐边 `start/stop`；包含 run/substep/边号、该步 `effectiveTilt,d,r`、`activeBefore/After`、request/grant/edge scale、转换前一子步的残差/active/grant，以及本步 apply 后 `dAfter/rAfter`。不能只记全局 relaxTriggered |
| `bottomCreep` | 每条 bottom edge 实际 request、allowed、H/L、方向；可核对 `10dt` 独立预算与速率截断 |
| `splits` | top/bottom split 前 donor 四相库存、base/biased heavy share、初始 H/L、mixed/light/heavy 来源、fallback H/L、最终 H/L |
| `budgets` | 每列 local `28dt` limit/used/remaining；bottom `10dt` requested/used/limit/clipped；shared `300dt` requested/used/remaining/clipped |
| `audit` | 守恒、库存、容量、finite/range、`maxSubstepBh/Bl`、sandTop 不变量、bottom/total/local 超额、relax 触发与错误码 |

审计公式：

```text
heavyTotalAfter-heavyTotalBefore = 0
lightTotalAfter-lightTotalBefore = 0
waterTotalAfter-waterTotalBefore = 0
max(localUsed[c]-28dt,0)=0
max(bottomUsed-10dt,0)=0
max(totalUsed-300dt,0)=0
maxSubstepBh=max(|bhAfter[c]-bhBefore[c]|)
maxSandTopInvariantError=max(|SAfter[c+1]-SAfter[c]-tan(effectiveTilt)|)
```

`tan30°` 只用于 §6.16 形式验收的完整 sandTop 不变量；thick 运行时 `relaxAngle` 触发阈值仍是现行 `angleDeg=4`，两者不得合并。`relaxCandidateCount` 按 `|deviation|>tan4°` 统计，`relaxTriggered` 表示本子步存在非零 request。

### 17.5 Phase schedule 与 runner 入口

入口：

```powershell
tools/run_p011_v210_natural_observation.ps1 `
  -Hdc <hdc.exe> -Target <deviceId> -Bundle com.example.helloworld `
  -Ability EntryAbility -Seed 20261016 -OutputRoot <fresh-output-root>
```

固定 schedule：

| sequence | phase | target | hold |
|---:|---|---:|---:|
| 1 | baseline | 0 | 3.0 s |
| 2 | positive | +15 | 5.1 s |
| 3 | positiveReturn | 0 | 2.0 s |
| 4 | negative | -15 | 5.1 s |
| 5 | negativeReturn | 0 | 3.0 s |

每条命令均通过真实 `aa start ... --ps wstilt <target>` 发布，并携带 `rendermode=4`、`testmode=true`、`forceinvert=false`、`seed=20261016`、`wspreset=thick`、`wssnapshot=true`、`wscols=128`、`wscapacity=192`、`wssandtopmax=28`。

Runner 生命周期：

1. 创建全新 OutputRoot；发现既有证据即 fail，不覆盖。
2. 清理目标 hilog collector，设置 16M app buffer，启动 frozen hilog capture。
3. 冷启动 sequence 1，等待 `P011_EVENT reset` 与同 runId 的首个 `P011_METRIC/P011_OBS_STORE`。
4. 依序发布 sequence 2..5；每相记录 command wall time，不跳过回零或反相。
5. 全程每 100ms 调 `snapshot_display -f` 采集原始分辨率 raster，另强制保存每相 endpoint；实际 capture 时间写入 manifest。
6. 结束后停止 collector，执行 hilog/schedule/ledger/raster 冻结审计，再从 `P011_OBS_STORE.filePath` 拉取 `ledger.ndjson`。
7. 运行 `test/p011_v210_natural_observation_check.py` 生成 `summary.json` 与报告；任何缺项输出 `invalid`，不得补造结论。

该 runner 不复用 v2.8 TC02 的 10 列/1000 capacity 压力 schedule；那是历史测试，不得外推为本默认场景。

### 17.6 Raster 标定

`SandCanvas` 在 `wssnapshot=true` 且首个 run 确定后输出一次：

```text
P011_RASTER_LAYOUT {"runId":"...","canvasWidth":W,"canvasHeight":H,
 "boxX":X,"boxY":Y,"boxWidth":BW,"boxHeight":BH,"cols":128,"capacity":192,
 "pixelsPerColumn":BW/128,"pixelsPerModelUnitY":BH/192}
```

标定规则：

1. 只使用 `snapshot_display` 得到的原始分辨率 JPEG，不 resize、不 crop 后再上采样；manifest 记录原图宽高和 SHA256。
2. 以 `P011_RASTER_LAYOUT` 的 sandbox 矩形为唯一坐标基准，不做 whole green mask 质心替代。
3. 在每列 `pixelsPerColumn` 桶内识别与底部连通的 bottom-heavy 绿色带；颜色阈值以现行渲染 `#24571F` 为中心，允许抗锯齿与半透明水层容差，阈值写入 `summary.json`。
4. 每列重沙界面为该绿色带的上边界像素；用布局把像素 y 转换为 model height：`capacity-(y+0.5-boxY)/BH*capacity`。位移符号取“界面升高为正、降低为负”，单位 px。
5. `A_B+`、`A_B-` 分别为正/反相期间每列边界位移相对本 run baseline 的最大绝对值；`A_H` 为绿色可见像素数最大变化；`ΔHeavy(x)` 为每列 raster 边界和 model heavy 库存最终变化向量。
6. `t50/t90/τ` 使用 raster capture 实际时间与最近 ledger `physicsTime` 对齐计算；采样不足或相位不完整时输出 `insufficientEvidence`。
7. whole green mask 质心、model `bh` 曲线和 ledger flux 仅作诊断，不得替代 raster 的 `A_B`。

`summary.json` 至少包含：

```json
{
  "schemaVersion": 1,
  "observationRouteId": "natural-observation-v2.10",
  "status": "invalid|visibleNaturalLimitObserved|ruleDriveTooWeak|intrinsicallySlow|insufficientEvidence",
  "phaseComplete": true,
  "auditPassed": true,
  "rasterValid": true,
  "transitionTelemetryAvailable": false,
  "relaxTransitionCounts": {"start": 0, "stop": 0},
  "relaxThresholdCalibration": {
    "thresholdCalibrationId": null,
    "thetaStartEstimateDeg": null,
    "thetaStartCi95Deg": null,
    "thetaStopEstimateDeg": null,
    "thetaStopCi95Deg": null,
    "fitTransitionCounts": {"start": 0, "stop": 0},
    "holdoutTransitionCounts": {"start": 0, "stop": 0},
    "hardRangeViolations": null,
    "status": "unavailable|insufficientEvidence|calibrated|invalid"
  },
  "A_B_plus_px": 0.0,
  "A_B_minus_px": 0.0,
  "A_H_pixels": 0.0,
  "t50_plus_ms": null,
  "t90_plus_ms": null,
  "t50_minus_ms": null,
  "t90_minus_ms": null,
  "tau_ms": null,
  "residual_px": {"positiveReturn": 0.0, "negativeReturn": 0.0},
  "driveDiagnostics": {}
}
```

`>=2px` 只沿用既有可见判据用于分类，不新增、不放宽、不替换任何原门槛。

### 17.7 线程模型与性能

1. 物理仍在 `SandCanvas` frame callback 所属 UI/render 线程执行；不新增物理 worker，不改变 substep 数量或 `dt` 计算。
2. Ledger 序列化与文件 IO 在 `P011NaturalObservationWorker` 后台线程执行；UI 线程只复制 typed arrays 并 postMessage，`O(cols)`，无 JSON stringify。
3. UI 每有效子步只发一条 compact `P011_OBS_METRIC`；完整 record 不进 hilog，不产生长数组日志。
4. 禁止丢弃落后 record。worker 队列、文件或关闭失败时输出 `P011_OBS_ERROR` 并标记本轮 `invalid`，不得截断账本继续判定。
5. raster 采集由 host runner 执行，不调用 UI API，不改渲染节拍；capture 耗时只记录在 manifest。
6. 若 observation 接线导致 frame 超时、ANR、dormant 异常或 runId 断裂，本轮实验无效；修复观测链路后重跑，不得减少记录字段。

### 17.8 异常处理与降级

| 故障 | 处理 |
|---|---|
| store/worker 创建失败 | 不创建 Probe，物理可继续；runner 收到 `P011_OBS_ERROR` 后停止并标记 `invalid`，不得回退到 model 推测 raster |
| worker 写文件失败 | 记录错误与最后完整 sequence；本轮 `invalid`，禁止从部分账本外推 `t50/t90` |
| hilog 丢失 heartbeat | 判观测链路失败；ledger 与 raster 身份仍需一致，否则 `invalid` |
| raster 缺相/无法识别 heavy 边界 | `rasterValid=false`，分类 `invalid`；不得用 `bh` 或 green centroid 补值 |
| field audit/error | 沿用现有 freeze 行为；观测记录错误码，实验 `invalid`，不得静默重放 |
| phase sequence 不完整 | `phaseComplete=false`，不得计算正反对称性或自然上限 |
| 性能超限 | 实验 `invalid`；不得降低采样、删除数组或关闭 store 换性能 |

任何观测异常都不允许修改物理参数、阈值或测试期望来“修复”。

### 17.9 硬约束与验收

Developer 实现验收：

1. `WaterSandField` 在 Probe 为 `null` 时的状态轨迹、预算、错误码和既有 telemetry 与修改前等价；可用同一输入对比完整 snapshot 序列。
2. Probe 开启时不写任何 field 状态；`settle/percolate/lift/slip/bottomCreep/suspensionDiffusion` grant 总和能逐列解释 `stateAfter-stateBefore`。
3. 每有效子步恰有一个 ledger record；dormant/noop 不伪造 `physicsDt>0` 的有效子步。
4. Phase sequence 为真实 `dual-input-v1` Want `0/+15/0/-15/0`，sequence `1..5`，无伪 script 事件。
5. 默认几何严格 `128/192/28`，thick 初始化和现行参数不变。
6. 不修改 `angleDeg=4`、`vSandTopMax=28`、`vBottomCreepMax=10`、`totalMotionBudget=300`、capacity、崩塌、守恒或任何效果门槛。
7. 不修改任何既有测试用例、历史报告或 v2.9 BLOCKED 结论。
8. `summary.json` 只按 algorithm §7.6 判定树分类；实验成立前状态仍为 `OBSERVATION_PENDING_HIL`，不得升级为 `READY_FOR_ARCHITECT`。
9. 新 ledger 每子步必须含 `relaxStateTransitions`（允许空数组）；schema/summary 必须明确 `transitionTelemetryAvailable`。若本次运行没有真实 sliding 状态，则设为 false，并禁止把空数组作为“未发生转换”的证明。
10. 逐边 transition probe 只复制状态机决定与实际 grant，不得写回 `sliding[]`、重算 request 或参与预算/admission；probe on/off 的同输入物理轨迹必须一致。

### 17.10 实施步骤

1. 新增 `P011NaturalObservation.ets` 数据契约、Probe 与纯审计。
2. 新增 store/worker，验证文件路径心跳与后台写入。
3. 在 `WaterSandField` 各实际 application 分支插入只读 record 调用，并新增可选 setter。
4. 在 `SandCanvas` 接线 store/probe、phase mapper 和 raster layout 心跳；不改渲染。
5. 新增 host runner 与观测 checker，先做本地 synthetic ledger/raster schema 校验，再上设备。
6. 执行一次默认场景完整 run，检查 phase、账本、守恒、budget、raster 与身份冻结审计。
7. 生成 `summary.json` 和 observation report；若 `invalid`，只修观测/执行链路后重跑。
8. 将结果交 designer 复核，再决定后续是物理增强还是预期校准；本节不授权第三步实现。
9. ledger schema v3 增加逐边 hysteresis transition capture 与显式 availability/status；旧 schema v1/v2 artifacts 只读兼容，不回写、不覆盖，也不将缺字段补成零事件。

### v2.12 双角标定所需的逐边转换事件

v2.10 observation framework ledger schema v3 增加 `relaxStateTransitions: P011RelaxStateTransitionRecord[]`，每个有效子步均存在该数组；没有边翻转时为空数组，不得伪造事件。每次实际 `sliding[c]` 从 false→true 或 true→false 时写一条事件，核心字段严格覆盖：

```text
(runId, substepId, c, effectiveTilt, d, r,
 activeBefore, activeAfter, requested, granted, budgetScale)
```

额外保存 `transition`、`phaseD`、`thresholdCalibrationId/thetaStartDeg/thetaStopDeg`、前一相邻采样 `(previousSubstepId, previousD, previousR, previousActive)`、对应前一 active grant、以及本子步 apply 后 `(dAfter,rAfter)`，用以夹出阈值区间并区分“达到停止阈值”与“请求被限流”。`effectiveTilt,d,r` 是作出转换判定时同一快照；`d/r` 采用 `d=S[c+1]-S[c]-tan(effectiveTilt)*dx`、`r=abs(d)/dx`。`granted` 指该边本子步实际应用的总量，`budgetScale=granted/requested`；request 为0时 scale 为 null，避免把无请求误记成全额通过。`previous*` 是紧邻同一 run、同一 edge 的有效子步样本，若中间有 substep gap、run/reset/几何变化则不配对并标该转换不可用于阈值 bracket。

状态机应在 collect 判定 start、stop 及 hold；请求和 grant 在同一 substep 的 apply 后回填事件，再由 `finishSubstep` 一次 append。若 stop 判定发生于 apply 后，`transitionPoint='postApply'` 且记录 apply 前后残差；不得把 postApply 残差冒充 collect 输入。request/grant 不得影响状态或预算。限流期间 active 保持时虽然不是转换事件，也必须保留在常规逐边 `slip` grant trace 中；校准器要把这些样本与转换事件关联，不能将 grant=0 误判为 stop。

该字段是 v2.10 observation framework ledger schema v3 的可选数据内容；历史 v1/v2 ledger 与当前没有 `sliding[]` 的 legacy run 输出空数组/`transitionTelemetryAvailable=false`，不可伪造 sliding 状态。双角阈值校准 run 采用显式 candidate grid，每个 run manifest 绑定 `thetaStartDeg/thetaStopDeg` 与唯一 `thresholdCalibrationId`；阈值并非隐式默认30/20。只有具备实际双阈值状态的 candidate run 才产生可用于 v2.12选择的事件；拟合 run 与独立 holdout run 分离。start 的相邻 inactive→active 样本夹出起动转折；stop 的相邻 active→inactive 样本夹出停止转折。基于 raster `30°±2°`、逐子步 hard gate、预算、停止后稳定时间联合筛选候选，再按 run 重采样计算95% bootstrap区间，并按正负 residual、±tilt、budgetScale分层。样本缺失、非连续、holdout越界或候选均不能同时满足坡度/时间/硬门槛时仍为 `DESIGN_PENDING_OBSERVATION`。Probe 不创建/修改 sliding 状态，避免观测本身改变被测物理。

阈值转换采集不得只复用 ±15° schedule 并假定它会经过约30°残差。新增校准 schedule `p011-v212-hysteresis-calibration-v1`，顺序为 `0/+15/0/+45/0/-45/0`，正负45°各保持5.1s，其余phase沿用 v2.10 时长；原始 `p011-v210-natural-observation` schedule 与旧 artifacts 完全不变。每个 candidate 使用独立全新 run，候选排序随机化；至少5个校准 run 与5个独立 holdout run（每组每个有效方向转换不少于20个），各自绑定同一显式 candidate pair。若 5.1s 内未观察到 stop transition，标记 `stopTransitionUnobserved`，不得把 phase end 当作停止点或用外推填角度。

变更记录：2026-10-19，依据 `algorithm-P011.md` §7 新增 §17。v2.9 目标形状与逐列搬运路线已由 designer 标记 `REJECTED`；本节仅授权零侵入自然规则观测、逐子步账本、真实 raster 标定和默认场景完整实验。2026-10-19 追加 ledger schema v3 的逐边 `relaxStateTransitions` 与独立 ±45° 阈值校准 schedule，旧 v1/v2 ledger artifacts 保持原样。

## 18. v2.11 工程方案：gravity-advection-v2.11（2026-10-19）

状态：`READY_FOR_DEVELOPER`。依据 `algorithm-P011.md` §8.1~§8.8。本节在 §17 的零侵入观测链路上实现物理增强；§17 的 Probe、worker sink、runner 采集规则与 raster 标定继续有效。除本节明确列出的测试契约修订外，developer 不得修改任何既有测试用例、历史报告或效果门槛。

### 18.1 工程结论与模块划分

1. `WaterSandParams.routeId` 升级为 `gravity-advection-v2.11`，禁止沿用 v2.8 routeId 发布行为变更；不新增参数，`bottomCreepVelocityGain` 只被激活为重力交换增益。
2. 新增纯物理模块 `P011GravityAdvection.ets`，只包含局部势计算、交换计划与应用函数；不 import ArkUI、AppStorage、sensor、file 或 worker。
3. 重力交换作为 `WaterSandField` 的独立 application 算子插入，不改动既有 `creepBottom` 的 shape request、方向保持、split 或 independent scaling。`bottomCreepVelocityGain` 不得写入 `targetShapeRequestRate` 或 `lastBottomCreepRate`。
4. v2.10 Probe 继续零侵入：collect pass 不记录 gravity grant；application pass 只在状态更新完成后复制诊断数据。Probe 为 `null` 时物理行为完全相同，只是没有 ledger。
5. 预算不新增通道、不改变上限：每次交换消耗 bottom `2q`、total motion `2q`。局部 `vSandTopMax=28` 账本不消耗，因为每列 `bh+bl` 与 `sandTop` 均不变。

| 文件 | 变更 | 职责 |
|---|---|---|
| `entry/src/main/ets/physics/P011GravityAdvection.ets` | 新增 | 路线常量、纯 plan/apply 接口、局部势与交换公式；无 UI/IO 依赖 |
| `entry/src/main/ets/physics/WaterSandField.ets` | 修改 | routeId 升级；在 collect/application 两阶段插入 gravity 算子；接入 bottom/total 预算与错误恢复 |
| `entry/src/main/ets/physics/P011NaturalObservation.ets` | 修改 | ledger schema 升级到 v2；新增 `routeId` 与 `gravityAdvection` 驱动诊断；保持 Probe 只读 |
| `entry/src/main/ets/physics/P011NaturalObservationStore.ets` | 修改 | 支持可选 artifact 前缀，默认保持 `p011-v210`；v2.11 传入 `p011-v211` |
| `entry/src/main/ets/pages/SandCanvas.ets` | 修改 | 仅按当前 route 选择 ledger 文件前缀；Probe/store/raster layout 接线不变 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | 不改 | 继续既有 Want 解析与 `dual-input-v1` 发布时序 |
| `entry/src/main/ets/physics/P011InputMode.ets`、`P011TiltWant.ets` | 不改 | 输入协议、watermark、模式隔离与倾角命令语义保持 |
| `tools/run_p011_v211_gravity_advection.ps1` | 新增 | 复用 §17.5 phase schedule 与 §17.6 raster 采集流程，输出 v2.11 独立证据目录 |
| `test/p011_v211_gravity_advection_check.py` | 新增 | 解析 v2 ledger/raster/manifest，执行原硬门槛与新增 raster/P95 判定 |
| `entry/src/test/**`、`entry/src/ohosTest/**` | 不改 | developer 不修改测试；§18.7 列出的契约修订交 tester 执行 |

### 18.2 核心接口

```ts
export const P011_GRAVITY_ADVECTION_ROUTE_ID = 'gravity-advection-v2.11';
export const P011_GRAVITY_ADVECTION_MODULE_VERSION = 1;

export interface P011GravityAdvectionParams {
  bottomCreepDeadZone: number;
  bottomCreepVelocityGain: number;
  vBottomCreepMax: number;
}

export interface P011GravityAdvectionState {
  sandTop: Float64Array;
  bh: Float64Array;
  bl: Float64Array;
}

export interface P011GravityAdvectionLimits {
  bottomRemaining: number;
  totalRemaining: number;
  totalScale: number;
}

export interface P011GravityAdvectionEdgePlan {
  interfaceIndex: number;
  donor: number;
  receiver: number;
  directionSign: number;
  surfaceDrive: number;
  heavyExcess: number;
  potential: number;
  requestRate: number;
  requestedHalf: number;
  rateLimitHalf: number;
  stockLimitHalf: number;
  requestedMotion: number;
}

export interface P011GravityAdvectionPlan {
  active: boolean;
  directionSign: number;
  edges: P011GravityAdvectionEdgePlan[];
  requestedMotionTotal: number;
}

export interface P011GravityAdvectionAppliedEdge extends P011GravityAdvectionEdgePlan {
  appliedHalf: number;
  appliedMotion: number;
  heavyFlux: number;
  lightFlux: number;
  bottomClippedMotion: number;
  totalClippedMotion: number;
  rejection: 'none' | 'deadZone' | 'nonPositivePotential' | 'stock' |
    'epsilon' | 'bottomBudget' | 'totalBudget';
}

export interface P011GravityAdvectionApplication {
  edges: P011GravityAdvectionAppliedEdge[];
  requestedMotionTotal: number;
  appliedMotionTotal: number;
  appliedQTotal: number;
  bottomBudgetUsed: number;
  totalBudgetUsed: number;
  maxColumnSumDelta: number;
  errorCode: 'nonFinite' | 'negativeStock' | 'columnSumInvariant' | null;
}

export function planP011GravityAdvection(
  dt: number,
  effectiveTilt: number,
  params: P011GravityAdvectionParams,
  state: P011GravityAdvectionState
): P011GravityAdvectionPlan;

export function applyP011GravityAdvection(
  state: P011GravityAdvectionState,
  plan: P011GravityAdvectionPlan,
  limits: P011GravityAdvectionLimits
): P011GravityAdvectionApplication;
```

公式与语义：

1. `directionSign=sign(effectiveTilt)`；`abs(effectiveTilt)<bottomCreepDeadZone` 时返回 inactive plan，不产生请求。
2. `surfaceDrive=directionSign*(sandTop[donor]-sandTop[receiver])`，`heavyExcess=bh[donor]-bh[receiver]`，`potential=surfaceDrive+heavyExcess`；`potential<=0` 的接口保留为零值诊断，不交换。
3. `requestRate=bottomCreepVelocityGain*max(0,potential)`；`requestedHalf=min(requestRate*dt/2,vBottomCreepMax*dt/2,bh[donor],bl[receiver])`，`requestedMotion=2*requestedHalf`。
4. 遍历顺序沿用现有 downhill 顺序：`directionSign>0` 时 interface `cols-2..0`，`directionSign<0` 时 `0..cols-2`。每条接口使用当前数组值，不做全局排序或目标形状拟合。
5. application 前重新校验当前 stock：`allowedHalf=min(requestedHalf*totalScale,bottomRemaining/2,totalRemaining/2,bh[donor],bl[receiver])`。`totalScale` 是既有共享预算比例；最终的 bottom/total remaining 硬裁剪保证不超上限。
6. 状态更新为 `bh[donor]-=q`、`bh[receiver]+=q`、`bl[donor]+=q`、`bl[receiver]-=q`；`q=allowedHalf`。heavy flux 为 donor→receiver 的 `q`，light flux 为 receiver→donor 的 `q`，总运动量为 `2q`。
7. `maxColumnSumDelta=max(|(bh[c]+bl[c])_after-(bh[c]+bl[c])_before|)`；超过 EPS 返回 `columnSumInvariant`，由 field 回滚并冻结。operator 内不调用 `updateSandTop()`。

### 18.3 WaterSandField 插入点与时序

```text
WaterSandField.step
  └─ each substep
       ├─ saveBackup / updateSandTop
       ├─ beginSubstepMotionBudget(dt, collect=true, 1)
       ├─ probe.beginSubstep(context, stateBefore)
       ├─ substep(dt)                         // collect pass
       │    ├─ updateTilt / waterLevel / diffuseSuspension
       │    ├─ settle / percolate
       │    ├─ creepBottom                    // shape request unchanged
       │    ├─ gravityAdvection.plan          // no state mutation
       │    │    └─ totalMotionBudgetRequested += plan.requestedMotionTotal
       │    └─ relax / lift / audit
       ├─ restoreMotionSnapshot
       ├─ beginMotionApplication(dt, sharedScale)
       ├─ applyMotionChannels(dt)             // application pass
       │    ├─ settle / percolate
       │    ├─ creepBottom                    // unchanged
       │    ├─ gravityAdvection.apply
       │    │    ├─ mutate bh/bl only
       │    │    ├─ consumeBottomBudget(2q)
       │    │    ├─ consumeTotalMotionBudget(2q)
       │    │    └─ probe.recordGravityAdvection(copy-only diagnostic)
       │    ├─ relax / lift
       │    └─ updateSandTop / audit
       └─ probe.finishSubstep(stateAfter, budgets, audit)
```

集成规则：

1. gravity plan 在 collect pass 只产生请求并计入 `totalMotionBudgetRequested`，不写 `bh/bl`，不调用 Probe record。
2. legacy `creepBottom` 的 independent bottom scale 只使用其自身 request total；gravity 使用应用时的 bottom remaining 硬裁剪。`budgets.bottomRequested` 输出 legacy request 与 gravity request 的总和，二者共享同一 `10dt` limit。
3. gravity 插入在 `creepBottom` 之后、`relaxAngle` 之前，保持既有通道顺序；不得因 gravity 预算不足回滚或重排其他通道。
4. Probe record 只接收 application 后复制的数字，不进入赋值表达式、预算判断或 `if` 条件；Probe 异常不得改变物理结果。
5. `gravityAdvection` 返回错误时映射：`nonFinite→NON_FINITE_VALUE`，`negativeStock→NEGATIVE_VALUE`，`columnSumInvariant→UNKNOWN`；随后沿用现有 restoreBackup/freeze/lastError 行为。

### 18.4 逐子步数据契约扩展

`P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION` 升级为 `3`。v1/v2 字段全部保留（包括 v2 `gravityAdvection`）；新增逐边状态转换字段只增不删，旧 checker 若遇到 v3 必须显式拒绝而不是忽略未知字段。

```ts
export interface P011NaturalObservationSubstepContext {
  // all v1 fields remain unchanged
  routeId: string; // 'gravity-advection-v2.11'
}

export interface P011GravityAdvectionDiagnostic {
  routeId: string;
  active: boolean;
  directionSign: number;
  edgeCount: number;
  requestedMotionTotal: number;
  appliedMotionTotal: number;
  appliedQTotal: number;
  bottomBudgetUsed: number;
  totalBudgetUsed: number;
  bottomClippedMotion: number;
  totalClippedMotion: number;
  maxColumnSumDelta: number;
  edges: P011GravityAdvectionAppliedEdge[];
}

export interface P011NaturalObservationRecord {
  // all v1 fields remain unchanged
  gravityAdvection: P011GravityAdvectionDiagnostic;
}

export class P011NaturalObservationProbe {
  recordGravityAdvection(diagnostic: P011GravityAdvectionDiagnostic): void;
}
```

契约：

1. 每个有效 application 子步恰好记录一个 `gravityAdvection` 对象；inactive/dead-zone/no-op 也记录聚合零值，缺失字段判 ledger invalid。
2. `edges` 长度为 `cols-1=127`，按实际遍历顺序保留全部接口；`potential<=0`、stock 不足、预算截断和 EPS 拒绝都必须可区分。
3. `appliedMotionTotal=sum(appliedMotion)`，`bottomBudgetUsed=appliedMotionTotal`，`totalBudgetUsed=appliedMotionTotal`；三者不一致判 audit invalid。
4. `stateAfter-stateBefore` 仍必须能由全部通道解释；gravity 部分满足每列 `Δbh+Δbl=0`，且 `S` 不因该算子改变。
5. `driveDiagnostics` 汇总至少输出 requested/applied/clipped motion、受限子步数、截断量、`potential` 分布与 `bottomCreepRate` 限制命中率；不得用诊断值替代 raster 判定。

### 18.5 Runner、phase schedule 与 raster 复用

新增 `tools/run_p011_v211_gravity_advection.ps1`，复用 §17.5/§17.6 的采集规则，不改 v2.10 runner 与历史证据：

| sequence | phase | target | hold |
|---:|---|---:|---:|
| 1 | baseline | 0 | 3.0 s |
| 2 | positive | +15 | 5.1 s |
| 3 | positiveReturn | 0 | 2.0 s |
| 4 | negative | -15 | 5.1 s |
| 5 | negativeReturn | 0 | 3.0 s |

固定参数：真实 `dual-input-v1` Want、`rendermode=4`、`testmode=true`、`forceinvert=false`、`seed=20261016`、`wspreset=thick`、`wssnapshot=true`、`wscols=128`、`wscapacity=192`、`wssandtopmax=28`。

输出路径：

```text
test/artifacts/p011-v211-gravity-advection-<timestamp>/
  hilog.txt
  ledger.ndjson
  schedule.csv
  raster/<phase>/t<capture-time-ms>.jpeg
  raster-manifest.json
  summary.json
  source-manifest.json
docs/test-report-P011-gravity-advection-v2.11.md
docs/effect-report-P011-gravity-advection-v2.11.md
```

验收判定：

1. 原硬门槛全部保持：材料守恒、库存、容量、`maxSubstepBh`、bottom `<=10dt`、total `<=300dt`、完整 `sandTop` 的 `|ΔS-tan(effectiveTilt)|<=tan30°`。
2. gravity 附加审计：每列 `bh+bl` 与 `sandTop` 不变，heavy/light 总量分别守恒，bottom/total 使用量均等于 `2q` 总和。
3. 同分辨率 raster 上 `A_B+` 或 `A_B-` 至少一个方向 `>=2px`；两个方向均记录，不做全量 dominance。
4. `t50/t90`、方向性、回零残余与 TEST/EXPERIENCE 切换残余必须输出；`t90` 按既有 P95 `<=100ms` 判定。
5. raster 识别、`P011_RASTER_LAYOUT`、100ms capture、每相 endpoint 与 manifest SHA256 规则完全复用 §17.6；不得 resize、crop 后上采样或用 model `bh` 替代 raster。

### 18.6 异常、降级与线程模型

| 情况 | 处理 |
|---|---|
| dead zone 或 `potential<=0` | 本接口 `q=0`，正常物理继续；ledger 记录拒绝原因 |
| stock 不足 | `q` 裁剪到当前 `bh[donor]`/`bl[receiver]`，不是错误；记录 clipped 量 |
| bottom/total 预算不足 | `q` 裁剪到剩余预算的一半，不超上限；记录受限子步数与截断量 |
| 数值非有限 / 负库存 / column sum invariant | 返回错误，field restoreBackup 并 freeze；本轮 run invalid |
| Probe/store/worker/raster 失败 | 沿用 §17.8：本轮 run invalid，不得用推测或 model 数据补造结论 |
| route/schema/runId 不一致 | run invalid；不得静默回退 v2.8 route |
| P95 `t90>100ms` | 效果验收 FAIL/invalid；不得降低采样、删除字段或关闭 store 换性能 |

线程模型：

1. 物理仍在 `SandCanvas` frame callback 所属 UI/render 线程执行；gravity 每子步 `O(cols-1)`，无新数组、无嵌套循环、无全局搜索。
2. ledger 序列化与文件 IO 继续由 `P011NaturalObservationWorker` 后台线程执行；UI 线程只复制数组与 postMessage。
3. raster 继续由 host runner 调 `snapshot_display` 采集，不调用 UI API，不改变渲染节奏。
4. 不得因性能新增物理 worker、减少 substep、降低 raster 采样或关闭观测来“修复”门槛。

### 18.7 旧测试契约冲突与交接

现有 `p011_velocityGainDoesNotOverrideShapeRateCap` 断言 `bottomCreepVelocityGain=0` 与 `=36` 的 `bh/bl` 状态完全相同，属于 v2.11 之前的旧契约。**manager 已授权修订该用例**，修订交 tester 执行，developer 不得私自修改。

新契约语义：

1. 两份输入的 `bottomCreepRate` 必须仍完全相同，且均 `<=vBottomCreepMax=10`；增益不得改变 `targetShapeRequestRate` 或绕过 shape rate cap。
2. 当 `potential>0` 时，`bottomCreepVelocityGain=36` 允许产生与 `=0` 不同的 `bh/bl` 状态差异。
3. 差异必须满足每列 `bh+bl` 不变、`sandTop` 不变、heavy/light 总量守恒。
4. bottom 与 total 预算使用量必须包含 `2q`，且不得超过 `10dt` / `300dt`。
5. 建议用例名保持不变以便追溯；断言从“状态无差异”改为“shape rate 不变 + 有界守恒交换”。测试资产中仅同步 v2.11 routeId 字面值，不得放宽其他行为期望。

Tester 待办：

1. 修订上述授权用例，并同步 route identity 断言；除该语义修订与机械 route 字面值同步外，不得修改其他断言。
2. 新增/扩展纯函数契约测试：dead zone、`potential<=0`、stock cap、rate cap、bottom/total 裁剪、`bh+bl` invariance、Probe null 等价。
3. 执行完整 v2.11 HIL run，生成独立 test/effect 报告；任何 hard gate 或 raster/P95 失败不得通过调参放宽。

### 18.8 实施步骤

1. tester 先完成 §18.7 授权测试修订；developer 等测试资产合并后再实现。
2. developer 新增 `P011GravityAdvection.ets`，先完成纯 plan/apply 单元契约，不接 UI。
3. 修改 `WaterSandField`：升级 routeId、接入两阶段 plan/apply、预算与错误映射，保持 shape request 不变。
4. 扩展 observation schema v3 与 Probe record，确认 Probe null 时完整 snapshot 序列不受影响。
5. 参数化 ledger artifact 前缀，新增 v2.11 runner 与 checker；先做 synthetic ledger/raster schema 校验。
6. 执行一次默认 `128/192/28`、`dual-input-v1`、`0/+15/0/-15/0` 完整 run，冻结 source/HAP/log/raster SHA256。
7. 检查 hard gates、`A_B>=2px`、P95 `t90<=100ms`、预算、守恒与 `bh+bl` invariance；失败时修复实现或回滚整条 route，不得运行时降级。
8. tester/effector 分别生成 `docs/test-report-P011-gravity-advection-v2.11.md` 与 `docs/effect-report-P011-gravity-advection-v2.11.md`，交 designer 复核。

变更记录：2026-10-19，依据 `algorithm-P011.md` §8 新增 §18。本节将 `bottomCreepVelocityGain` 激活为重力交换增益，同时保持既有预算、几何、门槛、守恒与崩塌约束不变；v2.10 观测 Probe 继续零侵入复用。

## 19. v2.12 工程方案：natural-relax-hysteresis-v1（2026-10-19）

### 19.1 工程结论与范围

**状态：`ARCHITECTURE_PENDING_OBSERVATION`。**

本节只完成架构设计，不授权直接实现。实现前仍必须满足 `algorithm-P011.md` §8.11 的两项前置条件：

1. 取得逐边起动/停止转折观测与留出验证，并以此确定 `thetaStart/thetaStop`。
2. Architect 复核所有会修改 `sandTop` 的通道都接入统一 hard guard，且不引入额外预算、几何或目标形状。

**目标**：

1. 将 `relaxAngle` 的单一触发阈值替换为逐边双阈值滞回状态机。
2. 保留既有 donor/receiver 方向、`splitTopV2` 材料拆分、局部/total 预算和守恒审计。
3. 为阈值标定提供逐边、逐子步、可复核的观测账本。

**非目标**：

1. 不修复 `slip-overshoot-v1` 的 `effectiveTilt` 参考跳变、pre-commit guard 缺失或预算竞争问题；该缺陷仍独立保持 `BLOCKED`。
2. 不新增目标形状、ROI 权重、预设列号或任何视觉优先级。
3. 不增加 bottom/total/local 预算，不改变几何、容量、材料守恒或 `tan30°` 硬门槛。
4. 不把 `angleDeg=4°`、`thetaStart/thetaStop` 与 `thetaHardMax=30°` 合并为同一语义。

### 19.2 模块划分与文件职责

| 文件 | 变更类型 | 职责 |
|---|---|---|
| `entry/src/main/ets/physics/P011RelaxHysteresis.ets` | 新增 | 纯状态机与阈值/残差计算；不 import ArkUI、AppStorage、sensor 或文件 API |
| `entry/src/main/ets/physics/WaterSandField.ets` | 修改 | 持有 `sliding[]`，接入 collect/apply，执行 hard guard 与既有预算 |
| `entry/src/main/ets/physics/P011NaturalObservation.ets` | 修改 | 扩展逐边滞回观测 schema，记录 active 转换、残差、请求与授予 |
| `entry/src/main/ets/physics/P011NaturalObservationStore.ets` | 修改 | 仅传递扩展 record，不改变后台写模型 |
| `entry/src/main/ets/physics/P011NaturalObservationWorker.ets` | 不变或最小修改 | 继续只做 JSON 序列化与追加写 |
| `tools/run_p011_v212_relax_hysteresis.ps1` | 新增 | 独立 runner，显式传入标定后的阈值并复用 raster 采集 |
| `test/p011_v212_relax_hysteresis_check.py` | 新增 | 校验状态机、预算、守恒、hard guard、阈值标定与 holdout 结果 |

### 19.3 核心接口

```ts
export interface P011RelaxHysteresisParams {
  thetaStartDeg: number;
  thetaStopDeg: number;
  thetaHardMaxDeg: number;
}

export interface P011RelaxHysteresisThresholds {
  slopeStart: number;
  slopeStop: number;
  slopeHardMax: number;
}

export interface P011RelaxHysteresisEdgeSample {
  interfaceIndex: number;
  d: number;
  r: number;
  activeBefore: boolean;
  activeAfter: boolean;
  requested: number;
  granted: number;
  budgetScale: number;
}

export function p011RelaxHysteresisThresholds(
  params: P011RelaxHysteresisParams
): P011RelaxHysteresisThresholds;

export function p011RelaxHysteresisNextActive(
  activeBefore: boolean,
  residual: number,
  thresholds: P011RelaxHysteresisThresholds
): boolean;

export function p011RelaxHysteresisRequested(
  residual: number,
  thresholds: P011RelaxHysteresisThresholds,
  dt: number,
  maxAvalancheFlux: number,
  donorSand: number,
  receiverCapacity: number
): number;
```

接口语义：

1. `d[c] = sandTop[c+1] - sandTop[c] - tan(effectiveTilt) * dx`，`r[c] = abs(d[c]) / dx`。
2. `slopeStart = tan(thetaStartDeg)`，`slopeStop = tan(thetaStopDeg)`，`slopeHardMax = tan(thetaHardMaxDeg)`。
3. 参数不变量为 `0 <= thetaStopDeg < thetaStartDeg <= thetaHardMaxDeg`，且 `thetaHardMaxDeg <= 30`。
4. `activeBefore=false` 且 `r >= slopeStart` 时进入滑行；`activeBefore=true` 且 `r <= slopeStop` 时退出；两阈值之间保持原状态。
5. `requested` 只在 `activeAfter=true` 时大于 0，计算沿用 `max(0, r - slopeStop) * dx / 2` 并受 `maxAvalancheFlux*dt`、donor 库存与 receiver 容量截断。
6. `granted` 与 `budgetScale` 由 `WaterSandField` 的既有预算/裁剪流程产生，不由状态机自行放大。

### 19.4 `WaterSandField` 插入点与时序

```text
WaterSandField.step
  └── each substep
       ├── saveBackup / updateSandTop
       ├── updateTilt(dt)
       ├── beginSubstepMotionBudget(dt, collect=true, 1)
       ├── probe.beginSubstep(context, stateBefore)
       ├── compute d/r from current sandTop
       ├── update sliding[c] by dual thresholds
       ├── collect requests only for active edges
       ├── restoreMotionSnapshot / beginMotionApplication
       ├── apply requests through existing clip + splitTopV2
       ├── recompute sandTop and d/r
       ├── update sliding[c] again only at next collect boundary
       └── probe.finishSubstep(stateAfter, budgets, audit)
```

关键时序要求：

1. `effectiveTilt`、`stateBefore`、grant 与 audit 必须使用同一子步快照；不得再出现 context 落后一子步的错位。
2. `sliding[c]` 在同一连续 run 内跨子步保留，预算限流不清状态。
3. reset、几何重建、列数变化、TEST↔EXPERIENCE 切换时清空 `sliding[]`。
4. 0° 暂停、倾角反向或输入抖动不清状态；每子步按当前 `effectiveTilt` 与实际 `sandTop` 重算 `d/r`。
5. 状态数组长度为 `cols-1`，不保存目标形状、相位 baseline 或 ROI。

### 19.5 Hard guard 与预算

对 `sandTop` 的写入者逐通道显式登记；guard 在**每个通道实际提交每笔材料增量之前**运行。不得等整个 `applyMotionChannels` 结束后才统一检查。guard 按下方合法态/违规态谓词对 `S'=S+deltaS` 计算受影响边的带符号 residual，并以闭式区间求出最大可准入 grant。通过后才写材料和记账；后续 writer 以刚提交后的真实 `S` 为基准重新检查。

本节记 `H=slopeHardMax=tan(thetaHardMaxDeg)`；每个残差都使用本子步已提交的 `effectiveTilt`。

| writer | `S` 写入类型 | 单笔写入对 `S` 的影响 | guard 覆盖边 |
|---|---|---|---|
| `relaxAngle` | 列间搬运型 | donor 列 `S` 减少 `q`、receiver 列 `S` 增加 `q`；总沉积量守恒 | 不能只查正在修正的边 `c`。一次 `c↔c+1` 搬运同时检查所有相邻差分：中央边 `c` 与外侧邻边 `c-1`、`c+1`（越界边忽略） |
| `creepBottom` | 列间搬运型 | donor 的 `bh+bl` 与 `S` 减少，receiver 的 `bh+bl` 与 `S` 增加，同一 grant 等量反向 | donor/receiver 两列及其相邻所有边；按实际 `deltaS[]` 联合预演 |
| `liftByShake` | 减少型（列内沉积转悬沙） | 单列 `bh/bl` 转入 `suspH/suspL`，该列 `S` 减少，不增加其他列 `S` | 写入列左右相邻边 `c-1`、`c`（越界边忽略） |
| `settleSandV2` | 增加型（悬沙回到同列沉积层） | 单列 `S` 增加 `heavy+light`，不改变其他列 | 写入列左右相邻边 `c-1`、`c`（越界边忽略） |
| `percolateHeavyConservative` | `S`-invariant（列内相间重分配） | 同列 `bh/bl/mh/ml` 重分配；每列 `bh+bl+mh+ml` 不变，故 `ΔS=0` | 不产生坡度边增量，但仍作为一个受事务跟踪的 writer 登记；记录 `SInvariant=true`、逐列 `deltaS=0` 和请求/授予/预算使用，纳入本子步 writer 顺序及审计 |

`relaxAngle` 的边级联采用闭式联合预演：每次候选边 grant `q` 都检查中央边及两条外侧邻边，不只检查目标边。对每个受影响边 `e`，令当前带符号残差 `d_e=(S[e+1]-S[e])/dx-tan(effectiveTilt)`，并令 `k_e=(∂deltaS[e+1]/∂q-∂deltaS[e]/∂q)/dx` 为每单位 grant 对该边坡度的有符号影响，则 `d'_e(q)=d_e+k_e*q`。合法态要求 `-H<=d'_e(q)<=H`；每个边给出一个关于 `q` 的闭区间，与 `0<=q<=qGranted` 取交集，取交集上界作为最大可准入 grant。`k_e=0` 时该边要么不限制 `q`，要么判该候选不可行。由于本笔材料事务的列增量对 grant 线性，本判定无需迭代或二分。commit 后下一个候选边再基于更新后的 `S` 独立求闭式上界。

### Guard 边界与状态谓词

本节 guard 只覆盖**材料 writer**，检查点是每笔 writer 增量提交前；它不覆盖 `updateTilt` 对参考角 `effectiveTilt` 的更新。倾角跳变可能在任何材料 writer 运行前就使 residual 越界，这类参考角可行性由 §20 动态可行域限幅处理。两个机制各自负责倾角更新与材料提交的边界；不得把材料 guard 描述为零违约保证，也不得用后续 `relaxAngle` 修复抹掉已记录的越界。

对每笔候选增量，按提交前全场状态分支：

1. **合法态**（所有边 `|d_e|<=H`）：准入 grant 必须令提交后所有边仍合法；受影响边按闭式约束求最大 `q`，未受影响边保持不变。
2. **违规态**（至少一边 `|d_e|>H`）：所有原本合法的边提交后仍须合法；每条原本违规边须满足 `|d'_e|<=|d_e|`（non-worsening），且不得新增越界边。此分支只允许在既有预算内逐步修复，不会把当前子步标成通过；run 保留 hard-gate failure，直至外层验收规则处理。

违规态同样是线性闭区间求解：原本合法边使用 `-H<=d'_e(q)<=H`，原本违规边使用 `-|d_e|<=d'_e(q)<=|d_e|`，再与 `0<=q<=qGranted` 求交。若交集为空，grant 置零；不得选取会扩大任一已有违规的部分 grant。

闭区间直接按系数符号求出：令每条边的容许界为 `L_e=H`（合法边）或 `L_e=|d_e|`（违规边），约束为 `-L_e<=d_e+k_e*q<=L_e`。当 `k_e>0`，该边区间是 `[(-L_e-d_e)/k_e, (L_e-d_e)/k_e]`；当 `k_e<0`，交换这两个端点；当 `k_e=0`，若 `|d_e|<=L_e` 则不收窄 `q`，否则本笔无可行 grant。将所有受影响边区间与 `[0,qGranted]` 相交，最终上端点即 `qAccepted`；若下端点大于上端点则 `qAccepted=0`。这给出单一标量 grant 的精确最大值，不需要二分或迭代。

### Helper、生命周期豁免与预算回写

`splitTopV2`、`splitBottomPhaseBalanced` 是材料拆分 helper，不具备独立 guard 或预算语义；只能由已经完成本笔 admission guard 的具名 writer 调用。helper 不得被无 guard 的直接路径、测试注入路径或其他 writer 复用来写材料。

下列离散生命周期操作明确豁免于逐笔物理材料 writer guard，理由是它们不是正常积分中的按额度材料提交；豁免不代表免除各自现有验证或可以静默通过：

| 操作 | 豁免理由 | 仍需执行 |
|---|---|---|
| `initialize` / `reset` | 以完整场景快照替换整个材料场，是新 run 的建场边界，无逐子步 grant 可供 admission | 场景参数、库存/容量与既有初始化 audit；新 run 首个子步记录实际初始坡度，若越硬界不得报告零违规通过 |
| `inject` | 外部测试/场景操作不属于物理通道，不消费本子步 local/bottom/total grant；已沉积态注入增加 `S`，悬沙态注入不改变 `S` | 输入范围、容量与守恒基线更新；记录 phase/state、注入身份及前后 `S`，若越硬界标为场景干预/硬门槛失败 |
| `invert180` | 离散相态变换，将全部沉积物映射至悬沙并镜像场，使沉积 `S` 变为零；不是连续搬运通道，无法解释为单笔 local grant | 保留现有全场容量/数值检查；记录显式变换事件及前后快照，不冒充普通材料 grant；后续物理提交仍由 §20/材料 guard 约束 |

任何其他能写 `S` 的路径均不自动豁免；必须加入 writer 清单和受影响边事务观测，否则 hard-guard 覆盖不完整。

guard 截断 grant 后，必须将 `qGranted-qAccepted` 同步退回原事务账本：`localUsed/localRemaining`、`bottomUsed/bottomRemaining`、`totalUsed/totalRemaining`、`channelUsed/channelRemaining` 四层均以最终接受量重算，且不突破各自上限。预算释放仅按既有分配顺序可供尚未执行的后续通道使用；不得重发当前 writer 被拒绝的那部分，不欠账、不跨子步结转。Probe 同时记录原 request、预 guard grant、接受 grant、拒绝量及四层预算回写前后值。

通用规则：

1. 滞回 `relaxAngle` 不消耗 bottom 预算；各通道仍使用既有 local `vSandTopMax*dt`、`maxAvalancheFlux*dt`、bottom `10*dt`（仅 creepBottom）与 total `300*dt` 额度。
2. 每笔提交后立即检查本笔受影响边；`applyMotionChannels` 结束时仍做全场审计 `r[c] <= slopeHardMax`，作为最终账本核验，不能替代逐 writer guard。

### 19.6 观测契约

v2.12 观测 record 必须新增：

```ts
interface P011RelaxHysteresisObservation {
  effectiveTilt: number;
  thresholds: P011RelaxHysteresisThresholds;
  edges: P011RelaxHysteresisEdgeSample[];
  hardGuardRejected: number;
  hardGuardClipped: number;
  maxResidualBefore: number;
  maxResidualAfter: number;
}
```

每个 edge record 必须包含：

1. `(runId, substepId, c, effectiveTilt, d, r)`。
2. `activeBefore/activeAfter` 与转换类型：`start`、`stop`、`holdActive`、`holdInactive`。
3. `requested/granted/budgetScale`。
4. donor/receiver 库存与容量截断原因。
5. hard guard 拒绝或截断量。

阈值标定输出必须包含：

1. `thetaStart/thetaStop` 的点估计与 95% bootstrap 区间。
2. 校准 run 与 holdout run 的样本数、误判率和 hard gate 违规数。
3. 正负残差、±倾角与阈值附近噪声的分层数据。
4. 若区间重叠或转折不可分辨，必须保留 `DESIGN_PENDING_OBSERVATION`，不得填默认值。

### 19.7 Runner 与 checker

1. 新 route：`natural-relax-hysteresis-v1`；artifact 前缀：`p011-v212-relax-hysteresis`。
2. Runner 必须显式传入 `thetaStartDeg/thetaStopDeg`，不得隐式使用 `30/20` 作为默认值。
3. Phase schedule 复用 v2.10/v2.12 观测序列，但校准 run 与 holdout run 必须分离，不得用同一 run 既估参又验收。
4. Checker 必须验证：
   - 参数不变量。
   - 状态转换与阈值边界语义。
   - 正负坡度与 ±倾角方向。
   - 限流期间 active 状态不丢失。
   - local/total/bottom 预算不越界。
   - 守恒、stock、capacity 与 `tan30°` hard gate。
   - 无目标形状、ROI 或列号优先级。
   - 校准与 holdout 的置信区间和误判率。
5. 任何 hard gate、预算、守恒或 holdout 失败均判 `invalid`，不得通过调参放宽。

### 19.8 线程模型与性能

1. 物理与状态机仍在 `SandCanvas` frame callback 所属 UI/render 线程执行。
2. 每子步 `O(cols-1)`，无全局搜索、无嵌套循环、无新增 worker。
3. ledger 序列化与文件 IO 继续由 `P011NaturalObservationWorker` 后台线程执行。
4. UI 线程只复制数组与 postMessage，不进行 JSON stringify。
5. 不得因性能新增物理 worker、减少 substep、降低 raster 采样或关闭观测。

### 19.9 异常处理

| 情况 | 处理 |
|---|---|
| 参数不变量不满足 | 构造/配置阶段失败，禁止进入 run |
| `effectiveTilt` 更新导致 residual 越界 | 记录 hard gate failure；只能用既有预算修复，否则 run invalid |
| 预算不足 | `granted` 被裁剪；`active` 保持，下一子步继续请求 |
| hard guard 拒绝/截断 | 记录拒绝量与原因，不重发、不补预算 |
| 数值非有限 / 负库存 / 容量越界 | 返回错误，restoreBackup 并 freeze |
| Probe/store/worker/raster 失败 | 本轮 run invalid，不得用推测数据替代 |
| route/schema/runId 不一致 | run invalid，不得静默回退 |

### 19.10 实施步骤

1. 新增 `P011RelaxHysteresis.ets`，先完成纯状态机、阈值与请求函数。
2. 修改 `WaterSandField`：持有 `sliding[]`，接入 collect/apply，并统一 `effectiveTilt` 采样时序。
3. 为所有修改 `sandTop` 的通道接入统一 pre-commit hard guard。
4. 扩展 `P011NaturalObservation` schema 与 Probe record。
5. 新增 v2.12 runner 与 checker，先做 synthetic ledger/raster schema 校验。
6. 使用标定 run 估计 `thetaStart/thetaStop`，再用独立 holdout run 复核。
7. 执行完整 HIL run，冻结 source/HAP/log/raster SHA256。
8. 生成 test/effect 报告；任何 hard gate 或 holdout 失败不得通过调参放宽。

### 19.11 与 `slip-overshoot-v1` 的关系

v2.12 不解除 `slip-overshoot-v1` 的 `BLOCKED` 状态。该缺陷的独立修复必须先处理：

1. `effectiveTilt` 更新前的可行性检查。
2. 所有修改 `sandTop` 操作的 pre-commit slope guard。
3. 现有 total/local/bottom 预算竞争。
4. 观测 context 与 grant/audit 子步对齐。

双角度滞回只改变“何时滑”，不改变参考坡度跳变、全局预算缩放或均匀通量抵消；不得作为该缺陷的单独修复。

### 19.12 变更记录

2026-10-19，依据 `algorithm-P011.md` §8.11 新增 §19。本节仅落盘架构设计；实现仍等待阈值标定与 hard guard 复核，不修改代码或测试用例。2026-10-19 二次修订 §19.5：补齐 settle/percolate writer、合法/违规态闭式 grant 交集、材料 guard 与 tilt 可行域边界、helper/生命周期豁免及四层预算回写。

### 19.13 观测①b：激活逐边转折遥测——交付验收 checklist（2026-10-19）

**状态：`READY_FOR_DEVELOPER`（仅授权观测遥测的激活与补全；不改变 §19.1 的 `ARCHITECTURE_PENDING_OBSERVATION`，不授权实现双阈值状态机，不标定 `thetaStart/thetaStop`）。**

依据：manager 2026-10-19 观测①b 任务（激活逐边转折遥测，为 v2.12 双角标定供数）、`algorithm-P011.md` §8.11 v2.10 逐边转换事件契约、§17 ledger schema v3 与 §17.9 观测隔离验收。本节是 developer 交付指令与验收口径，四项全部 MUST-PASS，逐项勾选。

#### 19.13.1 验收 checklist

- [ ] **□1 转折记录字段完整**：每条边转换时 activeBefore/activeAfter/transitionType(start|stop)/effectiveTilt/残差 r 全部非空。
  - 判定口径：正式七相 run ledger 的 `slip[]` 数组内每条边记录，六字段全部有效——`activeBefore`/`activeAfter` ∈ {true,false}（禁止 null/缺失）；`transitionType` ∈ {start,stop,holdActive,holdInactive} 且与 activeBefore/activeAfter 组合一致（按 checker `expected_transition` 规则：false→true=start，true→false=stop，true→true=holdActive，false→false=holdInactive）；`effectiveTilt` 为有限数；`d` 与 `r` 为有限数且 `r=|d|`。start/stop 转换事件是 v2.12 阈值标定的 bracket 样本来源；holdActive/holdInactive 保持类记录同样必须字段完整。
  - 字段契约锚点：`P011NaturalObservationEdgeGrant`（`P011NaturalObservation.ets:86-98`）。
  - 验证：`test/p011_v210_natural_observation_check.py` 的 `validate()` 对正式 run ledger 零 `activeState`/`transitionType`/`transitionMismatch`/`.r` 错误。

- [ ] **□2 无 unavailable 残留**：正式七相 run 的 ledger 中 transitionType=unavailable 的记录数必须为 0（合成自测 ledger 中允许）。
  - 判定口径：计数范围限定 `slip[]`（relax 滑移通道）——正式七相 run ledger 内全部 `slip[]` 记录中 `transitionType='unavailable'` 的计数必须为 0。
  - 工程边界（重要）：`bottomCreep[]` 与 `suspensionDiffusion[]` 的边记录不属于 relax 通道，无 sliding 语义；其 `transitionType='unavailable'` 是 schema 设计内的既定行为（`completeEdgeGrant` 对未携带 active 状态记录的标准标注），**不计入本项验收**，且**不得为凑零伪造这两个通道的 active 状态**——那会违反 §17.9.10（probe 只复制状态机决定，不伪造）与 algorithm §8.11 零侵入边界。若 manager 要求全 ledger 严格为 0，须先升级 schema 并授权非 relax 通道省略 transitionType 字段，属独立变更，不在本任务范围。
  - 合成自测 ledger（`test/test_p011_transition_telemetry.py` 构造的 unavailable/telemetryUnavailable 用例）允许出现 unavailable。
  - 验证：对正式 run `ledger.ndjson` 逐 record 统计 `slip` 数组内 unavailable 计数为 0。

- [ ] **□3 transitionTelemetryAvailable=true，且合成 ledger 校验转折记录完整**。
  - 判定口径：正式七相 run 每个 ledger record 的 `context.transitionTelemetryAvailable===true` 且 `context.ledgerSchemaVersion===3`（两者绑定：available=true 仅在 schema v3 合法）。实现锚点：`WaterSandField.ets` `beginNaturalObservation()` 的 context 写入点（当前实际行号 :1025；manager 任务书引用 :1010 为行号漂移，同一调用点）；常量 `P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION=3`（`P011NaturalObservation.ets:8`）、`P011_NATURAL_OBSERVATION_ROUTE_ID='natural-relax-hysteresis-v1'`（`P011NaturalObservation.ets:7`）。
  - 合成 ledger 校验：`python -m unittest discover -s test -p "test_p011_transition_telemetry.py"` 全部 PASS（现有用例已覆盖：available 状态校验 transition、缺失 activeState/transitionType 拒绝、available 绑定 schema v3、unavailable 状态拒绝伪造、guard 值与 availability 绑定）。
  - checker 边界：`validate_observation_contract()` 的 `runRole`/`thresholdCalibrationId`/`thetaStartDeg`/`thetaStopDeg` 分支仅适用于 v2.12 校准/holdout run（`runRole∈{calibration,holdout}`）；观测①b 正式七相 run 为观测 run（`runRole=null`），不适用该分支——不得因该分支报错而判观测①b FAIL，也不得为绕过它伪造校准身份或 candidate 阈值。

- [ ] **□4 物理逻辑与参数零改动**：relaxAngle 判据、预算、常量逐字不变（只加观测记录）。
  - 判定口径：diff 审查以下物理路径无任何语义变更——`relaxAngle()`（`WaterSandField.ets:1701-1804`：`tan(angleDeg)` 判据、`excess/2` 请求量、`maxAvalancheFlux*dt` 截断、donor/receiver 方向与 `receiverCapacity` 公式、`clipSubstepMotion` 裁剪）；预算通道（`beginSubstepMotionBudget`/`clipSubstepMotion`/`clipTotalMotion`/`clipBottomCreepMotion`：local `vSandTopMax*dt=28*dt`、total `totalMotionBudget=300*dt`、bottom `10*dt` 仅 creepBottom）；全部物理常量（`angleDeg=4`、`maxAvalancheFlux`、`totalMotionBudget=300`、`vSandTopMax=28`、`vBottomCreepMax=10`、`percolationRate`、扩散系数、`P011_HEAVY_SHARE_*` 等）逐字不变。
  - 允许变更范围（仅观测侧）：`recordSlip`/`recordBottomCreep`/`recordSuspensionDiffusion` 等遥测调用点、`naturalObservationSliding*` 观测影子状态数组（`relaxAngle()` 判据不读取它，不影响物理轨迹）、schema/契约文件、合成测试资产。
  - 行为等价验收：Probe=null 时同输入（固定 seed/schedule）完整状态序列与改动前逐项一致（沿用 §17.9.1 口径）；probe on/off 同输入物理轨迹一致（§17.9.10）。

#### 19.13.2 Developer 交付要求（通知）

1. 按 §19.13.1 四项 checklist 交付；四项全部勾选后，才可提交正式七相 run 的 ledger/raster/manifest 冻结证据。
2. 自验顺序：先跑合成 ledger 单测（□3），再执行正式七相 run 与专项计数（□1/□2），最后做 diff 审查与 probe=null 等价验证（□4）。
3. 任何一项不满足即 BLOCKED 返工；不得通过放宽口径、伪造通道状态或修改 checker 判定转绿。
4. 本任务不授权：实现双阈值状态机、填 `thetaStart/thetaStop` 默认值、修改 `relaxAngle` 判据/预算/常量、处理 slip-overshoot-v1 的 BLOCKED 事项。

变更记录：2026-10-19，依据 manager 观测①b 任务新增 §19.13。仅落盘验收 checklist 与 developer 交付指令；不修改代码、既有测试用例或历史结论。§19.1 状态保持 `ARCHITECTURE_PENDING_OBSERVATION`。

## 20. v2.14 动态倾角可行域限幅对接材料

**状态：BLOCKED / DESIGN_NOT_CALIBRATED。** 本节把 `algorithm-P011.md` §8.13 的逐子步动态可行域方案翻译为 architect 可据以细化模块与调用点的契约。它不授权实现，不改变任何预算、几何、硬阈值或体验门槛；v2.14 当前没有多档 omega 的真机 P95 数据，也没有批准的固定 `maxTiltOmega`。

### 20.1 输入、输出与不可变快照

每个物理子步开始时只建立一次不可变输入快照：

```ts
interface P011TiltFeasibilityInput {
  runId: string;
  substepId: number;
  dt: number;
  thetaBefore: number;       // 当前已提交 effectiveTilt，degree
  targetTilt: number;        // 输入层目标，degree
  maxTiltOmega: number;      // 待标定候选，degree/second
  sandTopBefore: Float64Array; // S0，cols 个
  budgetsBefore: ExistingBudgetSnapshot;
  channelStateBefore: ExistingChannelStateSnapshot;
}
```

规划器返回被接受的 `thetaNext`、本子步各既有通道的计划与实际 grant 预演、每边 `gBefore/deltaG/rNext`、可行区间、active constraint edge、clipped 请求与 BLOCKED 原因。所有角度内部运算须统一 degree/radian 转换；`sandTop` 高差先按 `dx` 归一为坡度。规划阶段不得写材料场、预算累计器、滑移状态、输入状态或 Probe。

### 20.2 两轮 planning pass 的确定时序

“两轮”指最多两次**纯规划/预算预演**，不是同一子步执行两次物理，也不是获得两份预算。两轮都从相同 `S0`、`thetaBefore` 与 `budgetsBefore` 起算；只有最终选中的计划提交一次。通道顺序和预算优先级必须复用当前 `WaterSandField` application 事务的单一权威定义，预演不得重新排序或另设优先级。

```text
snapshot S0, theta0, target, dt, budget B0, channel states
thetaRequested = moveToward(theta0, target, maxTiltOmega * dt)

PASS 1 (preview only, no mutation):
  plans1 = collect all existing channel requests from S0 at thetaRequested
  grants1 = allocate plans1 once against a copy of B0,
            using the existing channel order and all existing caps
  deltaG1[c] = sum of sandTop-slope effects of every granted channel at edge c
  I1 = intersect over c of:
       atan(g0[c] + deltaG1[c] - H) .. atan(g0[c] + deltaG1[c] + H)
  if I1 is nonempty:
    theta1 = clamp(thetaRequested, I1, and the max-omega step interval)
  else:
    mark target-angle candidate infeasible; do not invent theta1

if I1 is nonempty and theta1 == thetaRequested and grants1 are admissible:
  commit (theta1, plans1, grants1) exactly once
else:
  PASS 2 (replan from the same snapshot; discard pass 1):
    if target-angle candidate theta1 exists:
      plans2 = recollect all channel requests from S0 at theta1
      grants2 = allocate the complete plans2 against a fresh copy of B0,
                with the same existing order and caps
      compute deltaG2 and I2 from actual planned grants
  if theta1 exists and theta1 in I2 and all post-commit invariants are admissible:
    commit (theta1, plans2, grants2) exactly once
  else:
    evaluate the old-angle fallback plan at theta0 as the second-pass
    alternative, from S0 and another fresh copy of B0; do not commit either
    alternative until the complete old-angle plan passes the same checks
    commit theta0 plus that safe old-reference plan, with rejected requests clipped
    if no admissible committed state exists: BLOCKED; do not mutate this substep

after the single commit:
  recompute all 127 residuals from committed S and committed theta
  audit conservation, stock, capacity, every existing budget, and abs(r[c]) <= H
  publish context, grants, effectiveTiltRate, feasibility telemetry, and audit
```

`I1/I2` use the exact edge condition `|g0[c]+deltaG[c]-tan(theta)|<=H`, with `H=tan(30°)` and current `dx`; intersection is computed in slope space then mapped through monotone `atan` on the configured tilt domain. The accepted angle is additionally intersected with `[theta0-maxTiltOmega*dt, theta0+maxTiltOmega*dt]` and the directed interval from `theta0` toward `targetTilt`. Empty intersection is infeasible. A clamp in pass 1 requires pass 2 because channel requests, donor/receiver direction, and shared grants may depend on accepted effective tilt. If pass 2 produces an interval excluding `theta1`, its old-angle alternative is evaluated within pass 2 against a separate fresh copy of `B0`; no third planning pass is allowed. Each alternative is a complete all-channel allocation, and only one can commit.

`deltaG[c]` must be derived from the combined, actually previewed grants of every channel that can change `sandTop`, including settle, percolation, creep, relax/avalanche, lift/rise, and any route-specific advection. Compute it from the complete per-column planned `deltaS`, not a local request, a single edge's theoretical cap, or the sum of absolute fluxes; neighboring fluxes can cancel. Channels that only exchange `bh/bl` while preserving each column's `bh+bl` contribute zero `deltaS`, but still consume their existing budgets if their accounting contract says so. Any unmodeled `sandTop` writer makes the preview incomplete and prohibits a safety claim.

### 20.3 与现有通道及预算账本的接口

Architect 细化接口时应将每通道分成只读 planning 与单次 commit 两个角色，至少具备：

```ts
interface P011ChannelPlan {
  channelId: ExistingChannelId;
  requested: ExistingRequestShape;
  plannedGrant: ExistingGrantShape;
  deltaSandTopByColumn: Float64Array;
  budgetUse: ExistingBudgetUse;
  admissibility: 'admissible' | 'clipped' | 'rejected';
}

interface P011TiltFeasibilityResult {
  thetaRequested: number;
  thetaAccepted: number;
  feasibleThetaMin: number;
  feasibleThetaMax: number;
  limitingEdge: number | null;
  plans: P011ChannelPlan[];
  passCount: 1 | 2;
  fallbackUsed: boolean;
  status: 'accepted' | 'clamped' | 'held' | 'blocked';
  blockedReason: string | null;
}
```

`ExistingBudgetSnapshot` 是现行 local、bottom、total 和各通道 rate cap 的剩余额度快照；每轮均以同一快照的副本预演，pass 1 的扣账必须整体丢弃后才能启动 pass 2。最终 commit 只消费选中计划的 grant，累计一次真实 `requested/granted/clipped`。不允许通过计划重算重复扣账、结转被拒额度、创建 correction channel、把崩塌修复额度与原有 relax/total 预算叠加，或让预演本身改动 `sliding[]`/方向滞回状态。状态转换只在最终 commit 所用的已接受参考角与材料快照上推进一次。

现有 `updateTilt(dt)` 不得在 grants/audit 之前单独提交参考角。子步 context、`stateBefore`、选中计划、实际 grant、提交后 `stateAfter` 和 audit 必须共享同一 `runId/substepId`；`effectiveTilt` 表示 `thetaAccepted`，`effectiveTiltRate=(thetaAccepted-thetaBefore)/dt`。Probe 记录两轮诊断但不得参与规划或更改排序；pass 1/被丢弃 pass 2 只能作为诊断记录，不能记成实际 grant。

### 20.4 BLOCKED 与回退语义

| 条件 | 本子步处理 | run/验收语义 |
|---|---|---|
| 当前状态合法，投影后 angle 与真实预算 grant 可共同满足全部边界 | 按唯一选中计划提交；被 clamp 时记 `clamped` | 继续；clamp/hold 时长进入跟手性统计 |
| pass 1 clamp 后 pass 2 可行且符合原 cap | 丢弃 pass 1，commit pass 2 一次 | 继续；passCount=2 |
| pass 2 不闭合，但旧 `thetaBefore` 下可重算出合法计划 | 丢弃两轮预演，采用旧角度回退计划；拒绝请求量记 clipped，不结转 | 继续并标 `held/fallbackUsed`；纳入角度延迟与 P95，不可记作成功追随 |
| 可行域交集为空，或旧角度回退仍无法在原预算内形成合法提交状态 | 不提交倾角和材料状态；记录 `BLOCKED`、edge/预算/通道证据与原因 | 本 run 标 `BLOCKED/invalid`，硬安全门槛失败；不得静默暂停后仍判通过 |
| 子步开始时状态已违反 `H`，且原预算内不存在恢复到合法集合的实际 grant | 不允许用角度投影伪装恢复，不写材料；保留失败快照 | `BLOCKED`，run invalid；不得放宽 `tan30°` 或补预算 |
| grant 预演与真实 apply 不一致，或 commit 后任一边 `|r|>H` | 立即冻结/restoreBackup（沿用现有事务错误恢复），记录首个违规边和两套账本 | 硬失败，run invalid；说明规划器或通道接口不完整 |
| 有限值/库存/容量/守恒/任一既有预算失败 | 沿用当前 field 错误恢复，不允许靠后续子步冲销 | `BLOCKED/invalid` |

`BLOCKED` 不是把角度偷偷 clamp 到任意值后继续，也不是可忽略 warning：必须携带 `reasonCode`（如 `EMPTY_FEASIBLE_INTERVAL`、`NO_SAFE_OLD_TILT_PLAN`、`PREEXISTING_INFEASIBLE_STATE`、`PREVIEW_COMMIT_MISMATCH`）、`runId/substepId`、theta requested/accepted、最紧边、`g/deltaG/r`、通道 grants、预算剩余与拒绝量。只要发生 BLOCKED，该 run 不得满足“零越界”验收；恢复运行若由调用层选择继续，必须作为同一 run 的明确 invalid continuation 记录，不能抹除阻断事件。

### 20.5 复杂度、证据与状态

每个 planning pass 遍历既有 `O(cols)` 通道计划与 `O(cols-1)` 边。pass 2 可评估目标候选与旧角度回退两个完整分配，各自使用独立的 `B0` 副本；这是同一快照的备选预演，不串接预算，最终只提交一个候选。最多两轮规划、最多三个候选分配，复杂度仍为 `O(cols)` 时间与 `O(cols)` 临时存储，没有全局搜索、排序、额外物理线程或额外物理子步。Architect 应在详细设计中证明 pass 上限、预算优先序与并列分配确定性、全部 `sandTop` writer 已接入、预演纯度及 preview/commit 一致性。

硬安全公式只能证明单步可行性，不能证明用户体验：v2.14 仍须扫描候选 `maxTiltOmega` 并用冻结输入、真机 raster 和独立 run 测现有 P95 T90 `<=100ms`。动态投影开启后仍不达标即为安全/跟手约束冲突，保持 BLOCKED 并交 manager 裁决；不以降低安全上限、增加预算、删样本或单独报告 effectiveTilt 到位时间替代。

### 20.6 实现细则与证明义务

本节补齐 §20.5 要求的四项实现前证明，不改变 §20 的 `BLOCKED / DESIGN_NOT_CALIBRATED` 状态，也不授权绕过标定直接接入物理循环。现阶段的 `P011TiltPlanningPass.ets` 只是服务级骨架：`createPreview()` 返回阻断结果，`commit()` 抛出 `P011_PLANNING_PASS_BLOCKED`。后续实现必须先满足下列义务，才能把该骨架替换为可提交路径。

#### 20.6.1 Pass 上限与三候选闭包

候选角集合在本控制律下是封闭的三元集：

```text
C = {thetaRequested, theta1, theta0}
```

其中 `thetaRequested` 由 `moveToward(theta0, target, maxTiltOmega*dt)` 唯一产生；`theta1` 只在 pass 1 的可行区间非空时由 `clamp(thetaRequested, I1, max-omega interval)` 唯一产生；`theta0` 是子步开始时的已提交参考角。除此之外，本控制律不生成其他候选角。

因此 pass 上限证明如下：

1. **pass 1** 只在 `thetaRequested` 上收集请求并预演一次。若 `theta1==thetaRequested` 且分配可准入，则直接提交，`passCount=1`。
2. **pass 2** 只在 `theta1` 上重算请求并预演一次；若 `theta1` 不存在或不可准入，则同一 pass 内再评估 `theta0` 回退候选。二者共用同一 `S0/B0` 快照，但各自使用独立预算副本，最终只提交一个候选。
3. **不存在第三轮**。任何第四候选角 `theta*` 只有两种来源：
   - 若 `theta*` 仍在 `C` 中，则它已由前三候选覆盖，是冗余候选。
   - 若 `theta*` 不在 `C` 中，则它要么落在 `maxTiltOmega*dt` 之外，非法；要么位于 `theta0` 与 `theta1` 之间。后者在跟手性上被 `theta1` 支配，且若要证明安全仍需一次新的全通道重算，即第三轮 planning pass，超出本设计的固定复杂度承诺。

所以，在 §20.2 指定的控制律内，最多两轮 planning、三个候选分配已构成完整候选闭包；若三者均不可提交，正确结果只能是 `BLOCKED`，不得通过追加第四候选或第三轮搜索来继续尝试。

#### 20.6.2 预算优先序与请求比例分摊

预演与提交必须复用现行 `WaterSandField` application 事务的固定通道顺序，不得另设优先级。权威顺序为：

```text
settle → percolate → bottomCreep → relax → lift
```

若实现为完整性单独表示 `suspensionDiffusion`，它仍属于 collect 阶段的既有非 total 通道，不得插入或重排上述 apply 顺序。

共享 total 预算的确定分配流程如下：

1. 在固定通道顺序和固定列/边顺序内收集请求，先应用各通道自身 rate cap、local cap、bottom cap 与库存/容量约束，得到 `r_i`。
2. 计算 `R = Σ r_i`。若 `R <= B_total`，全部请求足额授予。
3. 否则计算唯一比例 `s = B_total / R`，并在固定顺序内执行 `g_i = min(r_i, r_i*s, localRemaining_i, channelRemaining_i, totalRemaining)`。
4. 每笔授予后立即扣减对应 local、bottom、channel 与 total 余量；不回补、不结转、不重发。

并列时的 tie-break 规则固定为：

```text
channelRank 升序 → columnIndex/interfaceIndex 升序
```

即先按 `settle < percolate < bottomCreep < relax < lift` 的固定通道等级，再按列号或边号从小到大。相同通道、相同列/边的请求天然唯一；不同请求之间不比较浮点数值大小，也不按请求量排序。

#### 20.6.3 并列分配的唯一化规则

同一预算下若存在多种合法分配，实现必须采用以下唯一化规则，而不是依赖浮点排序或实现细节：

1. 每个请求拥有固定优先键 `(channelRank, columnIndex)`；边通道使用 `(channelRank, interfaceIndex)`。
2. 分配只按该键的字典序遍历，键值完全由整数和固定枚举组成。
3. 若比例缩放后仍有因数值舍入产生的余量，不重新排序、不二次分配；按上述顺序逐项授予，并把未授予部分记入 `clipped/rejected`。
4. `limitingEdge` 在多条边给出相同约束值时，取 `interfaceIndex` 最小者。

因此任意两个实现只要输入快照、通道顺序、比例公式和枚举顺序一致，就必须得到同一份 `plannedGrant/budgetUse/deltaSandTopByColumn`，不得出现由排序稳定性或浮点比较引起的分叉。

#### 20.6.4 Preview / commit 一致性与失败路径

preview 与 commit 必须共享同一份纯规划函数，不允许复制两套相似逻辑。`P011TiltPlanningPassPreview` 的语义是：

1. `createPreview()` 只读 `S0/theta0/B0/channelState`，生成完整候选计划，不写任何物理或账本状态。
2. `discard()` 只丢弃 preview，不产生副作用。
3. `commit()` 是唯一提交路径，只执行最终选中的 `thetaAccepted` 与 `plans`，并一次性更新倾角、材料、预算和状态。

一致性要求为逐项精确相等，不使用容差：

```text
thetaAccepted === preview.thetaAccepted
plans[i].plannedGrant === committed plans[i].plannedGrant
plans[i].budgetUse === committed plans[i].budgetUse
plans[i].deltaSandTopByColumn[c] === committed deltaSandTopByColumn[c]
```

比较前须把 `-0` 规范化为 `0`，并显式拒绝 `NaN`。若任一项不一致，立即置 `PREVIEW_COMMIT_MISMATCH`，冻结本子步，执行 `restoreBackup()`，保留 preview 与 commit 两套诊断账本，不允许用后续子步或额外 correction 修复。该路径与 §20.4 的 `PREVIEW_COMMIT_MISMATCH` 行为一致。

### 20.7 实现验收 checklist：【阶段一·限幅】planning pass 实现（2026-10-19）

**状态：`CHECKLIST_PREPUBLISHED`。** 本节把 §20.2/§20.4/§20.6 的实现义务提前转为【阶段一·限幅】（planning pass 实现）的交付验收 checklist，供 developer 在实现授权下达后按此交付、一次写对。它是验收口径，不是实现授权：`maxTiltOmega` 仍未标定，§20 顶部的 `BLOCKED / DESIGN_NOT_CALIBRATED` 状态由标定流程解除后才可开始实现；实现完成不自动解除该状态，也不替代 §20.5 的 omega 扫描与真机 P95 T90 `<=100ms` 验收。七项全部 MUST-PASS，逐项勾选。

#### 20.7.1 验收 checklist

- [ ] **□1 两轮 pass 上限**：最多两轮三候选，无第三轮搜索；三候选均不可行则 BLOCKED。
  - 判定口径：候选角集合封闭为三元集 `C={thetaRequested, theta1, theta0}`（§20.6.1）；每子步 planning pass 次数 `<=2`，候选分配次数 `<=3`（pass 1 一次 + pass 2 内目标候选与旧角度回退各一次，pass 2 两候选共用同一 pass 编号）。代码中不得存在任何第三轮循环、重试、追加候选角或围绕 `theta0`~`theta1` 区间的再搜索路径；三候选均不可提交时唯一合法输出是 `status='blocked'`，不得以第四候选或放宽区间继续。
  - 验证：合成输入构造「pass 1 clamp → pass 2 目标候选不可行 → 旧角度回退亦不可行」序列，断言 `passCount<=2`、无第四次分配、结果为 BLOCKED 且携带 §20.4 全量证据。

- [ ] **□2 B0 独立副本**：pass 间/候选间预算不串接，每候选独立预算副本。
  - 判定口径：pass 1 与 pass 2 均从同一 `budgetsBefore` 快照（B0）的**独立副本**起算（§20.2/§20.3）；pass 1 的扣账在 pass 2 启动前整体丢弃；pass 2 内目标候选与旧角度回退各自使用独立 B0 副本，不共享累计器。预演不得改动真实 local/bottom/total/channelRateCap 累计器、`sliding[]` 或方向滞回状态；最终 commit 只消费选中候选的 grant，累计一次真实 `requested/granted/clipped`，无重复扣账、无被拒额度结转、无 correction channel、无 relax/total 预算叠加。
  - 验证：合成多候选序列后断言真实四层预算账本只反映最终 commit 的那一份；pass 1 诊断账本中的 grants 不出现在任何真实累计器。

- [ ] **□3 单次 commit**：只提交最终选中候选，其余丢弃无副作用。
  - 判定口径：每子步对倾角与材料的提交恰好一次，且只执行最终选中候选的 `thetaAccepted` 与 `plans`（§20.2 时序、§20.6.4）。被丢弃的 pass 1/未选中 pass 2 候选只允许作为 Probe 诊断记录（§20.3：pass 1/被丢弃 pass 2 只能作诊断，不能记成实际 grant），`discard()` 零副作用；commit 后 `effectiveTilt=thetaAccepted`、`effectiveTiltRate=(thetaAccepted-thetaBefore)/dt`，子步 context/stateBefore/选中计划/实际 grant/stateAfter/audit 共享同一 `runId/substepId`；`updateTilt` 不得在 grants/audit 之前单独提交参考角。
  - 验证：同输入下 probe on/off 物理轨迹一致；ledger 中每个子步恰好一组真实 grants；丢弃候选只出现在诊断字段。

- [ ] **□4 通道顺序不重排**：settle→percolate→bottomCreep→relax→lift 复用现有事务顺序。
  - 判定口径：预演与提交复用现行 `WaterSandField` application 事务的单一权威通道顺序（§20.6.2）；`suspensionDiffusion` 若单独表示仍属 collect 阶段既有非 total 通道，不得插入或重排 apply 顺序。实现不得在 planning pass 内另设优先级、按候选角切换顺序或按通道重要性重排。
  - 验证：planning pass 的 plans 遍历顺序与 `applyMotionChannels`（`WaterSandField.ets:1396` 起）现行顺序逐通道一致；合成断言不出现任何重排变体。

- [ ] **□5 优先键字典序**：(channelRank, columnIndex/interfaceIndex) 纯整数+固定枚举，禁浮点排序/按请求量排序。
  - 判定口径：请求分配与 tie-break 只按 `(channelRank, columnIndex)`（边通道 `(channelRank, interfaceIndex)`）字典序遍历（§20.6.2/§20.6.3）；键值完全由整数与固定枚举组成，不比较浮点数值、不按请求量排序、不依赖排序稳定性；比例缩放余量不重新排序、不二次分配，未授予部分记 `clipped/rejected`；`limitingEdge` 并列时取 `interfaceIndex` 最小者。同输入下任意两次运行产生逐项相同的 `plannedGrant/budgetUse/deltaSandTopByColumn`。
  - 验证：构造请求量并列/接近的合成序列，重复运行断言分配逐项确定一致；代码审查无 `sort()`/浮点比较出现在分配路径。

- [ ] **□6 preview/commit 精确相等**：thetaAccepted/plannedGrant/budgetUse/deltaSandTopByColumn 逐项 `===`，`-0` 规范化+NaN 拒绝。
  - 判定口径：preview 与 commit 共享同一份纯规划函数，不复制两套相似逻辑（§20.6.4）；四组量逐项精确相等、不使用容差，比较前 `-0` 规范化为 `0`、显式拒绝 `NaN`。任一不一致即 `PREVIEW_COMMIT_MISMATCH`：冻结本子步、`restoreBackup()`、保留 preview 与 commit 两套诊断账本，不得用后续子步或 correction 修复。
  - 验证：合成构造预演/提交分歧（含 `+0/-0` 与 `NaN` 注入）断言全部触发 `PREVIEW_COMMIT_MISMATCH` 且状态冻结；正常路径断言逐项 `===` 通过。

- [ ] **□7 BLOCKED 语义**：带 reasonCode + run 不得满足零越界验收，不得静默 clamp 继续。
  - 判定口径：BLOCKED 携带 §20.4 全量证据——`reasonCode`（`EMPTY_FEASIBLE_INTERVAL`/`NO_SAFE_OLD_TILT_PLAN`/`PREEXISTING_INFEASIBLE_STATE`/`PREVIEW_COMMIT_MISMATCH` 等）、`runId/substepId`、theta requested/accepted、最紧边、`g/deltaG/r`、通道 grants、预算剩余与拒绝量。发生 BLOCKED 的 run 不得满足「零越界」验收；调用层若选择继续，必须作为同一 run 的明确 invalid continuation 记录，不得抹除阻断事件；不得把角度偷偷 clamp 到任意值后继续，不得把 BLOCKED 降级为可忽略 warning。
  - 验证：合成触发各 reasonCode 场景断言证据字段完整；正式 checker（`p011_slip_formal_check.py` 硬约束口径）对含 BLOCKED 的 run 判 FAIL，不得因后续子步恢复而放行。

#### 20.7.2 Developer 交付要求（通知）

1. 实现授权下达后按 §20.7.1 七项 checklist 交付；七项全部勾选后才可提交限幅实现的合成测试证据，之后才进入 §20.5 的 omega 扫描与真机验收。
2. 实现落点：替换 `P011TiltPlanningPass.ets` 现有骨架（当前 `createPreview()` 返回阻断结果、`commit()` 抛 `P011_PLANNING_PASS_BLOCKED`，`P011TiltPlanningPass.ets:94-107`），接口签名沿用 §20.1/§20.3 契约；接入点为 `WaterSandField` 子步循环倾角更新处（现行 `updateTilt` 调用点 `WaterSandField.ets:796` 一带，最终落点以实现方案为准）。观测①b 遥测（§19.13）继续有效，两任务可并行但不互相修改对方的验收口径。
3. 任何一项不满足即 BLOCKED 返工；不得通过放宽口径、隐藏第三轮搜索、静默 clamp 或修改 checker 判定转绿。
4. 本 checklist 不授权：标定/填入 `maxTiltOmega` 点值、放宽 `tan30°`/预算/几何、跳过真机 P95 验收、修改 §19.13 观测①b 的四项口径。

变更记录：2026-10-19，依据 `algorithm-P011.md` §8.13 增加 v2.14 architect 对接材料。仅补充两轮可行域预演、现有通道/预算接口与 BLOCKED 事务语义；无代码、测试用例或硬门槛变更。状态保持 `BLOCKED / DESIGN_NOT_CALIBRATED`。

变更记录：2026-10-19，依据 manager【architect·⑤】任务新增 §20.7。仅落盘【阶段一·限幅】实现验收 checklist 与 developer 交付指令；不修改代码、既有测试用例或历史结论。§20 顶部 `BLOCKED / DESIGN_NOT_CALIBRATED` 状态不变，待 `maxTiltOmega` 标定流程解除。

## 21. SLIP checker 口径对齐审查：test-cases §11 vs architecture §19.7 对照表（2026-10-19）

**状态：审查完成，修订建议待 designer/tester 认领。** 依据 manager【architect·⑥】任务：SLIP 真机验证前审查两套 checker 体系的口径一致性，避免两套结论打架导致验证返工。

审查对象与定位：**体系 A** = test-cases.md §11 四条 MUST-PASS（正式校验器 `test/p011_slip_formal_check.py`，服务七相 SLIP 真机验证，证据 route `natural-observation-v2.10`）；**体系 B** = architecture §19.7 七项 checker 检查（规划中的 `test/p011_v212_relax_hysteresis_check.py`，服务 v2.12 双角标定校准/holdout run，route `natural-relax-hysteresis-v1`）。两者是**不同 run 类型**的 checker（SLIP 验证 run vs v2.12 标定 run），不是同一 run 的两套判据；对齐审查的目的是保证共享口径（tan30、预算、守恒、账本对齐）在两边语义一致、无矛盾，而不是要求逐项相同。体系 A 的 `p011_slip_formal_check.py` 复用 `p011_v210_natural_observation_check.py` 的 `validate_observation_contract()`（:262），后者对 schema v3 记录执行 B 体系的 runRole/阈值契约校验——两体系在实现层已发生耦合，口径一致性直接影响 SLIP 验证。

### 21.1 口径对照表

分类：**一致**（两边语义相同）/ **不一致**（两边语义矛盾）/ **单边缺失**（只有一边有，另一边按设计不需要或应补齐）。

| # | 口径项 | 体系 A（§11 / slip_formal_check） | 体系 B（§19.7 / v212 checker） | 分类 |
|---|---|---|---|---|
| 1 | tan30 硬约束·边集 | SLIP-01：每提交子步全部 127 边（`len(S)-1`，S 长度由 ledger 决定，实测 128 列） | §19.7.4.6「守恒、stock、capacity 与 tan30° hard gate」；§18.5 同口径表述「完整 sandTop 的 \|ΔS−tan(effectiveTilt)\|<=tan30°」 | **一致** |
| 2 | tan30 硬约束·倾角口径 | SLIP-01：committed tilt 取同子步 `P011_METRIC.effectiveTilt` post-update 快照（`sequence/frameId/substepId` 身份绑定）；缺失 post 快照时该子步全部边计违约（`missingSameSubstepPostTilt`），禁止用 context 旧值替代 | §19.7 未显式写倾角快照口径；§18.5/§19.4.1 要求「`effectiveTilt`、`stateBefore`、grant 与 audit 必须使用同一子步快照」，语义等价但未规定 committed tilt 的 metric 绑定方式 | **基本一致，B 侧表述欠精确**（无实质矛盾；建议 B 侧 checker 落地时直接复用 A 侧 metric 身份绑定实现） |
| 3 | tan30 硬约束·判定用状态 | SLIP-01：`stateAfter.S`（`after["S"][i+1]-after["S"][i]`，提交后状态）；§11.1 文字同时提及 stateBefore.S 与 stateAfter.S 作输入，实现只检 after | §18.5「完整 sandTop 的 \|ΔS−tan(effectiveTilt)\|」未区分 before/after；§19.5 hard guard 是**逐 writer 提交前**检查（admission），A 是**子步提交后**审计（audit）——guard 与 audit 是互补双层不是重复 | **一致（互补关系）**；注意 A 的 §11.1 文字「输入 stateBefore.S 与 stateAfter.S」与实现（只检 after）有文字性出入，建议 tester 澄清文字 |
| 4 | t90 口径 | SLIP-02：target-based，raster 全宽 OLS 线性坡度首次 `>=27°`（30° 目标的 90%）的相对时间，`<=5100ms`；明确禁止 relative-final 口径 | §19.7 无 t90（B 是标定 checker，不判收敛速度）；v2.11 §18.5.4 有另一口径 `t90 P95<=100ms`（heavy interface 响应，量纲与对象不同，不冲突） | **单边缺失（设计内）**——B 不需要 t90；但须注意 §18.5 的 100ms P95 与 SLIP-02 的 5100ms t90 是**两个不同指标**，报告与 checker 不得混用名称 |
| 5 | raster 收敛 | SLIP-02：±45° 相位最后一次 raster 全宽 OLS ∈ 30°±2°，`R²>=0.90`，报告 RMSE | §19.7 无 raster 收敛判定；标定候选筛选按 §8.11 用 raster 30°±2° 联合筛选（algorithm 层） | **单边缺失（设计内）** |
| 6 | raster 表面检测方法 | A 实现 `raster_surface()`：亮度>35 且非蓝色主导（`not(blue>red and blue>green)`）——**检测沙表面任意颜色** | v210 checker `raster()`：以 `#24571F` 为中心欧氏距离<=70 的**绿色带**——检测重沙边界 | **不一致（对象不同，需显式声明）**：A 检测「整体沙面」用于 OLS 坡度，B 检测「重沙界面」用于 A_B 位移；两者服务不同指标，**不是矛盾**，但两套 checker 都叫 "raster"，SLIP 验证与 v2.12 标定若共用采集 manifest，须在报告中显式区分表面/界面两个口径，防止误用对方实现 |
| 7 | 预算对账·三层上限 | SLIP-04：`localUsed<=localLimit`；`bottomUsed<=min(bottomRequested,bottomLimit)`；`totalUsed<=min(totalRequested,totalLimit)` | §19.7.4.5「local/total/bottom 预算不越界」；v210 checker 实现 `bottomBudgetExcess/totalBudgetExcess/maxLocalBudgetExcess<=1e-9`（audit 超额字段口径） | **一致**（A 逐子步对账 + B audit 超额零容差，语义同为不越界；两边 EPS 均 1e-9，v1 证据中 88 个 1e-15 量级浮点尾差两边均放行，无冲突） |
| 8 | 预算对账·clipping 零要求 | SLIP-04：`bottomClipped==0 且 totalClipped==0`（逐子步） | §19.7 无 clipped==0 要求；v210 checker 不检查 clipped；v2.11 §18.6 明确「预算不足→q 裁剪，记录 clipped 量，**不是错误**」 | **不一致（A 侧过严，与物理预期矛盾）**：v1 证据实证——`totalClipped>0` 971 子步、`bottomClipped>0` 742 子步，全部集中在 ±15°/±45° 倾斜相位（baseline 为 0）；总预算 `300*dt` 在多通道竞争下裁剪是**预算机制的设计内行为**（§8.12 归因已确认「total cap 只有 5.0 且用满」是常态），`clipped==0` 意味着要求所有通道请求永不超过预算余量，与共享预算的裁剪语义直接矛盾。**修订建议 A-1（交 tester）**：SLIP-04 的 clipping 零要求改为「clipped 量正确记账」（`bottomUsed<=bottomLimit`、`totalUsed<=totalLimit` 已由上限检查覆盖，clipped 字段只做对账验证：`clipped==requested−used` 在数值精度内成立），删除 `clipped==0` 判定；若 manager 坚持零裁剪，须先证明该要求与共享预算设计不矛盾，否则 SLIP 正式验证结构性无法通过 |
| 9 | 预算对账·relax 零要求 | SLIP-04：`relaxAppliedTotal==0`（audit 字段） | §19.7 无此要求；v210 checker 不检查 relaxAppliedTotal；§17/§18 体系将 relax 视为正常纠偏通道（`relaxRequestedTotal/relaxAppliedTotal` 仅作诊断） | **不一致（A 侧与崩塌物理预期矛盾，重大）**：v1 证据实证——`relaxAppliedTotal>0` 共 1738/2512 子步，其中 ±45° 相位（positiveOvershoot 270/270、negativeOvershoot 208/208）**全部子步** relax>0；§8.12 归因明确 relax 是「把 residual 往零推」的纠偏项，超角倾角下崩塌搬运必然触发 relax 通道。`relaxAppliedTotal==0` 等价于要求超角相位中崩塌通道零工作——与 SLIP-02 要验证的「±45° 崩塌收敛到 30°」目标**自相矛盾**（崩塌不工作就不可能收敛）。RED 基线中 1738 个预算违约子步全部来自该要求（`relaxOk=false`），属预期 RED 的口径部分而非物理缺陷证据。**修订建议 A-2（交 tester，SLIP 验证前必须裁决）**：删除 `relaxAppliedTotal==0` 判定；relax 合法性已由上限检查（total<=300dt）与 tan30 硬约束（SLIP-01）联合保证，无信息损失；若保留该要求，SLIP 正式验证**必然结构性 FAIL**，两套 checker 结论打架的直接来源 |
| 10 | 账本对齐 | SLIP-04：ledger `context.effectiveTilt` 与同子步 `P011_METRIC.effectiveTilt` 身份绑定（`sequence/frameId/substepId`），缺失或错配数=0；`missingPostTiltSubsteps` 同时计入 hard 约束（缺失即全边违约） | §19.7 未显式列出；§17.9.9/P011-TaskC 已修复 context 采样移至 updateTilt 后；v210 checker `validate()` 不做 metric 身份比对（`P011_OBS_METRIC` 心跳只数数量） | **单边缺失（B 侧应补齐）**——B 侧 checker 落地时建议复用 A 的 metric 身份比对；对 v2.12 标定 run，转折事件的 `effectiveTilt` 快照对齐同样关键（bracket 样本有效性依赖） |
| 11 | 守恒 | SLIP-04 未显式判守恒；A 通过 `validate_observation_contract()` 复用 B 体系 v210 checker 的 `validate()`（其中 `audit.conservation is True` 逐子步检查）——**实现层已覆盖**，但 §11 文字未列出 | §19.7.4.6「守恒、stock、capacity」显式列出；v210 checker 实现 `audit.finite/range/capacity/stock/conservation` 五项全 True | **一致（A 文字遗漏，实现已覆盖）**：**修订建议 A-3（交 tester）**：test-cases §11.4 判定标准补写「`audit.finite/range/capacity/stock/conservation` 全 True（经 `validate_observation_contract` 覆盖）」，使文字与实现一致，避免读者误判 SLIP 不查守恒 |
| 12 | 容量/stock | 同上：A 实现层经 v210 `validate()` 覆盖（`capacity/stock`），§11 文字未列 | §19.7.4.6 显式；v210 checker 实现 | **一致（同 #11，文字遗漏）** |
| 13 | 有限值 | 同上：`finite` 经 v210 `validate()` 覆盖；A 自身 hard 约束遍历中 `float()` 遇 NaN 会使 `error>tan30` 成立而计违约（隐式拒绝） | §19.7 经「参数不变量」与 v210 `finite` 覆盖 | **一致** |
| 14 | 相位终点稳定与悬沙 | SLIP-03：每相位最后 1s OLS 坡度变化<=1°；相位终点 `suspH+suspL<=0.05×初始沉积沙`（初始总量取首子步 stateBefore 的 bh+bl） | §19.7 无（标定 run 判定走 §8.11 标定协议的 hard-gate violation 与样本门槛） | **单边缺失（设计内）** |
| 15 | 转折遥测契约 | A 经 `validate_observation_contract()` 执行 B 体系的 runRole/thresholdCalibrationId/thetaStartDeg/thetaStopDeg 校验（schema v3 时） | §19.7.4.1~4.3、4.7（参数不变量、状态转换语义、方向、限流保持、无 ROI、置信区间） | **一致（共享实现）**；注意 §19.13.3 已裁定：SLIP 验证 run `runRole=null`，`validate_observation_contract` 的 v3 分支对 SLIP run 不适用——**实现细节差异**：当前 `validate_observation_contract()` 对所有 schema v3 记录要求 `runRole∈{calibration,holdout}`，SLIP 观测①b run 升级 schema v3 后 `runRole=null` 会被判 `:runRole` 错误。**修订建议 B-1（交 tester，观测①b 交付前）**：`validate_observation_contract()` 增加 runRole 判别——`runRole=null` 时跳过 runRole/thresholdCalibrationId/theta 字段校验（保留 `transitionTelemetryAvailable` 校验），仅 `runRole∈{calibration,holdout}` 时执行全量校验；否则 SLIP 正式 run（schema v3 + runRole=null）在该函数下结构性 FAIL，与 §19.13.3 裁决冲突 |
| 16 | route 隔离 | A 默认 route `natural-observation-v2.10`（取首 record context），`OBSERVATION_ROUTES` 接受 v2.10 与 v2.12 两个 route | B 专属 route `natural-relax-hysteresis-v1` + artifact 前缀 `p011-v212`，校准/holdout 分离 | **一致（隔离设计）**；建议 A 在结果 JSON 中显式断言单 route（当前只透传首 record 值，混 route 不报错）——**修订建议 A-4（交 tester，低优先级）** |
| 17 | 判定聚合 | A：overall = hard ∧ contract ∧ alignment ∧ budget ∧ convergence(±45°) ∧ phaseEndpoint(全部七相)；任何一项 FAIL 即整体 FAIL | B：§19.7.5「任何 hard gate、预算、守恒或 holdout 失败均判 invalid」 | **一致（全 AND 语义）** |

### 21.2 审查结论

1. **结构性矛盾 1 处（必须修订）**：#9 `relaxAppliedTotal==0`——与 SLIP-02 的崩塌收敛目标自相矛盾，±45° 相位 relax 全程工作（v1 实证 1738/2512 子步），保留则 SLIP 正式验证结构性 FAIL（修订建议 A-2）。
2. **口径过严 1 处（应修订）**：#8 `clipped==0`——与共享预算裁剪语义矛盾（v1 实证 971/742 子步裁剪，全部在倾斜相位），改为「clipped 记账正确」（修订建议 A-1）。
3. **实现层冲突 1 处（观测①b 交付前必须修订）**：#15 `validate_observation_contract()` 对 schema v3 一律要求 `runRole∈{calibration,holdout}`，与 §19.13.3 裁决（SLIP 验证 run `runRole=null` 不适用该分支）冲突，SLIP+观测①b 正式 run 会结构性 FAIL（修订建议 B-1）。
4. **文字遗漏 1 处**：#11/#12 守恒/容量/stock 在 A 实现层已覆盖但 §11 文字未列（修订建议 A-3，纯文字澄清）。
5. **表述欠精确 2 处**：#2 B 侧倾角快照口径、#3 A 侧 §11.1 before/after 文字——均无实质矛盾，随对应 checker 落地/文字维护时顺带澄清。
6. **设计内单边缺失（无需修订）**：#4/#5/#14（B 是标定 checker 不判收敛）、#10（B 侧落地时建议补 metric 身份比对）。
7. **同名异义提醒**：#6 两套 checker 的 "raster" 检测对象不同（A=沙表面亮度法，B=重沙界面绿色带），共用 manifest 时必须在报告中显式区分；#4 中 SLIP t90（5100ms 收敛）与 v2.11 P95 t90（100ms 响应）是两个不同指标。

修订建议去向：A-1/A-2/A-3/A-4 交 **tester**（`test/p011_slip_formal_check.py` 与 test-cases §11 维护方）；B-1 交 **tester**（`p011_v210_natural_observation_check.py` 维护方，观测①b 交付前）；涉及 #9 的口径裁决若 tester 认为超出授权（改动 MUST-PASS 判定标准），升级 **manager** 裁决后执行；**designer** 无需变更（#9 的算法侧语义——relax 为正常通道——在 §8.12 归因与 §17/§18 体系中已明确，无算法文档矛盾）。

变更记录：2026-10-19，依据 manager【architect·⑥】任务新增 §21。仅落盘口径对照表与修订建议；不修改代码、checker 实现或既有测试用例。修订建议待 tester（及必要时的 manager）认领后执行。

## 22. 【阶段一·B】验收评审：commit 4b1f635 限幅实现（2026-10-20）

**状态：`CONDITIONAL_PASS`——七项 checklist 全部通过；运行时全通道 pre-commit guard 未在本 commit 交付（对照裁决 B 完成标准的唯一实质缺口，见 R-1）；2 个文档化 legacy FAIL 已升级至 architect 裁决（R-2）。【阶段一·B】标记实现完成的前提见 §22.4。**

评审对象：commit `4b1f6355c730b062cae044ac73cfa1def7694bd1`（10 文件 +1842/-114）。评审方法：逐行 diff 审查（P011TiltPlanningPass.ets 全量、WaterSandField.ets 规划接入全量、测试全量、checker 全量）+ 证据独立重跑（Node 1:1 通道、p011_planning_pass_check.py、python p011 套件 40/40）+ 关键语义代码追踪（preview 纯度恢复链、collect/apply 分支、bottomClipped/totalClipped 账本语义、saveBackup 快照覆盖面）。

### 22.1 §20.7.1 七项 checklist 逐项核对

| 项 | 判定 | 核对证据 |
|---|---|---|
| □1 两轮 pass 上限 | **PASS** | `createPreview` 控制流封闭于 {thetaRequested→theta1→theta0 回退→BLOCKED}，无任何第三轮/第四候选路径；Node 证据 `directedThreeCandidateExhaustion`：status=blocked、passCount=2、**previewCalls=2**（≤3）；ArkTS 侧同名用例 + counting delegate 同步覆盖 |
| □2 B0 独立副本 | **PASS** | `previewChannelPlansAt` 每候选独立 `beginSubstepMotionBudget` 全新 local/bottom/total 副本；apply 后 `beginSubstepMotionBudget` 重置请求账本杜绝预演残留串接；`previewPurity_budgetLedgerSingleCommit` 验证 totalUsed 单次计入（>0 且 ≤limit，无双计数）；12 个 PlanningScalars 逐项 save/restore |
| □3 单次 commit | **PASS** | step 循环中 `planAndCommitTilt` 替换无条件 `updateTilt`，`updateTilt` 无残留调用点（仅保留私有定义）；`effectiveTilt=thetaAccepted` 单次写入；preview 纯度：probe 分离 + `restoreBackup`（覆盖 waterTop/bh/bl/mh/ml/suspH/suspL 全部被污染字段）+ 请求账本重置；probe on/off snapshot 相等用例 PASS；`singleCommitTiltAndRate` 验证 effectiveTiltRate=(thetaAccepted−thetaBefore)/dt |
| □4 通道顺序不重排 | **PASS** | `allocatePreviewPlans` 顺序 settle→percolate→bottomCreep→relax→lift 与 `applyMotionChannels` 逐通道一致（grep 验证）；suspensionDiffusion 留在 collect（S-invariant，deltaS=0）符合 §20.2 注记；bottomCreep 先独立预算后共享 total，镜像 `creepBottomRequests` 的 independentScale 语义 |
| □5 优先键字典序 | **PASS** | 分配遍历用固定列号/槽号升序（整数索引 for 循环），无 sort()、无浮点比较、无按请求量排序；limitingEdge tie-break 用严格比较保留最小 interfaceIndex（升序扫描 + `>`/`<`），`limitingEdgeTieBreakKeepsSmallestIndex` 用例覆盖全平局与部分平局两种构型 |
| □6 preview/commit 精确相等 | **PASS** | `p011PreviewCommitConsistent` 逐项 `===`：thetaAccepted、granted/clipped/rejected、budgetUse 四字段、deltaSandTopByColumn 逐列；`normalizeZero(-0)→0`、`Number.isFinite` 显式拒 NaN；`verifyConsistencyChannel` 用例覆盖正例/grant 分歧/-0 归一/NaN 拒绝四路径；产品侧 `verifyPreviewCommitConsistency` 在权威 collect 后用同一 `allocatePreviewPlans` 纯函数重算并比对，mismatch → `PREVIEW_COMMIT_MISMATCH` 冻结子步（restoreBackup + frozen） |
| □7 BLOCKED 语义 | **PASS** | 四个 reasonCode 常量齐备；BLOCKED 时 `TILT_PLANNING_BLOCKED` → step `ok=false`（run 级 invalid），参考角保持 thetaBefore、`effectiveTiltRate=0`，**不 freeze**（允许调用层显式 invalid continuation，与 §20.4「恢复运行须记为同一 run 的 invalid continuation」一致——注意与 §20.4 表格「不提交倾角和材料状态」语义吻合：不提交≠冻结引擎）；证据经 `getPlanningTelemetry()` 全量暴露（status/blockedReason/passCount/fallbackUsed/thetaRequested/limitingEdge）；Node 证据 `blockedSemantics`：status=blocked、reason=PREEXISTING_INFEASIBLE_STATE、okFalse=true、errorCode=5 |

两处实现口径偏离已核定为合规自适应而非违约：（a）候选角可行区间引入 `P011_PLANNING_PASS_ANGLE_EPS=1e-9` 角度域容差——§20.6.4 的精确相等只约束 plan 比较，区间交须吸收 atan 往返浮点噪声，否则已提交边界角会被伪拒绝（commit 注释已声明该边界，comparator 本身保持零容差）；（b）`blockedResult` 对 NO_SAFE_OLD_TILT_PLAN/PREEXISTING 置 `fallbackUsed=true`——§20.3 该字段语义为「回退路径被评估」，证据可追溯性不受损。

### 22.2 Pre-commit guard 完备性审查（任务第 2 步）

**裁决（R-1，实质缺口）：本 commit 交付的是「倾角可行性闸门」，不是 §19.5/裁决 B 定义的全通道 pre-commit 材料提交 guard。**

1. **已交付**：`planAndCommitTilt` 在子步首对 S0 全边检查 tan30（preexistingViolation + 可行域交集），倾角与材料由可行域联合判定——这覆盖了 §8.12 定性的首因「tilt 参考先越线」。BLOCKED 时不提交倾角与材料，与 §20.4 语义一致。
2. **未交付**：§19.5 的逐 writer 闭式 grant 拦截（relaxAngle 边级联三边预演、creepBottom/liftByShake/settleSandV2 受影响边闭式区间、违规态 non-worsening 分支、guard 截断后四层预算回写）。现状：材料增量在 apply 后才由 `audit(true)`（probe 开启时 `finishNaturalObservation` 的 `range` 字段）审计 tan30——**审计≠拦截**，与 v2.10 行为相同；checker 侧的 `hardGuardAvailable` 字段因此恒 false。这正是 commit message 自述的已知冲突根因：30° 场景中 bottomCreep 单边 deltaG 可达 ~0.33（vBottomCreepMax·2·dt），把贴边残差推过 tan30，旧引擎从不拦截，新倾角闸门如实暴露。
3. **运行时 gate 与补采 gate 豁免确认分离**：checker `SKIP_TAN30_PHASES`（±45° 平台及其 return 子步豁免参考线形式判定）明确引用 priority.md 2026-10-20 裁决，且该裁决明文「豁免仅用于标定 run 资格，不豁免阶段一实际提交的安全 guard」；实现侧 `planAndCommitTilt` 的 tan30 检查**无任何相位豁免**（对 ±45° 目标同样执行可行域检查，不可行即 clamp/BLOCKED）——两套口径未混用，合规。
4. **五 writer 清单对照**：settle（clipLocal 预演 ✓）、percolate（S-invariant 登记 ✓）、bottomCreep（独立预算+total 镜像 ✓）、relax（clipLocalPair ✓）、lift（clipLocal ✓）、suspensionDiffusion（S-invariant ✓）——**预演侧**全 writer 已接入（□含 deltaG 合并完整性）；**提交侧拦截**全 writer 未接入（上述第 2 点）。裁决 B 完成标准「每个提交子步按实际接受参考角检查全边 |ΔS−tan(effectiveTiltAccepted)|≤tan30°，零违约；只允许在完整事务可行时提交」——倾角侧满足，材料提交侧依赖预演≈真实的镜像等价性（commit 声称 preview L1 与 real L1 逐步 bit-identical，本评审通过 `verifyPreviewCommitConsistency` 的每子步强制比对接受该论证），但 §19.5 要求的逐笔 writer 拦截仍未落地。

### 22.3 测试证据复核（任务第 3 步）

1. **验证方法可信度：高**。Node 1:1 通道（`run_p011_planning_units.cjs`）经 TypeScript 编译器加载**真实产品 ArkTS 源文件**（WaterSandField.ets + P011TiltPlanningPass.ets），非复刻逻辑；本评审独立重跑：41 PASS / 2 FAIL 与 commit 证据一致，`blockedSemantics` 字段完整复现；`p011_planning_pass_check.py` 重跑 PASS；python p011 套件 40/40 OK（本评审重跑确认）。
2. **用例覆盖核对**：不可行域 hold（directed exhaustion，g0=2.5>2H 构型逼出三候选穷尽）✓；最紧边 tie-break（全平局+部分平局两构型）✓；-0/NaN 比较器 ✓；preview 纯度（probe on/off snapshot 相等）✓；预算单次计入 ✓；单 commit 倾角/速率语义 ✓；BLOCKED 证据字段（run 级 invalid、不 freeze、参考角保持）✓；30 步多子步 run 守恒+passCount≤2 ✓。§20.7.1 七项全部有对应用例。
3. **2 个 legacy FAIL 的定性（R-2，升级裁决）**：`bottomCreep_tiltMovesLowerLayers` 与 `p011_commandSignFlip...` 在 30° 倾角场景失败——**不是限幅实现的缺陷，而是新闸门如实暴露了旧引擎从未拦截的真实 tan30 违约**（本评审独立验证：e2f02a1 时代 `audit()` 无 tan30 检查，v1 证据 484 违约子步坐实该违约真实存在；checker 已将其列入白名单并按 PIPELINE step-5 升级）。**architect 裁决：该暴露是 §20 闸门的正确行为，不得为恢复测试绿色而放宽闸门。** 处置归 manager/designer：按 commit 列出的候选——(a) 预演已与 apply 等价（已做）；(b) 可行域留 margin（改判据，须 manager 授权）；(c) 重调 30° 测试场景参数（改测试期望，须 tester 授权）。在裁决落位前这两个用例保持文档化 FAIL，不阻塞【阶段一·B】实现完成判定（它们是旧物理行为暴露，非新实现回归）。
4. **测试资产完整性**：checker 对 41/2 的白名单边界正确（仅认 2 个具名 legacy，任何其它 FAIL 即判 FAIL）；`test_p011_transition_telemetry.py` 新增 3 个 runRole 判别用例（§21 B-1 落地）+ observation①b 回归（edgeIdentity hump 调至 0.4 避开闸门，同时保住 tan4 触发语义，合规）。

### 22.4 评审结论与出口

**判定：`CONDITIONAL_PASS`。【阶段一·B】标记实现完成，附两项条件：**

- **C-1（R-1，交 manager 排期）**：全通道 pre-commit 材料提交 guard（§19.5 逐 writer 闭式拦截 + 四层预算回写）未在本 commit 范围内。裁决 B 完成标准中「零违约提交」当前由「倾角可行域闸门 + 预演/apply 镜像等价 + 每子步 verify 比对」间接保证（材料增量若与预演偏离即 PREVIEW_COMMIT_MISMATCH 冻结），工程上成立；但 §19.5 的直接逐笔拦截作为独立防线仍未交付。建议作为【阶段一·B·续】单独任务排期（`P011TiltPlanningPass` 的 `hardGuardAvailable` 证据字段与 checker 白名单逻辑届时同步落地），不与限幅实现混同验收。
- **C-2（R-2，交 manager/designer 裁决）**：2 个 30° 场景 legacy FAIL 的处置（margin vs 场景重调 vs 维持文档化 FAIL）。裁决落位前不得改闸门、不得删用例。

次要发现（不阻塞，随下一提交清理）：`planningPreviewActive` 标志只写不读（:501/:1582/:1603），是死代码——probe 分离 + backup 恢复 + 请求账本重置已完整覆盖预演隔离，建议删除该标志或补上消费点；`allocatePreviewPlans` 头部有重复的文档注释块（两段 shadow-allocation 说明）。

**与 A 线汇合进 developer 收尾的前提**：A 线（双角滞回标定）状态不受本 commit 影响（`natural-relax-hysteresis` route 与标定协议未动）；B 线实现完成即按裁决 B 完成标准进入 tester 验证（真机 run + SLIP checker 修订口径），本评审不改变 v2.14 `BLOCKED / DESIGN_NOT_CALIBRATED` 状态（maxTiltOmega 仍未标定，§20.7 预发布条件继续适用）。

变更记录：2026-10-20，依据 manager【阶段一·B·评审】任务新增 §22。评审 commit 4b1f635：七项 checklist 全 PASS（证据独立重跑确认），guard 完备性认定为倾角闸门已交付/全通道材料拦截未交付（C-1），2 个 legacy FAIL 定性为如实暴露并升级裁决（C-2）。判定 CONDITIONAL_PASS，【阶段一·B】实现完成。

## 23. 【阶段一·B·续】复审 + R2 正式评审 + P-012 架构评审（2026-10-20）

**状态：B·续 `PASS_WITH_DEFECT`（R-3 已定性+已出工程裁决）；R2 `PASS / 可 commit`（白名单三用例裁决=m 维持白名单+单列 R-3 缺陷任务）；P-012 设计稿 `READY_FOR_ARCHITECT→APPROVED_WITH_CONDITIONS`（4 项条件）。**

本节收口三项评审：commit `891370d`（B·续 材料 guard）、R2 整改正式评审（含 F-② 白名单裁决）、P-012 渲染设计稿架构评审。评审方法同 §22：逐行 diff + 证据独立重跑（Node 1:1 通道 45/3 复现、checker PASS、python 40/40）+ 根因数值复现（directionAccuracy 用例 step 27 穿透机制逐边追踪）。

### 23.1 B·续复审：commit 891370d（§19.5 材料 guard）

逐项核对结果：

| 项 | 判定 | 证据 |
|---|---|---|
| 闭式区间求交 | **PASS** | `p011GuardAdmit`：`-L<=d+k*q<=L`（合法边 L=H、违规边 L=\|d\| 即 non-worsening），k 符号交换端点，k=0 不约束（d'=d 平凡 non-worsening），与 [0,q] 相交取上端，无迭代二分——与 §19.5 公式逐项一致；`k=0 且 \|d\|>L` 判不可行整笔拒绝，边界正确 |
| 受影响边集 | **PASS** | `materialGuardAdmitIncrement` 从 deltaColumns 推导：列 c 的移动约束边 c-1 与 c（列型 writer：settle/lift 两邻边）；donor/receiver 双列移动约束中央边+两条外侧邻边（pair 型：relax/bottomCreep）——与 §19.5 writer 表一致 |
| 四层预算回写 | **PASS（含说明）** | `materialGuardRefund`：local 双端点（pair）/单端点（column）回补 remaining 并同步减 used；total 回补；bottom 仅 creep。channelRateCap 当前引擎无此层账本（=0），由 per-writer 遥测覆盖——与 §19.5 四层要求在**现有账本结构**下等价，无缺失层 |
| percolate S-invariant 登记 | **PASS（按实现形态）** | percolate 不设数值 guard（ΔS=0 无需拦截），继续经 `recordPercolate` 全量遥测登记（requested/allowed/q/r）——§19.5 表格「作为一个受事务跟踪的 writer 登记」以遥测登记形式满足；**注意**：S-invariant 的显式登记字段（`SInvariant=true`、逐列 deltaS=0）未新增，属设计稿表述与实现的形态差异，遥测数据可推导等价信息，不构成缺口 |
| 五 writer 拦截覆盖 | **PASS** | settle（:2263 两邻边）/relax（:2398 中央+外侧）/bottomCreep（:2570 同 relax+bottom 回写）/lift（:2716 两邻边）/percolate（豁免+登记）——全部在 clip 之后、材料写入之前调用；`hardGuardAvailable=true` + `hardGuardRejected/hardGuardClipped` 真实值进 ledger（`telemetryFieldsReal` 用例验证非恒 false） |
| 预算对账（A-1 落地） | **PASS** | apply 成功后 `bottomClipped=max(0,R-U)`、`totalClipped=max(0,R-U)` 同源重算（:921-934），覆盖独立 bottom 缩放/库存容量/total 比例三层；独立验证 v1 证据 mismatch 742/486 的根因（旧 clipped 语义不同源）与修复正确性 |
| relaxAppliedTotal 真实值 | **PASS** | `recordRelaxAngle(..., appliedTotal)` 接线真实 apply 量（此前恒 0），probe 侧 :401 落账 |
| guard 截断不改 active 语义 | **PASS** | `activeAfter=requested>EPS`（fix B 契约）在 guard 裁剪后保持——guard 拒绝不误报 stop |

**R-3（新发现缺陷，本评审定位根因并出工程裁决）：预演不建模 guard 裁剪 → 倾角闸门可被穿透一步。**

数值复现（directionAccuracy 场景，8 列 thick、±20° 交替 240 步、totalBudget=1000）：

```text
step 27：tilt -16→-19.93（planStatus=clamped，预演承诺修复 deltaG>0）
  实际 apply：edge6 的 relax grant req=0.0141 → allowed=0（hardGuardRejected=0.0141，guard 拒绝）
  S 不动 → edge6 残差 0.5297→0.6056 越过 H=0.5774（越界纯由 tilt 前进贡献）
step 28：PREEXISTING_INFEASIBLE_STATE → run invalid（fail-safe 生效）
```

机制：planning pass 的预演分配器（`allocatePreviewPlans`）镜像 clip 语义但**不镜像 §19.5 guard**；预演承诺的修复性 deltaG 在实际 apply 中被 guard 裁到 0，`verifyPreviewCommitConsistency` 用同一分配器重算 committed 侧（两边都不知道 guard），比对一致 → `PREVIEW_COMMIT_MISMATCH` 不触发 → 穿透未被捕。**严重度：中**——fail-safe 在下一子步拦截（不产生静默持续越界，零越界验收不放过该 run），但 run 会报废且证据只在 telemetry。

**工程裁决（R-3-a，交 developer，授权范围：预演侧）**：把 guard 闭式准入纳入预演分配器——`allocatePreviewPlans` 对每笔候选 grant 逐边执行与 `materialGuardAdmitIncrement` 相同的闭式区间求交后再计入 plans（预演本来就是纯函数，加入 guard 镜像不改变纯度；这使预演 deltaG 与实际 apply 逐步一致，穿透在源头消除）。**替代方案（R-3-b，若 a 的预演复杂度不可接受）**：commit 后增加「实际 apply 后残差复检」——`applyMotionChannels` 返回后若任一边 \|d\|>H 且该子步 planning 状态非 blocked，则置 `PREVIEW_COMMIT_MISMATCH` 语义的 `GUARD_COMMIT_MISMATCH`（冻结本子步而非等待下一子步 PREEXISTING）。a 优先（预演保真优于事后捕获）；两者都不改物理、不放宽阈值。修复后 `p011_directionAccuracyCovers200SamplesAndSignSwitches` 预期恢复 PASS（其 ±20° 反转序列将不再穿透）；另两个 30° 用例见 §23.2。

**B·续判定：`PASS_WITH_DEFECT`。** §19.5 交付本体（闭式拦截+回写+登记）全部正确且证据可复现；R-3 是 §20 预演与 §19.5 guard 的**集成缝隙**（两个各自正确的机制组合出的窗口），不否定 B·续交付本身，作为独立缺陷任务（含 a/b 方案与验收：穿透场景数值复现转 PASS + 白名单可移除 directionAccuracy）。

### 23.2 R2 正式评审 + F-② 白名单裁决

R2 四项整改复核（对照外部评审初核）：

| 整改项 | 判定 | 证据 |
|---|---|---|
| F-① 白名单根因注释 | **PASS** | `p011_planning_pass_check.py:95-127` 注释完整（根因/预演等价声明/升级路径），891370d 追加 guard 语义演化说明 |
| F-② 口径冲突上报 | **PASS（本节裁决）** | 见下 |
| F-③ edgeIdentity 恢复 | **PASS** | hump=0.4 构型下 `p011_transitionTelemetry_edgeIdentityIsNotSlotIndex` PASS（Node 45/3 内） |
| G CountingPreviewDelegate 类化 | **PASS** | ArkTS 禁 object literal 带函数属性，类化实现与产品 delegate 同构 |

**F-② 裁决：三用例全部维持白名单（方案 m），不选 b（margin）/c（场景重调），方向上以 R-3-a 修复替代：**

1. `bottomCreep_tiltMovesLowerLayers` / `p011_commandSignFlip...`：§22 R-2 已定性为旧引擎真实越界的如实暴露；**margin（b）被否决**——在可行域或 guard 留 margin 等于放宽 tan30 判据（用 H'=H-ε 判定、H 提交），违反「不放宽硬门槛」红线；**场景重调（c）被否决**——把 30° 测试参数调到不越界等于删除「30° 倾角下 guard 行为」的测试覆盖，掩耳盗铃。维持白名单+文档化，其语义冲突的**根治**由阶段二机制能力审计（priority.md 裁决 5）处理：现有机制在 30° 参考下能否合法工作是阶段二问题，不是本阶段 checker 问题。
2. `p011_directionAccuracyCovers200SamplesAndSignSwitches`（891370d 新增白名单）：**根因已升级为 R-3 缺陷**（§23.1 数值复现：不是 30° 场景，是 ±20° 反转下预演-实际分歧穿透）——它留在白名单的条件是 R-3 未修复；**R-3-a 落地后必须移出白名单并恢复 PASS**（这是 R-3 修复任务的验收项，防止白名单永久化）。checker 注释已正确标注该语义演化，合规。
3. 白名单纪律：三个用例均为「旧物理行为暴露」非新实现回归；checker 只认具名白名单，任何其它 FAIL 判 FAIL——边界正确。

**R2 判定：`PASS / 可 commit`。** 891370d 已入库（835e663+891370d 两个 commit），被测 hash 以 git 记录为准，后续观测 run 绑定该 hash。commit message 自述证据与本评审独立重跑全部一致（45/3、checker PASS、40/40）。

### 23.3 P-012 渲染设计稿架构评审（docs/algorithm-P012-render-v1.md）

设计稿质量高：五维度方案/数据来源/预算/验收锚点齐备，落地顺序 E→A→B→C→D 与 effector 优先级一致，两条纪律红线（只读物理场、外观/物理验收隔离）贯穿。本地事实核对：`renderWaterSand` 入口（SandCanvas.ets:1517）、`WaterSandSnapshot` 字段清单（无速度场——设计稿如实声明并给出 §5.2 库存重分布代理）、SV-05/06/07 定义（TEST_PLAN_T1_T4.md:263-265）、Canvas2D 接口声明均属实。

**`APPROVED_WITH_CONDITIONS`，4 项条件（工程化时必须解决）：**

- **P1（集成点）**：设计稿 §11 已列 architect 待办——只读 adapter 与渲染入口细化（`renderWaterSand` 分支替换 vs 独立 renderer 类）、TypedArray/bitmap 生命周期归属、D 方向 atlas 实现档位。落 engineering 方案时按 architecture 文档体系新增 `architecture-P012.md`，不混入 P-011 文档。
- **P2（数据面确认）**：§5.2 相总量守恒校验需要前后快照的 `M_H/M_L` 总和比对——`getTotals()` 是 field 方法，renderer 只拿 snapshot 时须在 adapter 内对数组求和实现（O(128) 可接受）；设计稿「前后两份只读相库存快照」的持有与生命周期（§9 已列）在工程方案中细化到字段级。
- **P3（性能预算口径）**：CPU ≤1.50ms/帧 + ≤160 绘制 + ≤4MiB 是设计预算；工程方案须定义配对测量的执行主体（真机 FPS/帧时 P50/P95 采集脚本）与 `PERF_BLOCKED` 上报路径。当前 FPS 52-60 的基线采集须在外观 OFF 档完成，作为后续每档对照。
- **P4（验收资产冻结）**：§10.2 承认 effector 测量脚本/ROI 未附——工程化前必须冻结测量附表（Y/HF_h/maxGrad/row-mean 口径 + ROI 坐标），对参考帧与 OFF 档同口径重算基线，否则 E/A/B 各档验收数字不可比。此项可由 effector 出附表、tester 复核。

其余核对无保留：hybrid 接入边界（独立 adapter+证据标签+缺失字段 N/A）、生命周期分支（reset/切换/反转/倒立/DPI）、防作弊条款（不得以噪声截图认定运动、不得换指标）完备。E 的「去 127 上限+T(x)=x/(1+x) 肩部」、A 的 smoothstep 宽度换算（0.6084×wFull）、B 的 16 组裁片近似平流均为设计稿内已论证的合理工程取舍。

### 23.4 结论与路由

1. **B·续**：`PASS_WITH_DEFECT`——R-3 缺陷任务派 developer（方案 a 优先，验收=directionAccuracy 移出白名单恢复 PASS + 穿透数值复现转绿），修复 commit 后本节补记；tester 补直接拦截用例（manager 任务已列）。
2. **R2**：`PASS`，commit 已入库（HEAD=891370d），后续观测绑定此 hash。
3. **P-012**：`APPROVED_WITH_CONDITIONS`——P1-P4 随 `architecture-P012.md` 工程化解决；第一梯队（E+A+B）先行，C/D 按设计稿顺序跟进；SV-05/06/07 与量化锚点验收口径以设计稿 §10 为准。
4. 阶段一·B 的完成判定链：限幅实现（§22 CONDITIONAL_PASS）+ 材料 guard（本节 PASS_WITH_DEFECT）+ R-3 修复（待 developer）+ tester 真机验证（待执行）——R-3 修复完成前，B 线「零违约」声明以 fail-safe 口径成立（越界 run 必 invalid），完整口径待修复后闭合。

变更记录：2026-10-20，依据 manager 三任务合并评审指令新增 §23。B·续 PASS_WITH_DEFECT（R-3 预演-guard 集成缝隙已定性并出 a/b 方案）；R2 PASS 可 commit（白名单三用例裁决=维持+directionAccuracy 绑定 R-3 修复）；P-012 APPROVED_WITH_CONDITIONS（P1-P4）。三处收口：本节（②）+ 滴答任务 ①③ 结论。

## 24. 滑移收敛修正 relax-correction-weighted-v1：算法稿评审 + 工程落地方案（2026-10-20）

**状态：算法稿 `APPROVED`（1 项语义澄清转 manager）+ 工程方案 `READY_FOR_DEVELOPER`（排期在 R-3 修复 commit 之后）。**

评审对象：`docs/algorithm-P011-relax-correction-v1.md`（designer）。评审方法：数学复核（下降证明 Lipschitz 论证、均匀场 D=0 边界、加权分配算例）+ 代码对接面核对（guard API 拼写/通道顺序/账本结构/预留落点）。

### 24.1 算法稿评审

| 评审项 | 判定 | 说明 |
|---|---|---|
| 预算语义裁决（§1） | **APPROVED，转 manager 确认** | 「独立建账+优先预留+全额计 total」与 300/s 硬上限相容（ordinaryUsed+correctionUsed≤300dt 不变）；「额外额度 300+R」被正确拒绝。**转 manager**：任务原文「独立于 total 300/s」若指后者，须先裁决总预算定义——本工程方案按前者（预留）落地，若 manager 改判后者则本方案作废重设计 |
| 单笔下降证明（§3） | **APPROVED** | 数学复核：E=0.5Σmax(0,\|r\|−h)² 沿单笔线性路径的方向导数 = Σe·sign(r)·k，D=−dE/dq 定义正确；e·sign(r)·k 的 Lipschitz ≤1 论证成立 → E(q)≤E(0)−Dq+Kq²/2，q≤D/K 给出 E 降 ≥Dq/2；K=6/dx²（内部边）核算正确；截至 qDescent 后任何缩减保留下降性，正确 |
| 均匀场失效边界（§3 末） | **APPROVED** | 内部三边同号等 e 时 Σk=0 → D=0，权重 e²·γ 不解决通量抵消——设计稿如实声明不承诺、报 CORRECTION_STALLED，诚实且必要（§8.12 的空间抵消问题本方案只部分处理） |
| 量化预期（§8） | **APPROVED** | 1.28 倍上界推导保守合理（只取消竞争，未含新截断）；明确「不是收敛证明」；加权算例标注「只演示分配器」——无过度承诺 |
| guard API 对接（§5） | **APPROVED** | `p011GuardAdmit(requested, constraints, slopeHardMax)` 拼写与实际导出一致（P011MaterialGuard.ets:51）；`limitingEdge` 返回数组位置需 affectedEdgeIds 映射的提醒正确且必要 |
| 四层回写表（§6） | **APPROVED** | correction 不动 bottom、local 双端、total 一次；channelRateCap=0 现状被正确识别为需新增真实账本 |
| 与 R-3 的关系（§7） | **APPROVED** | 明确要求预演/apply 共享同一预留+排序+guard+退款算法——与 §23.1 R-3-a 的 shadow 镜像路线一致 |

### 24.2 工程落地方案

**接入点与调用链**（applyMotionChannels 内 relax 槽位替换）：

```text
step loop
  planAndCommitTilt（不变，但其 previewAt 委托的 shadow 镜像需同步扩展，见协同项）
  beginNaturalObservation + substep(collect)（不变）
  verifyPreviewCommitConsistency（不变）
  restoreMotionSnapshot
  computeCorrectionReservation(substepDt)     // 新增：候选角下计算 u/R_limit/p/P_i
  scale = min(1, (B−R_limit)/max(ordinaryRequested, EPS))  // 普通通道可用 total 缩减
  beginMotionApplication(substepDt, scale)
  applyMotionChannels:
    settle / percolate / bottomCreep   // 不变（local 可用量 L_i−P_i 经预留表传入）
    relaxAngle → applyCorrectionSlot   // 替换：冻结 (−w, edgeId) 序逐笔执行
    lift                                // 不变
  ledgerReconciliation（A-1，扩展 correction 分列）
```

**接口契约**（新增，均放 P011MaterialGuard.ets 或新文件 P011RelaxCorrection.ets——建议后者，职责独立）：

```ts
// P011RelaxCorrection.ets（新文件，纯函数+账本，不 import ArkUI）
export interface P011CorrectionCandidate {
  edge: number;              // 物理边号
  donor: number; receiver: number;
  r: number; e: number;      // 残差/超角量（当前 shadow S 下）
  d: number; k: number;      // 净贡献导数/二阶系数
  qNatural: number; qDescent: number; guardAdmissible: number;
  u: number;                 // min(全部 cap, L/degree)
  w: number;                 // e²·γ
  p: number;                 // 封顶加权分配结果
}
export function p011CorrectionReserve(
  sandTop: Float64Array,        // 当前（shadow 或真实）S
  effectiveTiltDeg: number,     // accepted tilt
  params: P011CorrectionParams, // h=tan(angleDeg)（现行）, maxAvalancheFlux, vSandTopMax, dt 由调用方传入
  budgets: { localRemaining: Float64Array; totalRemaining: number },
  stockReader: (col: number) => { donorStock: number; receiverCapacity: number }
): P011CorrectionReservation;   // { candidates, R_limit, P_i: Float64Array, order: number[] }
export function p011CorrectionAllocate(candidates, rLimit): void; // 封顶加权：u/w 升序扫描，无二分
// apply 侧逐笔：复用 p011GuardConstraintsForDelta + p011GuardAdmit（与 apply guard 同一推导）
```

**correction 账本与四层回写集成**：

1. 新增 `correctionBudgetUsed/Remaining/Requested/Clipped` 四标量 + `correctionReservation` 结构（含 P_i 数组），生命周期对齐 beginSubstepMotionBudget/reset。
2. `P011ExistingChannelId` 增加 `'correction'`（rank=6，suspensionDiffusion=5 之后）——影响面：`P011ExistingChannelRateCapRemaining` 加 correction 字段、`p011ChannelRank` 加分支、allocatePreviewPlans 加 correction 槽位镜像。
3. guard 拒绝回写：现有 `materialGuardRefund(rejected, [donor, receiver], false)` 已覆盖 local+total；correction 账本回写新增一行 `correctionRemaining += rejected; correctionUsed −= rejected`——不重复退 local/total（设计稿 §6 表格的「另列」要求）。
4. 遥测：`P011NaturalObservationAudit` 增加 `correctionRequested/Granted/Rejected/StalledCount` 字段；ledger schema 不升版本（字段向后兼容追加），checker 对旧 run 的缺失字段按 0 容忍。

**与 R-3-a 预演镜像的协同实现顺序（关键）**：

1. **先 R-3 后 correction**（排期硬约束）：R-3-a 的 shadow 镜像（6 数组+split 公式+逐笔重算 S）是 correction 预演的地基——correction 的预留计算（u/R/p）与逐笔执行都必须在 shadow 状态上复现，若 R-3 未 commit 先做 correction，两套改动在同一函数内冲突，review 成本翻倍。
2. correction 落地时 `allocatePreviewPlans` 的扩展点：在 relax 槽位镜像处先调 `p011CorrectionReserve`（读 shadow 数组），普通通道槽位的 totalScale 用 `1−R/B` 折算（与真实侧 `(B−R)/requested` 等价），correction 槽位逐笔 guard 镜像与 apply 共用同一 `p011CorrectionApplyOnce` 纯函数（含逐笔重算 D/K 与库存重验）——满足设计稿 §7「共享同一算法」与 §20.6.4 单一推导要求。
3. `verifyPreviewCommitConsistency` 的 committed 侧重算自动包含 correction（同一分配器函数），无需改比对逻辑；plans 数组多一个 correction 条目，`p011PreviewCommitConsistent` 按 channelId 逐项比较已覆盖。

**路线身份与观测**：新 route `relax-correction-weighted-v1`（观测 record 的 observationRouteId 显式传入，不复用 natural-observation-v2.10）；A/B/C/D 对照矩阵（设计稿 §9 表）由 tester 按 RED 先行落用例；route 隔离规则照旧——旧 run 不追认、checker 不共享绿色阈值。

### 24.3 实现排期建议（交 manager 排期）

| 顺序 | 任务 | 依赖 | 验收 |
|---|---|---|---|
| 1 | R-3-a 修复 commit（已在工作区，56/0 全绿待提交） | 无 | §23.1 R-3 验收三件套 |
| 2 | correction 工程实现（P011RelaxCorrection.ets + WaterSandField 接入 + 预演扩展 + 遥测字段） | ① | Node 通道含 correction 用例全绿；A/B/C/D 对照矩阵 RED 转绿；verify 每 substep PASS |
| 3 | tester 对照矩阵正式 run + 效果报告（E/maxResidual/ΔE_relax/挤占代价） | ② | 设计稿 §9 验证表全项；CORRECTION_STALLED 如实报告 |
| 4 | manager 裁决是否转正路线 | ③ | 效果证据充分后裁决；不达标则回退原 relax 路线（route 开关） |

### 24.4 次要评审意见（交 designer 修订，不阻塞工程）

1. §4.1 的 `maxAvalancheFlux` 「场景历史 80/s、引擎默认 24/s」——建议在设计稿标注当前 run 配置实际值来源（冻结配置字段名），避免实现时读错。
2. §3 的 γ 定义 `γ[c]=max(0,D[c])/(2e[c]/dx)` 在 e[c]→0⁺ 时发散（D>0 但 e→0 时 γ→∞）——w=e²γ=e·D·dx/2 则收敛，故 w 良定义；建议设计稿注明 w 用后一等价式计算，避免实现者直接算 γ 再乘 e² 引入数值问题。
3. §8 算例的「未饱和解 0.1230769」建议附一步推导（λ=5/(40×16+10×1)=5/650），方便 tester 复核。

变更记录：2026-10-20，依据 manager【architect·工程设计】任务新增 §24。算法稿 APPROVED（预算语义一项转 manager 确认）；工程方案 READY_FOR_DEVELOPER（排期= R-3 commit 之后）；接口契约/账本集成/预演协同/排期表落盘。

## 25. 观测③ v3 执行身份整改：工程核对结论（5 项清单，2026-10-20）

**状态：核对完成——1 项 PASS、4 项 GAP（含 2 项结构性缺口）。整改链对齐 v3 协议（`observation-protocol-P011-v3-preregistration-v1.md`，METHOD_FROZEN）尚有实质距离；GAP-1/GAP-2 为采集前必须完成的阻断项。**

核对对象：commit `d78335b`（R-3-a）+ `5e60f2d`（采集侧修复：九相映射+schema v3 planner 字段）当前 HEAD。核对方法：逐项代码追踪（relaxAngle 判据/active 判定/字段接线/schedule 映射）+ Node 通道重跑（59/0）+ python 套件重跑（40/40）+ v3 协议逐条对照。

### 25.1 五项清单逐项结论

**□1 真正双角状态机 —— GAP（结构性缺口，未开始）**

- 现状：`relaxAngle()` 判据仍是 `tan(params.angleDeg)`=tan(4°) 单阈值（:2517 一带）；active 判定仍是 `activeAfter = requested > EPS` 代理（:2653）；`naturalObservationSliding[]` 是观测影子数组（只被 recordSlip 读写），不是驱动物理的状态机；`setNaturalObservationRunMetadata` 接收 thetaStartDeg/thetaStopDeg 但只写入遥测 context，不进任何判据。
- 即 **v2 的 EXECUTION_THRESHOLD_IDENTITY_MISMATCH 根因未修**——候选阈值仍未进入状态机。§19（v2.12 工程方案）的 `P011RelaxHysteresis.ets`（`p011RelaxHysteresisNextActive` 三态状态机）已设计未实现；当前构建的 start/stop 转折记录仍将是旧请求态诊断，不满足 v3 协议 §2.1「状态翻转必须来自运行状态机」。
- 整改项（阻断采集）：实现 §19.3 三态状态机（sliding[] 驱动 relax 请求资格，start/stop 独立阈值，active 保持），接入 relaxAngle 的候选筛选；`executedThresholds` 由状态机实际读取值产生（协议 §1 表）。

**□2 decision 时点 —— GAP（字段层未实现）**

- 现状：ledger 无 `decisionG/decisionR/decisionAlpha/decisionOrdinal/thresholdUsed/transitionReason` 任何字段（grep 全空）；现有 slip 记录的 `d/r` 在 `completeEdgeGrant` 里由 **stateBefore** 计算（P011NaturalObservation.ets:417 一带 residualForEdge(stateBefore,...)），apply 后由 refreshEdgeGrantsAfterApply 更新 dAfter——两者都不是「状态机读残差选分支之前」的取值点。
- v3 协议 §2.1 要求：decision 取值点=状态机实际读残差、选择分支之前；若前面已有其它 writer 写入须反映当时真实材料状态；decisionOrdinal 标同一步内顺序；同边多次判定全部登记。
- 整改项：依赖 □1 状态机落地后在每次判定处记录（decision 时点由状态机调用点自然定义）；字段进 ledger schema（向后兼容追加）。

**□3 planner 事务语义 —— GAP（部分实现）**

- 已有：planner 状态机（P011TiltPlanningPass）具备 proposal(thetaRequested)/accepted/blocked/clamped/held 语义、commit 单次、BLOCKED 不冻结可继续（invalid continuation）；`getPlanningTelemetry()` 暴露 status/reason/passCount/fallback/limitingEdge。
- 缺口：①**无 attemptId/commitStatus/rollbackReason 任何 attempt 层字段**（grep 全空）——v3 协议 §2.1 attempt 表要求逐 attempt 可审计，BLOCKED 无子步也须有独立 attempt 结果；②`thetaRequested`（真实本步 proposal）不在 ledger context（只有 thetaAccepted）；③被回滚预演的状态隔离已由 R-3-a shadow 管线保证（预演在 shadow 数组上，不写真实 sliding/材料）✓——此子项 PASS；④受限边 limitingEdge 已有但按 constraints 数组位（§24 已记 affectedEdgeIds 映射要求）。
- 整改项：attempt 记录层（attemptId 生成 + BLOCKED/clamped attempt 也落账）+ thetaRequested 进 context + limitingEdge 物理边映射。

**□4 schema v3 字段与协议一致性 —— PASS（已核对部分）+ 1 项语义缺口**

- PASS 部分：`thetaAccepted/requestRejected/plannerHold` 三字段接线正确（:1192-1199）——null=lastPlanningStatus 空 ✓；rejected=blocked|clamped|held（accepted 不含）✓；hold=held ✓ 与协议 §2.1 及任务书语义完全一致；`thetaAccepted==effectiveTilt` 有专测（p011_ledgerSchemaV3_plannerFieldsPresent）。
- 缺口：**configured/executed 分列**——协议要求 manifest 层 `configuredThresholds`（candidate manifest 输入）与 `executedThresholds`（状态机实际读取+stateMachineVersion/hash）分列；现状两者混用 context 的 thetaStartDeg/thetaStopDeg（即 configured 的复制），无 executed 独立身份。依赖 □1 落地后才能产生真实 executed 值。
- 另注（tester 侧，developer 已在 commit message 自述）：checker 的 PHASES/OVERSHOOT_PHASES 仍是七相名，需 tester 在下个 pilot 前加九相名。

**□5 R-3 预演-guard 集成保真 —— PASS**

- d78335b 完整交付 R-3-a：共享 `p011GuardConstraintsForDelta` 单一推导；shadow 管线（6 数组+split 公式镜像+relax early-return gate 镜像+逐笔 recomputeShadowS）；白名单三用例移出后全绿。
- 本核对独立重跑：Node 通道 59/0（含 5e60f2d 新增 3 用例）、python 40/40、checker PASS。commit 自述的 240 步穿透场景 240/240 + preview==apply bit-identical（0 mismatch）在 §23.1 评审时已独立复现。**在当前 HEAD 下 shadow 位精确维持。**

### 25.2 结构性发现：5e60f2d 修复的是 v2 schedule，非 v3 schedule

**GAP-A（重要澄清，非缺陷）**：5e60f2d 实现的 `nine-phase-30-v2` schedule 是 **v2 预注册协议**的九相序列 `0/+15/0/+30/0/−15/0/−30/0`（每相一条命令、commandSequence 1-9）；**v3 协议**的平台序列是 `0→+15→+A→+15→0→−15→−A→−15→0`（A 参数化 15.1~29.9 网格、八条 ramp leg、每 leg 100ms 步进数十条命令）——**序列结构、A 参数化、leg 粒度三者均不同**。当前实现的 scheduleId 选择器只覆盖 v1/v2 两个固定 schedule，对 v3 的 A_selected（影子审计产出）无映射能力。
- 定性：这不是 5e60f2d 的实现错误——它修复的是 v2 pilot 的采集缺陷（任务书「九相映射参数化」表述与实现范围一致：参数化指 scheduleId 选择，非 A 参数化）；但**v3 正式 run 需要新的 schedule 通道**（A 值命令序列 + leg/segment 语义 + 平台 hold 5100ms + 与协议 §3.2 的半开区间归属）。
- 整改项（阻断 v3 采集）：新增 v3 schedule 通道——A 作为 schedule 参数传入，phase 映射按平台序（H0..H8），legId/segmentId 进 ledger（协议 §2.1 attempt 表），命令生成按 §3.2（100ms 步进、n=|T1−T0|/(0.1v) 整数校验）。此项与 □1 状态机同为采集前阻断项。

### 25.3 遗留风险清单与整改路由

| # | 项 | 性质 | 阻断级别 | 路由 |
|---|---|---|---|---|
| GAP-1 | 双角状态机未实现（EXECUTION_THRESHOLD_IDENTITY_MISMATCH 根因未修） | 结构性缺口 | **阻断采集** | developer（§19.3 设计已冻结，直接可实现） |
| GAP-2 | v3 schedule 通道缺失（A 参数化+八 leg+5100ms hold） | 结构性缺口 | **阻断 v3 采集**（不阻断 v2 复采） | developer（新增，需 architect 出接口契约——建议随 GAP-1 一并设计） |
| GAP-3 | decision 字段层（decisionG/R/Alpha/Ordinal/thresholdUsed/transitionReason） | 字段缺口 | 阻断采集（依赖 GAP-1） | developer（随 GAP-1 落地） |
| GAP-4 | attempt 审计层（attemptId/commitStatus/rollbackReason/thetaRequested/limitingEdge 物理边映射） | 字段缺口 | 阻断「BLOCKED 无子步也须有独立 attempt 结果」核验 | developer（planner 侧小改） |
| GAP-5 | configured/executed 分列 | 语义缺口 | 依赖 GAP-1 | developer（manifest+context 字段） |
| GAP-6 | checker 九相 phase 名（PHASES/OVERSHOOT_PHASES） | tester 侧 | 阻断下个 pilot 判定 | tester（developer 已自述移交） |

**结论**：□5（R-3 保真）与 □4 已实现部分确认 PASS；□1/□2/□3 存在实质缺口，其中 GAP-1（状态机）与 GAP-2（v3 schedule）是采集前的两个结构性阻断项。观测③ v3 的执行绑定（协议 §9「实现」行：真双角状态机、逐 decision 读取、planner+guard 事务一致）**尚未达成**——当前代码具备 planner 事务骨架与 guard/预演保真，但身份整改的核心（状态机驱动）未动工。建议 manager 将 GAP-1+GAP-2+GAP-3+GAP-4 打包为「v3 执行身份实现」单一 developer 任务（§19.3 已有设计，工作量可控），完成后 architect 复核+tester 核验再进 pilot。

变更记录：2026-10-20，依据 manager【观测③·v3身份】任务新增 §25。五项清单：□5 PASS、□4 PASS（1 语义缺口）、□1/□2/□3 GAP；结构性发现 GAP-A（5e60f2d=v2 schedule 修复，v3 schedule 通道缺失）；整改路由表落盘。

## 26. 观测③ v3 身份实现复核：§25 四项 GAP 转 PASS 判定（2026-10-21）

**状态：复核完成——GAP-1/2/3/4/5 中 4 项转 PASS（GAP-5 部分），1 项重大偏差（D-1：LEGACY 位精确不成立 + §24 预留未授权提前激活）。转 PASS 附偏差记录，D-1 交 manager 裁决。**

复核对象：commit `bb3063a`（实现）+ `0b7cfd2`（测试）+ `58bb01e`（文档）。复核方法：逐行代码审查（状态机/decision 采样/attempt 层/schedule 通道）+ 独立重跑（Node 74/0、python 40/40）+ **跨版本行为对比**（新 HEAD vs 5e60f2d worktree，同场景 240 步定点锚点序列）。

### 26.1 五项清单复核结论

**□1 执行身份 —— PASS**

- `P011RelaxHysteresis.ets` 按冻结设计实现：tan 空间阈值推导（`p011RelaxHysteresisThresholds`，不变量 `0≤stop<start≤hardMax≤30` 违规抛错）；三态转换 `p011RelaxHysteresisNextActive`（start `r>=slopeStart`、stop `r<=slopeStop`、hold）；请求公式 `max(0,r−slopeStop)/2` 截断 maxFlux/stock/capacity——与 §19.3 接口契约逐项一致。
- **状态机实际驱动**：`relaxAngle` collect 分支中 `relaxSliding[index]` 驱动资格（:3290-3315），候选阈值经 `applyRelaxHysteresisConfiguration` 进入 `relaxHysteresisThresholds`，状态机读取的就是该对象——**v2 的 EXECUTION_THRESHOLD_IDENTITY_MISMATCH 根因已修**。
- `executedThresholdsForLedger()` 读 `this.relaxHysteresisThresholds`（状态机同一对象，:1424-1425）+ stateMachineVersion/sourceCommit/sourceFileSha256/buildSha256（`setNaturalObservationExecutionIdentity`）；LEGACY（无候选）返回 null ✓；候选 run 若 executed 为 null 即 MISMATCH，checker 可判。

**□2 决策时点 —— PASS**

- `recordRelaxDecision` 在 `relaxAngle` collect 扫描内、`p011RelaxHysteresisNextActive` 判定处调用（:3296-3300）——取值点=读残差选分支之前，且反映同 collect pass 内 settle/percolate/creep 已写入后的当前材料状态（`sandTopScratch`），非 stateBefore 非 apply 后 ✓。
- 字段齐备：decisionG（signed slope）/decisionR（signed residual）/decisionAlpha=atan(|R|)·180/π/decisionOrdinal（per-edge per-substep 递增，`relaxDecisionOrdinals` 在 beginSubstepMotionBudget 清零）/thresholdUsed{branch,angleDeg,slopeLimit,comparator}/transitionReason（四值冻结词汇表）——协议 §2.1 每边 decision 表全字段落地。

**□3 attempt 审计 —— PASS**

- `p011AttemptCounter` 单调递增，attemptId=`{runId}-a{n}`；commitStatus 三态 committed/blocked/rolled_back；**BLOCKED 子步也落账**（step 循环 :1140-1144：planningError 非空时仍调 beginNaturalObservation，ledger 带 attemptId/commitStatus='blocked'/rollbackReason）✓ 协议「BLOCKED 无子步也须有独立 attempt 结果」满足。
- materialGuard 物理边映射：`p011GuardAffectedEdgesForDelta`（新导出）+ guard 证据带 affectedEdgeIds 同序 ✓。
- 预演回滚状态隔离：`planningSlidingBackup` 快照在 previewChannelPlansAt 的 finally 恢复（:2237-2267）——**manager 要求的独立确认1**：该模式与 §20.7-□2 的预演纯度要求一致（probe 分离+backup 恢复+请求账本重置三件套齐备，状态机数组新增第四件）✓。

**□4 schedule 通道 —— PASS**

- `P011ScheduleV3.ets`：九平台 `0→+15→+A→+15→0→−15→−A→−15→0`（H0..H8/L1..L8）、A 为参数（0<A<30 构造校验，无 30/29.9 默认）、冻结速率 0.5/1.0、leg 长度整除校验、5100ms hold、半开区间归属（leg 起点 marker 命令在 T0、第 n 步终点命令开平台）、CSV 契约（§9 列序/无 BOM/LF/末行 LF/禁止科学计数法）——协议 §3 全项落地。
- 与 5e60f2d 的 v1/v2 选择器并存（`P011_SCHEDULE_NINE_PHASE_30_V3` 新常量，v1/v2 映射不动）；segment 绑定经 `setP011CurrentCommandSegment` 进每条 ledger context ✓。
- `p011ScheduleV3ToCsv`/`p011ScheduleV3BindCommand` 纯函数，可独立测试（0b7cfd2 有 CSV 契约用例）。

**□5 无越权改动 —— PASS（planning pass）/ 偏差（LEGACY 行为，见 D-1）**

- planning pass 模块 diff=33 行，全部为 §24 预授权的类型扩展（`'correction'` 通道枚举+rank=6+RateCapRemaining 字段），无控制流改动 ✓。
- 物理常量零改动：angleDeg/totalMotionBudget=300/vSandTopMax=28/maxAvalancheFlux/vBottomCreepMax=10/MATERIAL_GUARD_HARD_MAX 全部原值 ✓；epsilon 无放宽 ✓。
- Node 74/0（13 新用例全绿）、python 40/40 独立重跑确认。

### 26.2 偏差记录（D-1，重大，交 manager 裁决）

**LEGACY 位精确不成立：§24 correction 分配器在 LEGACY run 也无条件激活，改变了旧 relax apply 行为。**

数值证据（跨版本对比，5e60f2d worktree vs 新 HEAD，同场景 8 列 thick、±20° 交替、totalBudget=1000）：

```text
5e60f2d 基线：240/240 步全 ok
新 HEAD LEGACY：238/240，step214 BLOCKED（NO_SAFE_OLD_TILT_PLAN）
定点锚点序列对比：首个分歧 step25——
  新代码 relaxRequestedTotal=0.040196 但 relaxAppliedTotal=0.000000，slip allowed 全 0
```

机制：`applyMotionChannels` 中 relax 槽位被 `applyCorrectionSlot` **无条件替换**（:1911-1914 注释「the ordinary relaxAngle path remains for the collect phase」）；`computeCorrectionReservation` 在 LEGACY（`slidingEligible=null`）时仍以 `|r|>tan(angleDeg)` 入池全部超差边（P011RelaxCorrection.ets:158-164「legacy runs keep the pure |r|>h trigger」），预留+加权分配器替换了旧的比例缩放 apply——**旧 relax apply 分支成为死代码**（:3343 注释自认「legacy only...Kept bit-for-bit」但实际不可达）。

偏差定性：
1. **与任务书「LEGACY 位保持真位精确」直接矛盾**——manager 要求的独立确认3 不通过。developer 的 LEGACY 测试（`p011_hysteresis_legacyRunKeepsPreV3Behavior`）只验短场景单步的遥测身份字段（executedThresholds=null、无 decision 字段），**未做行为轨迹对比**，所以测试绿但行为已变。
2. **与 §24.3 排期矛盾**：§24 明确「correction 工程实现排期在 R-3 修复 commit 之后」且为独立路线（`relax-correction-weighted-v1`，route 隔离、A/B/C/D 对照矩阵、效果报告后 manager 裁决转正）——bb3063a 把 §24 实现混入 v3 身份整改提交并**在 LEGACY 路径默认激活**，跳过了路线验证与裁决（对照矩阵未跑、效果报告未出、无 route 开关）。
3. 影响面：所有非候选 run（含 SLIP 正式验证、观测①b 补采、既有回归）的 relax apply 语义已变；step214 的 BLOCKED 是新行为（预留挤占普通通道额度，可行域更紧）。
4. **次要偏差（D-1b）**：§24.2 第 4 条约定的 correction 遥测字段（correctionRequested/Granted/Rejected/StalledCount 进 P011NaturalObservationAudit）未实现（audit keys 无 correction 字段），correction 行为目前只部分可观测（relaxAppliedTotal 反映 apply，但预留/拒绝/停滞无账面）。

**裁决选项（交 manager）**：
- **Option A（推荐，最小整改）**：correction 槽位加资格门——`relaxHysteresisThresholds !== null`（候选 run）才走 applyCorrectionSlot，LEGACY 走旧 relax apply 分支（恢复其可达性）。correction 路线按 §24.3 排期独立验证后再议默认开关。改动量小（applyMotionChannels 一处分支+死代码复活），恢复 LEGACY 位精确后 D-1 解除。
- **Option B**：接受 LEGACY 行为变化，为 §24 路线补交对照矩阵+效果报告追认（工作量大，且与「效果报告后裁决转正」的纪律不符——先上车后补票）。

### 26.3 §25 清单状态更新与解锁建议

| §25 项 | 原状态 | 复核后 |
|---|---|---|
| □1 双角状态机 | GAP | **PASS**（根因已修，executed 同源） |
| □2 decision 时点 | GAP | **PASS**（采样点+全字段落地） |
| □3 attempt 审计 | GAP | **PASS**（三态落账+BLOCKED attempt+物理边映射+预览纯度四件套） |
| □4 schema v3 字段 | PASS+configured/executed 缺口 | **PASS**（executedThresholds 落地；configured 在 manifest 侧由 checker 核对） |
| □5 R-3 保真 | PASS | **PASS**（shadow 位精确在 74/0 下维持） |

**解锁建议**：v3 身份本体（GAP-1..5）复核通过，tester 核验 6ad709d6 可按此范围解锁；**但 D-1（LEGACY 行为变更+§24 未授权激活）建议在 tester 核验前由 manager 裁决**——Option A 整改后 LEGACY 回归基线才稳定，否则 tester 的核验对象是一个行为已漂移的构建。GAP-6（checker 九相名）仍归 tester。

变更记录：2026-10-21，依据 manager【观测③·v3复核】任务新增 §26。四项 GAP 转 PASS（附偏差 D-1：LEGACY 位精确不成立——§24 correction 分配器未经授权在 LEGACY 激活且改变 apply 语义，跨版本对比 step25 分歧/step214 BLOCKED 实证；D-1b correction 遥测字段未落地）。Option A/B 裁决交 manager。

**D-1 状态更新（2026-10-21）：RESOLVED。** 依据 manager 裁决（Option A + 显式路线隔离），developer 整改 commit `742efa6`（+验证用例 `55d18ae`）落地。architect 收缩范围复核（route 身份设计 + 实现边界两点，其余验收 manager 已实跑覆盖——四场景证据+240 锚点+Node 81/81）：

1. **route 身份设计确认 PASS**：`P011_CORRECTION_ROUTE_ID='relax-correction-weighted-v1'` 常量 + 双条件门 `correctionRouteActive() = relaxHysteresisThresholds !== null（必要） ∧ naturalObservationRouteId === P011_CORRECTION_ROUTE_ID（充分）`——与 §24 路线隔离语义完全一致。身份边界正交性确认：v3 状态机（collect 域，`hysteresis !== null` 分支）与 correction 分配（apply 域，`correctionRouteActive()` 门）无耦合——natural-relax-hysteresis-v1 候选 run 只获得状态机身份（decision 字段/executedThresholds），不隐式获得 correction 分配器；correction route 反向也不依赖观测身份（armed 机器即可，manifest 层另行绑定 candidate）。ledger `observationRouteId` 随显式绑定记录（correction run 记 `relax-correction-weighted-v1`，其余默认 `natural-relax-hysteresis-v1`），checker 可按 route 分池。

2. **实现边界抽查 PASS**：apply 分流落点在 `applyMotionChannels` 的 relax 槽位（`if (correctionRouteActive()) applyCorrectionSlot else relaxAngle`）；预算折叠隔离在 step 循环 scale 计算（correction run 折 B−R_limit，其余原 totalLimit 比例）；**settle/percolate/creep/lift 四个 writer 通道函数体零 diff**（grep diff 确认）；shadow 镜像同步分流（correction 槽位镜像/旧 relax 槽位镜像，verify 一致性比对自动覆盖）。LEGACY 240 步轨迹与 5e60f2d 基线锚点序列逐位一致（本复核上轮独立重跑：240/240 + 无分歧，含 step25/step214 历史分歧点）。

遗留（不阻断）：①正式 correction run 的 runner 侧 route 绑定（want 参数→SandCanvas→setNaturalObservationRouteId 接线）尚未交付——是 correction A/B 对照 run 的前置，属该路线任务范围；②D-1b（correction 遥测字段）按裁决单独处理（55d18ae 已含 RED 基线）。

**D-1 解除，tester v3 核验（6ad709d6）解锁。**

### 26.4 v3 播放器接口决策（2026-10-21，轻量版）

背景：EXECUTION_CHANNEL_BLOCKED 裁决选项 1（want 传 `p011schedule=peakADeg:rate`，应用内 `p011BuildScheduleV3` 自驱 100ms 节拍）。两个接口决策如下：

**决策 1·与 want 单命令通道并存 = 互斥（播放中拒绝单命令，不中断）。**
- 理由：v3 采集 run 的资格依赖「无外部输入」（协议 §5.1/bracket 资格「no reset/source switch」）；单命令打断会制造未预注册的输入源，整 run 报废。拒绝优于中断：中断语义（部分执行+中断标记）需要 ledger 新增生命周期事件与 checker 分支，收益为零（协议没有定义任何「被打断的 v3 run」的合格用途）。
- 行为：播放器激活期间收到 p011TiltWant 单命令 → 不消费、不推进 sequence，回 warning（want 解析层加 `SCHEDULE_ACTIVE_COMMAND_REJECTED`），ledger 不产生新 command 记录；播放器结束后恢复正常。反向：单命令先到、播放 want 后到 → 播放 want 同样被拒（testmode 单命令会话进行中不接受 schedule），两侧对称。
- 记账：拒绝事件只进 want 解析 warning 日志（hilog），不进物理 ledger——被拒绝的命令从未成为输入，无「已执行部分」需要记账。

**决策 2·节拍漂移 = 只记录不追赶，漂移进 attempt 层新字段。**
- 理由：协议 §3.2 要求「实际接受时间/间隔照实记录并执行既有输入时序闸门」——追赶（catch-up）会用跳步伪造节拍，正是「设备执行不能用多等数秒冒充 5100ms hold」要防的；如实带漂移标记让 checker 按既有闸门判命令资格。
- 实现：每条 schedule 命令记录 `scheduledOffsetMs` 与 `actualDispatchMs`；attempt 层新增 `scheduleDriftMs = actual − scheduled`（单字段，随既有 attempt 落账路径，无 schema 升版）；阈值 >50ms 时同 attempt 附 `SCHEDULE_DRIFT_EXCEEDED` 标记（非 error）。checker 侧由 tester 按既有输入时序闸门消费 drift 字段（哪些 drift 水平取消 bracket 资格是 checker 判定域，不在播放器内拦）。

**附带确认·planner 闸门 = schedule 命令逐条照常过 planner+guard，无直驱旁路。**
- 理由：①影子审计（协议 §4.1）背书的是「该 A/初态/dt 集合在含 planner+guard 的真实执行代码下安全」——但审计绑定具体初态/dt 集合，真机执行仍有运行时差异（调度、温控、实际 dt 轨迹），运行时闸门是最后防线；②「直驱」意味着旁路 planAndCommitTilt，那会让 ledger 的 planner 字段（thetaAccepted/requestRejected/plannerHold）失真，checker 对账（§25 清单 3/4）全部失效；③实现上零成本——播放器只替换命令来源（setTilt 目标），物理步进路径不变，planner 每子步照跑。
- 结论：播放器=命令发生器，不是执行模式。clamped/held/BLOCKED 照常出现且如实落账——这正是影子审计要对照的真机行为。

风险清单（3 项）：
1. **互斥拒绝的死锁窗口**：播放器异常退出（未清激活标志）会永久拒绝单命令——播放器状态必须挂生命周期（reset/退出清理），测试覆盖「播放器崩溃后单命令恢复」。
2. **drift 字段的消费口径**：tester 需在 checker 里定义 drift 阈值（哪些命令因 drift 失格）——本决策只产出数据，判定口径归 tester 任务，防止两处定义漂移。
3. **100ms 节拍与帧率耦合**：播放器节拍用单调时钟（非帧计数），低帧率（<10fps）时段可能出现单帧多命令——setTilt 是目标覆盖语义（最后一条生效），中间命令无物理子步消化；这是既有 want 语义的自然延伸，可接受，但 drift 字段会如实反映该窗口。

变更记录：2026-10-21，依据 manager【观测③·v3播放器方案】任务新增 §26.4（轻量）。决策 1=互斥拒绝；决策 2=只记录不追赶+scheduleDriftMs 字段；附带确认=逐条过 planner+guard 无直驱。developer 播放器任务（6ad8476f…8cda）解锁。

### 26.5 schedule 模式 metadata 武装路径工程语义（2026-10-21）

补 §26.4 盲区（来源 effect-report-P011-obs3v3-pilot-v2.md §5）：metadata 唯一设置点在 `freshScriptCommand`（要求 wstilt 被消费），纯 schedule want 到不了；带 wstilt 撞 §26.4 互斥——两条路都到不了 metadata，正式 pilot 必 INVALID。**developer 移交方案确认采纳**：`p011schedule` want 解析成功时从同 want 独立参数（`p011runrole/thresholdcalibrationid/thetastartdeg/thetastopdeg`）直接武装，复用既有 `setNaturalObservationRunMetadata`，零 schema 变更。五点工程语义：

1. **武装时序**：metadata 四参数随 `p011schedule` want 一起解析、在 `player.start()` **之前**设置（先武装后启动——播放器启动时状态机已 armed，首个子步即产生 executedThresholds/decision 字段）。与互斥窗口的关系确认：metadata 设置走 setter 直通车，不经 freshScriptCommand 消费路径，**不受 SCHEDULE_ACTIVE_COMMAND_REJECTED 影响**；互斥门只拦「tilt 命令消费」（wstilt），不拦「run 身份配置」——两者正交。

2. **参数校验语义（fail-closed，照既有 warning 体系）**：四参数全部可选但**成组校验**——缺任一或任一非法（thetastartdeg 非有限/不满足 0≤stop<start≤30、p011runrole 非 calibration|holdout、thresholdcalibrationid 空串）时**整个 schedule want 拒绝启动**（播放器不 armed，AppStorage 序列不推进），warning 枚举照 P011TiltWant 既有命名风格（`INVALID_SCHEDULE_METADATA`/`MISSING_SCHEDULE_METADATA`，hilog 输出）。理由：半武装（播放器跑起来但 executedThresholds=null）就是 pilot-v2 报废的根因，fail-closed 把该状态在入口处消灭；「照常启动+部分 metadata」的容错没有合格用途。纯 smoke（不带四参数）仍是合法形态——此时 runRole=null/阈值 null=LEGACY 语义，用于执行链验证（见第 4 点）。

3. **与 D-1 双条件门关系确认**：schedule 武装只触碰 `setNaturalObservationRunMetadata`（候选阈值→`applyRelaxHysteresisConfiguration`→armed 状态机→executedThresholds），**不触碰 `setNaturalObservationRouteId`**——correction 路线仍由独立参数 `p011route` 显式绑定（D-1 后续接线已就位），默认 null=旧 apply。correction 路线状态不受本变更影响（保持 CANDIDATE）；v3 候选 run（armed+无 correction route）走旧 relaxAngle apply——与 §26 D-1 RESOLVED 的正交性完全一致。

4. **smoke/正式 run 区分**：沿用 pilot-v2 报告先例——诊断性 smoke = 不带四参数的纯 schedule want（LEGACY 语义，runRole=null）；正式 run = 四参数成组武装（runRole=calibration|holdout）。**不新增 runRole 枚举值**（协议 `analysisRole=pilot/fit/holdout` 是 manifest 层分析身份，不进 ledger runRole）；smoke 的 runId 在报告中显式标记诊断性、不占 fit/holdout 槽位——报告纪律沿用 v2/v3 pilot 惯例，checker 不需要新字段识别 smoke（runRole=null 天然隔离：v210 checker 的 v3 分支只对 runRole∈{calibration,holdout} 全量校验，null 走观测分支）。

5. **风险清单 + 单测范围**：
   - 风险①先启动后武装的竞态：若实现顺序颠倒，播放器首个平台的子步以 LEGACY 跑——单测断言「schedule want 处理完成后 relaxHysteresisEnabled()===true（带参数）且首个 ledger record 的 executedThresholds 非空」。
   - 风险②四参数与单命令路径的重复设置：freshScriptCommand 与 schedule 武装都可能调 setNaturalObservationRunMetadata——后写覆盖先写，语义一致（同 run 重复武装同值无害，异值=want 层配置冲突，warning 提示但以最后到达为准，符合既有 AppStorage 单写者惯例）。
   - 风险③校验漂移：本节四参数校验与 P011TiltWant 既有四参数校验（:120-126 INVALID/MISSING 模式）必须同规则——建议抽共享校验函数，防两处阈值范围将来漂移。
   - 单测范围：①成组合法→armed+播放器启动；②缺一参→拒绝启动（播放器 null）；③非法值（stop≥start）→拒绝；④纯 smoke（零参数）→启动+LEGACY；⑤correction route 参数不随 schedule 武装（p011route=null）。

变更记录：2026-10-21，依据 manager【观测③·§26.5】任务新增。方案=developer 移交方案确认采纳（schedule want 独立参数直接武装，零 schema 变更）；五点语义（时序先启动/校验 fail-closed/D-1 正交/smoke=零参数 LEGACY/风险与单测）落盘。developer 实现任务解锁。

### 26.6 schedule want 身份三参数武装语义（§26.5 追加，2026-10-21）

来源：第三次停机 `SCHEDULE_EXEC_IDENTITY_NOT_CARRIED`（effect-report-P011-obs3v3-pilot-v3.md §5）——69345cc 武装只覆盖 metadata 四参数；身份三参数（`execsourcecommit/execsourcefilesha256/execbuildsha256`）仍只在 wstilt 路径解析（P011TiltWant:242-251），schedule run 的 `executedThresholds.sourceCommit/双 SHA` 落空串（`executedThresholdsForLedger` 的 `?? ''` 兜底，WaterSandField:1468-1470）→ checker 判 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH`。修复与 §26.5 完全同模式，四点确认：

1. **成组可选解析**：身份三参数在 `parseP011ScheduleWant` 内作为独立第二组，照 metadata 组同模式 fail-closed——全无=allAbsent 合法；缺任一（partial）→ `MISSING_SCHEDULE_EXEC_IDENTITY` 拒绝整个 want；全有→逐项正则校验（commit `/^[0-9a-fA-F]{6,64}$/`、文件/构建 SHA `/^[0-9a-fA-F]{64}$/`，与 wstilt 路径同规则），非法 → `INVALID_SCHEDULE_EXEC_IDENTITY`。正则抽共享校验函数（照 §26.5 风险③先例，放 P011TiltWant，两侧共用防漂移）。

2. **武装调用**：SandCanvas 武装块（现 :1116-1123 metadata armed 分支）内随 `setNaturalObservationRunMetadata` 一并调用 `wsField.setNaturalObservationExecutionIdentity(commit, fileSha, buildSha)`——接口已存在（WaterSandField:1388，签名 `(string|null)×3`），wstilt 路径 :1495-1498 已有调用先例；成组校验通过后三参数必然全非空，直接调用。AppStorage 键照 metadata 四键模式：EntryAbility 写 `p011ScheduleExecSourceCommit/ExecSourceFileSha256/ExecBuildSha256`，SandCanvas 读。

3. **空/缺省语义（组间绑定规则，核心）**：两组必须**同现或同缺**——`metadata 全有 ∧ 身份全有` = 正式 run 合法启动（executedThresholds 三字段非空）；`两组全无` = 纯 smoke，runRole=null/阈值 null → executedThresholds=null（身份字段根本不落账），LEGACY 同前 ✓；`metadata 全有 ∧ 身份全无` = **拒绝**（正式 run 身份缺省必然产出 MISMATCH INVALID run——第三次停机的状态在入口消灭，fail-closed 原则的第二次应用）；`身份全有 ∧ metadata 全无` = **拒绝**（武装身份但状态机不 armed，executedThresholds 恒 null 身份无处落账，无合格用途）。

4. **校验次序**：两组校验独立执行（纯函数无副作用，warning 各自收集），组间绑定（XOR 拒绝）在两组校验之后、`player.start()` 之前判定；任一 warning 或绑定失败 → 播放器不 armed、AppStorage 序列不推进（与 §26.5 第 2 点同口径）。次序本身无语义，绑定规则必须显式实现为上述第 3 点的四种组合枚举。

单测追加（照 §26.5 五场景模式）：①两组全有→armed+三身份字段非空；②metadata 有+身份缺→拒绝；③身份 partial/非法→拒绝；④两组全无→smoke 启动+executedThresholds=null；⑤身份有+metadata 无→拒绝。`?? ''` 兜底保留不动（防御性默认，正常路径不再触达）。

变更记录：2026-10-21，依据 manager【观测③·§26.5追加】任务新增（第三次停机修复依据，manager 16:1x 批准追加确认形式）。四点语义：成组 fail-closed/武装块一并调用/组间同现或同缺（fail-closed 第二次应用）/绑定在 start 前显式判定。developer 实现任务（6ad87955…e58 后或合并）解锁。
