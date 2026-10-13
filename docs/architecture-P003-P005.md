# 工程落地方案：P-003 + P-005（场景二水中双沙与测试体系）

> 方案版本：v1　日期：2026-10-13　状态：DRAFT  
> 维护人：architect  
> 上游依据：`docs/algorithm-P003-P005.md` v1  
> 关联说明：P-003 与 P-005 共享同一状态结构、指标口径和测试钩子，采用联合工程方案；P-003 负责实现能力，P-005 负责把能力转成可执行客观/主观验证。  
> 约束：本方案只做工程设计，不修改上游算法文档和其他角色文档。

---

## 0. 上游矛盾与待确认项（提请 manager 处理）

### 0.1 阻塞项：重沙渗透公式破坏分相守恒

`docs/algorithm-P003-P005.md` §2.3 要求 `VH = Σ(hs + suspH)` 与 `VL = Σ(hl + suspL)` 各自守恒，但 §3.3 给出：

```text
hl[c] -= q
hs[c] += q
```

该操作会让 `VH` 增加 `q`、`VL` 减少 `q`，只保持 `VH + VL` 不变，无法满足 P-003 验收中 water、重沙、轻沙三相各自守恒的要求。列内“重沙下沉、轻沙上浮”的物理交换在仅有 `hs/hl` 两个厚度标量的表示下不能直接表达，需要 designer 给出保相交换公式或明确放弃该子机制。

**工程处理**：

- developer 不得按上述公式实现 `percolateHeavy()`。
- `WaterSandParams.percolationRate` 保留接口与面板项，但其 v1 实现必须返回 `q = 0`，并在 `WS_EVENT` 中输出 `percolation_disabled_pending_designer`，直到 designer 更新算法方案。
- 若 manager/designer 后续允许临时禁用作为正式 v1 行为，需在算法文档变更记录中确认；architect 不自行改算法。

### 0.2 待确认项：物理步长口径

算法参数表写 `dt = 1/120s`，伪代码又规定 `substeps = ceil(dt / (1/60))` 且最多 2 个子步。两者在 60Hz 帧率下不一致。本工程方案按伪代码口径落地：帧间隔先夹到 `1/30s`，最多拆 2 个不超过 `1/60s` 的子步；`1/120s` 作为 120Hz 设备上的自然帧步长。若 designer 要求严格固定 `1/120s`，需更新子步上限为 4 并同步调整指标。

### 0.3 测试环境前提：FPS 指标依赖 120Hz 显示

当前 `SandCanvas` 使用 `postFrameCallback`，帧率受显示刷新率限制。`fpsP95 >= 100` 只能在 120Hz 或更高刷新率的真机/模拟器上测得；60Hz 环境应输出 `N/A(environment_refresh_rate)`，不得把显示上限误判为实现性能失败。

---

## 1. 现状与差距

### 1.1 现有模块盘点

| 模块 | 现状 | 与本需求差距 |
|---|---|---|
| `entry/src/main/ets/physics/SandGrid.ets` | CA 两相沙模型，支持四向重力、渗透、滑移、`liquidMode` | 无水相、无三相守恒账本、无悬浮态；不适应平滑沙丘目标 |
| `entry/src/main/ets/physics/FluidField.ets` | 列标量两相液体高度场，`heavy/light/ero`，保守迁移 | 只有平行界面假设，无沙堆轮廓、沉积/悬浮、安息角 |
| `entry/src/main/ets/pages/SandCanvas.ets` | `MODE_SAND=0`、`MODE_FLUID=1`、`MODE_HYBRID=2`、`MODE_LIQUID=3`；`MODE_COUNT=4`；面板循环模式；传感器/手势/测试模式/强制倒立已有 | 无 `MODE_WATERSAND`，无水沙状态与渲染分支，无 WS 指标日志 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | Want 参数已支持 `testmode`、`forceinvert`、`rendermode` | `rendermode=4` 可直入新模式，但缺少水沙预设参数 |
| `entry/src/main/ets/pages/Index.ets` | 独立 Hello 页面，未被 `EntryAbility` 加载 | 不改动 |
| `entry/src/test/List.test.ets`、`entry/src/ohosTest/ets/test/List.test.ets` | Hypium 测试入口，已注册 `SandGridTest` 等 | 未注册 `WaterSandFieldTest` |
| `entry/src/test/SandGridTest.ets` | 既有 CA 用例 | 本方案不改 `SandGrid`，仅回归；不因新模型改旧断言 |
| `test/*.py`、`test/*.md` | 已有两相视频/像素/倒立判定脚本与用例文档 | 无三相指标解析、分段判定和像素交叉验证脚本 |

