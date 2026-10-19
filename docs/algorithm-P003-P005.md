# 算法方案：P-003 + P-005

> 版本：v2.4　日期：2026-10-14　状态：DRAFT  
> 维护人：designer  
> 关联问题：P-003（场景二“水中两种重量的沙”三相模型）+ P-005（场景二测试用例与判定脚本缺口）  
> 变更原因：2026-10-13，manager 指定本轮处理 P-003 + P-005；两问题共享同一组状态、指标与测试钩子，采用联合方案，避免算法与验收口径分裂。
> 2026-10-14 变更原因：处理 `docs/review-external-20261014.md` 指出的参数不联立、沉降缺通量限幅、90°/倒立事件语义冲突；追加 v2.1，保留 v1/v2 历史内容。
> 2026-10-14 v2.2 变更原因：处理外部复核续评——v2.1 的每帧固定预算随刷新率改变物理，且峰值余量过薄；改为 `vSandTopMax=28 列/秒`，并把峰值目标区间设为 `0.55~0.75`。
> 2026-10-14 v2.3 变更原因：处理外部复核 §8——写明峰值核算的有效沉降速度假设与敏感度、标注 `t90` 数值来源、统一子步记法，并补充倒立两帧对照验证；不改变既有验收阈值。
> 2026-10-14 v2.4 变更原因：manager 已将 `slopeLimit` 姿态修正 routed 为独立小修；追加相对坡度判据、与 v2.2 共享预算的交互和 tester 断言，developer 在 v2.2 合入完成后实现。
> 2026-10-14 复核确认：处理外部评审 §9——v2.3/v2.4 结论维持不变；仅澄清 §17.2 的 `liftScale` 口径并补 §17.3 的复算脚本，不升算法版本。

---

## 1. 问题与目标

P-003 需要在水中同时模拟重沙、轻沙与水相，并表现出沉降分选、安息角堆积、摇晃悬浮、再堆积与倒立相序。P-005 需要把上述行为拆成可直接执行的客观/主观测试用例与判定脚本指标。

当前仓库已有：

- `SandGrid.ets`：两相沙粒 CA，具备渗透、滑移与安息角雏形，但没有水相和三相守恒账本。
- `FluidField.ets`：两相液体列标量场，具备守恒迁移与平界面求解，但没有沙沉积、悬浮与安息角。
- 渲染层已有 `MODE_SAND / MODE_FLUID / MODE_HYBRID / MODE_LIQUID`，尚无 `MODE_WATERSAND`。

本方案新增独立的 `WaterSandField`，不改动既有两相模型；渲染层新增第四个物理渲染模式 `MODE_WATERSAND`。

---

## 2. 列标量三相模型

### 2.1 坐标与容量

- 容器为 2D 截面，沿水平方向离散为 `N` 列，列间距 `dx = 1`。
- 每列容量为 `H`，表示该列从“重力底部”向上的最大高度。
- `tiltDeg`：重力方向在列平面上的投影角；`0°` 为重力竖直向下，正角表示列号增大方向更低。
- `dt`：固定物理步长，建议 `1/120s`；帧间隔超过 `1/60s` 时拆成最多 2 个子步。
- 所有相均以“等效列高”为体积单位，不引入质量与密度换算误差。

### 2.2 状态定义

每列 `c` 保存 6 个标量：

| 状态 | 类型 | 含义 |
|---|---|---|
| `waterTop[c]` | `Float64Array` | 水/悬浮液自由表面高度，从当前重力底部向上计量 |
| `sandTop[c]` | 派生 | 沉积沙顶面高度，`sandTop[c] = hs[c] + hl[c]` |
| `hs[c]` | `Float64Array` | 已沉积重沙等效高度 |
| `hl[c]` | `Float64Array` | 已沉积轻沙等效高度 |
| `suspH[c]` | `Float64Array` | 悬浮重沙等效高度，分布于该列水体内 |
| `suspL[c]` | `Float64Array` | 悬浮轻沙等效高度，分布于该列水体内 |

补充量：

```text
waterDepth[c] = max(0, waterTop[c] - sandTop[c])
waterVolume[c] = max(0, waterTop[c] - sandTop[c] - suspH[c] - suspL[c])
Vw = Σ waterVolume[c]
VH = Σ (hs[c] + suspH[c])
VL = Σ (hl[c] + suspL[c])
```

列内沉积沙采用“重沙在下、轻沙在上”的粗粒化排序表示。该排序不是瞬时强排，而是通过有限速率渗透实现；渲染时允许 1~2 像素混合带，避免硬边界。

### 2.3 守恒不变量

任意操作只允许以下三种形式：

1. **同相空间转移**：`a[c] -= q; a[d] += q`。
2. **同相状态转移**：沉积与悬浮互转，如 `suspH[c] -= q; hs[c] += q`。
3. **同列表面交换**：重沙下沉、轻沙上浮，`hs += q; hl -= q`，两者总量不变。

禁止直接根据目标面重写整列，禁止先清空再按总量重建。初始化后记录 `Vw0/VH0/VL0`，每帧输出漂移：

```text
driftW = |Vw - Vw0| / max(Vw0, ε)
driftH = |VH - VH0| / max(VH0, ε)
driftL = |VL - VL0| / max(VL0, ε)
```

实现使用 `Float64Array`，每个转移量先被容量约束裁剪，再同时扣减与增加，保证数值误差远低于 0.1%。

---

## 3. 动力学公式

### 3.1 水面 leveling

给定当前 `sandTop` 与悬浮沙总量，先求目标平面：

```text
S = Vw + VH_susp + VL_susp
targetTop[c] = b + tan(tiltDeg) * (c - (N - 1) / 2)
targetTop[c] = clamp(targetTop[c], sandTop[c], H)
选择 b，使 Σ (targetTop[c] - sandTop[c]) = S
```

`b` 用二分求解，复杂度 `O(N log(H/ε))`。随后做**保守误差迁移**：

```text
err[c] = waterTop[c] - targetTop[c]
positiveErr = Σ max(err[c], 0)
budget = min(positiveErr, waterLevelRate * positiveErr * dt + ε)
for each donor with err[donor] > 0 and receiver with err[receiver] < 0:
    q = min(err[donor], -err[receiver], budget,
            waterTop[donor] - (sandTop[donor] + suspH[donor] + suspL[donor]),
            H - waterTop[receiver])
    waterTop[donor] -= q
    waterTop[receiver] += q
```

静置且 `tiltDeg = 0` 时，`targetTop` 为水平线；倾斜时水面目标斜率为 `tan(tiltDeg)`。该迁移从正误差列流向负误差列，`Σ waterTop` 不变；在 `sandTop/susp` 固定的该子步内，`Vw/VH/VL` 均不变。禁止对每列独立 lerp 到目标面，否则会改变水相总量。

### 3.2 沉降与分选

> 2026-10-14：本节为 v1/v2 历史公式；生产实现以 §16.3 的 v2.2 速率化通量限幅为准。上文 `vH=16/vL=6` 与“2.7 倍”为 v1 历史参数；v2.2 生产默认为 `vH=64/vL=38`，重沙沉降速度约为轻沙 `64/38≈1.68` 倍。

假设每列悬浮沙在水深内近似均匀分布，沉降通量为：

```text
depth[c] = max(waterDepth[c], ε)
depH[c] = min(suspH[c], suspH[c] * vH * dt / depth[c])
depL[c] = min(suspL[c], suspL[c] * vL * dt / depth[c])
suspH[c] -= depH[c]; hs[c] += depH[c]
suspL[c] -= depL[c]; hl[c] += depL[c]
```

参数约束：

```text
vH > vL > 0
```

推荐默认值：`vH = 16` 列/秒，`vL = 6` 列/秒，重沙沉降速度约为轻沙 2.7 倍。该公式使悬浮量指数衰减，不会产生负值或瞬时清空。

### 3.3 重沙渗透与列内分选

> 2026-10-13 变更说明：本节 v1 公式状态为 **REJECTED**。原因：`hl[c] -= q; hs[c] += q` 把轻沙计成重沙，使 `VH` 增加 `q`、`VL` 减少 `q`，破坏 §2.3 的重沙/轻沙分相守恒；developer 禁止实现该公式。v2 修订见本文 §15，保留本节仅作历史记录。

新沉积重沙先落在沙堆表面，再以有限速率穿过上方轻沙层：

```text
q = min(hl[c], permRate * dt * (hs[c] + hl[c]) / H)
hl[c] -= q
hs[c] += q
```

该操作只改变重/轻沙在列内的上下排序，不改变 `VH + VL`。推荐 `permRate = 6/s`。

全局分选来自两个机制：

1. 重沙沉降快，先在注入区/沙堆内部形成核心。
2. 轻沙悬浮时间长，水平扩散更强；安息角滑移优先搬运表层轻沙，使其覆盖沙堆上部与外围。

### 3.4 安息角滑移

> 2026-10-14 v2.4 注记：本节 `abs(d)` 判据仅是 `tiltDeg=0` 的历史特例；生产实现必须使用 §18.2 的相对坡度判据 `abs(d - tan(tiltDeg))`。

设安息角为 `α`，邻列坡度阈值为：

```text
slopeLimit = tan(α)
```

对每对邻列 `(c, c+1)`：

```text
d = sandTop[c+1] - sandTop[c]
if |d| > slopeLimit:
    excess = |d| - slopeLimit
    q = min(excess / 2, maxAvalancheFlux * dt)
    从较高列顶部取出 q，加到较低列顶部
```

从高列顶部取沙时按“表层优先”规则：轻沙优先，轻沙不足时才移动重沙。取出的量按相位分别转移到低列，保证 `VH/VL` 不变。推荐 `α = 30°`，`maxAvalancheFlux = 24` 列²/秒。

每次 `step` 执行 `K = 4~6` 次松弛扫描，方向交替，消除扫描偏置。收敛后要求：

