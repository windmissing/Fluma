# 工程方案：P-001 两种不互溶液体油水分层

> 版本：v1.0　日期：2026-10-15　状态：DRAFT  
> 算法依据：`docs/algorithm-P001.md`（`liquid-ca-m1p-v1.0`）  
> 用例范围：`TC-11~13`、`EC-OL-01~03`  
> 本文只描述工程落地方案，不改变算法语义；实现与测试若发现算法矛盾，回传 designer 更新算法版本。

## 1. 目标与工程决策

P-001 在现有 `rendermode=3` 液体路径上落地两相不互溶液体：静置分层、摇晃暂混、180° 倒立重排。工程侧以新增独立 `LiquidCAField` 为核心，不复用 `WaterSandField` 的三相状态，避免污染既有 P-003/P-005/P-011 路线。

核心决策：

1. **新增专用模型**：`LiquidCAField` 持有 `cells/phase/shade/active` 与两相计数，所有运行期更新只做移动、占位交换或相位交换。
2. **独立工作线程**：物理仿真运行在专用 Worker，UI 帧回调只发送时间戳与渲染最新快照，避免 24,000 格扫描阻塞主线程。
3. **固定子步**：Worker 内使用 `dt=1/120s` 累加器，单渲染帧最多执行 2 个物理子步，禁止用真实帧间隔直接放大 `dt`。
4. **可测性优先**：`LIQ_EVENT/LIQ_FRAME/LIQ_METRIC` 与模拟层指标为权威；缺失钩子时测试结论只能是 `BLOCKED(missing_hook)`。
5. **无静默降级**：P-001 v1 不切换到 `column-relax-v1`。Worker 或网格预算失败时输出 `model_error`，不改变技术路线。

## 2. 状态量设计

### 2.1 模拟层状态

新增 `LiquidCAField` 内部状态：

| 状态 | 类型 | 语义与约束 |
|---|---|---|
| `cells` | `Uint8Array(N)` | `0=EMPTY`、`1=SAND`；P-001 两相液体均为 `SAND`，越界由坐标检查视为墙。 |
| `phase` | `Uint8Array(N)` | `0=重相`、`1=轻相`；仅 `cells[i]===SAND` 时有效。 |
| `shade` | `Uint8Array(N)` | 0~7 渲染颗粒纹理；初始化后只读，不参与物理。 |
| `activeBits` | `Uint8Array((N+7)>>3)` | 需检查格子的位图标记；`move/swap` 后同步置位自身与 8 邻域。 |
| `heavyCount` / `lightCount` | `number` | 初始化后只读；任何子步后不得变化。 |
| `initialHeavyCount` / `initialLightCount` | `number` | 指标与守恒判定基准。 |
| `gravityDown` | `boolean` | `true=(0,+1)`，`false=(0,-1)`；`invert180()` 只翻转该值。 |
| `shakeEnergy` | `number` | 当前能量，范围 `[0,1]`，按固定子步逼近目标并衰减。 |
| `targetShakeEnergy` | `number` | 脚本/Want 注入目标，范围 `[0,1]`。 |
| `accumulatorSeconds` | `number` | 固定步长累加器；Worker 独占，不暴露给 UI。 |
| `idleSubsteps` | `number` | 连续无状态变化子步数；达到 12 后 `dormant=true`。 |
| `dormant` | `boolean` | 静默标记；新事件唤醒，静默后停止随机探测。 |
| `rngState` | `number` | 32 位确定性伪随机状态；同一 seed 序列完全可复现。 |
| `simulationTimeSeconds` | `number` | 已执行物理子步的 `dt` 总和，不随渲染丢帧增长。 |
| `eventSequence` / `frameSequence` | `number` | 单调递增，事件与帧日志对齐依据。 |

列缓存使用 `Int32Array(cols)` 预分配：

- `heavyInterfacePx`：每列重力远端最近的重相界面像素坐标；无重相时使用哨兵值。
- `liquidTopPx` / `liquidBottomPx`：每列液体边界，用于铺平与表面迁移。
- `columnLiquidCount`：每列液体格子数，用于总液面高度。
- `mixedBoundaryCount`：每列混合带格子计数，供 250ms 指标复用。

不变量：

