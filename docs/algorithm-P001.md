# 算法方案：P-001 场景一“两种不互溶液体油水分层”

> 版本：v1.0　日期：2026-10-15　状态：DRAFT（待实现与双报告闭环）  
> 技术路线：`liquid-ca-m1p-v1.0`  
> 用例来源：`docs/test-cases.md` 的 TC-11~13，`docs/effect-cases.md` 的 EC-OL-01~03。  
> 报告隔离：本路线客观与主观报告必须分别写入 `docs/test-report-P001-liquid-ca-m1p-v1.md`、`docs/effect-report-P001-liquid-ca-m1p-v1.md`；比较基线只能是同一技术路线上一版。跨路线只允许定性参考，禁止全量 dominance 结论。

## 1. 问题与目标

P-001 需要在密封二维容器内表现两种不互溶液体：

1. 静置时重相在下、轻相在上，界面近似水平且稳定零抖动。
2. 强摇晃时两相短暂混合成液滴或碎块，停止后自然恢复分层。
3. 180° 倒立与翻回时，两相按新的重力方向连续重排，不瞬移、不丢体积。
4. 在鸿蒙端侧满足高帧率、低内存、低物理时延与确定性可测性。

本方案以模拟层粒子/格子计数为守恒权威口径，渲染像素只做交叉验证。缺失 `LIQ_*` 钩子时，测试只能给出 `BLOCKED(missing_hook)`，不得用像素结果替代正式结论。

## 2. 候选技术路线与选型

### 2.1 `liquid-ca-m1p-v1.0`（选定）

在 `SandGrid` M1' 液体模式基础上，保留二维格子与两相相位数组，用“移动/交换不改类型”的元胞自动机表达沉降、浮升、铺平、摇晃混合与倒立重排。优点是天然精确守恒、局部连续、可生成液滴碎块，且与现有 `rendermode=3` 实现方向一致。

### 2.2 `column-relax-v1`（REJECTED）

每列维护重相高度、轻相高度与混合带高度，用浅水方程或松弛方程更新。计算量为 `O(cols)`，最适合低端设备；但摇晃液滴、局部卷曲与自然碎块需要额外合成，TC-12 主观自然度风险高。保留为低端降级候选，不作为 P-001 v1 主路线。

### 2.3 `sph-v1`（REJECTED）

粒子 SPH 可获得最真实的界面与卷曲，但邻居搜索、压力求解与粒子数量对端侧算力和内存压力过高，难以稳定达到 `fpsP95 >= 100`。在当前鸿蒙端侧约束下否决。

## 3. 方案思路

### 3.1 状态定义

内部状态使用紧凑数组：

| 数组/变量 | 含义 |
|---|---|
| `cells: Uint8Array` | 格子类型：`EMPTY / SAND / WALL`，本方案中两相液体均使用 `SAND` 类型。 |
| `phase: Uint8Array` | 相位：`0=重相`，`1=轻相`。 |
| `shade: Uint8Array` | 渲染颗粒纹理，不参与物理判定。 |
| `active: Uint8Array` 或等价集合 | 标记当前帧需要检查的液体格子；静默区可跳过。 |
| `heavyCount / lightCount` | 两相格子数，只能在初始化时写入，运行期只读。 |
| `gravity` | 归一化重力方向；`invert180()` 只翻转该向量，不私改数组。 |
| `shakeEnergy` | 归一化摇晃能量，`0~1`，由脚本注入并按时间衰减。 |
| `dormant` | 静默标记；无输入且连续 12 个物理子步无状态变化后置 true。 |

推荐物理频率为 `120Hz`，固定子步 `dt=1/120s`。若渲染帧间隔超过 `1/60s`，单帧最多补两个子步，不得把真实帧间隔直接当作一个大 `dt`。

### 3.2 数学与逻辑原理

1. **精确守恒**：所有运行期操作只改变格子位置或交换两格相位，不创建、不删除、不改类型，因此任意时刻满足：

   `heavyCount(t) = heavyCount(0)`，`lightCount(t) = lightCount(0)`。

2. **浮力势能下降**：令 `s` 为沿重力方向递增的坐标，相位密度 `rhoHeavy > rhoLight`，势能定义为 `U = -sum(rho_phase * s)`。重相在轻相上方时，一次向下交换使 `Delta U = -(rhoHeavy-rhoLight) < 0`，系统向稳定分层演化。