### 1.2 新增 / 修改 / 不动清单

**新增**：

- `entry/src/main/ets/physics/WaterSandField.ets`
- `entry/src/test/WaterSandFieldTest.ets`
- `entry/src/ohosTest/ets/test/WaterSandFieldTest.ets`
- `test/water_sand_check.py`
- `test/water_sand_pixel_check.py`
- `test/record_tc_ws.py`

**修改**：

- `entry/src/main/ets/pages/SandCanvas.ets`
- `entry/src/main/ets/entryability/EntryAbility.ets`
- `entry/src/test/List.test.ets`
- `entry/src/ohosTest/ets/test/List.test.ets`

**不动**：

- `SandGrid.ets`
- `FluidField.ets`
- 既有 0~3 渲染模式的绘制逻辑
- `entry/src/main/ets/pages/Index.ets`
- `docs/algorithm-P003-P005.md`
- tester/effector 维护的测试与效果文档

---

## 2. 模块设计

### 2.1 文件与职责

新增 `entry/src/main/ets/physics/WaterSandField.ets`，只承载场景二物理模型：

```text
WaterSandField
├── 类型定义：参数、区域、场景、指标、快照、错误信息
├── 状态账本：waterTop / hs / hl / suspH / suspL + 派生 sandTop
├── 保守操作：settle / diffuse / avalanche / lift / waterLevel / invert180
├── 审计：三相总量、漂移、NaN/负值、容量、休眠条件
└── 测试接口：configure / initialize / inject / snapshot / getMetrics
```

`SandGrid` 与 `FluidField` 继续服务既有模式。`WaterSandField` 不继承、不包装、不改写它们；`SandCanvas` 在模式切换时只激活其中一个模型。

### 2.2 六状态组织

六状态为逻辑状态，其中 `sandTop` 是派生值，不做持久化存储，避免 `hs + hl` 与 `sandTop` 出现双账本漂移：

| 状态 | 存储 | 说明 |
|---|---|---|
| `waterTop[c]` | `Float64Array` | 自当前重力底部向上的自由表面高度 |
| `hs[c]` | `Float64Array` | 已沉积重沙等效高度 |
| `hl[c]` | `Float64Array` | 已沉积轻沙等效高度 |
| `suspH[c]` | `Float64Array` | 悬浮重沙等效高度 |
| `suspL[c]` | `Float64Array` | 悬浮轻沙等效高度 |
| `sandTop[c]` | 每个子步写入 scratch | `hs[c] + hl[c]`，只读派生 |

私有 scratch 建议复用以下数组，构造时一次性分配，不在 `step()` 中新建：

```text
sandTopScratch: Float64Array
targetTopScratch: Float64Array
errorScratch: Float64Array
transferScratch: Float64Array
smoothScratch: Float64Array
```

初始化后记录：

```text
initialWater = Σ max(0, waterTop - sandTop - suspH - suspL)
initialHeavy = Σ (hs + suspH)
initialLight = Σ (hl + suspL)
```

所有状态转移必须使用“先裁剪、再同时扣减/增加”的保守形式。禁止清空重建、按目标面整列重写或渲染层回写物理状态。

### 2.3 公共接口定义

以下接口为开发契约，函数名、参数和返回值不得随意变更：