1. `heavyCount === initialHeavyCount`、`lightCount === initialLightCount`。
2. `phase` 有效值只有 0/1；`cells` 不因摇晃或倒立改变类型。
3. `gravityDown` 只能经 `invert180()` 修改；UI、渲染与测试脚本不得直接写物理数组。
4. `dormant=true` 时若界面高度差大于 1 格，必须执行一次确定性铺平，不得永久冻结。
5. 稳态物理路径不得创建数组、对象或字符串；事件与 250ms 指标序列化属于测试输出路径。

### 2.2 初始几何

按屏幕容器推导 `cols/rows/cellPx` 后，固定初始化区域：

- 轻相：行区间 `[25%, 50%)`
- 重相：行区间 `[50%, 85%)`
- 其他区域为空，总填充率 60%。

若 `cols * rows > 24000`，按整数倍增大 `cellPx` 并重算网格；仍超过则输出 `model_error` 并停止 P001 路径。缩放后的 `cols/rows/cellPx` 必须写入 init 事件。

## 3. 模块划分

| 文件 | 类型 | 职责 |
|---|---|---|
| `entry/src/main/ets/physics/LiquidCAField.ets` | 新增 | 状态、确定性随机、压力门控沉降、铺平、摇晃混合、固定子步、指标计算。 |
| `entry/src/main/ets/physics/LiquidCATypes.ets` | 新增 | 配置、指标、事件、快照与 Observer 接口。 |
| `entry/src/main/ets/workers/LiquidCAWorker.ets` | 新增 | Worker 入口；持有 `LiquidCAField`，按命令顺序执行 init/reset/shake/invert/frame。 |
| `entry/src/main/ets/physics/LiquidCAController.ets` | 新增 | 主线程会话；Worker 生命周期、命令序列、三缓冲快照、背压与降级。 |
| `entry/src/main/ets/physics/LiquidCAWant.ets` | 新增 | Want 参数解析、校验、默认值与 `onNewWant` 命令转换。 |
| `entry/src/main/ets/physics/LiquidCATelemetry.ets` | 新增 | `LIQ_*` hilog 输出；帧路径使用格式化占位符，避免字符串拼装。 |
| `entry/src/main/ets/pages/SandCanvas.ets` | 修改 | `rendermode=3` 分支接入 controller；输入事件、快照渲染与模式切换。 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | 修改 | 解析启动 Want 与 `onNewWant`，写入/转发 P001 参数。 |
| `entry/src/test/LiquidCAFieldTest.ets` | 新增 | 守恒、确定性、固定步长、倒立、静默与铺平单元测试。 |
| `entry/src/test/LiquidCAHookTest.ets` | 新增 | 钩子 schema、事件序列与无效输入测试。 |
| `test/liquid_ca_check.py` | 新增 | 解析 hilog，计算 TC-11~13 指标、P95 与报告 JSON。 |

不修改 `WaterSandField`、P-003/P-005/P-011 报告与测试口径。

## 4. 核心接口

### 4.1 类型定义

```ts
export interface LiquidCAConfig {
  cols: number;
  rows: number;
  cellPx: number;
  seed: number;
  pSettle: number;
  qMixBase: number;
  pInertia: number;
  shakeDecayRate: number;
  maxCells: number;
}

export enum LiquidInputSource {
  WANT = 0,
  SCRIPT = 1,
  PANEL = 2,
  INTERNAL = 3
}

export interface LiquidCAEventPayload {
  type: string;
  sequence: number;
  eventTimeMs: number;
  boundary: boolean;
  source: LiquidInputSource;
  forceInvert: boolean;
  shakeIntensity: number;
  errorCode: string;
  cols: number;
  rows: number;
  cellPx: number;
  initialHeavy: number;
  initialLight: number;
  seed: number;
  scaled: boolean;
}

export interface LiquidCAFramePayload {
  sequence: number;
  frameTimeMs: number;
  physicsTimeMs: number;
  snapshotTimeMs: number;
  substeps: number;
  clippedTimeMs: number;
  valid: boolean;
}

export interface LiquidCAMetricPayload {
  sequence: number;
  metricTimeMs: number;
  heavyCount: number;
  lightCount: number;
  emptyCount: number;
  initialHeavyCount: number;
  initialLightCount: number;
  heavyDrift: number;
  lightDrift: number;
  heavyCentroidX: number;
  heavyCentroidY: number;
  lightCentroidX: number;
  lightCentroidY: number;
  interfaceRowsPx: number[];
  interfaceRangeCells: number;
  mixedBandShare: number;
  simulationTime: number;
  dormant: boolean;
  finite: boolean;
  gravityInverted: boolean;
}

export interface LiquidRenderSnapshot {
  cols: number;
  rows: number;
  cellPx: number;
  gravityInverted: boolean;
  cells: Uint8Array;
  phase: Uint8Array;
  shade: Uint8Array;
}

export interface LiquidCAObserver {
  onEvent(event: LiquidCAEventPayload): void;
  onFrame(frame: LiquidCAFramePayload): void;
  onMetric(metric: LiquidCAMetricPayload): void;
}
```