3. **压力门控沉降**：重相只有在其反重力侧仍被同相重相覆盖时，才允许与下方轻相交换。该条件模拟连续重相压力，避免孤立液滴无限穿透，并把交换集中在相界面附近，形成连续重排前沿。

4. **界面扩散铺平**：对相邻列的重相界面高度 `hHeavy[i]` 与总液面高度 `hTotal[i]`，当高度差不小于 2 格时，从高列向低列做一次界面相位交换或表面粒子迁移。宏观等价于 `dh/dt = D * d2h/dx2`，终态满足 `max(h)-min(h) <= 1` 格。

5. **摇晃混合**：当 `shakeEnergy > epsilon` 时，对相边界邻域内的异相相邻格按概率 `qMix = q0 * shakeEnergy` 做相位交换，同时允许液体在空格内做不超过 1 格的受控惯性移动。混合带宽度近似 `w(t) ~ sqrt(DMix * t)`，可在 6 秒强摇晃内达到可测峰值；停止后 `shakeEnergy` 按指数衰减，沉降与铺平接管。

6. **倒立重排时序**：设稳定单相层厚约为 `h` 格，界面前沿每列需约 `h` 次串行交换，物理频率为 `f=120Hz`，则重排时间的标定关系为：

   `T_rearrange ~= h / (pSettle * f)`。

   v1.0 初始取 `pSettle=0.08`，目标 `T_rearrange=2.5~3.5s`。允许在相同路线内用 0、1、2 三个固定随机种子校准到 `[0.06, 0.12]`；不得为单个用例临时改参。

### 3.3 伪代码

```text
function init(cols, rows, cellPx):
    allocate cells, phase, shade, active
    initialize lower region as heavy, upper region as light
    heavyCount = count(phase == 0)
    lightCount = count(phase == 1)
    gravity = (0, +1)
    shakeEnergy = 0
    dormant = false
    emit LIQ_EVENT init {boundary:true, cols, rows, cellPx,
                         initialHeavy:heavyCount, initialLight:lightCount}

function onInvert180(enabled):
    emit LIQ_EVENT invert {boundary:true, forceInvert:enabled}
    gravity = enabled ? (0,-1) : (0,+1)
    wake()

function onShake(intensity):
    if intensity > 0: emit LIQ_EVENT shake {boundary:true, intensity}
    targetShakeEnergy = clamp(intensity, 0, 1)
    wake()

function frame(realDt):
    accumulator += clamp(realDt, 0, 1/60)
    substeps = 0
    while accumulator >= 1/120 and substeps < 2:
        physicsSubstep(1/120)
        accumulator -= 1/120
        substeps += 1
    emit LIQ_FRAME {frameTimeMs, physicsTimeMs}
    every 250ms: emitMetrics()

function physicsSubstep(dt):
    shakeEnergy = approach(shakeEnergy, targetShakeEnergy, dt)
    if shakeEnergy > 0.001:
        shakeEnergy *= exp(-decayRate * dt) when input is stopped
        stirBoundaryCells(shakeEnergy)
    changed = settleAndLevel(gravity)
    if changed == 0 and shakeEnergy <= 0.001:
        idleSubsteps += 1
        if idleSubsteps >= 12: dormant = true
    else:
        idleSubsteps = 0
        dormant = false

function settleAndLevel(g):
    changed = 0
    for each active liquid cell in scan order from gravity-far side to gravity-near side:
        p = phase(cell)
        below = cell + g

        if cells[below] == EMPTY:
            moveOccupancy(cell, below)
            changed += 1
            continue

        if p == HEAVY and cells[below] == SAND and phase[below] == LIGHT:
            above = cell - g
            if cells[above] == SAND and phase[above] == HEAVY:
                if random01(seed) < pSettle:
                    swapPhase(cell, below)
                    changed += 1
                    continue

        if isSurfaceCell(cell):
            for neighbor in lateralNeighbors(cell):
                if columnTotalHeight[cell] - columnTotalHeight[neighbor] >= 2:
                    moveSurfaceLiquid(cell, neighbor)
                    changed += 1
                    break

    for each adjacent column pair (i, i+1):
        if abs(interfaceRow[i] - interfaceRow[i+1]) >= 2:
            swapPhaseAtInterface(highColumn, lowColumn)
            changed += 1
    return changed

function stirBoundaryCells(E):
    for each active boundary cell whose 8-neighborhood contains opposite phase:
        if random01(seed) < qMix * E:
            choose a valid opposite-phase neighbor
            swapPhase(cell, neighbor)
    for a bounded subset of surface cells:
        if random01(seed) < pInertia * E:
            moveOccupancyAtMostOneCell(cell, inertialDirection)

function emitMetrics():
    heavyCentroid, lightCentroid = weightedCentroids()
    interfaceRows = topmostHeavyRowPerColumn(ignoreLeftAndRight10Percent)
    interfaceRangeCells = (max(interfaceRows)-min(interfaceRows)) / cellPx
    mixedBandShare = mixedBandCells / (heavyCount + lightCount)
    emit LIQ_METRIC {
      heavyCount, lightCount, emptyCount,
      heavyCentroid, lightCentroid,
      interfaceRows, interfaceRangeCells, mixedBandShare,
      finite:true, simulationTime, dormant
    }
```