```ts
export enum WaterSandPhase {
  WATER = 0,
  HEAVY = 1,
  LIGHT = 2
}

export enum WaterSandInjectionState {
  DEPOSITED = 0,
  SUSPENDED = 1
}

export interface WaterSandParams {
  angleDeg: number;
  heavySettleSpeed: number;
  lightSettleSpeed: number;
  percolationRate: number;
  waterLevelRate: number;
  heavyDiffusion: number;
  lightDiffusion: number;
  shakeDecay: number;
  heavyLiftRate: number;
  lightLiftRate: number;
  maxAvalancheFlux: number;
  avalanchePasses: number;
  maxTiltOmegaDegPerSec: number;
}

export interface WaterSandRegion {
  startCol: number;
  endColInclusive: number;
  bottom: number;
  height: number;
}

export interface WaterSandScene {
  waterRegions: WaterSandRegion[];
  heavyDepositedRegions: WaterSandRegion[];
  lightDepositedRegions: WaterSandRegion[];
  heavySuspendedRegions: WaterSandRegion[];
  lightSuspendedRegions: WaterSandRegion[];
}

export interface WaterSandInjection {
  phase: WaterSandPhase;
  amount: number;
  startCol: number;
  endColInclusive: number;
  state: WaterSandInjectionState;
}

export interface WaterSandTotals {
  water: number;
  heavy: number;
  light: number;
}

export interface WaterSandDrift {
  water: number;
  heavy: number;
  light: number;
}

export interface WaterSandMetrics {
  driftWater: number;
  driftHeavy: number;
  driftLight: number;
  depositedHeavyFraction: number;
  depositedLightFraction: number;
  suspensionFraction: number;
  separationRatio: number;
  lowerHeavyShare: number;
  upperHeavyShare: number;
  t90Heavy: number;
  t90Light: number;
  maxSlopeDeg: number;
  maxCurvature: number;
  waterSlopeDeg: number;
  waterAmplitude: number;
  waterAmplitudePx: number;
  phaseOrderScore: number;
  maxSandTopDeltaPerSecond: number;
  physicsTimeMs: number;
  dormant: boolean;
}

export interface WaterSandStepResult {
  ok: boolean;
  stepped: boolean;
  substeps: number;
  moved: boolean;
  dormant: boolean;
  physicsTimeMs: number;
  errorCode: WaterSandErrorCode | null;
  errorColumn: number;
}

export enum WaterSandErrorCode {
  NEGATIVE_VALUE = 1,
  NON_FINITE_VALUE = 2,
  CAPACITY_OVERFLOW = 3,
  WATER_SOLVE_FAILED = 4,
  UNKNOWN = 99
}

export interface WaterSandSnapshot {
  cols: number;
  capacity: number;
  waterTop: Float64Array;
  sandTop: Float64Array;
  hs: Float64Array;
  hl: Float64Array;
  suspH: Float64Array;
  suspL: Float64Array;
}

export class WaterSandField {
  readonly cols: number;
  readonly capacity: number;

  constructor(cols: number, capacity: number);
  configure(params: WaterSandParams): void;
  initialize(scene: WaterSandScene): void;
  reset(scene?: WaterSandScene): void;
  setTilt(deg: number): void;
  setShakeInput(value: number): void;
  inject(spec: WaterSandInjection): void;
  invert180(): void;
  step(dt: number): WaterSandStepResult;
  getTotals(): WaterSandTotals;
  getDrift(): WaterSandDrift;
  getMetrics(pixelScale?: number): WaterSandMetrics;
  snapshot(): WaterSandSnapshot;
  copyStateTo(target: WaterSandSnapshot): void;
  getLastError(): WaterSandErrorCode | null;
}
```

### 2.4 接口语义

- `configure(params)`：校验 `0 < angleDeg < 45`、`0 < lightSettleSpeed < heavySettleSpeed`、`percolationRate > 0`、扩散/沉降/流动速率非负。非法参数抛出 `WaterSandParamError`，不部分应用。
- `initialize(scene)` / `reset(scene?)`：重建全部状态与初始总量；`scene` 为空时使用内置默认混合注入场景。总容量溢出直接抛错，不裁剪。
- `setTilt(deg)`：保存目标倾角，输入夹到 `[-89, 89]`；实际角速度限制在 `step()` 内执行。非有限数抛错。
- `setShakeInput(value)`：保存 `[0, 1]` 内的瞬时扰动输入。
- `inject(spec)`：按列均分注入总量；水注入后重解水面，沙注入可指定沉积/悬浮。注入不得造成越界或负值。
- `invert180()`：只做同相镜像转移与再悬浮，随后重解水面，并输出事件由 `SandCanvas` 记录。
- `step(dt)`：夹 `dt <= 1/30`，按最多 2 个子步执行算法伪代码顺序；数值异常时冻结状态并返回 `ok=false`，不在帧回调中抛异常导致应用退出。
- `snapshot()`：测试用，可分配新数组；生产渲染使用 `copyStateTo(target)` 复用 `WaterSandSnapshot`。

### 2.5 与既有模型关系