首轮配置固定为：`pSettle=0.08`、`qMixBase=0.30`、`pInertia=0.20`、`shakeDecayRate=1.8`、`maxCells=24000`。`pSettle` 只允许在算法声明的 `[0.06,0.12]` 内按 seed 0/1/2 统一校准；其余参数若需调整，必须回传算法方案升版。

### 4.2 模型接口

```ts
export class LiquidCAField {
  constructor(config: LiquidCAConfig);

  initialize(): LiquidCAEventPayload;
  reset(seed: number, eventTimeMs: number): LiquidCAEventPayload;
  setShake(intensity: number, eventTimeMs: number, source: LiquidInputSource): LiquidCAEventPayload;
  invert180(enabled: boolean, eventTimeMs: number, source: LiquidInputSource): LiquidCAEventPayload;
  stepFrame(realDtSeconds: number): LiquidCAFramePayload;
  metrics(): LiquidCAMetricPayload;
  copyRenderState(target: LiquidRenderSnapshot): boolean;
  setObserver(observer: LiquidCAObserver | null): void;
}
```

语义约束：

1. `initialize/reset` 重建全部数组并重置 RNG；`seed` 必须为非负整数。
2. `setShake` 将 `intensity` 夹取到 `[0,1]`，`intensity>0` 输出 shake 事件并唤醒。
3. `invert180` 只翻转 `gravityDown`，输出 boundary 事件并唤醒。
4. `stepFrame(realDtSeconds)` 不直接使用该值做物理，内部固定为最多两个 `1/120s` 子步。
5. `metrics()` 返回复用对象；`interfaceRowsPx` 只在 250ms 指标路径生成，物理子步不访问该数组。
6. `copyRenderState` 只读复制，不触发指标、事件或物理。

### 4.3 Controller 与 Worker 消息

```ts
export class LiquidCAController {
  constructor(renderer: LiquidCARenderer, telemetry: LiquidCATelemetry);
  start(config: LiquidCAConfig): Promise<void>;
  stop(): void;
  onVsync(frameTimeMs: number): void;
  setShake(intensity: number, source: LiquidInputSource): void;
  setForceInvert(enabled: boolean, source: LiquidInputSource): void;
  reset(seed: number, source: LiquidInputSource): void;
}

export interface LiquidCARenderer {
  render(snapshot: LiquidRenderSnapshot, frame: LiquidCAFramePayload): void;
  onModelError(errorCode: string): void;
}
```

Worker 消息采用 FIFO JSON 消息：

| 主线程 → Worker | 字段 | 说明 |
|---|---|---|
| `init` | `config` | 创建模型并输出 init。 |
| `reset` | `seed,eventTimeMs,source,sequence` | 重建模型。 |
| `shake` | `intensity,eventTimeMs,source,sequence` | 设置目标摇晃能量。 |
| `invert` | `enabled,eventTimeMs,source,sequence` | 翻转重力。 |
| `frame` | `frameTimeMs,sequence` | 推进固定子步并返回快照。 |
| `stop` | `sequence` | 释放模型与 Worker。 |

Worker → 主线程：

| 消息 | 内容 |
|---|---|
| `event` | `LiquidCAEventPayload` |
| `frame` | `LiquidCAFramePayload` + 可转移快照 buffer |
| `metric` | `LiquidCAMetricPayload` |
| `error` | `errorCode,message,sequence` |

命令使用同一 `sequence` 序列，Worker 按接收顺序执行；事件不得跨过其触发帧被延迟输出。

## 5. 数据流与时序

### 5.1 启动

```text
aa start --ps rendermode=3 --ps testmode=true --ps forceinvert=false
  -> EntryAbility.onCreate/onNewWant
  -> LiquidCAWant.parse()
  -> AppStorage + SandCanvas.aboutToAppear
  -> LiquidCAController.start(config)
  -> LiquidCAWorker init
  -> LIQ_EVENT init
  -> 首帧快照渲染
```