```text
max(|sandTop[c+1] - sandTop[c]|) <= tan(α + 2°)
```

### 3.5 悬浮沙水平扩散

对 `suspH/suspL` 使用保守有限体积扩散：

```text
qH = DH * dt * (suspH[c+1] - suspH[c])
qL = DL * dt * (suspL[c+1] - suspL[c])
suspH[c] += qH; suspH[c+1] -= qH
suspL[c] += qL; suspL[c+1] -= qL
```

每步将 `q` 裁剪到不会使任一端为负。推荐 `DH = 4` 列²/秒，`DL = 10` 列²/秒，使轻沙扩散范围更广。

### 3.6 摇晃、扬起与再堆积

> 2026-10-14：本节为 v1/v2 历史公式；生产实现以 §16.4 的 v2.2 能量目标、可用沙量与速率化通量约束为准。

引入全局扰动能量 `E ∈ [0,1]`：

```text
E(t + dt) = clamp(E(t) * exp(-shakeDecay * dt) + shakeInput(t), 0, 1)
```

推荐 `shakeDecay = 2.2/s`。每列先取一个表层带：

```text
band[c] = min(sandTop[c], max(0.5, tan(α)))
surfaceHeavyShare[c] =
    clamp((hs[c] - max(0, sandTop[c] - band[c])) / band[c], 0, 1)
surfaceLightShare[c] = 1 - surfaceHeavyShare[c]
```

再按表层相位组成扬起：

```text
liftH[c] = E * liftHRate * band[c] * surfaceHeavyShare[c] * dt
liftL[c] = E * liftLRate * band[c] * surfaceLightShare[c] * dt
```

`liftH/liftL` 分别从 `hs/hl` 转移到 `suspH/suspL`，并受 `suspH + suspL <= waterDepth` 约束。推荐 `liftHRate = 0.35/s`，`liftLRate = 0.55/s`。轻沙更易扬起，但重沙沉降更快；停止摇晃后两者均必然回到沉积态，不允许永久悬浮。

### 3.7 倒立与旋转

> 2026-10-14：本节为 v1/v2 历史语义；生产实现以 §16.6 的 v2.2 事件式重力坐标系切换语义为准。

物理状态始终定义在“重力对齐坐标系”中，渲染坐标只做显示变换，不因倒立分支重写数据。

#### 普通倾斜

直接使用 `tiltDeg` 参与水面目标与安息角坡度计算；传感器/手势输入需做角速度限制：

```text
tiltDeg(t + dt) = tiltDeg(t) + clamp(targetTilt - tiltDeg(t), -maxOmega * dt, maxOmega * dt)
```

推荐 `maxOmega = 720°/s`，使翻转紧跟手部姿态但仍保持连续渐变。

#### 180° 倒立事件

`invert180()` 触发一次坐标镜像与沙相再悬浮：

```text
for c in 0..N-1:
    c2 = N - 1 - c
    newSuspH[c2] += hs[c] + suspH[c]
    newSuspL[c2] += hl[c] + suspL[c]
    newHs[c2] = 0
    newHl[c2] = 0

按 Vw + VH + VL 重新求解 waterTop 平面
E = max(E, 0.8)
```

该事件只做同相转移，因此 `Vw/VH/VL` 精确不变。视觉上，倒立时通过 `ctx.rotate(π)` 围绕画布中心旋转显示；重力底部出现在视觉顶部，重沙仍向重力底部沉降，数据模型与渲染分支解耦。

---

## 4. `MODE_WATERSAND` 渲染方案

新增模式常量：

```text
MODE_WATERSAND = 4
```

渲染顺序：

1. **背景**：容器渐变与边框。
2. **水体**：用平滑 Path 绘制 `waterTop` 到 `sandTop` 的半透明蓝色区域；边缘忽略 5% 端点后做二次贝塞尔平滑。
3. **悬浮沙**：每列绘制 0~8 个确定性伪粒子。粒子种子由 `(column, slot, phase)` 决定，位置在 `[sandTop, waterTop]` 内随时间低速漂移；数量与透明度由 `suspH/suspL / waterDepth` 控制。避免每帧随机数导致闪烁。
4. **沙堆底层重沙**：从重力底部到 `smooth(hs)` 绘制重沙色渐变。
5. **沙堆上层轻沙**：从 `smooth(hs)` 到 `smooth(sandTop)` 绘制轻沙色渐变。
6. **混合带**：重/轻沙界面附近绘制 1~2 像素透明渐变；不允许硬锯齿边界。
7. **水面高光**：沿 `waterTop` 绘制 1 像素高亮线。
8. **调试层**：测试模式可选显示守恒量、坡度、悬浮比例与 FPS。

渲染要点：

- 物理分辨率 `N = 120~160` 列，不逐像素模拟。
- Path 采样后做两轮 3 点滑动平均，得到平滑沙丘轮廓。
- 倒立只改变画布变换矩阵，不复制或翻转物理数组。
- 渲染层不得写物理状态，测试像素统计与 `getTotals()` 可分别验证。

---

## 5. 参数映射

| 用户/配置项 | 算法参数 | 默认值 | 作用 |
|---|---:|---:|---|
| 沙粒粗细 | `N` | 128 | 列数，值越大轮廓越细 |
| 安息角滑块 | `α` | 30° | 控制沙堆最大坡度 |
| 重沙沉降速度 | `vH` | 16 | 重沙水中沉降列速 |
| 轻沙沉降速度 | `vL` | 6 | 轻沙水中沉降列速 |
| 分选强度 | `permRate` | 6/s | 重沙穿过轻沙的速率 |
| 水面流动性 | `waterLevelRate` | 8/s | 水面趋近平面的速率 |
| 摇晃强度 | `E` | 0~1 | 扬起与扰动能量 |
| 混合带宽 | `mixBandPx` | 1~2 | 重/轻沙视觉过渡 |
| 物理步长 | `dt` | 1/120s | 固定步长 |

参数必须满足：

```text
0 < vL < vH
0 < α < 45°
0 < permRate
0 <= E <= 1
```

---

## 6. 伪代码

```text
class WaterSandField(N, H):
    init(waterRegions, heavySand, lightSand, params):
        assert totalVolume <= N * H
        initialize six Float64Array state arrays
        record Vw0, VH0, VL0

    step(dt, tiltDeg, shakeInput):
        dt = min(dt, 1 / 30)
        substeps = ceil(dt / (1 / 60)); substeps = min(substeps, 2)
        for each substep:
            updateShakeEnergy(shakeInput, dt / substeps)
            relaxWaterTop(tiltDeg, dt / substeps)
            diffuseSuspension(dt / substeps)
            settleSand(dt / substeps)
            percolateHeavy(dt / substeps)
            relaxAngle(K passes, dt / substeps)
            liftByShake(dt / substeps)
            enforceCapacityAndNonNegative()
            auditConservation()

    relaxWaterTop(tiltDeg, dt):
        S = Vw + sum(suspH) + sum(suspL)
        target = solveClippedPlane(S, sandTop, tan(tiltDeg))
        migrate waterTop conservatively from positive-error columns
        to negative-error columns, with budget = rate * positiveError * dt

    settleSand(dt):
        for c:
            depth = max(waterTop[c] - sandTop[c], ε)
            dH = min(suspH[c], suspH[c] * vH * dt / depth)
            dL = min(suspL[c], suspL[c] * vL * dt / depth)
            suspH[c] -= dH; hs[c] += dH
            suspL[c] -= dL; hl[c] += dL

    splitTop(c, q):
        fromLight = min(hl[c], q)
        fromHeavy = q - fromLight
        hl[c] -= fromLight; hs[c] -= fromHeavy
        return (fromHeavy, fromLight)

    relaxAngle(K, dt):
        for pass in 0..K-1:
            leftToRight = pass is even
            for each adjacent pair in scan direction:
                d = sandTop[right] - sandTop[left]
                if abs(d) <= tan(α): continue
                q = min((abs(d) - tan(α)) / 2, maxFlux * dt)
                (qH, qL) = splitTop(highColumn, q)
                lowColumn.hs += qH; lowColumn.hl += qL

    invert180():
        create new zero arrays for hs, hl, suspH, suspL
        for c: c2 = N - 1 - c
            newSuspH[c2] += hs[c] + suspH[c]
            newSuspL[c2] += hl[c] + suspL[c]
        replace state arrays
        solve waterTop plane again
        E = max(E, 0.8)

    auditConservation():
        assert Vw, VH, VL are finite and non-negative
        assert drift < 0.1%
```

---

## 7. 复杂度与端侧评估

### 7.1 复杂度

- 水面平面求解：`O(N log(H/ε))`，`N=128` 时约 `128 × 20` 次简单加法。
- 沉降/渗透/容量检查：`O(N)`。
- 安息角松弛：`O(KN)`，`K=4~6`。
- 悬浮扩散：`O(N)`。
- 渲染：Path 数量为常数级，伪粒子为 `O(8N)`，无逐像素物理循环。

总复杂度：`O(KN + N log(H/ε))`。单帧物理操作约 `10^4` 量级。

### 7.2 内存

以 `N=128`、`H=192`、`Float64Array` 计算：

```text
6 个状态数组：6 × 128 × 8 = 6144 B
目标/临时数组：约 3 × 128 × 8 = 3072 B
渲染采样数组：约 4 × 128 × 4 = 2048 B
总计 < 16 KB
```

远低于端侧内存约束，且无持久粒子对象分配。

### 7.3 时延

- Node 单测环境物理步预期 `< 1ms`。
- ArkTS 真机物理步预期 `< 2ms`。
- Path 渲染预期 `< 3ms`。
- 单帧总预算 `< 5ms`，低于 100fps 的 10ms 帧预算，保留 2 倍以上余量。

若真机 P95 帧时间超过 10ms，优先降 `N` 到 96、悬浮粒子到每列 4 个，不改变物理公式。

---

## 8. 异常与边界条件