| 场景 | `SandGrid` | `FluidField` | `WaterSandField` |
|---|---|---|---|
| 细沙 0 | step + render | 不 step | 不 step |
| 流体 1 | step + render | 不 step | 不 step |
| 混合 2 | 保留实例 | step + render | 不 step |
| 液体 3 | step + render | 不 step | 不 step |
| 水沙 4 | 不 step | 不 step | step + render |

模式切换语义：

- 切出 `MODE_WATERSAND` 时冻结 `WaterSandField`，不把状态写入 `SandGrid` 或 `FluidField`。
- 切回 `MODE_WATERSAND` 时从冻结状态继续，除非用户点击“重置”。
- 旧模式的 `field` 与 `fluidField` 继续保留，便于模式来回切换，但内存中新增状态小于 16KB。

---

## 3. 渲染集成

### 3.1 模式接入

在 `SandCanvas.ets` 修改：

```ts
const MODE_WATERSAND = 4;
const MODE_COUNT = 5;
```

面板模式文案追加：

```text
renderMode === MODE_WATERSAND ? '模式:水沙' : ...
```

`rendermode=4` 通过既有 Want 参数直入，不需要新增启动页。`EntryAbility` 只更新注释和新增水沙预设参数解析，不改变页面加载路径。

### 3.2 数据流与时序

```text
传感器 / 手势 / Want(testmode, forceinvert)
    ↓
SandCanvas 姿态与测试模式控制器
    ↓ setTilt() / setShakeInput() / invert180()
WaterSandField.step(frameDt)
    ↓ copyStateTo(reusableSnapshot)
renderWaterSand(ctx, snapshot)
    ↓
Canvas Path / 伪粒子 / 混合带 / 高光
    ↓
WS_METRIC / WS_EVENT / WS_FRAME hilog
    ↓
test/water_sand_check.py
```

帧内顺序：

1. `MyFrameCallback.onFrame(frameTimeInNano)` 把纳秒转为毫秒传入 `frame(frameTimeMs)`。
2. 计算与上一帧的间隔，作为 `WS_FRAME.frameTimeMs`。
3. 若 `renderMode === MODE_WATERSAND`，调用 `setTilt` / `setShakeInput` / `invert180` 的边沿事件，再执行 `step(dt)`。
4. `copyStateTo()` 到复用快照。
5. 绘制 `MODE_WATERSAND`，随后返回，不落入旧模式 `SandGrid.step()`。
6. 每 250ms 输出 `WS_METRIC`；事件立即输出 `WS_EVENT`。

### 3.3 线程模型

- v1 物理与渲染均在 UI 线程的 frame callback 内执行。模型为 `O(KN + N log(H/ε))`，目标单步 `< 2ms`，同步执行低于 10ms 帧预算，不引入 Worker 通信成本。
- `step()` 最多 2 个子步；所有 scratch 和渲染快照复用，禁止每帧分配大数组。
- 若真机 `physicsTimeMs P95 > 2ms` 或 `fpsP95 < 100`，先切换性能档：`N=96`、每列伪粒子 4 个；仍不达标再立项 Worker 版，v1 不做异步物理。
- 数值错误时冻结模型、停止物理 step、保留最后一帧画面并输出 `WS_EVENT model_error`，避免崩溃或自动修复掩盖问题。

### 3.4 平滑 Path

渲染前对 `waterTop / sandTop / hs` 做两轮 3 点滑动平均：

```text
smooth[i] = (src[max(0,i-1)] + src[i] + src[min(n-1,i+1)]) / 3
```

坐标映射：

```text
x(c) = boxX + c * boxW / (cols - 1)
y(height) = boxY + (capacity - height) * boxH / capacity
```

水面 Path 忽略两端 5% 列采样，用二次贝塞尔连接内部点；沙堆轮廓使用全列平滑。绘制顺序严格为：

1. 容器背景与边框底色。
2. 水体：`waterTop` 到 `max(sandTop, 0)` 的半透明蓝色区域。
3. 悬浮伪粒子。
4. 重沙层：底部到 `smooth(hs)`。
5. 轻沙层：`smooth(hs)` 到 `smooth(sandTop)`。
6. 重/轻沙界面混合带。
7. 水面高光。
8. 测试模式调试层。

推荐色：

```text
水体：rgba(64,148,222,0.42)
水面高光：rgba(210,240,255,0.70)
重沙：#24571F → #63A35D
轻沙：#D9A64A → #F6E2A7
混合带：重沙色与轻沙色的透明渐变
```