实现约束：

1. `swapPhase` 只交换 `phase`，不交换 `cells` 类型，因此两相体积恒定。
2. `moveOccupancy` 与 `swapPhase` 必须维护 `active` 与列高度缓存，避免每帧额外全量重建。
3. 随机数使用可注入固定种子的伪随机序列；正式测试必须记录种子。
4. 事件边界帧不参与相邻帧连续性判定，但必须通过事件日志与渲染方向交叉验证。
5. `dormant=true` 后停止随机探测，只有新事件唤醒；若此时界面高度差仍大于 1 格，必须触发一次确定性铺平而不是永久冻结。

## 4. 输入与输出数据

### 4.1 输入

| 输入 | 类型/范围 | 说明 |
|---|---|---|
| `rendermode` | 固定 `3` | 液体模式。 |
| `testmode` | `true` | 禁用真实传感器，输入只来自脚本或 Want 参数。 |
| `forceinvert` | `false/true` | TC-13 事件开关；初始化时必须为 `false`。 |
| `shake` | `0~1` | 脚本注入的归一化摇晃强度。 |
| `seed` | 非负整数 | 可选；用于复现随机搅拌。 |
| 初始化状态 | 模拟层固定初始化 | 重相在下、轻相在上，记录 `cols/rows/cellPx/initialHeavy/initialLight`。 |

### 4.2 输出

| 输出 | 频率 | 内容 |
|---|---|---|
| `LIQ_EVENT` | 事件触发时 | `init/reset/shake/invert/mode/model_error`，含 `boundary`、时间戳、`forceInvert`、目标倾角与摇晃强度。 |
| `LIQ_METRIC` | 每 250ms | 两相计数、质心、每列界面行、界面波动格数、混合带占比、有限数标记、仿真时间、静默状态。 |
| `LIQ_FRAME` | 每渲染帧 | `frameTimeMs/physicsTimeMs`，用于 FPS 与逐帧连续性诊断。 |
| 渲染帧 | 每帧 | 颜色可按当前预设映射，但物理结论只依赖模拟层。 |

## 5. 复杂度与端侧约束评估

设 `N=cols*rows`，`C=cols`，`A=活动液体格数`。

| 项目 | 复杂度 | 说明 |
|---|---|---|
| 最坏物理子步 | `O(N)` | 全格扫描兜底；单子步只允许一格垂直位移或一格界面交换。 |
| 典型物理子步 | `O(C+A)` | 静止时大部分格子非 active，只检查列高度与活动前沿。 |
| 指标输出 | `O(N)` | 每 250ms 一次，可复用列高度缓存。 |
| 内存 | `3N + O(C)` 字节级数组 | `cells/phase/shade` 各 1 字节，另加列高度与 active 标记。 |
| 分配 | 稳态 0 次堆分配 | 所有数组初始化时预分配，帧内禁止新建数组或字符串拼装进入物理路径。 |

端侧预算：