1. **初始化超容量**：若 `Vw + VH + VL > N × H`，直接抛出参数错误，不静默裁剪。
2. **负值与 NaN**：每次子步后检查 6 个数组；发现负值或 NaN 时终止测试并输出列号，禁止自动修正掩盖守恒错误。
3. **悬浮容量**：`suspH[c] + suspL[c] <= waterDepth[c]`；扬起量超容量时裁剪，不得突破水面。
4. **无水场景**：沙仍可沉降与安息角滑移，水面相关指标跳过，守恒指标仍必须通过。
5. **单相沙缺失**：允许 `VH=0` 或 `VL=0`，分选指标标记 `N/A`，守恒与安息角仍测试。
6. **近满容器**：水面求解被上下界裁剪后可能出现部分干列/满列；求解器必须返回可行解或报错，不得重置状态。
7. **极端倾斜**：`tiltDeg` 限制在 `[-89°, 89°]`；90° 横置需要未来切换主轴，不属于 v1 范围。
8. **连续倒立**：倒立事件可重复触发；每次都只做镜像与同相转移，`Vw/VH/VL` 不变。
9. **长时静置**：当输入不变、`suspFraction <= 0.001`、最大坡度低于安息角阈值且水面误差低于 `0.01` 列时进入休眠；任一条件变化立即唤醒，防止零漂移和耗电。
10. **模式切换**：切换出 `MODE_WATERSAND` 时冻结 `WaterSandField`，不把状态写入旧两相模型。

---

## 9. 客观量化验收指标

### 9.1 指标定义

#### 三相守恒

```text
maxDrift = max(driftW, driftH, driftL) × 100%
PASS: maxDrift < 0.1%
```

测试覆盖：静止、倾斜、180° 倒立、往复摇晃、倒立后摇晃、长时静置。

#### 分选比

按每列沙堆高度分成下 1/3 与上 1/3：

```text
H_low = Σ min(hs[c], sandTop[c] / 3)
H_high = Σ max(0, hs[c] - 2 * sandTop[c] / 3)
L_low = Σ min(hl[c], sandTop[c] / 3)
L_high = Σ max(0, hl[c] - 2 * sandTop[c] / 3)
lowerHeavyShare = H_low / max(H_low + L_low, ε)
upperHeavyShare = H_high / max(H_high + L_high, ε)
separationRatio = upperHeavyShare / max(lowerHeavyShare, ε)
PASS: separationRatio <= 0.5
```

若上下区均为空或单相沙缺失，标记 `N/A`，不得计 FAIL。

#### 沉降时序

```text
depH(t) = Σ hs[c] / VH0
depL(t) = Σ hl[c] / VL0
t90H = first time depH(t) >= 0.9
t90L = first time depL(t) >= 0.9
PASS: t90H < t90L and t90H / t90L <= 0.7
```

同时校验配置参数满足 `vH > vL`。

#### 安息角

```text
slopeDeg = atan(max |sandTop[c+1] - sandTop[c]|)
curvature = max |sandTop[c+1] - 2*sandTop[c] + sandTop[c-1]|
PASS: slopeDeg <= α + 2° and curvature <= 0.35 列
```

曲率指标用于防止分段直线或局部尖峰。

#### 水面水平与倾斜

静置 3 秒后：

```text
waterSlopeDeg = atan(max |waterTop[c+1] - waterTop[c]|)
waterAmplitude = max(waterTop) - min(waterTop)，忽略两端 10% 列
PASS: waterSlopeDeg <= 2° and waterAmplitude <= 2 像素
```

倾斜输入时，稳态水面斜率应满足：

```text
|measuredSlope - tan(tiltDeg)| / max(|tan(tiltDeg)|, ε) <= 15%
```

#### 倒立相序

倒立后静置 4 秒，在重力坐标系内计算：

```text
phaseOrderScore = lowerHeavyShare - upperHeavyShare
PASS: phaseOrderScore >= 0.30
```

同时要求视觉倒立变换为 180°，重沙位于视觉顶部对应的重力底部，数据层不因渲染分支改变。

#### 摇晃悬浮与再堆积

```text
suspFraction(t) = (Σ suspH + Σ suspL) / (VH0 + VL0)
peakSusp = max suspFraction during shake
residualSusp = suspFraction after 8s rest
PASS: peakSusp >= 0.5 and residualSusp <= 0.05
```

再堆积后必须再次通过分选比与安息角指标。

#### 无瞬移

```text
maxSandTopDelta = max |sandTop[t] - sandTop[t-1]| / dt
PASS: maxSandTopDelta <= 30 列/秒，单帧位移 <= 0.5 列
```

渲染若出现单帧超过 2 像素的轮廓跳变，主观测试也必须扣分。

#### 长时静置零漂移

静置 30 秒，取后 5 秒：

```text
bedDrift = max |sandTop[end] - sandTop[start]|
PASS: bedDrift <= 0.25 列，且 maxDrift < 0.1%
```

#### FPS

```text
fpsP95 = 1000 / p95(frameTimeMs)
PASS: fpsP95 >= 100
```

测试期间必须开启物理步耗时与渲染帧耗时统计，禁止只在空闲界面测 FPS。

---

## 10. 可测试钩子

### 10.1 Node 单测接口

新增 `WaterSandField` 应暴露：

```ts
export class WaterSandField {
  constructor(cols: number, capacity: number)
  configure(params: WaterSandParams): void
  initialize(scene: WaterSandScene): void
  setTilt(deg: number): void
  setShakeInput(value: number): void
  invert180(): void
  step(dt: number): WaterSandStepResult
  getTotals(): { water: number; heavy: number; light: number }
  getDrift(): { water: number; heavy: number; light: number }
  getMetrics(): WaterSandMetrics
  snapshot(): WaterSandSnapshot
}
```

`WaterSandMetrics` 至少包含：

```text
driftWater, driftHeavy, driftLight
depositedHeavyFraction, depositedLightFraction
suspensionFraction
separationRatio
lowerHeavyShare, upperHeavyShare
t90Heavy, t90Light
maxSlopeDeg, maxCurvature
waterSlopeDeg, waterAmplitudePx
phaseOrderScore
maxSandTopDeltaPerSecond
physicsTimeMs
```

### 10.2 ArkTS/E2E 钩子

- 模式入口：`rendermode=4` 或面板显示 `模式:水沙`。
- 强制倒立：`forceinvert=true`，触发 `invert180()`，hilog 输出 `WS_EVENT invert180`。
- 测试模式：`testmode=true`，禁用传感器，完全由脚本/手势输入重力。
- 指标日志：每 250ms 输出一次 `WS_METRIC {json}`；事件帧立即输出 `WS_EVENT {type}`。
- FPS：渲染循环输出 `WS_FRAME {frameTimeMs, physicsTimeMs}`。
- 调试快照：测试模式可导出 `getMetrics()` JSON，供 `hdc shell hilog` 或自动化脚本采集。

### 10.3 判定脚本建议

新增 `test/water_sand_check.py`，输入 hilog 或录制 JSON：

1. 解析 `WS_METRIC/WS_EVENT/WS_FRAME`。
2. 分段提取：初始、摇晃、倒立、静置、长时静置。
3. 输出每项指标 `PASS/FAIL/N/A` 与实测值。
4. 支持 `--conservation --sorting --settling --angle --water --invert --shake --fps --all`。
5. 像素分类脚本复用该输出做交叉验证：水体、重沙、轻沙、混合带分类，模拟层与渲染层偏差应 `<= 5%`。

---

## 11. 客观测试用例骨架（给 tester）

| 用例 ID | 场景 | 判定 |
|---|---|---|
| `TC-WS-01` | 初始化后 60s，混合倾斜、倒立、摇晃序列 | 三相漂移均 `< 0.1%` |
| `TC-WS-02` | 混合注入后静置 10s | `separationRatio <= 0.5` |
| `TC-WS-03` | 同一初始悬浮分布，记录沉积比例曲线 | `t90H/t90L <= 0.7` |
| `TC-WS-04` | 静置后采样沙堆轮廓 | `slope <= α+2°`，`curvature <= 0.35` |
| `TC-WS-05` | 水平静置 3s | 水面斜率 `<= 2°`，波动 `<= 2px` |
| `TC-WS-06` | 倾斜 `15°/30°` 后稳态 | 水面斜率误差 `<= 15%` |
| `TC-WS-07` | 180° 倒立后静置 4s | 相序分数 `>= 0.30`，三相守恒 |
| `TC-WS-08` | 强摇晃 3s 后静置 8s | 峰值悬浮 `>= 0.5`，残余悬浮 `<= 0.05` |
| `TC-WS-09` | 摇晃后再堆积 | 分选、安息角、无瞬移全部 PASS |
| `TC-WS-10` | 静置 30s | 后 5s 沙堆漂移 `<= 0.25` 列 |
| `TC-WS-11` | 真机连续 30s 动态序列 | `fpsP95 >= 100` |

这些用例可直接对齐 `VV-07`（三相守恒/分类）、`VV-08`（分选、沉降、安息角）、`VV-09`（悬浮、倒立、动态性能）。

---

## 12. 主观评分标准（给 effector）

总分 5 分制，保留 1 位小数；单项 1=明显错误，2=较差，3=可接受，4=良好，5=优秀。

| 维度 | 权重 | 5 分标准 | 关键扣分 |
|---|---:|---|---|
| 水面观感 | 20% | 水面平滑、水平/倾斜方向正确、无锯齿 | 水面沙丘化、闪烁、斜率反向 |
| 沙丘轮廓 | 25% | 平滑沙丘、坡度自然、边缘无方块感 | 尖峰、台阶、轮廓抖动 |
| 重/轻沙分区 | 20% | 重沙内部/下部集中，轻沙上部/外围清晰，混合带自然 | 随机花斑、分区不可辨 |
| 摇晃与再堆积 | 20% | 扬起、悬浮、沉降、再堆积时序自然连续 | 粒子瞬移、永久悬浮、堆积失败 |
| 稳定性与缺陷 | 15% | 无卡顿、无闪烁、无破面、静止后完全稳定 | 单帧跳变、卡顿、颜色闪烁 |

