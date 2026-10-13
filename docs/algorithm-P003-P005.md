# 算法方案：P-003 + P-005

> 版本：v1　日期：2026-10-13　状态：DRAFT  
> 维护人：designer  
> 关联问题：P-003（场景二“水中两种重量的沙”三相模型）+ P-005（场景二测试用例与判定脚本缺口）  
> 变更原因：2026-10-13，manager 指定本轮处理 P-003 + P-005；两问题共享同一组状态、指标与测试钩子，采用联合方案，避免算法与验收口径分裂。

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

再按表层相位组成扬起：

```text
liftH[c] = E * liftHRate * band[c] * surfaceHeavyShare[c] * dt
liftL[c] = E * liftLRate * band[c] * surfaceLightShare[c] * dt
```

`liftH/liftL` 分别从 `hs/hl` 转移到 `suspH/suspL`，并受 `suspH + suspL <= waterDepth` 约束。推荐 `liftHRate = 0.35/s`，`liftLRate = 0.55/s`。轻沙更易扬起，但重沙沉降更快；停止摇晃后两者均必然回到沉积态，不允许永久悬浮。

### 3.7 倒立与旋转

物理状态始终定义在“重力对齐坐标系”中，渲染坐标只做显示变换，不因倒立分支重写数据。

#### 普通倾斜

直接使用 `tiltDeg` 参与水面目标与安息角坡度计算；传感器/手势输入需做角速度限制：

```text
tiltDeg(t + dt) = tiltDeg(t) + clamp(targetTilt - tiltDeg(t), -maxOmega * dt, maxOmega * dt)
```

推荐 `maxOmega = 180°/s`，使翻转渐变而非跳变。

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

## 迭代记录

- 2026-10-13：v1 创建。原因：为 P-003 设计 `WaterSandField` 列标量三相模型，并为 P-005 提供量化指标、用例骨架与可测试钩子。