1. `physicsTimeMs.p95 <= 4ms`，为 `fpsP95 >= 100` 留出渲染与 UI 余量。
2. 若由屏幕尺寸推导的格子数超过 24,000，按整数倍增大 `cellPx` 或合并行列，保证最坏扫描规模受控；缩放参数必须写入 init 事件。
3. 单次摇晃期间活动格比例按不超过 30% 设计；若超过 30%，优先收缩搅拌边界而不是降低帧率。
4. 使用 `Uint8Array/Int32Array` 与整数坐标，不引入浮点粒子矩阵或邻居列表。
5. 渲染层可以做 1~2 像素界面抗锯齿，但不得反向修改物理状态。

## 6. 预期效果与验收指标

### 6.1 TC-11 / EC-OL-01：静置水平界面

**预期效果**：重相稳定在下、轻相在上；总液面与相界面近似水平，末 5 秒无自发流动、无零抖动伪影。

**客观验收**：

| 指标 | MUST-PASS | v1.0 目标 |
|---|---|---|
| 进程稳定性 | 无 crash/ANR/fatal/非预期退出 | 同左 |
| `phaseDrift` | 重、轻相均 `<0.001`，全时段逐样本判定 | 等于 0 |
| 稳态相序 | 重相质心位于轻相下方 | 同左 |
| `interfaceRangeCells` | TIERED `<=2` 格及格 | `<=1` 格 |
| 稳态相邻帧像素分类变化 | TIERED `<=0.10%` 及格 | `0%` |
| 数值有效性 | `LIQ_*` 全部有限 | 同左 |
| 事件纪律 | 无未输入触发的 `invert/shake/reset/model_error` | 同左 |

**主观验收**：EC-OL-01 的分层可读性、界面平滑度、稳定流畅度均 `>=6` 分，且 HG-OL-01~04、HG-OL-07 不触发。`U-OL-03`（1~2 像素界面）与 `U-OL-04`（真实感细节）为 v1.0 加分目标，不阻塞首轮验证，但问题 CLOSED 前必须转入常规软门槛。

### 6.2 TC-12 / EC-OL-02：摇晃暂时混合与恢复

**预期效果**：强摇晃产生连续的液滴、碎块或暂混带；停止后混合占比先升后降，2~3 秒内主要分层恢复，4 秒内达到稳定界面。

**客观验收**：

| 指标 | MUST-PASS | v1.0 目标 |
|---|---|---|
| `phaseDrift` | 重、轻相均 `<0.001`，激烈摇晃段逐样本判定 | 等于 0 |
| 混合曲线 | 混合段可测上升，恢复段带小噪声收敛下降 | 峰值 `>=20%`，终态 `<=2%` |
| `recoveryTime` | `<=4s` | `<=3s` |
| 恢复终态相序 | 重相在下、轻相在上 | 同左 |
| 恢复终态界面 | `<=3` 格 | `<=2` 格 |
| 永久乳浊 | 30 秒后不得存在 | 同左 |
| 事件对齐 | 摇晃边界与帧时间戳可对齐，无未输入倒立/reset | 同左 |

**TIERED 及格口径**：峰值混色占比、恢复时间、终态混色占比三项最低分 `>=6`，即峰值 `>=10%`、恢复 `<=4s`、终态混色 `<=5%`。v1.0 目标对应最低 8 分。

**主观验收**：动态自然度、恢复可信度、分层可读性、稳定流畅度均 `>=6` 分；HG-OL-01~05、HG-OL-07 不触发。`U-OL-02` 属于本场景核心目标，v1.0 验收不得以“可暂失败”豁免。

### 6.3 TC-13 / EC-OL-03：180° 倒立与翻回重排

**预期效果**：倒立与翻回均只改变重力方向，不旋转或私改物理数组；两相沿新的重力方向连续交换，重排时间自然，不出现整块瞬移或颜色跳变。

**客观验收**：

| 指标 | MUST-PASS | v1.0 目标 |
|---|---|---|
| `phaseDrift` | 重、轻相均 `<0.001` | 等于 0 |
| 相序 | 每次事件后符合当前重力坐标系，翻回后恢复正立相序 | 同左 |
| `rearrangeTime` | 每次均在 `2~4s` | `2.5~3.5s` |
| 连续性 | 相邻帧质心位移与像素分类变化连续，无整块瞬移 | 最大质心位移 `<=1` 格 |
| 事件纪律 | 必须经 `invert180()`/`forceinvert` 事件触发并带 boundary | 同左 |
| 倒立审计 | 不得绕过 API 私改物理数组 | 同左 |