### 3.5 悬浮伪粒子

每列最多 8 个，禁用 `Math.random()`。确定性相位函数：

```text
seed(c, slot, phase) = ((c * 131 + slot * 137 + phase * 139) % 257) / 257
yUnit(c, slot) =
  sandTop[c] +
  ((slot + 0.5 + 0.20 * sin(timeSec * 0.8 + seed * 2π)) / 8) *
  max(0, waterTop[c] - sandTop[c])
```

每相粒子数量：

```text
count = ceil(8 * min(1, suspPhase[c] / max(waterDepth[c], ε)))
```

半径 1.0~1.8px，透明度按悬浮浓度映射，最大不超过 0.75。粒子只读物理状态，不回写 `suspH/suspL`。

### 3.6 混合带

- 围绕 `smooth(hs)` 的界面绘制，宽度为 `mixBandPx`，默认 1.5px，面板范围 1~2px。
- 使用从重沙色到轻沙色的线性渐变，两端透明度低于主体层。
- 混合带只影响视觉，不表示相间质量交换，不得参与守恒统计。

### 3.7 倒立渲染矩阵

物理状态始终在重力对齐坐标系内绘制，倒立只改变 Canvas 变换：

```ts
ctx.save();
ctx.translate(centerX, centerY);
if (visualInverted) {
  ctx.rotate(Math.PI);
}
ctx.translate(-centerX, -centerY);
drawGravityAlignedWaterSand();
ctx.restore();
```

姿态映射规则：

```text
normalizedAngle = ((tiltX % 360) + 360) % 360
visualInverted = normalizedAngle > 90 && normalizedAngle < 270
effectiveTilt =
  visualInverted ? clamp(normalizedAngle - 180, -89, 89)
                 : clamp(normalizedAngle, -89, 89)
```

- `visualInverted` 从 false 变 true 时调用一次 `invert180()`。
- 从 true 变 false 时也调用一次 `invert180()`，把数据坐标系镜像回普通姿态。
- 连续倒立期间 `effectiveTilt` 驱动水面与安息角，渲染矩阵保持 `rotate(π)`。
- 渲染层不得复制或翻转物理数组，不得因倒立分支改变 `Vw/VH/VL`。

---

## 4. 参数映射

### 4.1 默认参数

| 工程字段 | 算法参数 | 默认值 | 校验/范围 |
|---|---:|---:|---|
| `cols` | `N` | 128 | 构造参数，96/128/160 三档 |
| `capacity` | `H` | 192 | 构造参数，v1 固定 |
| `angleDeg` | `α` | 30° | `(0, 45)` |
| `heavySettleSpeed` | `vH` | 16 | `> lightSettleSpeed > 0` |
| `lightSettleSpeed` | `vL` | 6 | `> 0` |
| `percolationRate` | `permRate` | 6/s | 上游矛盾未解决前不生效 |
| `waterLevelRate` | - | 8/s | `> 0` |
| `heavyDiffusion` | `DH` | 4 | `>= 0` |
| `lightDiffusion` | `DL` | 10 | `>= 0` |
| `shakeDecay` | - | 2.2/s | `> 0` |
| `heavyLiftRate` | - | 0.35/s | `>= 0` |
| `lightLiftRate` | - | 0.55/s | `>= 0` |
| `maxAvalancheFlux` | - | 24 | `> 0` |
| `avalanchePasses` | `K` | 5 | `[4, 6]` |
| `maxTiltOmegaDegPerSec` | `maxOmega` | 180 | `> 0` |
| `mixBandPx` | - | 1.5 | `[1, 2]` |
| `dt` | `dt` | 帧 dt | 夹到 `1/30`，最多 2 子步 |

默认场景建议：

```text
cols = 128
capacity = 192
waterRegions = [{ startCol: 0, endColInclusive: 127, bottom: 0, height: 110 }]
heavySuspendedRegions = [{ startCol: 0, endColInclusive: 127, bottom: 0, height: 24 }]
lightSuspendedRegions = [{ startCol: 0, endColInclusive: 127, bottom: 0, height: 16 }]
heavyDepositedRegions = []
lightDepositedRegions = []
```

### 4.2 面板可调项

进入 `MODE_WATERSAND` 后显示：

