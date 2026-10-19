# 工程落地方案：P-003 + P-005（场景二水中双沙与测试体系）

> 方案版本：v1　日期：2026-10-13　状态：DRAFT  
> 维护人：architect  
> 上游依据：`docs/algorithm-P003-P005.md` v1  
> 关联说明：P-003 与 P-005 共享同一状态结构、指标口径和测试钩子，采用联合工程方案；P-003 负责实现能力，P-005 负责把能力转成可执行客观/主观验证。  
> 约束：本方案只做工程设计，不修改上游算法文档和其他角色文档。
> 当前生效：v2.2（2026-10-14），见 §11；快照传输补充见 §12；历史 v1/v2/v2.1 内容保留，不作为当前实现基线。

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
| `maxTiltOmegaDegPerSec` | `maxOmega` | 720 | `> 0` |
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
| `wspreset` | string | `default` / `sorting` / `thick` / `settling` / `angle` / `nowater` / `heavyonly` / `lightonly` / `nearfull` |
| `wscols` | number | 96/128/160，覆盖预设 |
| `wscapacity` | number | 正整数，测试专用 |
| `wsmixband` | number | 1~2 |
| `wssnapshot` | boolean | true 时在 init/reset 后输出一条完整快照 |

`wspreset=thick` 仅用于演示与主观效果：`capacity=192`、重沙 `72`、轻沙 `72`（总沙 `144`，75% 容量），水层 `38`（20% 容量，底部 `144`、顶部 `182`），保留 `10` 空气（约 5% 容量）。该预设不替换 `sorting` 的既有测试口径。

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
- 2026-10-13：追加 v2 适配。原因：designer 已在 algorithm v2 §15 引入 `bh/bl/mh/ml` 有序层 + 混合表层账本与守恒渗透公式，解除 v1 §0.1 分相守恒阻塞项；本节同步状态结构、公共 API、渲染、测试钩子、实施步骤、风险与 fallback 决策，v1 内容保留为历史方案。
- 2026-10-14：追加 v2.1 适配。原因：designer 基于外部评审在 algorithm v2.1 §16 联立重调默认参数，新增整帧沙顶运动预算、目标驱动摇晃能量 ramp，并澄清事件式倒立与 H06 语义；本节只做工程落地适配，不改变 v2 分相守恒账本、`q/r` 公式、`schemaVersion=2` 与全部 TC/EC 阈值。
- 2026-10-14：追加 v2.2 适配。原因：designer 复核确认 v2.1 的每帧定值预算在 30/60/120Hz 下物理不一致，已将 `maxSandTopFrameDelta=0.48` 标记 REJECTED 并改为 `vSandTopMax=28 列/秒` 的子步速率预算；本节同步参数接口、预算调度、钩子字段、实施步骤与频率一致性测试位置。
- 2026-10-14：追加 WS_SNAPSHOT 传输方案。原因：真机 hilog 将超长 `WS_SNAPSHOT` JSON 截断在约 4KB，导致 E2E 判定脚本输出 `BLOCKED(missing_hook)`；本节选定文件落盘 + 小事件索引方案，保持 payload `schemaVersion=2` 不变，并给出 tester 拉取与校验要求。
- 2026-10-14：追加 dormant 帧语义与 rate 判定补充。原因：reviewer 发现 dormant 帧使 `WS_FRAME.substeps=0/substepDt=0`，而 `water_sand_check.py` 将其按 active 帧校验导致 rate 误报；本节明确 dormant 帧仍输出 `WS_FRAME`、零子步计划合法、脚本只对 active 帧计算 rate，并补充四个可实施测试口径。

---

## 9. 工程方案 v2 适配（2026-10-13）

> 上游依据：`docs/algorithm-P003-P005.md` v2 §15。
> 适用范围：仅 `MODE_WATERSAND` 与 `WaterSandField`；`SandGrid`、`FluidField` 与既有模式 0~3 不改。
> 结论：v1 §0.1 阻塞项已由 algorithm v2 解决，developer 可以按本节领取实现任务；禁止实现或回退 v1 的 `hl -= q; hs += q` 公式。

### 9.1 阻塞项解除确认

v1 §0.1 的缺陷是把“位置交换”误实现为“相位转换”：

```text
ΔVH = +q
ΔVL = -q
```

v2 将每列沉积沙拆为有序层与混合表层，渗透由穿越交换 `q` 与成对分选 `r` 组成：

```text
mh -= q + r
bh += q + r
bl += r - q
ml += q - r
```

逐列分相增量为：

```text
ΔVH = -(q + r) + (q + r) = 0
ΔVL = (q - r) + (r - q) = 0
```

沉降、滑移、扬起、倒立和水面迁移同样只做同相等量减增；任意操作序列是零增量操作的有限复合，因此满足 algorithm v2 §15.5 的守恒证明。工程上不再需要把 `percolationRate` 置 0，也不再输出 `percolation_disabled_pending_designer`。

### 9.2 状态账本与外部兼容

#### 9.2.1 canonical 状态

`WaterSandField` 内部持久化 7 个逐列状态：

| 状态 | 存储 | 说明 |
|---|---|---|
| `waterTop[c]` | `Float64Array` | 自当前重力底部向上的自由表面高度 |
| `bh[c]` | `Float64Array` | 底部有序重沙层厚度 |
| `bl[c]` | `Float64Array` | 位于 `bh` 之上的有序轻沙层厚度 |
| `mh[c]` | `Float64Array` | 顶部混合表层中的重沙厚度 |
| `ml[c]` | `Float64Array` | 顶部混合表层中的轻沙厚度 |
| `suspH[c]` | `Float64Array` | 悬浮重沙等效高度 |
| `suspL[c]` | `Float64Array` | 悬浮轻沙等效高度 |

列内排序固定为：

```text
底部 bh → 中部 bl → 顶部 mh + ml → 悬浮水体
```

`hs/hl/sandTop` 不作为内部第二账本持久化，避免双账本漂移：

```text
hs[c] = bh[c] + mh[c]
hl[c] = bl[c] + ml[c]
sandTop[c] = bh[c] + bl[c] + mh[c] + ml[c]
mixStart[c] = bh[c] + bl[c]
```

`getTotals()` 与守恒审计使用：

```text
Vw  = Σ max(0, waterTop[c] - sandTop[c] - suspH[c] - suspL[c])
VH  = Σ (bh[c] + mh[c] + suspH[c])
VL  = Σ (bl[c] + ml[c] + suspL[c])
```

#### 9.2.2 状态写入语义

- `initialize(scene)` / `reset(scene?)`：`heavyDepositedRegions` 写入 `bh`，`lightDepositedRegions` 写入 `bl`，悬浮区域写入 `suspH/suspL`；这是场景声明的初始有序层。
- `inject(spec)` 且 `state=DEPOSITED`：新增沙落在当前沙堆顶部，写入 `mh/ml`，后续由渗透分选进入有序层。
- `inject(spec)` 且 `state=SUSPENDED`：写入 `suspH/suspL`。
- `settleSandV2(dt)`：`suspH→mh`、`suspL→ml`，不直接写 `bh/bl`。
- `percolateHeavyConservative(dt)`：只按 v2 §15.3 更新 `bh/bl/mh/ml`。
- `relaxAngle(dt)` / `liftByShake(dt)`：统一调用私有 `splitTopV2(c, q)`，发送列与接收列均保持同相总量。
- `invert180()`：按算法将 `bh+mh+suspH` 全部转为 `suspH`，`bl+ml+suspL` 全部转为 `suspL`，再重解水面；禁止只翻转渲染矩阵却改写相总量。

私有表层拆分接口：

```ts
private splitTopV2(column: number, amount: number): WaterSandSplitResult;

export interface WaterSandSplitResult {
  heavy: number;
  light: number;
}
```

取料顺序为混合层等比例优先，其次 `bl`，最后 `bh`；接收列一律把结果加入自身 `mh/ml`。该接口不对外公开，避免调用方绕过守恒约束。

### 9.3 公共 API 变化

既有类方法签名保持不变，只做 additive 扩展；以下定义覆盖 v1 §2.3 中对应字段的旧语义：