主观验收：

```text
weightedScore >= 4.0
且任一单项 >= 3.0
且摇晃/倒立过程中无“瞬移、破面、永久悬浮”三类致命缺陷
```

主观用例建议：

| 场景 ID | 操作 | 观察重点 |
|---|---|---|
| `EC-WS-01` | 初始混合注入后静置 10s | 水面、沙丘、两沙分区 |
| `EC-WS-02` | 倾斜 15°/30° 往复 5 次 | 沙滑移与水面响应 |
| `EC-WS-03` | 强摇晃 3s 后静置 8s | 悬浮、再沉降、再堆积 |
| `EC-WS-04` | 180° 倒立再翻回 | 倒立相序与回复自然度 |
| `EC-WS-05` | 静置 30s | 零抖动、零漂移、无闪烁 |

---

## 13. ASSUMPTION

1. 场景二按 2D 截面建模，不考虑深度方向的第三维体积差异。
2. 沙相不可压缩、不溶于水，单位列高等效单位体积。
3. 每列悬浮相在竖直方向近似均匀，足以支撑沉降时序与视觉云雾效果。
4. 列内重沙/轻沙可用粗粒化上下层表示，不需要逐粒子记录位置。
5. 手势/传感器可提供连续倾角与归一化摇晃强度；测试模式可完全脱离传感器。
6. `MODE_WATERSAND` 可作为第 4 个渲染模式加入，不影响既有 0~3 模式。
7. 真机目标画布不超过 1080p 物理分辨率；若分辨率显著提高，需要重新标定渲染耗时。

---

## 14. 预期效果与闭环条件

- 客观闭环：`TC-WS-01..11` 全部 PASS，或单项合理标记 `N/A` 且必测项全部 PASS。
- 主观闭环：5 个主观场景完成评分，加权分 `>= 4.0`，无致命缺陷。
- 性能闭环：真机 `fpsP95 >= 100`，物理步 P95 `< 2ms`。
- 若任一客观或主观测试未达标，本方案保持 `OPEN`，由 designer 根据失败指标输出 v2；不得仅因实现完成或用例编写完成而关闭。

---

## 15. 方案 v2 — §3.3 守恒修订

> 版本：v2　日期：2026-10-13　状态：DRAFT  
> 修订原因：v1 §3.3 的 `hl -= q; hs += q` 破坏重沙/轻沙分相守恒，被 architecture 标记为阻塞项。本节只替换 §3.3 的渗透/列内分选机制，并修正分选指标的剖面计算口径；v1 的全部量化阈值、TC-WS/EC-WS 用例与主观标准保持不变，不得放宽。

### 15.1 候选方案对比

| 候选 | 核心思路 | 守恒性 | 分选效果 | 复杂度/稳定性 | 结论 |
|---|---|---|---|---|---|
| A. 有序层 + 混合表层账本 | 每列拆为 `bh/bl` 有序层与 `mh/ml` 混合表层；重沙下沉时等量轻沙上浮 | 每次交换对 `VH/VL` 的增量和严格为 0 | 可表达“重沙穿过轻沙”，配合差速沉降满足分选 | `O(N)`，新增 4 个状态数组，数值稳定 | **选定** |
| B. 标量反转量记账 | 在 `hs/hl` 之外仅记录“反转厚度” | 可做到分相总量不变 | 只能表达单一反转界面，无法表示多次沉积/滑移后的分层 | 实现简单但状态不完备，指标易被账本欺骗 | REJECTED |
| C. K 层元胞相标记迁移 | 每列 K 个小层，相邻层做重沙下移/轻沙上浮的等量交换 | 严格守恒 | 分层最精确，可复现更细混合带 | `O(NK)`，需新增 `2NK` 数组与快照口径，端侧成本高于必要 | 备选 fallback |
| D. 禁用渗透，仅依赖差速沉降与表层滑移 | `percolationRate` 有效置 0 | 守恒 | 初始注入可分选，但已混合沙堆的二次分选弱 | 最低复杂度 | REJECTED：与 TC-WS-01 “不得置 0 绕过 §3.3”冲突 |

选型理由：A 用最小状态扩展表达必需的垂直相序，同时保持 `O(N)`；它比 B 完备，比 C 轻量，符合 HarmonyOS 端侧算力、内存与时延约束。若后续主观混合带效果不足，C 可作为 v3 候选，但仍必须提供同样的分相守恒证明。

### 15.2 v2 状态与不变量

每列新增 4 个非负厚度账本：

```text
bh[c]  有序重沙层厚度（最底）
bl[c]  有序轻沙层厚度（位于 bh 之上）
mh[c]  混合表层中的重沙厚度
ml[c]  混合表层中的轻沙厚度
```

列内物理排序约定为：

```text
底部 bh → 中部 bl → 顶部混合层 mh + ml
```

与既有外部状态兼容：

```text
hs[c] = bh[c] + mh[c]
hl[c] = bl[c] + ml[c]
sandTop[c] = hs[c] + hl[c]
VH = Σ (hs[c] + suspH[c])
VL = Σ (hl[c] + suspL[c])
```

禁止直接改写 `VH0/VL0`，禁止按目标总量重建整列；所有操作只允许对同一相执行等量减增。

### 15.3 守恒渗透公式

渗透由两类等量交换组成：

1. **穿越交换**：混合表层的重沙穿过有序轻沙层；`q` 份重沙下移，`q` 份有序轻沙上移进入混合层。
2. **成对分选**：混合层内的重沙与轻沙成对转为有序层；`r` 份重沙进入 `bh`，`r` 份轻沙进入 `bl`。

```text
through = min(mh[c], bl[c])
pair    = min(mh[c], ml[c])
budget  = percolationRate * dt * (through + pair)

q = min(through, 0.5 * through, budget)
r = min(pair, 0.5 * pair, max(0, budget - q), mh[c] - q)

mh[c] -= q + r
bh[c] += q + r
bl[c] += r - q
ml[c] += q - r
```

约束：

```text
q >= 0, r >= 0
q <= bl[c], r <= ml[c]
q + r <= mh[c]
percolationRate > 0
```

推荐默认值仍为 `percolationRate = 6/s`。`0.5` 上限用于数值防过冲，不改变物理方向。v2 实现后不得输出 `percolation_disabled_pending_designer`。

### 15.4 伪代码

```text
percolateHeavyConservative(dt):
    for c in 0..N-1:
        if sandTop[c] <= ε:
            continue

        through = min(mh[c], bl[c])
        pair = min(mh[c], ml[c])
        if through + pair <= ε:
            continue

        budget = percolationRate * dt * (through + pair)
        q = min(through, 0.5 * through, budget)
        r = min(pair, 0.5 * pair,
                max(0, budget - q), mh[c] - q)

        mh[c] -= q + r
        bh[c] += q + r
        bl[c] += r - q
        ml[c] += q - r

        assert all operands are finite and non-negative

settleSandV2(dt):
    # 沉降先进入混合表层，再由渗透分选。
    for c:
        depth = max(waterTop[c] - sandTop[c], ε)
        dH = min(suspH[c], suspH[c] * vH * dt / depth)
        dL = min(suspL[c], suspL[c] * vL * dt / depth)
        suspH[c] -= dH; mh[c] += dH
        suspL[c] -= dL; ml[c] += dL

splitTopV2(c, q):
    # 安息角滑移/扬起从列顶取沙：优先混合层，再取 bl，最后取 bh。
    # 返回同相转移量；接收端一律加入自身混合层。
    fromMix = min(q, mh[c] + ml[c])
    mixScale = fromMix / max(mh[c] + ml[c], ε)
    qH = mh[c] * mixScale
    qL = ml[c] * mixScale
    remaining = q - fromMix

    fromLight = min(remaining, bl[c])
    qL += fromLight
    remaining -= fromLight

    fromHeavy = min(remaining, bh[c])
    qH += fromHeavy

    mh[c] -= mh[c] * mixScale
    ml[c] -= ml[c] * mixScale
    bl[c] -= fromLight
    bh[c] -= fromHeavy
    return (qH, qL)
```

`relaxAngle` 与 `liftByShake` 均调用 `splitTopV2`；接收列执行 `mh[c] += qH; ml[c] += qL`。表层搬运的相位组成连续，接收沙先落在混合表层，再由渗透分选，避免轮廓与颜色跳变。

### 15.5 守恒证明

对任一列的渗透操作：

```text
ΔVH_column = Δ(mh + bh + suspH)
           = (-(q + r) + (q + r) + 0)
           = 0

ΔVL_column = Δ(ml + bl + suspL)
           = ((q - r) + (r - q) + 0)
           = 0
```

其余操作同样只做同相等量减增：

- 沉降：`suspH→mh`、`suspL→ml`，只改变相内位置，不改变 `VH/VL`。
- 滑移/扬起：`splitTopV2` 返回 `(qH,qL)`，发送列减去的量与接收列/悬浮相增加的量按相完全相等。
- 倒立：`bh+mh+suspH→suspH`，`bl+ml+suspL→suspL`，只做同相镜像。
- 水面 leveling：只迁移 `waterTop`，且以 `sandTop/susp` 固定的该子步为前提，不产生跨相项。

任意操作序列是上述零增量操作的有限复合，因此在实数数学意义下：

```text
Σ hs + Σ suspH ≡ VH0
Σ hl + Σ suspL ≡ VL0
```

浮点实现必须在容量裁剪后同时执行减法与加法，禁止先清空再重建；使用 `Float64Array` 时舍入误差应远低于 `0.1%` 验收阈值。

### 15.6 分选指标口径

v1 §9.1 用聚合 `hs/hl` 推导上下区相占比，在 v2 的分层账本下不足以表示垂直相序。v2 只替换该指标的计算输入，阈值不变：

