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