```ts
export interface WaterSandSnapshot {
  schemaVersion: 2;
  cols: number;
  capacity: number;
  waterTop: Float64Array;
  bh: Float64Array;
  bl: Float64Array;
  mh: Float64Array;
  ml: Float64Array;
  suspH: Float64Array;
  suspL: Float64Array;
  hs: Float64Array;
  hl: Float64Array;
  sandTop: Float64Array;
}

export interface WaterSandSeparationProfile {
  lowerHeavyShare: number | null;
  upperHeavyShare: number | null;
  separationRatio: number | null;
}

export interface WaterSandMetrics {
  ledgerVersion: 2;
  driftWater: number;
  driftHeavy: number;
  driftLight: number;
  depositedHeavyFraction: number;
  depositedLightFraction: number;
  suspensionFraction: number;
  lowerHeavyShare: number | null;
  upperHeavyShare: number | null;
  separationRatio: number | null;
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
```

接口语义：

- `snapshot()`：分配完整 `schemaVersion=2` 快照；`hs/hl/sandTop` 由 canonical 四账本重算。
- `copyStateTo(target)`：写入全部 12 个数组；`target.schemaVersion !== 2` 或任一数组长度不等于 `cols` 时抛出 `WaterSandSnapshotError`，不做部分写入。
- `getMetrics(pixelScale?)`：`lowerHeavyShare/upperHeavyShare/separationRatio` 在单相或统计区为空时返回 `null`，日志与报告层呈现为 `N/A`。
- `getTotals()` / `getDrift()`：仍返回既有 `WaterSandTotals/WaterSandDrift`，数值来源改为 v2 canonical 账本。
- `configure(params)`：继续要求 `percolationRate > 0`；默认值 `6/s` 必须真实生效。
- `step(dt)`：仍夹到 `1/30s` 并最多拆 2 个不超过 `1/60s` 的子步；每个子步按“水面 → 悬浮扩散 → 沉降 → 渗透分选 → 扬起/安息角 → 审计”的固定顺序执行。

### 9.4 分选指标实现

新增私有指标接口：

```ts
private computeSeparationProfile(): WaterSandSeparationProfile;
private mixedOverlap(start: number, end: number, mixStart: number, sandTop: number): number;
```

计算步骤：

1. 对每列取 `lowerBound = sandTop/3`、`upperBound = 2*sandTop/3`、`mixStart = bh+bl`、`mixTotal = mh+ml`。
2. 有序层按几何高度直接归属；混合层与统计区间的重叠高度按 `mh/mixTotal`、`ml/mixTotal` 比例归属。
3. 汇总得到 `H_low/L_low/H_high/L_high`，再计算 `lowerHeavyShare`、`upperHeavyShare` 与 `separationRatio`。
4. 阈值保持 `separationRatio <= 0.5`；不得因 v2 状态扩展调整阈值。
5. 单相场景返回 `null`，判定脚本输出 `N/A`，不得用 `0` 伪装通过。

`computeSeparationProfile()` 只读 canonical 账本，禁止从 `hs/hl` 聚合值反推上下区相序。

### 9.5 渲染适配

`renderWaterSand()` 改用分层账本绘制：

1. 水体：`waterTop` 到 `sandTop`。
2. 有序重沙：底部到 `smooth(bh)`。
3. 有序轻沙：`smooth(bh)` 到 `smooth(bh+bl)`。
4. 混合表层：`smooth(bh+bl)` 到 `smooth(sandTop)`，颜色按 `mh/(mh+ml)` 在重沙色与轻沙色之间插值。
5. 界面视觉混合带宽度仍由 `mixBandPx` 控制，只影响渲染，不参与守恒与分选统计。

外部兼容的 `hs/hl` 只用于测试脚本和调试输出，渲染层不得把它们当作层界面；重/轻界面必须使用 `bh` 与 `bh+bl`。

### 9.6 测试钩子落地

#### `WS_EVENT`

`init/reset` 事件追加稳定字段：

```text
WS_EVENT init {preset:sorting,cols:128,capacity:192,ledgerVersion:2,percolationRate:6}
WS_EVENT reset {preset:sorting,ledgerVersion:2,percolationRate:6}
```

删除 v1 阻塞期的 `percolation_disabled_pending_designer` 正常路径；若该事件出现，tester 必须判实现缺陷，而不是可接受的降级。`model_error/dormant/wake/invert180` 事件保持 v1 语义。

#### `WS_METRIC`

每 250ms 输出一次，字段与 `WaterSandMetrics` 一一对应：

```text
WS_METRIC {ledgerVersion:2,driftWater:0.0,driftHeavy:0.0,driftLight:0.0,
           lowerHeavyShare:0.82,upperHeavyShare:0.21,separationRatio:0.26,...}
```

单相或统计区为空时输出 `null`，报告层显示 `N/A`。所有漂移仍按每相 `<0.1%` 硬门槛逐样本判定。

#### `WS_FRAME`

保持：

```text
WS_FRAME {frameTimeMs:8.2,physicsTimeMs:1.1}
```

仅在 `testmode=true` 时每帧输出；`fpsP95` 继续要求至少 100 个有效样本，60Hz 环境输出环境刷新率不足的 `N/A`。

#### `WS_SNAPSHOT`

`wssnapshot=true` 或脚本请求时输出 `schemaVersion=2` 快照，数组字段为：

```text
waterTop, bh, bl, mh, ml, suspH, suspL,
hs, hl, sandTop
```

其中 `hs/hl/sandTop` 必须由四账本重算。脚本缺失任一 v2 字段时输出 `BLOCKED(missing_hook)`，不得默认为 0。`test/water_sand_check.py` 必须用快照重算三相漂移与 `separationRatio`，并与最近 `WS_METRIC` 交叉校验；两者结论不一致时按 HG-06 判 FAIL。`--self-check-defect` 继续验证“重沙增加、轻沙减少”的合成缺陷会被判 FAIL。

### 9.7 实施步骤（developer 可直接领取）

#### Step V2-1：状态与 API 骨架

文件：`entry/src/main/ets/physics/WaterSandField.ets`

- 定义 `WaterSandSnapshot.schemaVersion=2`、`WaterSandSeparationProfile`、`WaterSandSplitResult`、`WaterSandSnapshotError`。
- 分配 `waterTop/bh/bl/mh/ml/suspH/suspL` 与派生 scratch；不再持久化 `hs/hl`。
- 实现 `initialize/reset/inject/snapshot/copyStateTo/getTotals/getDrift` 的 v2 语义。
- 初始化后记录 `Vw0/VH0/VL0`；所有转移先裁剪、再同时扣减与增加。

#### Step V2-2：守恒动力学

文件：`entry/src/main/ets/physics/WaterSandField.ets`

- 实现 `percolateHeavyConservative(dt)`，严格使用 algorithm §15.3 的 `q/r` 公式与容量约束。
- 实现 `settleSandV2(dt)`：悬浮沙先进入 `mh/ml`。
- 实现 `splitTopV2()`，并让 `relaxAngle/liftByShake` 统一调用。
- 实现 `invert180()` 的同相再悬浮与水面重解。
- 每个子步末尾执行非负、有限、容量与分相漂移审计；异常冻结状态并返回错误码。

#### Step V2-3：分相指标

文件：`entry/src/main/ets/physics/WaterSandField.ets`

- 实现 `computeSeparationProfile()` 与 `mixedOverlap()`。
- 更新 `WaterSandMetrics` 输出，单相返回 `null`。
- 保持 `separationRatio <= 0.5` 与三相漂移 `<0.1%` 阈值不变。

#### Step V2-4：渲染与模式

文件：`entry/src/main/ets/pages/SandCanvas.ets`

- 新增 `MODE_WATERSAND=4`、`MODE_COUNT=5` 与复用快照。
- `renderWaterSand()` 按 `bh/bh+bl/mix` 分层绘制，不复用 `hs` 作为界面。
- 帧回调中仅在水沙模式调用 `WaterSandField.step()`，切出时冻结、切回时恢复。

#### Step V2-5：参数面板与 Want

文件：`entry/src/main/ets/pages/SandCanvas.ets`、`entry/src/main/ets/entryability/EntryAbility.ets`