```text
lowerBound = sandTop[c] / 3
upperBound = 2 * sandTop[c] / 3
mixTotal[c] = mh[c] + ml[c]
mixStart[c] = bh[c] + bl[c]
mixedOverlap(c, a, b) =
    max(0, min(b, sandTop[c]) - max(a, mixStart[c]))

H_low[c] = min(bh[c], lowerBound)
         + mixedOverlap(c, 0, lowerBound)
           * mh[c] / max(mixTotal[c], ε)

L_low[c] = max(0, min(bh[c] + bl[c], lowerBound) - bh[c])
         + mixedOverlap(c, 0, lowerBound)
           * ml[c] / max(mixTotal[c], ε)

H_high[c] = max(0, min(bh[c], sandTop[c]) - upperBound)
          + mixedOverlap(c, upperBound, sandTop[c])
            * mh[c] / max(mixTotal[c], ε)

L_high[c] = max(0, min(bh[c] + bl[c], sandTop[c])
                  - max(bh[c], upperBound))
          + mixedOverlap(c, upperBound, sandTop[c])
            * ml[c] / max(mixTotal[c], ε)

lowerHeavyShare = Σ H_low / max(Σ (H_low + L_low), ε)
upperHeavyShare = Σ H_high / max(Σ (H_high + L_high), ε)
separationRatio = upperHeavyShare / max(lowerHeavyShare, ε)
PASS: separationRatio <= 0.5
```

`WS_SNAPSHOT` 必须在既有 `hs/hl/sandTop` 基础上追加导出 `bh/bl/mh/ml`（或等价的逐列相位剖面），供 tester 重算分选；这是 additive 扩展，不删除既有字段。`hs/hl` 仍分别等于 `bh+mh`、`bl+ml`。

### 15.7 复杂度与验收

- 时间：渗透、沉降、容量检查均为 `O(N)`，相比 v1 不增加扫描维度。
- 内存：新增 4 个 `Float64Array`，`N=128` 时增加 `4096 B`；总状态内存约 `10 KB`，完整运行态低于 `20 KB`。
- 时延：新增每列常数次比较、乘法与赋值，预计物理步增幅 `< 0.1ms`，仍满足 `physicsTimeMs P95 < 2ms`。
- 客观验收：`TC-WS-01..12` 全部按原阈值执行；TC-WS-01 必须保持 `percolationRate=6/s`，不得置 0，且三相漂移全程 `< 0.1%`。
- 单测验收：覆盖穿越交换、成对分选、随机操作序列下的 `driftHeavy/driftLight < 1e-12`（Node 参考阈值，不替代 E2E 的 `<0.1%`），以及滑移/扬起/倒立后的分相守恒。
- 主观验收：沿用 `EC-WS-01..08` 与 v1 评分表，`weightedScore >= 4.0` 且任一单项 `>= 3.0`，不得因实现 v2 降低任一评分锚点。

### 15.8 ASSUMPTION

1. `bh/bl/mh/ml` 的粗粒化分层足以表达场景二的视觉分选，不需要逐颗粒位置。
2. 混合表层内两相在竖直方向近似均匀，其与统计区间的重叠高度按体积比例归属相位。
3. `percolationRate` 同时控制穿越交换与成对分选，总交换量受 `0.5` 防过冲上限约束。
4. v2 需 architect 同步更新工程状态结构；在架构确认前，developer 仍不得实现 v1 §3.3。

---

## 16. 方案 v2.2 — 参数联立、速率化通量与倒立语义修订

> v2.3 注记：本节保留 v2.2 历史核算；峰值假设、`t90` 来源、生效目标区间、记法澄清与倒立验证的补充口径见 §17。

> 版本：v2.2　日期：2026-10-14　状态：DRAFT  
> 修订原因：外部评审指出 v2 默认参数无法同时满足 TC-WS-07/08、沉降在浅水时可一帧清空、以及 90° 事件与“连续 ±180°”/H06 措辞冲突；v2.1 初步修复后，复核续评指出每帧固定预算会随刷新率改变物理。v2.2 改用速率化共享预算，不放宽任何既有阈值，不改变 §15 的分相守恒账本与 `q/r` 渗透公式。v2.1 的 `maxSandTopFrameDelta=0.48 列/渲染帧` 表达式 **REJECTED**，原因是 120Hz 时等效 57.6 列/秒、30Hz 时仅 14.4 列/秒。

### 16.1 外部评审逐项评估

| 评审点 | 评估 | 依据与处理 |
|---|---|---|
| TC-WS-08 峰值/残余不联立 | **认同，且实际实现更不利** | 评审按 `E=1` 得峰值约 2.4%；当前 `E` 更新式在持续 `shakeInput=1` 时稳态约为 `1/2.2=45.5%`，峰值上限更低。v2.2 给出联立参数组与能量目标语义。 |
| TC-WS-07 倒立 4 秒余量不足 | **认同** | 以默认场景 `waterTop≈110、sandTop≈40` 估算，倒立后水深约 110，原 `vH=16/vL=6` 在 4 秒内沉积不足，且 `E=0.8` 会继续扬起。v2.2 提高沉降速度并核算余量。 |
| 沉降缺通量限幅 | **认同** | 当前 `depth=max(...,ε)` 时 `v*dt/depth` 被夹到 1，悬浮沙可一帧全沉。且重/轻两相分别限幅仍可能叠加突破限速，必须使用子步合计速率预算。 |
| 90°/G3/H06 语义冲突 | **部分认同** | 技术事实是“±89° 连续求解 + 跨 90° 事件切换重力坐标系”，不是单一连续求解器。v2.2 明确该边界；若 G3 要求严格无事件连续过 90°，需 manager 另立 v3 目标。H06 应禁止渲染层私改数据，不应禁止经审计的 `invert180()` 物理事件。 |
| 场景一方案未落 docs | **接受，但不属于当前方案范围** | 场景一（油水分层）是独立 P-001，可使用完全不同方案。建议 manager 另行路由 designer，不在本文预留或兼容。 |
| 签名通道未打通 | **认同，非算法层** | 阻塞安装与录制，建议 manager 路由 developer/构建维护者处理签名配置；tester 保留 BLOCKED，不得伪造安装结果。 |
| `fpsP95>=100` 需 120Hz 设备 | **认同，非算法层** | 算法性能预算不变；建议 manager 同步 goals 口径，tester 在低于 100Hz 环境输出 `N/A(environment_refresh_rate)`。 |
| 2D 列标量表达力边界 | **认同** | 本方案目标是平滑 2D 截面与可读分选，不承诺三维颗粒层次。建议 effector 按该边界评分；若目标要求三维高保真，需 manager 另立方案。 |

### 16.2 v2.2 参数组

| 参数 | v2 默认 | v2.2 默认 | 目的 |
|---|---:|---:|---|
| `vH` | 16 | **64** | 保证倒立 4s 沉降与 8s 残余悬浮 |
| `vL` | 6 | **38** | 保持 `vH>vL`，`t90H/t90L≈0.64` |
| `liftHRate` | 0.35/s | **36/s** | 强摇晃 3s 可悬浮超过 50% 总沙 |
| `liftLRate` | 0.55/s | **40/s** | 轻沙更易扬起，形成自然差异 |
| `shakeRampRate` | 无 | **8/s** | 能量约 0.125s 升到 1，避免瞬时跳变 |
| `shakeDecay` | 2.2/s | 2.2/s | 停止摇晃后快速衰减 |
| `vSandTopMax` | 无 | **28 列/秒** | 低于 30 列/秒限速；按 `substepDt` 分配，速率一致（积分离散差 `<0.5%`） |
| `percolationRate` | 6/s | 6/s | 不改变 v2 分选账本 |
| `maxAvalancheFlux` | 24 列²/s | 24 列²/s | 继续作为自然通量上限，但受子步速率预算约束 |

参数约束：

```text
0 < vL < vH
0 <= liftHRate <= liftLRate
0 < shakeRampRate, 0 < shakeDecay
0 < vSandTopMax <= 30
```

### 16.3 沉降速率化通量限幅

`step(frameDt)` 先确定子步数，再为每个子步建立共享速率预算。禁止使用“每渲染帧固定预算”：

```text
substeps = ceil(frameDt / (1/60)), clamped to [1, 2]
substepDt = frameDt / substeps
substepBudget[c] = vSandTopMax * substepDt
```

每个子步内，所有会改变 `sandTop` 的沉降、安息角滑移与扬起操作共享 `substepBudget[c]`。沉降使用两相合计限幅，禁止重/轻各自独立消耗全部预算：

```text
depth[c] = max(waterTop[c] - sandTop[c], ε)

desiredH = min(suspH[c], suspH[c] * vH * substepDt / depth[c])
desiredL = min(suspL[c], suspL[c] * vL * substepDt / depth[c])

scale = min(1, remainingSubstepBudget[c] / max(desiredH + desiredL, ε))
depH = desiredH * scale
depL = desiredL * scale
remainingSubstepBudget[c] -= depH + depL

suspH[c] -= depH; mh[c] += depH
suspL[c] -= depL; ml[c] += depL
```

浅水时 `depth→ε` 不再导致一帧清空；最多受 `vSandTopMax * substepDt` 约束。60Hz 单子步预算约 `0.467` 列，120Hz 约 `0.233` 列；30Hz 拆成两个 `1/60s` 子步时，每个子步仍约 `0.467` 列，合计速率保持 28 列/秒。该限幅只缩小转移量，减增仍同相成对执行，不改变 §15.5 的守恒证明。

### 16.4 摇晃能量与扬起限幅

v2.2 将 `shakeInput` 定义为扰动目标，而不是每秒注入量：

```text
if shakeInput > 0:
    target = clamp(shakeInput, 0, 1)
    E = clamp(E + clamp(target - E, -shakeRampRate*substepDt, shakeRampRate*substepDt), 0, 1)
else:
    E = E * exp(-shakeDecay * substepDt)
```