`testmode=true` 后，`SandCanvas` 禁用加速度传感器，手势只作为调试输入；TC-12/TC-13 正式输入只能来自 Want、脚本或 controller API。

### 5.2 渲染帧

```text
UI vsync(frameTimeMs)
  -> controller.onVsync()
  -> post frame message
  -> Worker:
       clamp realDt <= 1/60
       accumulator += realDt
       while accumulator >= 1/120 and substeps < 2:
           physicsSubstep(1/120)
       build frame payload + snapshot
       every 250ms simulation time: build metric payload
  -> main thread:
       LIQ_FRAME
       optional LIQ_METRIC
       renderer.render(snapshot)
       return snapshot buffer to Worker
```

UI 线程不做格子扫描、随机探测或指标全量遍历；Worker 未返回前最多允许 2 个 tick 排队，超过后合并 tick，并在 `LIQ_FRAME.valid=false`、`clippedTimeMs` 中记录。正式性能验收要求 `valid=true` 比例为 100%。

### 5.3 快照传输

Worker 与 UI 使用 3 组可转移 `ArrayBuffer` 轮转：

1. Worker 写入 `cells/phase/shade` 并转移所有权给主线程。
2. UI 只读渲染，渲染完成后将 buffer 所有权转回 Worker。
3. 若两组均未归还，使用第三组；三组均忙则输出 `SNAPSHOT_BACKPRESSURE`。

以 24,000 格计算，单快照约 72KB，三缓冲约 216KB；100fps 传输量约 7.2MB/s，不产生稳态堆分配。

## 6. 固定步长规则

1. Worker 中 `PHYSICS_DT_SECONDS = 1 / 120`。
2. 每个渲染 tick 的 `realDt` 先夹取到 `[0, 1/60]`。
3. `accumulator >= PHYSICS_DT_SECONDS` 且本 tick 子步数 `<2` 时执行一个子步。
4. 执行 2 个子步后若仍有剩余时间，将累加器截断到 `PHYSICS_DT_SECONDS`，差额记入 `clippedTimeMs`；不得追帧。
5. `simulationTime` 只按实际执行子步累加。
6. 事件在下一个子步前生效；事件边界帧由测试标记，不参与相邻帧连续性判定。
7. `dormant=true` 后仍保留固定帧调度，但跳过随机探测；事件唤醒时 `idleSubsteps=0`。

## 7. Want 参数

| 参数 | 类型/默认值 | 处理规则 |
|---|---|---|
| `rendermode` | `3` | 整数；非 3 时不启动 P001 模型。 |
| `testmode` | `false` | `true` 禁用真实传感器，启用正式测试钩子。 |
| `forceinvert` | `false` | 初始必须为 false；启动传 true 时置 false 并输出 `INIT_FORCE_INVERT_INVALID`。 |
| `shake` | `0.0` | `[0,1]`；启动注入一次 shake 事件，通常测试从 0 开始。 |
| `seed` | `0` | 非负整数；正式轮次固定 0/1/2。 |
| `cellpx` | `4` | 正整数；用于复现网格尺寸，最终受 `maxCells` 约束。 |

`EntryAbility.onCreate` 负责冷启动解析；`onNewWant` 负责运行中更新。`shake` 与 `forceinvert` 在 `onNewWant` 中转换为 controller 命令；`seed` 变化触发 reset 事件。无效参数使用默认值并输出 `mode/model_error`，不得 crash。

示例：

```text
aa start -b com.example.helloworld -a EntryAbility \
  --ps rendermode 3 --ps testmode true \
  --ps forceinvert false --ps seed 0
```

TC-12 通过 `onNewWant` 依次注入 `shake=1.0` 与 6 秒后 `shake=0.0`；TC-13 依次注入 `forceinvert=true`、`forceinvert=false`。

## 8. 事件与测试钩子

### 8.1 `LIQ_EVENT`

输出事件：`init/reset/shake/invert/mode/model_error`。所有事件必须包含：

```json
{
  type: invert,
  sequence: 12,
  event_time_ms: 123456.789,
  boundary: true,
  source: 0,
  force_invert: true,
  shake_intensity: 0.0,
  error_code: "
}
```

`init/reset` 额外输出 `cols/rows/cell_px/initial_heavy/initial_light/seed/scaled`。

### 8.2 `LIQ_FRAME`

每有效渲染帧输出：