- 分选强度滑杆直接映射 `percolationRate`，范围 `0~12`，但最终配置仍要求 `>0`；默认 `6/s`。
- 解析 `wspreset/wscols/wscapacity/wsmixband/wssnapshot`，保持 `testmode` 的传感器隔离语义。
- `rendermode=4` 直入水沙模式；旧模式参数和调用路径不改。

#### Step V2-6：单测

文件：

- `entry/src/test/WaterSandFieldTest.ets`
- `entry/src/ohosTest/ets/test/WaterSandFieldTest.ets`
- `entry/src/test/List.test.ets`
- `entry/src/ohosTest/ets/test/List.test.ets`

新增断言：

- 穿越交换、成对分选、随机操作序列下 `driftHeavy/driftLight < 1e-12`。
- `hs == bh+mh`、`hl == bl+ml`、`sandTop` 一致。
- 滑移、扬起、倒立、倒立后摇晃均保持分相守恒。
- `computeSeparationProfile()` 与独立参考实现一致；单相返回 `null`。
- `percolationRate=6/s` 真实产生非零 `q/r`，且不输出禁用事件。

#### Step V2-7：脚本与验证交接

文件：

- `test/water_sand_check.py`
- `test/water_sand_pixel_check.py`
- `test/record_tc_ws.py`

- 解析 `schemaVersion=2` 快照并重算分相守恒与分选指标。
- 保持 `--self-check-defect` 回归。
- developer 交付单测与本地构建结果；tester 执行 `TC-WS-01..12`；effector 执行 `EC-WS-01..08`。若客观失败，反馈 designer，不得放宽阈值或临时改公式闭环。

### 9.8 性能、风险与 fallback

#### 性能与内存

- 状态数组从 5 个增加到 7 个，另有 `sandTop/hs/hl` 等派生 scratch；`N=128` 时新增 4 个 `Float64Array` 约 `4096B`，完整运行态仍低于 `20KB`。
- 渗透、沉降、容量检查均为 `O(N)`，预计单步增幅 `<0.1ms`，目标仍是 `physicsTimeMs P95 <2ms`。
- 物理与渲染继续在 UI 线程 frame callback 内同步执行；若 P95 超标，先降 `N=96` 与每列 4 个伪粒子，再评估 Worker，不在 v2 首版引入异步通信。

#### 主要风险

| 风险 | 处理 |
|---|---|
| `hs/hl` 与四账本漂移 | 四账本为唯一 canonical 状态，派生值每次导出时重算；禁止渲染或脚本回写 |
| `q/r` 容量裁剪后破坏守恒 | 裁剪必须先于扣减，减增成对执行，并在单测用 `1e-12` 阈值审计 |
| 混合层渲染与指标口径不一致 | 渲染只读同一快照；指标使用 algorithm §15.6 几何积分，不使用像素占比替代 |
| v2 视觉混合带不足 | 先调 `mixBandPx` 与渲染插值，不得改物理账本或阈值 |
| 数值错误 | 冻结状态、输出 `model_error`，保留最后一帧，禁止自动修正 |

#### C 方案 fallback 条件

algorithm v2 选定方案 A，C（每列 K 层账本）仅作为后续 fallback，不在本轮实现。切换必须同时满足：

1. 方案 A 通过全部客观硬门槛，但 `EC-WS-04` 混合带/分区观感低于 6 分，且多设备复测确认不是渲染或参数问题。
2. designer 发布 v3，给出 K 层方案的逐相守恒证明、`O(NK)` 性能与快照扩展设计。
3. architect 补充 `schemaVersion=3`、内存预算与脚本兼容方案，并追加变更记录。

若方案 A 出现分相守恒失败，优先判定实现或算法缺陷并回滚最近动力学改动，不得直接切换 C 规避问题。

#### 回滚

- **功能回滚**：关闭 `MODE_WATERSAND` 入口，保留模型与单测；不得恢复 v1 缺陷公式。
- **性能回滚**：先降 `N` 与伪粒子数量，再考虑渲染降档；物理公式与守恒阈值不变。
- **完整回滚**：按 v2 文件清单 revert 本轮改动，不回退其他 agent 的并行修改。

---

## 10. 工程方案 v2.1 适配（2026-10-14）

> 2026-10-14 导航：本节预算接口已被 §11 v2.2 取代；本节仅保留为历史设计记录。

> 上游依据：`docs/algorithm-P003-P005.md` v2.1 §16。
> 兼容基线：保留 v2 §9 的 `bh/bl/mh/ml` canonical 账本、`hs/hl/sandTop` 派生映射、`q/r` 守恒渗透公式、`schemaVersion=2` 与全部 TC/EC 阈值。
> 工程结论：v2.1 是参数与动力学调度修订，不是账本 schema 升版；developer 需返工 `step()` 调度与三类沙顶运动算子，其余步骤做增量修改。

### 10.1 参数 API 与默认值

在 `WaterSandParams` 中新增两个字段：

```ts
export interface WaterSandParams {
  angleDeg: number;
  heavySettleSpeed: number;
  lightSettleSpeed: number;
  percolationRate: number;
  waterLevelRate: number;
  heavyDiffusion: number;
  lightDiffusion: number;
  shakeDecay: number;
  shakeRampRate: number;
  heavyLiftRate: number;
  lightLiftRate: number;
  maxSandTopFrameDelta: number;
  maxAvalancheFlux: number;
  avalanchePasses: number;
  maxTiltOmegaDegPerSec: number;
}
```

`defaultWaterSandParams()` 必须返回：

| 工程字段 | 算法字段 | v2.1 默认 | 校验 |
|---|---:|---:|---|
| `heavySettleSpeed` | `vH` | `64` | `> lightSettleSpeed > 0` |
| `lightSettleSpeed` | `vL` | `38` | `> 0` |
| `heavyLiftRate` | `liftHRate` | `36/s` | `>= 0` 且 `<= lightLiftRate` |
| `lightLiftRate` | `liftLRate` | `40/s` | `>= 0` |
| `shakeRampRate` | - | `8/s` | `> 0` |
| `maxSandTopFrameDelta` | - | `0.48` | `> 0` 且 `<= 0.5` |
| `shakeDecay` | - | `2.2/s` | `> 0` |
| `percolationRate` | - | `6/s` | `> 0`，v2 公式不变 |
| `maxAvalancheFlux` | - | `24` | `> 0`，作为自然通量上限并受整帧预算约束 |

参数面板建议范围：

| 面板项 | 范围 | 说明 |
|---|---:|---|
| 重沙沉降 | `16~96`，step `2` | 需保持 `vH > vL` |
| 轻沙沉降 | `8~64`，step `2` | 需保持 `vH > vL` |
| 重沙扬起 | `0~48/s`，step `1` | 需保持 `liftHRate <= liftLRate` |
| 轻沙扬起 | `0~56/s`，step `1` | 需保持 `liftHRate <= liftLRate` |
| 摇晃爬升 | `1~16/s`，step `1` | 目标驱动能量 ramp |

`maxSandTopFrameDelta` 不放入普通面板，避免用户绕过 HG-04 安全余量；可用测试 Want 参数 `wsmaxtopdelta` 覆盖，范围仍为 `(0, 0.5]`，默认 `0.48`。`configure()` 对非法参数继续整体拒绝，不部分应用。

设计核算结果必须写入测试期望，但不得替代 E2E 实测：

| 指标 | v2.1 预测 | 阈值 |
|---|---:|---|
| TC-WS-08 峰值悬浮 | 约 `52.8%` | `>= 50%` |
| TC-WS-08 静置 8s 残余 | 约 `0.54%` | `<= 5%` |
| 倒立 4s 重沙沉积 | 约 `94.5%` | 支撑 `phaseOrderScore >= 0.30` |
| 倒立 4s 轻沙沉积 | 约 `82.2%` | 支撑相序差 |
| `t90H/t90L` | 约 `0.636~0.643` | `<= 0.7` |
| 单帧沙顶位移 | `<= 0.48` 列 | HG-04 `<= 0.5` 列 |

### 10.2 整帧沙顶运动预算接口

#### 10.2.1 调度契约