扬起仍从列顶调用 `splitTopV2`，但必须同时受三重约束：理论通量、当前可取沉积沙、子步剩余预算：

```text
band[c] = min(sandTop[c], max(0.5, tan(α)))
shareH = mh[c] / max(mh[c] + ml[c], ε)
shareL = 1 - shareH

desiredRate = E * band[c] * (liftHRate * shareH + liftLRate * shareL)
liftBudgetRate = remainingSubstepBudget[c] / substepDt
availableRate = sandTop[c] / substepDt
actualRate = min(desiredRate, liftBudgetRate, availableRate)
scale = actualRate / max(desiredRate, ε)

liftH = E * band[c] * liftHRate * shareH * substepDt * scale
liftL = E * band[c] * liftLRate * shareL * substepDt * scale
remainingSubstepBudget[c] -= liftH + liftL
```

随后按 `splitTopV2` 取沙并转入 `suspH/suspL`。安息角滑移同样从 `remainingSubstepBudget[c]` 扣减。预算只限制转移速率，不改变相位分配。

### 16.5 联立核算

> 2026-10-14 v2.3 注记：下表 `63.6%` 是 v2.2 在 `vBar≈52.5`（悬浮相重:轻约 50:50）隐含假设下的历史数值；按模型自身参数联立的生效预期见 §17.2，峰值目标区间修订为 `0.65~0.80`。

默认测试几何取 `waterTop≈110`、初始沉积沙 `sandTop≈40`、水深约 70；倒立事件后 `sandTop=0`、水深约 110。用一维均匀列模型核算，不包含扩散与滑移的次要修正。与 v2.1 不同，v2.2 核算显式包含 `vSandTopMax=28` 共享预算：

- **非绑定条件**：单个子步内 `desiredSettle + desiredLift + desiredAvalanche <= vSandTopMax * substepDt` 时，预算不改变自然通量。
- **绑定事实**：TC-WS-08 强摇晃期 `E≈1` 时理论扬起约 `21.9 列/秒`，悬浮量上升后沉降通量继续增加，两者合计超过 `28 列/秒`，预算会绑定；下表按“先沉降、后扬起”的共享预算顺序重算。
- **目标区间**：v2.2 历史目标为 `0.55~0.75`；v2.3 生效目标为 `0.65~0.80`，避免压在 `0.50` 验收阈值或悬浮相过轻导致的上沿风险。实测低于 `0.65` 或高于 `0.80` 都应反馈 designer 复核参数。

| 指标 | v2 结果 | v2.2 预测 | 阈值 |
|---|---:|---:|---|
| TC-WS-08 峰值悬浮 | 约 1%~2.4% | **约 63.6%** | v2.2 目标 `55%~75%`；v2.3 生效目标 `65%~80%`；验收 `>=50%` |
| TC-WS-08 静置 8s 残余 | 远小于 5% | **约 0.70%** | `<=5%` |
| TC-WS-07 4s 重沙沉积 | 约 50% | **约 95.0%** | 支撑 `phaseOrderScore>=0.30` |
| TC-WS-07 4s 轻沙沉积 | 约 20%~29% | **约 83.1%** | 支撑相序差 |
| TC-WS-03 `t90H/t90L` | 0.375 理论值 | **约 0.633~0.635**（30/60/120Hz） | `<=0.7` |
| 沙顶运动限速 | 浅水可全量沉降 | **28 列/秒** | `<=30 列/秒` |

v2 的 TC-WS-08 峰值区间下界来自当前实现的 `E≈45.5%` 稳态，上界来自评审假设的 `E=1`。v2.2 的 `E` 在约 0.125s 内升至 1；峰值约 63.6%、残余约 0.70% 是共享预算绑定后的结果。以上为设计核算，最终仍以 TC-WS 全链路实测为准；不得因接近阈值而放宽。

### 16.6 90° 与倒立事件语义

v2.2 明确采用**事件式重力坐标系切换**：

1. `tiltDeg ∈ [-89°, 89°]` 时连续求解，不做数据镜像。
2. 输入角跨越 `±90°` 时触发一次 `invert180()`：物理模型在重力坐标系中做同相镜像与再悬浮，输出 `WS_EVENT invert180`，并执行三相守恒审计。
3. 渲染层只做 180° 显示变换，不得私有改写 `WaterSandField` 状态。
4. 从 180° 回到 `±89°` 以内时再次触发事件切回连续求解区间。

这不是“单一连续过 90° 的列标量求解器”。如果 G3 的“连续 ±180°”被解释为禁止坐标系事件，当前 2D 列标量方案不满足，需要 manager 另立 v3 目标并评估格点/主轴切换模型。

H06 的正确语义应是：**渲染分支、UI 分支或脚本绕过 `WaterSandField` API 直接改写物理数组 = FAIL**；经 `invert180()` 执行、带事件日志且通过三相守恒审计的坐标切换不判 FAIL。该措辞需 effector/manager 同步澄清。

`maxSandTopDelta` 的无瞬移统计在 `init/reset/inject/invert180` 事件边界重置上一帧参考值；事件本身用守恒、事件日志和录像连续性判定。普通物理子步的沙顶运动受 `vSandTopMax * substepDt` 约束，`substepDt<=1/60` 时单子步不超过约 `0.467` 列。

### 16.7 兼容与实施影响

**影响已实现代码，developer 需要同步**：

- 默认参数改为 `vH=64`、`vL=38`、`liftHRate=36`、`liftLRate=40`。
- 新增 `shakeRampRate=8/s` 与 `vSandTopMax=28` 配置项及参数校验。
- `step()` 需要按子步创建/传递共享沙顶运动预算，预算为 `vSandTopMax * substepDt`，不得按渲染帧使用固定值。
- `settleSandV2`、`liftByShake`、`relaxAngle` 必须消耗同一预算；重/轻沉降用合计缩放。
- 摇晃能量更新改为目标驱动 ramp/decay。
- `invert180()` 后重置无瞬移统计的上一帧参考值，并输出事件。

**不改变已实现 v2 语义**：

- `bh/bl/mh/ml` canonical 账本与 `hs/hl/sandTop` 派生关系不变。
- §15.3 的 `q/r` 守恒渗透公式不变。
- `schemaVersion=2` 快照字段不需要因 v2.2 新增；若架构选择导出参数版本，应 additive 扩展，不删除字段。
- 分选指标、三相守恒、TC-WS/EC-WS 阈值不变。

**仅文档/流程澄清**：

- 事件式 90° 语义与 H06 解释。
- 2D 表达力边界。
- 场景一、签名、设备刷新率 N/A 语义的路由。

### 16.8 v2.2 验收补充

- 单测：浅水 `depth=ε` 时，单个子步 `sandTop` 运动量 `<= vSandTopMax * substepDt`。
- 单测：重/轻两相同时沉降时，合计运动量仍 `<= vSandTopMax * substepDt`，不得分别各占全部预算。
- 单测：持续 `shakeInput=1` 时 `E` 在约 0.125s 达到 1；停止后按 `2.2/s` 衰减。
- 单测：强摇晃 3s 的简化均匀列峰值悬浮落在 v2.3 生效目标 `0.65~0.80`（v2.2 历史 `0.55~0.75`），静置 8s 残余 `<=0.05`。
- 单测：倒立 4s 后 `phaseOrderScore>=0.30`，三相漂移 `<1e-12`（Node 参考阈值）。
- E2E：TC-WS-03/07/08/09 使用 v2.2 参数组重跑，所有既有阈值不变。

---

## 17. 方案 v2.3 — v2.2 复核收尾

> 版本：v2.3　日期：2026-10-14　状态：DRAFT  
> 修订原因：外部复核 §8 确认速率限幅机制闭环，但指出 `63.6%` 峰值隐含 `vBar≈52.5`、`t90` 来源未标注、§16.4 记法混用，以及倒立物理镜像与显示旋转需两帧对照验证。本节只收尾这些问题，不改变守恒账本、`vSandTopMax=28`、既有参数组和 TC-WS/EC-WS 验收阈值。

### 17.1 版本兼容

**影响实现/测试口径，developer 需要同步**：

- §16.4 伪码中的 `dt` 统一为 `substepDt`。这是记法修正，不改变公式行为；实现中应确认所有速率公式都使用当前物理子步时长。
- TC-WS-08 的设计目标区间由 v2.2 的 `0.55~0.75` 修订为 `0.65~0.80`。这只是设计诊断带，不改变既有硬验收 `peakSusp>=0.50` 与 `residualSusp<=0.05`，也不降低任何分档标准。
- 倒立事件建议增加两帧对照检查；该检查属于验证补充，不改变 `invert180()` 事件语义。

**不影响已实现代码**：

- `vH=64`、`vL=38`、`liftHRate=36/s`、`liftLRate=40/s`、`vSandTopMax=28 列/秒` 不变。
- `bh/bl/mh/ml` 守恒账本、`q/r` 渗透公式、共享沙顶预算和三相守恒审计不变。
- §17.2 的 `vBar` 与峰值核算是设计口径澄清，不是新的运行时参数。

### 17.2 峰值悬浮假设与敏感度

v2.2 的 `63.6%` 来自一维瞬态积分，隐含有效沉降速度 `vBar≈52.5`。反解关系为：

```text
f = suspensionFraction = 24.5 / (vBar - 14)
0.636 = 24.5 / (52.5 - 14)
```

该 `vBar` 对应悬浮相重:轻约 `50:50`，是一个可复算但未显式写明的历史假设。v2.3 改用模型自身参数联立推导悬浮相构成：

```text
band = tan(30°) ≈ 0.577
desiredLiftHeavy = 0.577 × 36 ≈ 10.39 列/秒
desiredLiftLight = 0.577 × 40 ≈ 11.55 列/秒
liftScale = remainingBudgetRate / desiredLiftRate ≈ 14 / 21.94 ≈ 0.639
```