**TIERED 及格口径**：重排时长与过程平滑度最低分 `>=6`，即时长落在 `2.0~4.0s` 且最大相邻帧质心位移 `<=2` 格。v1.0 目标对应最低 8 分。

**主观验收**：分层可读性、界面平滑度、动态自然度、恢复可信度、稳定流畅度均 `>=6` 分；HG-OL-01~07 全部不触发。`U-OL-01` 属于本场景核心目标，v1.0 验收不得以“可暂失败”豁免。

### 6.4 性能与稳定性

| 指标 | 验收阈值 |
|---|---|
| `fpsP95` | `>=100`，每个用例至少 100 个有效帧样本 |
| `physicsTimeMs.p95` | `<=4ms` |
| 增量内存 | 物理层稳态 0 次堆分配；长时运行无持续增长 |
| 测试轮次 | TC-11~13 各至少 3 轮，固定种子分别为 0/1/2 |
| 一致性 | 模拟层与像素层同指标冲突时判数据一致性失败，不得择优报告 |

## 7. 测试方法与闭环判定

1. **执行配置**：`rendermode=3`、`testmode=true`、`forceinvert=false`，初始化后禁用传感器。TC-12 摇晃与 TC-13 倒立只能由脚本或 Want 参数注入。
2. **TC-11**：静置 30 秒，采集至少 100 帧；末 5 秒为稳态段，逐样本判定漂移、相序、界面与像素变化。
3. **TC-12**：延续 TC-11 稳态，注入 `shake=1.0` 左右往复 6 秒，随后静置 30 秒；按事件边界划分混合段与恢复段，绘制 `mixedBandShare` 时间曲线。
4. **TC-13**：正立稳态后开启倒立，静置至少 8 秒；再关闭倒立，静置至少 8 秒。两次重排分别计算 `rearrangeTime`、相序与质心连续性。
5. **EC-OL-01~03**：每场景按 `docs/effect-cases.md` 执行两轮；同一维度两轮差值 `>=1` 时追加第三轮并取中位数。评分员不感知路线版本。
6. **PASS 条件**：同一路线内，TC-11~13 全部 MUST-PASS 通过、TIERED 最低分 `>=6`，`fpsP95>=100`，EC 适用维度 `>=6` 且无 HG 硬门槛触发。
7. **FAIL 分流**：若守恒或相序错误、参数无法满足时间窗，归因算法方案并迭代 v1.1；若算法与测试用例正确但事件/钩子/渲染实现错误，移交 architect；若用例阈值或数据口径疑似错误，上报 manager，不得私自修改用例。

## 8. 假设与风险

1. **ASSUMPTION-01**：设备渲染回调可提供 100Hz 以上有效帧；若硬件上限低于 100Hz，性能验收必须记录为环境 BLOCKED，不得放宽阈值。
2. **ASSUMPTION-02**：`LIQ_EVENT/LIQ_METRIC/LIQ_FRAME` 将由实现层补齐；缺失时正式结论只能是 `BLOCKED(missing_hook)`。
3. **ASSUMPTION-03**：TC-12 的“左右交替强 swipe”可等价映射为 `shake=1.0` 的脚本注入，且映射关系记录在报告中。
4. **ASSUMPTION-04**：当前倒立 OFF 后物理不回正若复现，优先按实现缺陷处理；本算法要求倒立事件只翻转重力向量，不改变相位数组。
5. **风险-01**：随机搅拌可能让部分液滴永久滞留远端。压力门控与界面铺平必须保证 30 秒内收敛；若不能收敛，v1.1 增加有限次“回流引导”，仍不得直接瞬移。
6. **风险-02**：大屏格子数可能超过预算。必须先缩放网格，不得通过降低物理频率换取帧率。
7. **风险-03**：渲染抗锯齿可能让像素混色与模拟混合带出现小幅差异。以模拟层为权威，像素层只要求趋势一致且报告差异。

## 9. 变更记录

- 2026-10-15：创建 v1.0。选定 `liquid-ca-m1p-v1.0`，登记 `column-relax-v1` 与 `sph-v1` 为 REJECTED 候选；建立 TC-11~13、EC-OL-01~03 的量化验收、复杂度预算、端侧约束与失败分流。