`step(frameDt)` 的一次调用对应一个渲染帧。`SandCanvas` 每帧最多调用一次 `step()`；测试也必须按“一帧一次”构造数据。预算在 `step()` 入口创建并覆盖其全部子步：

```text
substeps = clippedDt <= 1/60 ? 1 : 2
substepDt = clippedDt / substeps
for c: frameMotionBudgetRemaining[c] = maxSandTopFrameDelta
```

禁止在每个子步重置预算，禁止沉降、滑移、扬起各自持有独立 `0.48` 预算。

#### 10.2.2 私有实现接口

```ts
private frameMotionBudgetRemaining: Float64Array;
private frameMotionBudgetUsed: Float64Array;
private frameBudgetClipped: boolean;

private beginFrameMotionBudget(): void;
private clipFrameMotion(source: number, target: number, requested: number): number;
private markFrameBudgetClipped(): void;
```

语义：

- `source` 是发生沙顶变化的列；`target=-1` 表示单列变化。
- 沉降与扬起只影响本列，`target=-1`。
- 安息角滑移同时影响 donor 与 receiver；允许量为 `min(requested, remaining[donor], remaining[receiver])`，并在两侧各扣减同一允许量，确保任一列整帧累计变化不超过 `0.48`。
- `frameMotionBudgetUsed[c]` 记录本帧累计消耗；`frameBudgetClipped` 在任一理论通量被预算缩小时置 `true`。
- 预算是每帧 scratch，不属于守恒状态；子步失败回滚物理状态后，下一次 `step()` 重新初始化预算。

#### 10.2.3 算子接入

**沉降 `settleSandV2(substepDt)`**：

```text
desiredH = min(suspH, suspH * vH * substepDt / depth)
desiredL = min(suspL, suspL * vL * substepDt / depth)
allowed = clipFrameMotion(c, -1, desiredH + desiredL)
scale = allowed / max(desiredH + desiredL, ε)
depH = desiredH * scale
depL = desiredL * scale
```

重/轻沉降必须先合成 `desiredH + desiredL` 再进入预算，不得分别各消耗 `0.48`。浅水 `depth=ε` 时最多沉积 `0.48` 列。

**扬起 `liftByShake(substepDt)`**：

```text
desiredH = E * band * liftHRate * shareH * substepDt
desiredL = E * band * liftLRate * shareL * substepDt
allowed = min(desiredH + desiredL, remaining[c], sandTop[c])
scale = allowed / max(desiredH + desiredL, ε)
```

随后仍通过 `splitTopV2()` 取沙并转入 `suspH/suspL`。预算只限制总量，不改变 `splitTopV2()` 的相位分配。

**安息角 `relaxAngle(substepDt)`**：

```text
allowed = clipFrameMotion(highColumn, lowColumn, desiredTransfer)
```

若预算不足，本帧只转移 `allowed`，剩余坡度在后续帧继续松弛；不得为通过 HG-04 修改坡度阈值。

**不需要预算的算子**：

- `percolateHeavyConservative()`：只重排 `bh/bl/mh/ml`，`sandTop` 不变。
- 水面 leveling 与悬浮扩散：不改变 `sandTop`。
- `inject()` / `invert180()`：属于事件边界，不消耗普通帧预算，但必须触发统计参考重置。

### 10.3 摇晃能量 ramp

`shakeInput` 改为持续目标值，而不是一次性注入量：

```ts
setShakeInput(value: number): void;
```

- `setShakeInput(value)` 只保存 `clamp(value, 0, 1)`，不直接修改能量。
- `step()` 在每个子步按 `substepDt` 更新能量；目标值跨子步保持不变。
- 只有调用 `setShakeInput(0)` 才进入衰减分支；`step()` 不得自动清零目标。
- UI/测试脚本在摇晃段结束后必须显式调用 `setShakeInput(0)`。

每个子步执行：

```text
if shakeInput > 0:
    target = clamp(shakeInput, 0, 1)
    E = clamp(E + clamp(target - E, -shakeRampRate*substepDt,
                                      shakeRampRate*substepDt), 0, 1)
else:
    E = E * exp(-shakeDecay * substepDt)
```

`invert180()` 保留既有 `E = max(E, 0.8)` 语义，并立即重置无瞬移统计参考。

### 10.4 事件式倒立与 H06 工程语义

生产实现采用事件式重力坐标系切换：

1. `tiltDeg ∈ [-89°, 89°]`：连续求解，不做数据镜像。
2. 输入角跨越 `±90°`：调用一次 `invert180()`，输出事件并执行三相守恒审计。
3. 渲染层只做 `rotate(π)` 显示变换，不得读取或改写物理数组。
4. 从倒立区间回到 `±89°` 内：再次触发 `invert180()` 切回连续区间。

H06 的工程判定：

- FAIL：渲染分支、UI 分支或脚本绕过 `WaterSandField` API 直接改写 `bh/bl/mh/ml/suspH/suspL/waterTop`。
- PASS：通过 `invert180()` API 执行坐标切换，事件日志存在，且事件前后三相守恒审计通过。
- 事件本身不计入普通帧 `maxSandTopDelta`，但仍必须通过守恒、事件日志和录像连续性判定。

若 manager 后续将 G3 解释为“禁止坐标系事件的单一连续求解器”，当前 2D 列标量方案不满足，需要另立 v3 目标；developer 不得在 v2.1 内私自实现连续主轴切换。

### 10.5 测试钩子扩展

#### `WS_EVENT`

`init/reset` 追加参数版本：

```text
WS_EVENT init {preset:default,cols:128,capacity:192,ledgerVersion:2,
               paramsVersion:"v2.1",vH:64,vL:38,liftHRate:36,liftLRate:40,
               shakeRampRate:8,maxSandTopFrameDelta:0.48}
```

事件边界统一追加字段：

```text
WS_EVENT reset   {preset:default,boundary:true,...}
WS_EVENT inject  {phase:heavy,state:deposited,amount:8,boundary:true}
WS_EVENT invert180 {count:1,reason:"cross90",boundary:true,
                    driftWater:0,driftHeavy:0,driftLight:0}
```

`boundary=true` 表示脚本必须重置 HG-04 的上一帧 `sandTop` 参考；不得把 init/reset/inject/invert 引起的合法状态变化计入普通帧瞬移。

#### `WS_METRIC`

在既有字段上 additive 追加：

```text
shakeEnergy: number
maxSandTopFrameDeltaObserved: number
frameBudgetUsedMax: number
frameBudgetClipped: boolean
```

`maxSandTopFrameDeltaObserved` 是普通物理帧内观测到的逐列最大变化，不包含事件边界；预期 `<=0.48`，脚本仍按 HG-04 `<=0.5` 判定。配置项仍命名为 `maxSandTopFrameDelta`，二者不得混用。

#### `WS_FRAME`

```text
WS_FRAME {frameTimeMs:8.2,physicsTimeMs:1.1,
          maxSandTopFrameDeltaObserved:0.31,frameBudgetUsedMax:0.31,
          frameBudgetClipped:false}
```

该输出仅在 `testmode=true` 时启用，用于定位预算裁剪与帧率关系。

#### `WS_SNAPSHOT`

`schemaVersion` 保持 `2`，不新增必填字段。脚本按日志顺序处理：

1. 遇到 `boundary=true` 事件后丢弃上一帧 `sandTop` 参考。
2. 对相邻普通帧快照重算 `max(abs(sandTop[end][c] - sandTop[start][c]))`。
3. 普通帧结果必须 `<=0.5`；模型内部预算目标为 `<=0.48`。
4. 事件帧改用守恒、事件日志与录像连续性判定，不做跨边界位移判定。

`test/water_sand_check.py` 需要识别上述 additive 字段；旧日志缺失 `boundary` 时不得默认通过，应输出 `BLOCKED(missing_hook)`。

### 10.6 对 Step V2-1..V2-7 的影响