| 面板项 | 控件 | 范围 | 生效时机 |
|---|---|---:|---|
| 安息角 | Slider | 10~44°，step 1 | 下一 step |
| 重沙沉降 | Slider | 4~30，step 1 | 下一 step，需保持 `vH > vL` |
| 轻沙沉降 | Slider | 1~15，step 1 | 下一 step，需保持 `vH > vL` |
| 分选强度 | Slider | 0~12，step 1 | 仅写参数；当前事件提示待上游确认 |
| 水面流动性 | Slider | 1~16，step 1 | 下一 step |
| 混合带宽 | Slider | 1~2px，step 0.5 | 下一帧渲染 |
| 测试扰动 | Slider | 0~1，step 0.1 | 写 `setShakeInput`，`E` 自然衰减 |
| 沙粒粗细 | 三态按钮 | 96/128/160 | 点击“重置”后生效 |

`flowSpeed` 只属于旧模式，在水沙模式隐藏，不得复用为沉降速度或子步数量。

---

## 5. 测试钩子与判定体系

### 5.1 启动参数

保留既有参数：

```text
--ps testmode true
--ps forceinvert true
--ps rendermode 4
```

新增水沙测试参数：

| Want 参数 | 类型 | 说明 |
|---|---|---|
| `wspreset` | string | `default` / `sorting` / `settling` / `angle` / `nowater` / `heavyonly` / `lightonly` / `nearfull` |
| `wscols` | number | 96/128/160，覆盖预设 |
| `wscapacity` | number | 正整数，测试专用 |
| `wsmixband` | number | 1~2 |
| `wssnapshot` | boolean | true 时在 init/reset 后输出一条完整快照 |

`testmode=true` 沿用现有语义：`useSensor=false`，手势保持 24 小时，传感器数据即使回调到达也不参与姿态。水沙分支不得重新启用传感器。

### 5.2 hilog 钩子

统一 tag：`WaterSand`，domain 使用 `0x0000`。

```text
WS_EVENT init {preset:default,cols:128,capacity:192}
WS_EVENT reset {preset:default}
WS_EVENT mode {renderMode:4}
WS_EVENT tilt {targetDeg:30,effectiveDeg:30}
WS_EVENT shake {input:1.0}
WS_EVENT invert180 {count:1}
WS_EVENT percolation_disabled_pending_designer {}
WS_EVENT dormant {value:true}
WS_EVENT wake {reason:tilt}
WS_EVENT model_error {code:1,column:17}

WS_METRIC {driftWater:0.0,driftHeavy:0.0,driftLight:0.0,...}

WS_FRAME {frameTimeMs:8.2,physicsTimeMs:1.1}
```

输出规则：

- `WS_EVENT`：事件帧立即输出。
- `WS_METRIC`：每 250ms 输出一次，JSON 字段与 `WaterSandMetrics` 一一对应。
- `WS_FRAME`：`testmode=true` 时每个渲染帧输出；普通模式不输出，避免日志影响体验。
- `wssnapshot=true`：init/reset 后输出 `WS_SNAPSHOT`，包含六状态数组与初始总量。
- 所有 JSON 使用稳定字段顺序，便于脚本解析与 diff。

### 5.3 Node / Hypium 单测结构

新增 `WaterSandFieldTest.ets`，并同时注册到：

- `entry/src/test/List.test.ets`
- `entry/src/ohosTest/ets/test/List.test.ets`

用例结构：

| 用例 | 断言 |
|---|---|
| `configure_invalidParams_throw` | 非法速率/角度/vH<=vL 抛错且旧参数不变 |
| `initialize_capacityOverflow_throw` | 超容量抛错，不裁剪 |
| `conservation_static` | 静置多帧三相漂移均 `< 0.1%` |
| `conservation_tiltShakeInvert` | 倾斜、摇晃、倒立、倒立后摇晃序列守恒 |
| `settling_heavyFasterThanLight` | `t90H < t90L` 且比值 `<= 0.7` |
| `sorting_separationRatio` | 静置 10s 后 `separationRatio <= 0.5`；单相输出 N/A |
| `angle_overLimitRelax` | 超角沙堆坡度 `<= α+2°`，曲率 `<= 0.35` |
| `angle_subLimitZeroDrift` | 亚角沙堆后 5s 漂移 `<= 0.25` |
| `water_horizontalRest` | 水面斜率 `<= 2°`，幅度换算像素 `<= 2px` |
| `water_tiltSteadyState` | 15°/30° 斜率误差 `<= 15%` |
| `invert_phaseOrderAndConservation` | 倒立后相序分数 `>= 0.30` 且守恒 |
| `shake_liftAndResettle` | 峰值悬浮 `>= 0.5`，静置 8s 残余 `<= 0.05` |
| `boundary_noWater_singlePhase_nearFull` | 对应指标 N/A 或通过，守恒仍必须通过 |
| `dormancy_enterAndWake` | 满足条件休眠，输入变化唤醒 |
| `numericalError_freezeNoAutoRepair` | 构造异常后 `ok=false`、状态不变、错误列正确 |