```text
LIQ_FRAME sequence=%d frame_time_ms=%.3f physics_time_ms=%.3f snapshot_time_ms=%.3f substeps=%d clipped_time_ms=%.3f valid=%d
```

### 8.3 `LIQ_METRIC`

每 250ms 模拟时间输出。`interface_rows_px` 忽略左右各 10% 列；`interface_range_cells=(max-min)/cellPx`。`mixed_band_share` 定义为 8 邻域内存在异相的液体格子数除以两相总格子数。

关键字段：

- `heavy_count/light_count/empty_count`
- `heavy_drift/light_drift`
- `heavy_centroid_x/y`、`light_centroid_x/y`
- `interface_rows_px`、`interface_range_cells`
- `mixed_band_share`
- `simulation_time`、`dormant`
- `finite`、`gravity_inverted`

任一数值非有限时，立即停止物理推进并输出 `LIQ_EVENT model_error`，测试判 FAIL 而不是忽略样本。

## 9. 验收指标对齐

### 9.1 TC-11 / EC-OL-01

| 验收项 | 工程实现口径 |
|---|---|
| `phaseDrift` | `heavy_drift/light_drift`，全时段逐样本 `<0.001`，目标 0。 |
| 稳态相序 | `heavy_centroid_y > light_centroid_y`（屏幕坐标向下）。 |
| `interfaceRangeCells` | `<=2` 及格，目标 `<=1`。 |
| 零抖动 | 末 5 秒 `dormant=true` 且像素分类变化 0%；事件纪律要求无未输入事件。 |
| 主观 | 分层可读性、界面平滑度、稳定流畅度 `>=6`，HG-OL-01~04/07 不触发。 |

### 9.2 TC-12 / EC-OL-02

| 验收项 | 工程实现口径 |
|---|---|
| `phaseDrift` | 摇晃段逐样本 `<0.001`，目标 0。 |
| 混合曲线 | `mixed_band_share` 混合段上升、恢复段带小噪声下降；峰值目标 `>=20%`，终态 `<=2%`。 |
| `recoveryTime` | 从 `shake=0` 事件到 `mixed_band_share<=2%` 且相序恢复；MUST `<=4s`，目标 `<=3s`。 |
| 恢复终态 | 相序正确、界面 `<=3` 格（目标 `<=2` 格）、30 秒后无永久乳浊。 |
| 主观 | 动态自然度、恢复可信度、分层可读性、稳定流畅度 `>=6`，HG-OL-01~05/07 不触发。 |

### 9.3 TC-13 / EC-OL-03

| 验收项 | 工程实现口径 |
|---|---|
| `phaseDrift` | 倒立与翻回全程 `<0.001`，目标 0。 |
| 相序 | 按当前 `gravity_inverted` 坐标判定；翻回后恢复正立相序。 |
| `rearrangeTime` | 事件到质心进入并保持稳态窗口；每次 `2~4s`，目标 `2.5~3.5s`。 |
| 连续性 | 相邻有效帧质心位移 `<=1` 格；事件边界帧排除。 |
| 审计 | 必须存在 boundary `invert` 事件；禁止直接改数组。 |
| 主观 | 五个维度均 `>=6`，HG-OL-01~07 不触发。 |

### 9.4 性能与稳定性

- `fpsP95 >= 100`，每用例至少 100 个有效帧样本。
- `physicsTimeMs.p95 <= 4`，只统计物理子步耗时，不含指标序列化。
- TC-11~13 各至少 3 轮，seed 固定 0/1/2。
- 模拟层与像素层冲突时判数据一致性失败，不得择优报告。

## 10. 性能与内存评估

### 10.1 CPU

| 项目 | 估算 |
|---|---:|
| 最坏单子步 | `O(N)`，`N<=24000`。 |
| 典型单子步 | `O(C+A)`，摇晃活动格设计上限 30%。 |
| 单帧最多子步 | 2 个，最坏 `O(2N)`。 |
| 指标 | 每 250ms 一次 `O(N)`，不在 `physicsTimeMs` 内。 |
| 快照传输 | 约 7.2MB/s @ 100fps、24,000 格。 |

若活动格比例超过 30%，优先收缩搅拌边界，不降低物理频率；若 `physicsTimeMs.p95>4`，先缩放网格，不放宽 `fpsP95`。

### 10.2 内存

- 模型数组：`3N` 字节，24,000 格约 72KB。
- `activeBits`：约 `N/8` 字节，约 3KB。
- 列缓存与指标缓存：`O(cols)`，通常小于 8KB。
- 三缓冲快照：约 216KB。
- Worker 与 controller 常驻对象：小于 64KB。