| 步骤 | 影响级别 | 修改内容 |
|---|---|---|
| Step V2-1 | 增量 | 扩展 `WaterSandParams` 与默认值；新增预算 scratch、帧参考快照和参数校验 |
| Step V2-2 | **返工** | `step()` 建立整帧预算；沉降/滑移/扬起共享预算；改为目标驱动能量 ramp；重置事件边界统计 |
| Step V2-3 | 增量 | `WS_METRIC` 追加能量与预算字段；`maxSandTopDeltaPerSecond` 按整帧 `frameDt` 计算，而不是子步 `dt` |
| Step V2-4 | 增量 | 渲染逻辑不变；`SandCanvas` 保证一帧一次 `step()`，只更新事件日志与帧统计输出 |
| Step V2-5 | 增量 | 面板默认值/范围更新；新增 `wsmaxtopdelta` Want；摇晃输入目标持续到显式置 0 |
| Step V2-6 | 增量 | 增加浅水限幅、两相合计限幅、能量 ramp、事件边界、倒立 4s 与 TC-WS-08 联立测试 |
| Step V2-7 | 增量 | 脚本解析 `paramsVersion/boundary` 与预算字段；事件边界重置参考；保留全部原阈值 |

单测补充：

- `depth=ε` 时，一帧 `sandTop` 增量 `<=0.48`。
- 重/轻同时沉降时，合计增量 `<=0.48`。
- `shakeInput=1` 持续约 `0.125s` 后 `shakeEnergy≈1`。
- `shakeInput=0` 后按 `2.2/s` 衰减。
- 强摇晃 3s 峰值悬浮 `>=0.5`，静置 8s 残余 `<=0.05`。
- 倒立 4s 后 `phaseOrderScore>=0.30`，Node 参考漂移 `<1e-12`。
- 事件边界不参与普通帧位移，但守恒审计必须通过。

### 10.7 与 developer 并行修复的合并顺序

当前 developer（Kepler）正在修复 reviewer 退回的审计、脚本、单测与原子性问题。建议顺序：

1. **先落 Kepler 修复**：审计口径、脚本解析、单测基线和子步原子回滚是 v2.1 的安全基座；先合入并跑通 v2 基线，避免在同一补丁里同时改公式与修复流程。
2. **再独立合入 v2.1**：以参数/API → 预算与能量 → 钩子/UI → 单测/脚本的顺序拆小补丁，方便失败时定位。
3. **预算纳入原子性约定**：预算 scratch 每帧重置，不作为守恒状态；若 Kepler 的原子修复要求所有 scratch 参与回滚，则将 `beginFrameMotionBudget()` 放在子步循环外，失败后丢弃本帧预算并在下一帧重建。
4. **脚本先兼容旧字段再追加新字段**：先保证 v2 审计与 HG-06 交叉校验稳定，再让 `boundary=true`、`paramsVersion` 与预算字段成为必需钩子。
5. **单测基线先固定再重跑 v2.1 预测值**：避免用 v2.1 新期望掩盖 reviewer 修复前的既有缺陷。
6. **禁止混用回滚策略**：若 v2.1 参数组失败，可回退默认参数，但不得移除整帧预算；预算是 HG-04 安全约束，不是可调观感参数。

### 10.8 性能、风险与回滚

- 新增 2 个 `Float64Array`（`N=128` 时约 `2048B`）和每列常数次预算计算；预计物理步增幅 `<0.05ms`，仍满足 `physicsTimeMs P95 <2ms`。
- 预算会延缓浅水快速沉降与超陡滑移，这是预期行为；不得为了视觉瞬时响应提高 `maxSandTopFrameDelta` 超过 `0.5`。
- `shakeInput` 持续目标若忘记清零，会造成持续扬起；UI 与测试脚本必须在段末调用 `setShakeInput(0)`。
- v2.1 参数组失败时，先回退 `vH/vL/lift*` 默认值并保留预算与 ramp；仍失败才反馈 designer 重新联立核算。
- C fallback 条件与 v2 §9.8 相同；v2.1 的主观混合带不足不能作为直接切换 C 的理由。

---

## 11. 工程方案 v2.2 适配（2026-10-14）

> 上游依据：`docs/algorithm-P003-P005.md` v2.2 §16。
> 取代关系：本节取代 v2.1 §10 中“每渲染帧定值预算 `maxSandTopFrameDelta=0.48`”的接口与钩子；v2.1 的参数组、目标驱动能量 ramp、事件式倒立语义继续有效。
> 兼容基线：`bh/bl/mh/ml` 账本、`q/r` 公式、`schemaVersion=2`、分选指标与全部 TC/EC 阈值不变。

### 11.1 v2.1 预算缺陷与 v2.2 决策

v2.1 的定值预算：

```text
frameBudget[c] = 0.48
```

在不同刷新率下等效速率不同：

| 刷新率 | 每帧预算 | 等效速率 |
|---:|---:|---:|
| 120Hz | `0.48` | `57.6 列/s` |
| 60Hz | `0.48` | `28.8 列/s` |
| 30Hz | `0.48` | `14.4 列/s` |

因此该表达式已由 algorithm v2.2 标记 **REJECTED**。工程实现不得保留 `maxSandTopFrameDelta` 作为生效参数、面板项或测试覆盖项；若 developer 分支已实现该字段，必须在 v2.2 补丁中移除，而不是兼容保留。

v2.2 改用速率预算：

```text
vSandTopMax = 28 列/秒
substepBudget[c] = vSandTopMax * substepDt
```

该形式保证同一模拟时长下的物理结果不依赖 30/60/120Hz 渲染刷新率。

### 11.2 参数接口

`WaterSandParams` 使用以下字段：

```ts
export interface WaterSandParams {
  angleDeg: number;
  heavySettleSpeed: number;
  lightSettleSpeed: number;
  percolationRate: number;
  waterLevelRate: number;
  heavyDiffusion: number;
  lightDiffusion: number;
  shakeDecay: number;
  shakeRampRate: number;
  heavyLiftRate: number;
  lightLiftRate: number;
  vSandTopMax: number;
  maxAvalancheFlux: number;
  avalanchePasses: number;
  maxTiltOmegaDegPerSec: number;
}
```

字段语义：

| 工程字段 | 算法字段 | 类型 | 默认值 | 校验 |
|---|---|---|---:|---|
| `heavySettleSpeed` | `vH` | `number` | `64` | `> lightSettleSpeed > 0` |
| `lightSettleSpeed` | `vL` | `number` | `38` | `> 0` |
| `heavyLiftRate` | `liftHRate` | `number` | `36/s` | `>= 0` 且 `<= lightLiftRate` |
| `lightLiftRate` | `liftLRate` | `number` | `40/s` | `>= 0` |
| `shakeRampRate` | - | `number` | `8/s` | `> 0` |
| `vSandTopMax` | - | `number` | `28 列/s` | `> 0` 且 `<= 30` |

`vH/vL = 64/38 ≈ 1.68`，用于 `t90H/t90L <= 0.7` 的联立核算。`maxSandTopFrameDelta` 不再出现在公共参数接口中。

Want 测试参数使用：

```text
wssandtopmax=<number>
```

范围 `(0, 30]`，默认 `28`。v2.1 的 `wsmaxtopdelta` 不得继续解析为生效配置；若为兼容旧脚本保留别名，必须仅输出弃用错误，不得静默映射到 `vSandTopMax`。

### 11.3 子步预算实现接口

私有 scratch 与接口：

```ts
private substepMotionBudgetRemaining: Float64Array;
private substepMotionBudgetUsed: Float64Array;
private substepBudgetClipped: boolean;

private beginSubstepMotionBudget(substepDt: number): void;
private clipSubstepMotion(source: number, target: number, requested: number): number;
private markSubstepBudgetClipped(): void;
```

`step(frameDt)` 调度：

```text
clippedDt = min(frameDt, 1/30)
substeps = clippedDt <= 1/60 ? 1 : 2
substepDt = clippedDt / substeps

for each substep:
    beginSubstepMotionBudget(substepDt)
    execute waterLevel / diffusion / settle / percolate / relax / lift
```

`beginSubstepMotionBudget(substepDt)` 对每列执行：

```text
substepMotionBudgetRemaining[c] = vSandTopMax * substepDt
substepMotionBudgetUsed[c] = 0
```

预算在每个子步开始时重置，不得跨子步累计，也不得按渲染帧初始化。

`clipSubstepMotion(source, target, requested)` 语义：