单测不得依赖 Canvas、传感器或随机数；所有场景通过 `WaterSandScene` 构造。

### 5.4 判定脚本结构

#### `test/water_sand_check.py`

输入：

```text
python test/water_sand_check.py --input ws_hilog.txt --all --report test/reports/ws_check.json
```

模块结构：

```text
parse_lines()      # 解析 WS_METRIC / WS_EVENT / WS_FRAME / WS_SNAPSHOT
build_segments()   # init → shake → invert → rest → long_rest
evaluate_*()       # conservation/sorting/settling/angle/water/invert/shake/fps
write_json()
write_markdown()
main()
```

CLI 选择器：

```text
--conservation --sorting --settling --angle --water --invert --shake --fps --all
```

判定规则：

- PASS：达到算法 §9 指标。
- FAIL：有实测值且未达阈值。
- N/A：单相缺失、上下区为空、无水场景的水面项、60Hz 环境的 FPS 项。
- FPS 至少需要 100 个 `WS_FRAME` 样本；`fpsP95 = 1000 / p95(frameTimeMs)`。
- 输出 JSON 供 CI/manager 汇总，Markdown 供 tester 报告引用。

#### `test/water_sand_pixel_check.py`

输入录制帧与颜色标定，分类水体、重沙、轻沙、混合带，输出各相像素占比，并与最近 `WS_METRIC/WS_SNAPSHOT` 的模型占比交叉验证。偏差阈值 `<= 5%`。该脚本只做视觉交叉验证，不替代模型守恒判定。

#### `test/record_tc_ws.py`

负责真机/模拟器执行编排：

1. 安装指定 HAP。
2. 以 `rendermode=4`、`testmode=true`、`wspreset=*` 启动。
3. 按用例注入倾角、摇晃、`forceinvert` 序列。
4. 采集 hilog 与连拍帧。
5. 调用 `water_sand_check.py` 与 `water_sand_pixel_check.py`。

脚本只生成测试产物，不修改应用代码。

---

## 6. 实施步骤

### Step 1：类型与状态骨架

文件：`entry/src/main/ets/physics/WaterSandField.ets`

- 定义全部接口、枚举、错误码。
- 分配 5 个持久状态数组和 scratch。
- 实现 `configure/initialize/reset/snapshot/copyStateTo/getTotals/getDrift`。
- 实现容量校验和默认场景。

### Step 2：保守动力学

文件：`entry/src/main/ets/physics/WaterSandField.ets`

- 实现 `setTilt/setShakeInput/step` 子步调度。
- 实现水面保守迁移、悬浮扩散、沉降、表层扬起、安息角滑移。
- `percolationHeavy()` 只保留空实现并输出待确认事件，不实现上游矛盾公式。
- 实现数值审计和错误冻结。

### Step 3：指标与注入

文件：`entry/src/main/ets/physics/WaterSandField.ets`

- 实现 `WaterSandMetrics` 全字段。
- 实现 `inject()`、`invert180()`、休眠/唤醒。
- `t90`、分选、坡度、曲率、水面、相序、无瞬移指标在模型内计算，避免 UI 层重复口径。

### Step 4：渲染与模式

文件：`entry/src/main/ets/pages/SandCanvas.ets`

- 新增 `MODE_WATERSAND=4`、`MODE_COUNT=5`。
- 新增 `WaterSandField` 实例、复用快照、姿态边沿控制。
- 新增 `renderWaterSand()`、平滑 Path、伪粒子、混合带、倒立矩阵。
- 在 frame callback 中增加水沙分支，确保不 step 旧模型。

### Step 5：参数面板与 Want