> 2026-10-14 复核注记：上方 `10.39/11.55` 是已乘 `share=0.5` 的分相贡献，合计 `21.94`；若改用未乘 `share` 的 `20.785/23.094`，合计 `43.879`，统一缩放为 `14/43.879≈0.319`。两种口径等价，均得到 `6.63:7.37`，单一口径见 §19.1。

在强摇晃稳态、共享预算绑定且总通量为 `14 列/秒`（扬起与沉降各占一半预算）时，两相实际扬起通量约为 `6.63 : 7.37`。稳态下各相悬浮量满足 `S_i ∝ actualLift_i / v_i`，因此：

```text
S_H:S_L ≈ (6.63/64) : (7.37/38) ≈ 35:65
vBar = 0.35 × 64 + 0.65 × 38 ≈ 47.1
f = 24.5 / (47.1 - 14) ≈ 0.74
```

所以 v2.3 的生效峰值预期为 `0.70~0.75`，中心值约 `74%`；静置 8s 残余按同一 `vBar` 积分约 `0.5%~0.8%`。TC-WS-08 峰值设计目标区间修订为 `0.65~0.80`：低于 `0.65` 提示扬起不足，高于 `0.80` 提示悬浮相过轻或残余风险。该区间不替代、也不放宽既有验收阈值。

敏感度为：

```text
f(vBar) = 24.5 / (vBar - 14)
df/dvBar = -24.5 / (vBar - 14)^2
```

在 `vBar≈47.1` 处，`vBar` 每下降约 `2 列/秒`，峰值约上升 `4~5` 个百分点；若 `vBar` 降到约 `44.5`，峰值将接近 `0.80` 上沿。实测如偏离目标带，应优先记录分相悬浮构成并复核 `liftHRate/liftLRate` 与预算绑定顺序，不得通过放宽阈值闭环。

### 17.3 `t90` 来源与差异说明

闭式基准：

```text
constant depth, non-binding settling:
    t90_i = ln(10) × depth / v_i
    t90H / t90L = vL / vH = 38 / 64 ≈ 0.594
```

v2.2 表中的 `0.633~0.635` 是**一维数值积分结果**，不是闭式值。两者差异主要来自完整模型中 `depth=waterTop-sandTop` 随沉降逐渐变浅，使后续单位时间的指数衰减更快；子步离散只解释 30/60/120Hz 之间约 `0.002` 的微差（`<0.5%`）。数值结果 `0.633~0.635` 比 `0.594` 更保守，仍低于 `0.70` 阈值。

复算口径为：每个子步先计算当前 `depth` 与两相期望沉降，再按共享预算求同一 `scale`，随后推进 `suspH/suspL`；分别记录两相沉积分数首次达到 `0.9` 的时间。不需要引入新的闭式近似替代实测。

> 2026-10-14 复核注记：一维复算脚本与 `S0` 敏感度见 §19.2。默认 `S0≈36~40` 时比值约 `0.632~0.636`；若取 `S0≈14`，会得到约 `0.610`。因此 `0.611` 不是同一几何的反例。

### 17.4 速率一致表述

`vSandTopMax * substepDt` 保证的是**物理速率一致**，不是逐比特完全相同的轨迹。指数沉降的离散步长误差为 `O(v × substepDt / depth)`；在 30/60/120Hz 的规定子步划分下，积分离散差预期 `<0.5%`。因此 HG-04 仍按“单个物理子步”检查，30Hz 一帧两个子步必须分别判定，不得相加后套用 `0.5` 列。

### 17.5 倒立两帧对照验证

为防止物理层同相镜像与显示层 180° 旋转重复翻转，建议在 EC-WS-07 或专项 E2E 中增加事件相邻两帧检查：

1. 事件前一帧：记录 `tiltDeg`、物理六状态、渲染矩阵与至少一个可识别相位标记；确认未发生倒立显示。
2. 事件后首帧：确认恰好出现一次 `WS_EVENT invert180`；物理层只执行经审计的同相镜像，渲染层只应用一次 180° 显示变换。
3. 断言合成几何变换等于预期的单一 180° 语义：重沙在映射后的重力底部一侧，事件前后不出现“双翻转回正”或重力底部跳到对侧。
4. 三相守恒、快照重算与渲染像素交叉验证在同一事件边界继续生效。

若检查发现物理镜像与显示旋转叠加后回到事件前朝向，应判 FAIL，并优先排查渲染分支是否私有镜像或二次调用 `invert180()`。

### 17.6 范围外问题归属评估

> 2026-10-14 v2.4 更新：manager 已将 `slopeLimit` 姿态修正 routed 为独立小修，生效方案见 §18。

- **slopeLimit 姿态修正**：认同为算法层缺陷。当前 `abs(d)<=tan(α)` 只适用于重力竖直；倾斜时应比较 `abs(d - tan(tiltDeg))` 与 `tan(α)`，否则 `tiltDeg=45°` 时容器水平沙面可能不崩塌。该修正会改变倾斜场景动力学，超出 v2.3 收尾范围；建议 manager 决策是列入高优先级 v3，还是批准为独立小修并同步 developer/tester。
- **percolationRate 常数**：当前场景固定两种沙、固定水体与验收时限，`6/s` 可作为标定工程近似；但若扩展密度差、黏度或干沙区，应参数化为与 `Δρ/μ` 相关并增加干湿门控。该问题不阻塞 TC-WS/EC-WS 当前验收，建议 manager 登记为后续参数化任务。

---

## 18. 方案 v2.4 — slopeLimit 姿态修正独立小修

> 版本：v2.4　日期：2026-10-14　状态：READY_FOR_DEVELOPER  
> 修订原因：manager 路由决策将外部复核指出的 `slopeLimit` 姿态缺陷作为独立小修立即处理，不并入 v3。历史 §3.4/§6 的 `abs(d)<=tan(α)` 只在重力竖直时成立；倾斜时会把“相对重力水平的目标坡面”误当作超角，或在容器水平但重力系已超过安息角时不崩塌。本节只修正 `relaxAngle` 的触发判据与预算交互，不改变守恒账本、v2.2 参数组或既有验收阈值。

### 18.1 修正判据

`relaxAngle` 必须使用与 `relaxWaterTop` 相同的重力系有效倾角 `effectiveTiltDeg`，不得从渲染矩阵或设备姿态重新推导：

本修正只用于 `|effectiveTiltDeg| < 90°` 的连续求解区间；跨越 `±90°` 仍按 v2.2 的 `invert180()` 事件语义处理，不得在 `tan(90°)` 附近直接求解。

```text
targetSlope = tan(effectiveTiltDeg)
slopeLimit = tan(α)
d = sandTop[right] - sandTop[left]
deviation = d - targetSlope

if abs(deviation) <= slopeLimit:
    continue
```

触发条件等价于：

```text
abs(d - tan(effectiveTiltDeg)) > tan(α)
```

`α` 的语义为**崩塌角**：相对目标坡面的坡度超过 `tan(α)` 才触发滑移；未超过时保持静止。依据是本节判据成对定义了“触发”为 `abs(deviation) > tan(α)`、“不触发”为 `abs(deviation) <= tan(α)`，因此它不是最终收敛角本身。

`deviation > 0` 表示右列相对目标坡面过高，`right` 为 donor；`deviation < 0` 表示左列相对目标坡面过高，`left` 为 donor。以 `effectiveTiltDeg=45°`、`α=30°`、水平沙面 `d=0` 为例：`targetSlope=1`、`deviation=-1`、`abs(deviation)=1 > tan(30°)≈0.577`，因此必须从左列向右列滑移。

### 18.2 伪代码

```text
relaxAngleV24(K, effectiveTiltDeg, substepDt):
    targetSlope = tan(effectiveTiltDeg)
    slopeLimit = tan(α)

    for pass in 0..K-1:
        leftToRight = pass is even
        for each adjacent pair (left, right) in scan direction:
            d = sandTop[right] - sandTop[left]
            deviation = d - targetSlope
            if abs(deviation) <= slopeLimit:
                continue

            donor = deviation > 0 ? right : left
            receiver = deviation > 0 ? left : right
            excess = abs(deviation) - slopeLimit

            naturalQ = min(excess / 2, maxAvalancheFlux * substepDt)
            q = min(
                naturalQ,
                sandTop[donor],
                H - sandTop[receiver],
                remainingSubstepBudget[donor],
                remainingSubstepBudget[receiver]
            )
            if q <= ε:
                continue

            (qH, qL) = splitTopV2(donor, q)
            mh[receiver] += qH
            ml[receiver] += qL

            remainingSubstepBudget[donor] -= q
            remainingSubstepBudget[receiver] -= q
```

说明：

- `K=4~6` 次方向交替扫描保持不变；`maxAvalancheFlux` 仍是每次邻接交换的自然通量上限。
- `q` 同时受 donor 可取沙、receiver 容量、两列剩余子步预算约束；任一约束不足时本次交换缩小或跳过，下一子步继续。
- 一次 `q` 的滑移会让 donor 的 `sandTop` 下降 `q`、receiver 的 `sandTop` 上升 `q`，因此两列的 `remainingSubstepBudget` 都扣减 `q`。
- 保持 v2.2 的步骤顺序与共享预算，不新增预算参数，也不为滑移开辟旁路。

### 18.3 行为与守恒影响

**守恒**：`splitTopV2(donor, q)` 只按相位取出 `qH/qL`，接收列只做 `mh += qH`、`ml += qL`：

```text
ΔVH = -qH + qH = 0
ΔVL = -qL + qL = 0
ΔVw = 0
```

预算缩放只改变 `q` 的大小，不改变相位配对，因此三相守恒证明不受影响。