物理层稳态目标为 0 次堆分配；hilog 与 JSON 序列化仅发生在测试钩子路径，且 `LIQ_FRAME` 使用占位符避免每帧字符串拼装。

### 10.3 风险与处理

| 风险 | 等级 | 处理 |
|---|---|---|
| Worker 传输成为瓶颈 | 中 | 三缓冲 + backpressure 指标；若 `snapshotTimeMs.p95>1`，评估 SharedArrayBuffer 或双缓冲复用。 |
| 设备低于 100Hz | 高 | 记录环境 BLOCKED，不放宽阈值。 |
| 随机液滴滞留 | 高 | 保持压力门控与铺平；若 30 秒不收敛，回传算法 v1.1 增加有限回流引导。 |
| hilog 体积影响帧率 | 中 | 只输出规定钩子；帧日志用格式化字段，禁用调试级冗余日志。 |
| Worker 初始化失败 | 中 | 输出 `WORKER_INIT_FAILED`；仅当 `N<=12000` 且单帧试算 `<=2ms` 时允许同一模型主线程同步兜底，否则停止。 |
| 网格超过预算 | 中 | 整数倍增大 `cellPx`；仍超限输出 `GRID_TOO_LARGE`。 |
| 指标非有限 | 高 | 停止推进并 `model_error`，不渲染伪造数据。 |

## 11. 实施步骤

1. 新增 `LiquidCATypes` 与 `LiquidCAField`，先实现数组状态、初始化、确定性 RNG 与只读指标。
2. 实现压力门控沉降、界面铺平、摇晃搅拌与 `invert180`；补齐 active 位图与列缓存。
3. 增加单元测试：守恒、同 seed 确定性、倒立审计、静默零抖动、固定子步与铺平终态。
4. 新增 Worker 与 Controller，完成消息序列、三缓冲快照、backpressure 与 stop 生命周期。
5. 接入 `SandCanvas.rendermode=3`：初始化、渲染快照、面板 shake/invert/reset；`testmode=true` 时强制禁用传感器。
6. 扩展 `EntryAbility` Want 解析与 `onNewWant` 命令转换，落实启动 `forceinvert=false` 纪律。
7. 新增 `LiquidCATelemetry` 与 hook 测试，确保日志 schema 与事件时序稳定。
8. 新增 `test/liquid_ca_check.py`，完成 hilog 解析、指标计算、报告输出与 seed 分组。
9. 在目标设备执行 TC-11~13 各 3 轮，先调 `pSettle` 至共同区间，再测性能与长时内存。
10. 输出客观与主观报告，按算法指定路径隔离保存；失败按算法第 7 节分流。

## 12. 测试交接

### 12.1 必交物

- `docs/test-report-P001-liquid-ca-m1p-v1.md`
- `docs/effect-report-P001-liquid-ca-m1p-v1.md`
- 原始 hilog 与逐帧录制文件
- seed 0/1/2 的指标 JSON
- 设备型号、系统版本、屏幕刷新率、CPU/内存采样配置

### 12.2 客观脚本判定

1. 启动参数必须包含 `rendermode=3/testmode=true/forceinvert=false`。
2. 缺失任一 `LIQ_*` 钩子：`BLOCKED(missing_hook)`。
3. TC-11：30 秒静置，末 5 秒逐样本判定。
4. TC-12：静置稳态后 `shake=1.0` 6 秒，再 `shake=0.0` 静置 30 秒。
5. TC-13：正立稳态后倒立 8 秒、翻回 8 秒，分别统计两次重排。
6. 每轮至少 100 个有效帧；`valid=false` 帧不计入 P95，但比例必须为 0。
7. 指标冲突时输出 `DATA_CONSISTENCY_FAIL`，不得择优。

### 12.3 主观评审

- 每场景两轮盲评；同维度分差 `>=1` 时追加第三轮取中位数。
- 评分员不感知路线版本与参数版本。
- EC-OL-01~03 适用维度均 `>=6`，对应 HG 硬门槛不得触发。

## 13. 变更记录

- 2026-10-15：创建 v1.0。基于 `liquid-ca-m1p-v1.0` 设计独立 `LiquidCAField`、Worker 线程模型、固定 120Hz 子步、Want/事件钩子、性能内存预算、TC-11~13/EC-OL-01~03 验收映射与测试交接。