文件：`entry/src/main/ets/pages/SandCanvas.ets`、`entry/src/main/ets/entryability/EntryAbility.ets`

- 扩展 `PanelParams` 与面板控件。
- 增加 `wspreset/wscols/wscapacity/wsmixband/wssnapshot` 解析。
- 保持 `testmode` 传感器隔离语义不变。

### Step 6：单测

文件：

- `entry/src/test/WaterSandFieldTest.ets`
- `entry/src/ohosTest/ets/test/WaterSandFieldTest.ets`
- `entry/src/test/List.test.ets`
- `entry/src/ohosTest/ets/test/List.test.ets`

- 先跑新增模型单测，再跑既有 `SandGridTest` 回归。
- 不修改既有旧模式断言。

### Step 7：测试钩子与脚本

文件：

- `entry/src/main/ets/pages/SandCanvas.ets`
- `test/water_sand_check.py`
- `test/water_sand_pixel_check.py`
- `test/record_tc_ws.py`

- 实现 WS 日志、帧耗时、快照输出。
- 实现脚本解析、分段、阈值判定与报告输出。

### Step 8：验证交接

- developer 交付：代码、单测结果、可进入 `MODE_WATERSAND` 的说明。
- tester 交付：`TC-WS-01..11` 客观结果。
- effector 交付：`EC-WS-01..05` 主观评分。
- 任一客观数值未达标时，由 designer 根据失败指标更新算法，不得由 developer 私自调公式闭环。

---

## 7. 风险与回滚

### 7.1 数值稳定性

| 风险 | 处理 |
|---|---|
| 负值 / NaN | 冻结状态、返回错误码和列号、输出 `WS_EVENT model_error`，禁止自动修正 |
| 水面二分无解 | 返回 `WATER_SOLVE_FAILED`，保留上一帧状态 |
| 容量溢出 | 初始化/注入直接拒绝；step 内容量约束先裁剪转移量 |
| 长时零漂移 | 休眠条件满足后不再执行状态转移，输入变化立即唤醒 |
| 分选渗透公式矛盾 | v1 禁用该子机制，等待 designer 更新 |

### 7.2 性能

- 默认 `N=128`、每列 8 个伪粒子，物理目标 `< 2ms`，整帧目标 `< 5ms`。
- `WS_FRAME` 只在测试模式输出；`WS_METRIC` 250ms 节流。
- 渲染快照与 scratch 全部复用；Path 数量为常数级。
- 若 `fpsP95 < 100`：先降到 `N=96`、每列 4 粒子；仍失败才评估 Worker 或渲染降档。
- 60Hz 显示环境不得判 FAIL，应输出环境刷新率不足的 N/A。

### 7.3 对既有模式影响面

- 只新增 `renderMode=4` 分支和 `MODE_COUNT` 递增，不修改 0~3 的物理调用。
- `SandGrid`、`FluidField` 不改，旧单测应保持通过。
- `flowSpeed` 在水沙模式隐藏，避免参数语义污染。
- 模式切换只冻结/恢复 `WaterSandField`，不做状态转换。

### 7.4 回滚策略

- **配置回滚**：将 `MODE_COUNT` 恢复为 4，面板不再循环到水沙；`rendermode=4` 启动时夹回默认模式。新增 `WaterSandField.ets` 可暂留但不被调用。
- **功能回滚**：移除 `SandCanvas` 水沙分支、面板扩展和 Want 参数解析，保留模型与单测待上游修复后复用。
- **完整回滚**：按文件清单 revert 本轮新增/修改，不回退其他 agent 已提交改动。
- 若守恒失败，优先回滚最近一次动力学改动，不得通过放大容差或改测试阈值闭环。

---

## 8. 交付边界

- developer 按本方案实现，不得改写上游算法公式；遇到指标不达标，反馈 designer。
- tester 使用 `TC-WS-01..11` 与 `test/water_sand_check.py` 输出客观数据。
- effector 使用 `EC-WS-01..05` 与主观评分标准输出观感结论。
- manager 需先处理 §0.1 的渗透守恒矛盾；该项解决前，P-003 不应因“代码完成”关闭。

## 变更记录

- 2026-10-13：初始 v1。原因：manager 选定 P-003 + P-005，用户要求每个问题独立方案文档；本文件给出 WaterSandField 模块、渲染、参数、测试钩子、实施步骤与回滚设计，并标记上游渗透公式守恒矛盾。