- `target=-1`：单列变化，允许量为 `min(requested, remaining[source])`。
- 安息角滑移：允许量为 `min(requested, remaining[donor], remaining[receiver])`，并在 donor/receiver 两侧各扣减同一允许量。
- 任一理论通量被缩小时调用 `markSubstepBudgetClipped()`。
- 预算是子步 scratch，不参与守恒状态备份；子步失败回滚物理状态后丢弃本子步预算，下一子步重新初始化。

各算子接入：

| 算子 | 预算处理 |
|---|---|
| 沉降 | `desiredH + desiredL` 合计后进入预算，按 `scale` 同步缩放两相 |
| 扬起 | 理论通量、可取沉积沙、子步剩余预算三者取最小，再经 `splitTopV2()` 保相转移 |
| 安息角滑移 | donor/receiver 双列共享预算，预算不足时剩余坡度下一子步继续松弛 |
| 渗透 | 不消耗预算，`sandTop` 不变 |
| 水面/悬浮扩散 | 不消耗预算，`sandTop` 不变 |
| 事件 | 不消耗普通预算，触发统计参考重置 |

非绑定条件：

```text
desiredSettle + desiredLift + desiredAvalanche
    <= vSandTopMax * substepDt
```

满足该条件时预算不改变自然通量。TC-WS-08 强摇晃期预算会绑定；工程不得为了达到峰值悬浮目标私自绕过预算。

### 11.4 v2.2 联立核算期望

这些数值写入单测与回归参考，不替代 E2E 实测：

| 指标 | v2.2 预测 | 判定 |
|---|---:|---|
| TC-WS-08 峰值悬浮 | 约 `63.6%` | 目标 `55%~75%`，验收 `>=50%` |
| TC-WS-08 静置 8s 残余 | 约 `0.70%` | `<=5%` |
| TC-WS-07 4s 重沙沉积 | 约 `95.0%` | 支撑 `phaseOrderScore>=0.30` |
| TC-WS-07 4s 轻沙沉积 | 约 `83.1%` | 支撑相序差 |
| TC-WS-03 `t90H/t90L` | 约 `0.633~0.635` | `<=0.7` |
| 沙顶运动限速 | `28 列/s` | HG-04 速率上限 `30 列/s` |

实测峰值低于 `0.55` 或高于 `0.75` 时，即使仍满足 `>=0.50` 验收线，也应反馈 designer 复核参数；不得由 developer 私自调节预算或阈值。

### 11.5 测试钩子更新

#### `WS_EVENT`

`init/reset` 输出：

```text
WS_EVENT init {ledgerVersion:2,paramsVersion:"v2.2",
               vH:64,vL:38,liftHRate:36,liftLRate:40,
               shakeRampRate:8,vSandTopMax:28}
```

不得输出 `maxSandTopFrameDelta`。事件边界仍使用 `boundary:true`，`invert180` 仍输出 `reason:"cross90"` 与三相漂移审计。

#### `WS_METRIC`

v2.1 的 `maxSandTopFrameDeltaObserved/frameBudgetUsedMax/frameBudgetClipped` 改为：

```text
maxSandTopSubstepDeltaObserved: number
maxSandTopSpeedObserved: number
substepBudgetUsedMax: number
substepBudgetClipped: boolean
```

计算口径：

```text
maxSandTopSubstepDeltaObserved = max_c |sandTopAfterSubstep[c] - sandTopBeforeSubstep[c]|
maxSandTopSpeedObserved = maxSandTopSubstepDeltaObserved / substepDt
```

普通物理子步要求 `maxSandTopSpeedObserved <= 30`；模型内部预算目标为 `<=28`。事件边界不参与该统计。

#### `WS_FRAME`

```text
WS_FRAME {frameTimeMs:8.2,physicsTimeMs:1.1,substeps:1,substepDt:0.008333,
          maxSandTopSubstepDeltaObserved:0.18,
          maxSandTopSpeedObserved:21.6,
          substepBudgetUsedMax:0.18,
          substepBudgetClipped:false,dormant:false}

WS_FRAME {frameTimeMs:8.3,physicsTimeMs:0.1,substeps:0,substepDt:0,
          maxSandTopSubstepDeltaObserved:0,
          maxSandTopSpeedObserved:0,
          substepBudgetUsedMax:0,
          substepBudgetClipped:false,dormant:true}
```

30Hz 帧包含 2 个子步时，`WS_FRAME` 汇报本渲染帧内最严格的子步观测值，不把两个子步相加后冒充“单子步位移”。脚本必须同时记录 `substeps/substepDt`。

##### Dormant 帧语义（2026-10-14 补充）

1. **是否输出 `WS_FRAME`**：输出。`testmode=true` 时每个渲染帧都输出 `WS_FRAME`，包括 dormant 帧；这保留 FPS、帧耗时和状态连续性证据，不得因休眠停止输出。
2. **零子步计划是否合法**：合法，但仅限 dormant 帧。dormant 帧必须输出：

```text
substeps=0
substepDt=0
maxSandTopSubstepDeltaObserved=0
maxSandTopSpeedObserved=0
substepBudgetUsedMax=0
substepBudgetClipped=false
dormant=true
```

`frameTimeMs` 仍必须为正，`physicsTimeMs` 照常报告实际耗时。active 帧必须满足 `substeps∈{1,2}` 且 `substepDt>0`；`dormant=false` 但 `substeps=0` 的普通帧判实现缺陷。

3. **字段兼容性**：`dormant:boolean` 是 v2.2 的 additive 字段，不删除、不重命名既有字段。旧 v2.2 日志缺失该字段时，脚本可用最近的 `WS_EVENT dormant/wake` 推断；新 E2E 证据应直接输出 `WS_FRAME.dormant`，缺失时至少标记 `missing_hook` 或 `dormantInferred`。
4. **脚本处理**：`water_sand_check.py` 的 `rate` 只统计 `dormant=false` 的 active 帧；dormant 帧不参与子步调度、速度、预算和 HG-04 rate 判定，也不得因 `substepDt=0` 报 non-positive dt。dormant 帧仍参与 FPS 样本计数，并单独报告 `dormantFrames`。若全部帧均为 dormant，rate 输出 `BLOCKED(insufficient_active_frames)`，不得用 0 速度伪装 PASS。

#### `WS_SNAPSHOT`

`schemaVersion=2` 与既有字段不变。脚本仍按事件边界重置参考；普通快照对齐时需要结合最近 `WS_FRAME.substeps/substepDt` 判断采样属于一个还是两个物理子步。

### 11.6 30/60/120Hz 一致性测试

工程位置：

- 主单测：`entry/src/test/WaterSandFieldTest.ets`
- 真机/模拟器单测：`entry/src/ohosTest/ets/test/WaterSandFieldTest.ets`
- 日志回归：`test/water_sand_check.py`

测试场景：

1. 使用小规模确定性均匀列（建议 `cols=8`），禁用或最小化扩散与滑移干扰。
2. 分别以 `frameDt=1/30`、`1/60`、`1/120` 驱动同一模拟时长。
3. 30Hz 调用 `step(1/30)`，内部必须拆 2 个 `1/60` 子步；60Hz 拆 1 个；120Hz 拆 1 个。
4. 对比同一时间点的 `suspH/suspL/bh/bl/mh/ml/sandTop` 与关键指标。

断言：

- 三相漂移 `<1e-12`。
- 每个子步 `sandTop` 变化 `<= vSandTopMax * substepDt + 1e-12`。
- 30Hz 单帧包含两个子步时，分别检查两个子步，不把两个子步相加后套用 `0.5` 单帧位移口径。
- 30/60/120Hz 的峰值悬浮、残余悬浮、沉积分数差异 `<=0.005`。
- 30/60/120Hz 的 `t90H/t90L` 差异 `<=0.005`。
- 强摇晃 3s 峰值悬浮落在 `0.55~0.75`。
- 静置 8s 残余悬浮 `<=0.05`。
- 倒立 4s 后 `phaseOrderScore>=0.30`。

`water_sand_check.py` 需要校验 `paramsVersion=="v2.2"`、`vSandTopMax==28`、`substeps` 与 `substepDt` 的乘积关系；缺失任一字段输出 `BLOCKED(missing_hook)`。

#### Reviewer 补充测试（2026-10-14）