**分选**：表层仍按 `splitTopV2` 的轻沙优先/表层组成规则搬运，接收沙进入混合表层后再由既有 `q/r` 渗透分选。倾斜场景的超角滑移会更真实地向重力下坡方向搬运表层沙；`tiltDeg=0` 时 `targetSlope=0`，公式退化为历史 `abs(d)<=tan(α)`，直立静置与分选用例不应出现行为回退。

**安息角**：收敛目标从“绝对水平附近”改为“相对重力目标坡面附近”。倾斜稳态应满足：

```text
max |sandTop[c+1] - sandTop[c] - tan(effectiveTiltDeg)| <= tan(α + 2°)
```

**v2.2 预算交互**：安息角滑移继续计入每列 `remainingSubstepBudget`。若沉降已消耗大部分预算，滑移会被限速并延后到后续子步，这是预期的物理限速，不得通过提高 `vSandTopMax` 或绕过预算来加速通过测试。`vSandTopMax=28 列/秒` 与 HG-04 的单子步口径不变。

### 18.4 tester 断言点

1. **倾斜哨兵**：初始水平沙床、`effectiveTiltDeg=45°`、`α=30°` 时，首个可用子步必须发生非零滑移；收敛后满足 `max |d - 1| <= tan(32°)`。
2. **边界不动作**：构造 `d=tan(effectiveTiltDeg)+tan(α)` 与 `d=tan(effectiveTiltDeg)-tan(α)` 两个边界形态，在浮点容差内不得触发滑移；略微越界必须触发。
3. **直立回归**：`effectiveTiltDeg=0` 时既有 TC-WS-04 行为不变，`maxSlopeDeg<=32°`。
4. **相对坡度指标**：倾斜用例不得用绝对 `maxSlopeDeg` 判定；从快照计算 `max |d - tan(effectiveTiltDeg)| <= tan(32°)`。若需要新增专用指标，由 manager/tester 协调，不影响本判据。
5. **预算**：每个物理子步内 `max |ΔsandTop[c]| <= vSandTopMax * substepDt + tol`；30Hz 单帧两个子步分别检查，不得相加后套用 `0.5` 列。
6. **守恒与相位**：滑移前后 `VH/VL/Vw` 不变；Node 单测阈值 `<1e-12`，E2E 沿用三相漂移 `<0.1%`。
7. **排序不回退**：直立 TC-WS-02 的 `separationRatio<=0.5` 与既有分档阈值不变；倾斜哨兵只额外验证相对坡度与守恒。

### 18.5 实施顺序与兼容

- developer 先完成 v2.2 合入，再实现本小修，避免同一 `WaterSandField.ets` 冲突。
- 不新增参数、不修改快照 schema、不改变 `vH/vL/lift*`、`vSandTopMax`、`percolationRate` 或 TC-WS/EC-WS 阈值。
- 若架构或测试脚本需要新增 `maxRelativeSlopeDeg` 字段，由 manager 路由 architect/tester；本节的快照重算公式已足够先行验证。

---

## 19. 2026-10-14 外部评审 §9 复核处理记录（不升版）

> 结论：v2.3 与 v2.4 的算法结论均维持不变；本节只补两处可复算说明，并登记一条非算法路由。

### 19.1 `liftScale` 单一口径

外部评审 §9.1 指出 §17.2 的 `14/21.94` 与 `14/43.879` 两条口径容易混淆。统一采用“未乘 `share` 的总期望速率”口径：

```text
desiredLiftTotal = 20.785 + 23.094 = 43.879 列/秒
bindingLiftTotal = 14 列/秒
globalLiftScale = 14 / 43.879 ≈ 0.319

actualLiftHeavy = 20.785 × 0.319 ≈ 6.63 列/秒
actualLiftLight = 23.094 × 0.319 ≈ 7.37 列/秒
```

这与 §17.2 的“先乘 `share=0.5`、再按 `14/21.94≈0.639` 缩放”完全等价；后续 `S_H:S_L≈35:65`、`vBar≈47.1`、`f≈0.74` 不变。

### 19.2 `t90` 复算脚本与 `S0` 敏感度

一维复算伪代码如下：

```text
vH = 64; vL = 38
for S0 in {14, 36, 38, 40}:
    SH = SL = S0 / 2
    dt = 1/120
    while SH > 0.1 * S0 / 2 or SL > 0.1 * S0 / 2:
        depth = 70 + SH + SL
        SH = max(0, SH - SH * vH / depth * dt)
        SL = max(0, SL - SL * vL / depth * dt)
    record t90H, t90L, ratio
```

复算结果：

| `S0` | `t90H/t90L` | 说明 |
|---:|---:|---|
| 14 | 约 `0.610` | 非默认几何，接近外部评审独立复算值 |
| 36 | 约 `0.632` | 默认几何下沿 |
| 38 | 约 `0.634` | 默认几何中值 |
| 40 | 约 `0.636` | 默认几何上沿 |

因此 §17.3 的 `0.633~0.635` 对应默认 `S0≈36~40`；外部评审的 `0.611` 对应 `S0≈14`，不是同一预设。该场景中最大自然沉降速率约 `18.55 列/秒 < 28`，共享预算不绑定，差异主要来自时变 `depth`，不是预算节流。

### 19.3 非算法路由

外部评审 §9.6.1 指出的 EC-WS-07 Run B “0°→180° 连续旋转”措辞与事件式 90° 语义仍不一致。该问题不属于算法层，不修改本文件；建议 manager 路由 effector 将 Run B 标注为参考/压力运行，或改为 `0°→178°→0°`。

---

## 20. 2026-10-14 v2.5 底层蠕变（真机效果补强）

> 状态：REJECTED　后续路线：bottom-creep-v2.7

> 原因：v2.4 的 `relaxAngle()` 只通过 `splitTopV2()` 从沙堆顶部取样，导致倾斜时上表面可动而 `bh/bl` 底层几乎没有横向迁移；真机反馈为“下层沙子不动”。v2.5 在 `percolateHeavyConservative()` 与 `relaxAngle()` 之间新增 `creepBottom(dt)`。

### 20.1 规则

- 输入：`effectiveTilt`、新增参数 `bottomCreepRate`、独立 `vBottomCreepMax` 子通道预算。
- 方向：书面确认 `effectiveTilt > 0` 时左列为 donor、右列为 receiver；`effectiveTilt < 0` 时反向；`effectiveTilt = 0` 不动。
- 强度：`q <= bottomCreepRate * dt * min(1, |effectiveTilt| / 20°)`，同时受 donor 底层余量、receiver 容量和 `vBottomCreepMax=10 列/秒` 子通道预算约束。
- 响应：`maxTiltOmegaDegPerSec=720°/s`，传感器平滑改为 `0.4 旧值 + 0.6 新值`，降低“手已倾斜、沙仍滞后”的输入延迟。
- 取样：`splitBottom(donor, q)` 按 `bh:bl` 同比例取出底层重沙与轻沙，接收列写回 `bh/bl`，不改悬浮层。
- 守恒：`ΔVH = -qH + qH = 0`，`ΔVL = -qL + qL = 0`，`ΔVw = 0`。
- 预设：默认 `bottomCreepRate=36`；`thick` 真机调试预设为 `96`，用于保证轻微倾斜时底层边界肉眼可见。

### 20.2 验证口径

- 底层运动：倾斜前后 `bh[c]` 或 `bl[c]` 至少一列发生变化，且变化方向与 `effectiveTilt` 的下坡方向一致。
- 守恒：三相总量漂移继续沿用 `<0.1%` 阈值。
- 限速：底层蠕变单独检查 `q <= vBottomCreepMax * substepDt + tol`；不得占用或绕过 `vSandTopMax` 共享子步预算。
- 排序：不改变既有 `TC-WS-02` 分选阈值；底层蠕变只做等比例搬运，不引入新的重轻交换规则。

---

## 迭代记录

- 2026-10-13：v1 创建。原因：为 P-003 设计 `WaterSandField` 列标量三相模型，并为 P-005 提供量化指标、用例骨架与可测试钩子。
- 2026-10-13：追加 v2，并将 v1 §3.3 标记 REJECTED。原因：v1 渗透公式把轻沙改计为重沙，破坏 `VH/VL` 分相守恒；v2 引入有序层 + 混合表层账本，用等量对向交换实现守恒渗透，全部既有量化阈值不变。复盘见 `docs/retro-P003-P005-v1.md`。
- 2026-10-14：追加 v2.1。原因：外部评审发现默认参数与 TC-WS-07/08 阈值不联立、沉降缺整帧通量限幅、90°/H06 语义冲突；v2.1 给出自洽参数组、共享沙顶运动预算与事件式倒立语义，不放宽任何阈值。
- 2026-10-14：追加 v2.2。原因：外部复核确认参数组可行，但指出 v2.1 的每帧固定预算随刷新率改变物理且峰值余量过薄；v2.2 改为 `vSandTopMax=28 列/秒`，按共享预算重算峰值/残余，并设定峰值目标区间 `0.55~0.75`。
- 2026-10-14：追加 v2.3。原因：处理外部复核 §8——显式写明 `vBar` 来源与峰值敏感度，将峰值目标区间修订为 `0.65~0.80`，标注 `t90` 数值积分来源，统一 `substepDt` 记法，并补充倒立两帧对照验证；不改变既有验收阈值。
- 2026-10-14：追加 v2.4。原因：manager 将 `slopeLimit` 姿态修正 routed 为独立小修；本节给出相对坡度判据、预算交互与 tester 断言，developer 在 v2.2 合入完成后实现。
- 2026-10-14：外部评审 §9 复核确认。原因：v2.3/v2.4 结论维持不变；补充 `liftScale` 单一口径与 `t90` 复算脚本，登记 EC-WS-07 Run B 为非算法路由。
- 2026-10-14：追加 v2.5。原因：真机反馈 v2.4 上表面可动但底层不动；新增 `bottomCreepRate` 与 `creepBottom()`，按倾角方向等比例搬运 `bh/bl` 底层，保持三相守恒与共享子步预算。