以下四个用例必须落地，不能只停留在文档描述：

1. **`dormant_zeroSubstepStepResult`**
   构造平坦、无悬浮、无摇晃且水面已达平衡的场景，连续 `step(1/60)` 直到 `dormant=true`；再调用一次 `step(1/60)`。断言 `stepped=false`、`substeps=0`、`substepDt=0`、三个运动观测值为 `0`、`substepBudgetClipped=false`，且快照与调用前逐列一致、三相漂移不变。

2. **`frameHook_emitsDormantFrame`**
   在 `testmode=true` 下进入 dormant 后继续渲染至少 3 帧。断言每帧均输出 `WS_FRAME`，`frameTimeMs>0`，`dormant=true`，且零子步字段满足上述语义；同时只输出一次 `WS_EVENT dormant`，不得重复输出。

3. **`rateChecker_skipsDormantFrames`**
   构造包含一条合法 active `WS_FRAME` 和一条 dormant `WS_FRAME` 的合成日志，dormant 行使用 `substeps=0/substepDt=0`。运行 `water_sand_check.py --rate`，断言结果 PASS，`activeFrames=1`、`dormantFrames=1`，不出现 `non-positive frame/substep dt`，active 行仍完整执行预算与速度校验。

4. **`rateChecker_legacyDormantInference`**
   构造缺失 `WS_FRAME.dormant` 的旧 v2.2 日志，但在零子步帧前存在 `WS_EVENT dormant`、后续存在 `WS_EVENT wake`。断言脚本能推断 dormant 区间并跳过该帧，报告 `dormantSource="event"`；active 帧仍按 v2.2 字段校验。若既无字段又无事件，输出 `BLOCKED(missing_hook)`，不得误判 rate FAIL。

### 11.7 对 Step V2-1..V2-7 的影响

| 步骤 | v2.2 影响 | 修改内容 |
|---|---|---|
| Step V2-1 | **返工** | 删除 `maxSandTopFrameDelta`；新增 `vSandTopMax`；预算 scratch 从 frame 版改为 substep 版 |
| Step V2-2 | **返工** | 每个子步调用 `beginSubstepMotionBudget(substepDt)`；沉降/滑移/扬起共享子步预算 |
| Step V2-3 | **返工** | 指标改为 `maxSandTopSubstepDeltaObserved/maxSandTopSpeedObserved/substepBudget*` |
| Step V2-4 | 增量 | `SandCanvas` 仍一帧一次 `step()`；`WS_FRAME` 输出子步数、子步时长与子步观测值 |
| Step V2-5 | 增量 | `wsmaxtopdelta` 替换为 `wssandtopmax`；面板不暴露限速参数 |
| Step V2-6 | **返工** | 增加 30/60/120Hz 一致性、非绑定/绑定预算、浅水限幅与共享预算测试 |
| Step V2-7 | 增量 | 脚本解析 v2.2 字段并按子步口径校验 HG-04 速率 |

### 11.8 与 developer 并行修复的合并顺序

1. **先合入 reviewer 修复**：审计、脚本、单测与原子性修复先形成 v2 基线，避免与 v2.2 公式返工混在同一变更中。
2. **丢弃未合入的 v2.1 定值预算补丁**：若 Kepler 分支已实现 `maxSandTopFrameDelta`，不要在其上兼容叠加；rebase 时直接替换为 `vSandTopMax`。
3. **再按小步合入 v2.2**：参数接口 → 子步预算 → 指标/日志 → 一致性单测 → 脚本。
4. **预算与原子性合并**：子步预算是 scratch，每子步重置；物理状态回滚后不恢复旧预算，下一子步重新计算。
5. **脚本合并**：先保留 reviewer 修复的守恒/HG-06 逻辑，再替换帧预算字段为子步速率字段；不得同时接受 v2.1 与 v2.2 字段作为通过依据。
6. **测试合并**：先跑 reviewer 基线单测，再新增 30/60/120Hz 一致性；若两者失败，分别归因，不得用 v2.2 期望掩盖审计或原子性缺陷。

### 11.9 风险与回滚

- **30Hz HG-04 解释风险**：`vSandTopMax=28` 在 30Hz 渲染帧内理论合计位移约 `0.933` 列。当前 TC-WS-11 的 E2E 环境要求 120Hz，30Hz 仅用于物理一致性单测；若 manager 未来要求 30Hz E2E 也执行“相邻渲染帧 <=0.5 列”硬门槛，则该阈值与速率预算存在冲突，需 designer/manager 重新裁定，developer 不得私自加帧级上限。
- 性能：仅将预算计算从每帧改为每子步，复杂度仍为 `O(N)`；预计物理步增幅不超过 v2.1 预估的 `<0.05ms`。
- 稳定性：预算不足会延缓浅水沉降与滑移，这是预期限速；不得通过提高 `vSandTopMax>30` 或移除预算通过测试。
- 回滚：v2.2 参数组失败时可回退 `vH/vL/lift*`，但不得回退到 `maxSandTopFrameDelta=0.48`；该表达式已被上游 REJECTED。

---

## 12. WS_SNAPSHOT 4KB 传输方案（2026-10-14）

> 问题：真机 hilog 将超长 `WS_SNAPSHOT` JSON 截断在约 4KB，导致 `test/water_sand_check.py` 对真实日志输出 `BLOCKED(missing_hook)`。
> 目标：完整、可校验、可自动拉取的 E2E 快照证据，不改变物理状态与 `schemaVersion=2` payload。
> 适用范围：P-007/P-008 相关 E2E 客观验收；不改变 P-003/P-005 物理算法。

### 12.1 候选与选型

| 方案 | 优点 | 缺点 | 结论 |
|---|---|---|---|
| hilog 分片 `WS_SNAPSHOT_PART` | 不需要文件权限；可继续复用 hilog 采集通道 | 需要拆包/重组/序号/校验；hilog 高负载可能丢行或乱序；Base64/JSON 转义增加体积；仍可能被系统日志限流 | 不作为主方案 |
| 应用文件落盘 + `hdc file recv` | 完整 JSON 不受 hilog 单行长度限制；可用字节数与 SHA-256 精确校验；日志只输出小索引事件；便于归档与复测 | 需要应用沙箱路径与文件拉取权限；需要清理策略 | **选定** |

**选型结论**：E2E 采用“文件落盘 + 小事件索引”。hilog 只负责传递路径、字节数与校验值；完整 `WaterSandSnapshot` JSON 写入应用沙箱文件，由 tester 通过 `hdc file recv` 拉取。分片方案仅在文件通道被设备策略阻断时作为后续 fallback 评估，本轮不实现。

### 12.2 模块与线程职责

新增独立传输模块，建议文件：

```text
entry/src/main/ets/physics/WaterSandSnapshotTransport.ets
```

职责划分：

| 模块 | 职责 |
|---|---|
| `WaterSandField` | 只生成 `WaterSandSnapshot`，不做文件 I/O，不感知传输方式 |
| `WaterSandSnapshotTransport` | 序列化、原子写文件、计算字节数与 SHA-256、清理旧快照 |
| `SandCanvas` | 在 `wssnapshot=true` 时调用传输模块，并在写文件成功后输出 `WS_SNAPSHOT_FILE` |
| `test/water_sand_check.py` | 解析索引事件、拉取文件、校验字节与哈希、还原 payload |
| `test/record_tc_ws.py` | 编排 `hdc file recv`，把快照目录传给判定脚本 |

线程与时序：

1. `SandCanvas` 在测试模式下复制/生成快照，不在渲染热路径做磁盘写。
2. 文件写入使用测试专用后台任务或等价的异步 I/O；任务输入是快照的普通数组副本，禁止后台任务回写物理状态。
3. 写文件成功后，UI 线程输出小体积 `WS_SNAPSHOT_FILE` 事件。
4. 写文件失败时输出 `WS_EVENT snapshot_file_error`，不得输出成功索引。
5. 快照频率保持现有语义：init/reset 后与测试请求/周期采样点，不改成每帧输出。

### 12.3 文件路径与命名

应用沙箱根目录：

```text
<context.filesDir>/WaterSandSnapshots/
```

单个快照文件：

```text
ws-<sessionId>-<sequence:06d>.json
```

示例：

```text
.../files/WaterSandSnapshots/ws-3f9c1a2b7d4e5f60-000001.json
```

字段语义：

- `sessionId`：本次应用运行的 16 位小写十六进制标识；同一 run 内不变。
- `sequence`：快照序号，从 `1` 开始单调递增；`reset` 不清零。
- `reason`：`init` / `reset` / `periodic` / `requested`。
- `frameIndex`：输出快照时的渲染帧序号。
- `simulationTimeMs`：模型模拟时间。

保留策略：

- 测试模式最多保留最近 `128` 个快照或总大小 `32MB`，以先到者为准。
- 清理只删除 `ws-*.json` 与对应 `.tmp`，不得删除目录外文件。
- 生产模式默认不写快照文件；只有 `wssnapshot=true` 时启用。

写入必须原子化：

1. 先写 `<final>.tmp`。
2. 关闭并确认写入完成。
3. 原子重命名为 `<final>.json`。
4. 只有最终文件存在且字节数/哈希计算完成后，才输出成功事件。

### 12.4 文件格式

文件为 UTF-8 JSON，无 BOM，使用 LF 换行；顶层是传输 envelope，`payload` 保持既有 `WaterSandSnapshot` 结构：

```json
{
  "transportVersion": 1,
  "kind": "WaterSandSnapshot",
  "sessionId": "3f9c1a2b7d4e5f60",
  "sequence": 1,
  "reason": "init",
  "frameIndex": 0,
  "simulationTimeMs": 0,
  "payload": {
    "schemaVersion": 2,
    "cols": 128,
    "capacity": 192,
    "waterTop": [],
    "bh": [],
    "bl": [],
    "mh": [],
    "ml": [],
    "suspH": [],
    "suspL": [],
    "hs": [],
    "hl": [],
    "sandTop": []
  }
}
```

约束：

- `transportVersion=1` 表示文件传输 envelope，不改变 payload schema。
- `payload.schemaVersion` 必须为 `2`。
- `payload` 字段顺序保持 `schemaVersion, cols, capacity, waterTop, bh, bl, mh, ml, suspH, suspL, hs, hl, sandTop`。
- 所有数组长度必须等于 `payload.cols`。
- `hs/hl/sandTop` 仍由 `bh/bl/mh/ml` 派生，文件导出不得重算或修改。
- envelope 可紧凑序列化；文件字节数与 SHA-256 面向整个文件的原始字节计算。

### 12.5 hilog 钩子

每次快照写文件成功后输出：

```text
WS_SNAPSHOT_FILE {"transportVersion":1,"schemaVersion":2,
                  "sessionId":"3f9c1a2b7d4e5f60","sequence":1,
                  "reason":"init","frameIndex":0,
                  "path":"<absolute sandbox path>",
                  "bytes":18432,"sha256":"<lowercase hex>"}
```

字段：

- `bytes`：最终 `.json` 文件的 UTF-8 字节数。
- `sha256`：整个文件原始字节的 SHA-256，小写十六进制。
- `path`：应用沙箱绝对路径，tester 必须原样传给 `hdc file recv`。

应用启动或快照目录初始化后输出一次：

```text
WS_SNAPSHOT_DIR {"transportVersion":1,"sessionId":"...","path":"..."}
```

写文件失败输出：

```text
WS_EVENT snapshot_file_error {"sequence":1,"reason":"write_failed",
                              "errorCode":13,"path":"..."}
```

既有单行 `WS_SNAPSHOT` 保留为本地/单元测试兼容钩子；E2E 真机验收不得依赖它。若文件钩子与旧钩子同时存在，判定脚本以 `WS_SNAPSHOT_FILE` 为准。

### 12.6 tester 判定脚本适配

`test/record_tc_ws.py`：

1. 采集 hilog 时保留 `WS_SNAPSHOT_DIR` 与 `WS_SNAPSHOT_FILE`。
2. 对每个成功索引事件执行：

```text
hdc file recv "<path>" "<localSnapshotDir>/<fileName>"
```

3. 若单文件事件丢失，可按 `WS_SNAPSHOT_DIR` 拉取整个 `WaterSandSnapshots` 目录或枚举 `ws-*.json`。
4. 将本地快照目录传给 `water_sand_check.py`。

`test/water_sand_check.py` 新增参数：

```text
--snapshot-dir <localDir>
--pull-snapshots
--device <serial>
```

校验流程：

1. 解析 `WS_SNAPSHOT_FILE`，提取 `sessionId/sequence/path/bytes/sha256`。
2. 读取本地文件原始字节，校验 `len(bytes)==bytes`。
3. 计算 SHA-256，校验与事件一致。
4. 解析 JSON，校验 `transportVersion==1`、`kind=="WaterSandSnapshot"`。
5. 校验 `payload.schemaVersion==2`、数组长度、事件与 envelope 的 `sessionId/sequence` 一致。
6. 使用 payload 走既有三相守恒、分选指标、HG-04 与 HG-06 交叉校验。
7. 按同一 `sessionId` 校验 `sequence` 单调且无缺失；缺失时输出 `BLOCKED(missing_snapshot)`。

判定分类：

- 缺少 `WS_SNAPSHOT_FILE/DIR`、文件路径不可用、`hdc file recv` 权限失败：`BLOCKED(snapshot_transport)` 或 `BLOCKED(environment_file_access)`。
- 文件字节数不符、SHA-256 不符、JSON 截断、`transportVersion` 不符、payload schema 不符：`FAIL(snapshot_transport)`。
- 只有旧版 `WS_SNAPSHOT` 且已截断：`BLOCKED(missing_hook)`，不得默认通过。

### 12.7 对 schemaVersion=2 与实施的影响

**schema 影响**：

- `WaterSandSnapshot.schemaVersion` 保持 `2`，不新增、不删除、不重排 payload 字段。
- 新增 `transportVersion=1` 只属于文件 envelope，不进入 `WaterSandField` 公共快照接口。
- `WS_SNAPSHOT_FILE` 是 additive 钩子；旧 `WS_SNAPSHOT` 不删除，但不再作为真机 E2E 证据。

**实施步骤**：

1. 新增 `WaterSandSnapshotTransport.ets`，实现原子写、字节数、SHA-256、sessionId/sequence 与清理。
2. `SandCanvas` 在 `wssnapshot=true` 时调用传输模块，并输出 `WS_SNAPSHOT_DIR/FILE`。
3. `record_tc_ws.py` 增加 `hdc file recv` 编排。
4. `water_sand_check.py` 增加文件校验与 payload 还原。
5. 真机回归验证一条完整快照可被拉取、哈希通过、HG-02/HG-06 不再因截断阻塞。

**与并行修复的合并顺序**：

1. 先合入 reviewer 的审计、脚本、单测与原子性修复，保持判定基座稳定。
2. 快照传输是独立 I/O/测试通道补丁，可与 v2.2 速率预算并行，但不要混入同一物理公式补丁。
3. 若发生 `SandCanvas` 冲突，优先保留 reviewer 修复，再叠加快照传输调用。
4. tester 脚本先保留旧 `WS_SNAPSHOT` 解析，再新增文件优先逻辑；真实 E2E 报告必须标明使用的传输通道。

### 12.8 风险与回滚

- **文件权限风险**：若设备策略禁止 `hdc file recv` 读取应用沙箱，tester 输出 `BLOCKED(environment_file_access)`，不得伪造快照；再由 manager 决定是否启用分片 fallback。
- **磁盘占用风险**：使用 `128` 个文件或 `32MB` 上限，并只清理 `WaterSandSnapshots` 目录内匹配文件。
- **事件丢失风险**：保留 `WS_SNAPSHOT_DIR`，tester 可枚举目录补拉；缺失序号仍输出 `BLOCKED(missing_snapshot)`。
- **哈希算法风险**：HarmonyOS 侧与 Python 侧必须同用 SHA-256；事件与文件均使用小写十六进制。
- **回滚**：关闭 `wssnapshot` 或移除传输调用即可恢复无文件模式；此时真机 E2E 快照类判定回到 `BLOCKED(missing_hook)`，不得改用截断日志通过。
