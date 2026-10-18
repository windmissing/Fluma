# 测试用例说明（test-cases）

> 维护人：tester。客观测试用例；验收指标必须 100% 覆盖 docs/algorithm.md 中的量化指标。

## P-001：（问题标题）— 对应算法版本：v1

| 用例 ID | 前置条件 | 输入 | 期望输出 | 判定标准 | 覆盖的验收指标 |
|---|---|---|---|---|---|
| TC-001 | | | | | |

---

## P-003 / P-005：场景二“水中两种重量的沙”客观测试用例（用例先行版）

> 版本：v1　日期：2026-10-13　对应算法：`docs/algorithm-P003-P005.md` v1 + `docs/architecture-P003-P005.md` v1  
> 说明：本节为客观测试，不覆盖主观观感；主观用例由 effector 维护在 `docs/effect-cases.md`。  
> 变更原因：P-005 要求先补齐可执行用例，不等待 P-003 修复；同时为 algorithm v1 §3.3 的分相守恒缺陷建立回归防线。

### 1. 公共前置与数据来源

除特殊说明外，所有用例均使用以下传感器隔离配置：

```text
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wssnapshot true
```

- `testmode=true` 必须禁用真实传感器；倾角、摇晃与倒立只能由测试脚本、Want 参数或 `forceinvert` 注入。
- 物理步长固定为 `1/120s`；帧间隔超过 `1/60s` 时按架构文档的子步规则执行，不得把帧间隔直接当作 `dt`。
- hilog 统一采集 tag `WaterSand`；`WS_EVENT` 标记分段边界，`WS_METRIC` 每 250ms 输出指标，`WS_FRAME` 输出帧耗时，`WS_SNAPSHOT` 输出六状态数组与初始总量。
- 像素交叉验证只用于渲染层与模型层一致性检查，不能替代模拟层三相守恒判定。
- 每个用例至少保留原始 hilog、判定 JSON/Markdown、录制帧索引与被测 HAP/commit 标识。

| 指标类别 | 主要数据来源 | 交叉数据来源 |
|---|---|---|
| 三相总量与漂移 | `WS_SNAPSHOT` 初始总量 + `WS_METRIC.driftWater/driftHeavy/driftLight` | `WS_SNAPSHOT` 六状态重算 |
| 分选、沉降、坡度、水面、相序、悬浮、瞬移 | `WS_METRIC` 对应字段 | `WS_SNAPSHOT.sandTop/hs/hl/suspH/suspL` 重算 |
| 分段边界 | `WS_EVENT init/reset/tilt/shake/invert180/dormant/wake/model_error` | 录制时间戳 |
| FPS | `WS_FRAME.frameTimeMs`，至少 100 个样本 | `WS_FRAME.physicsTimeMs` |
| 水体/重沙/轻沙/混合带像素占比 | 录制帧 + 颜色标定 | 最近 `WS_METRIC/WS_SNAPSHOT` |

### 2. 用例总览

| 用例 ID | 覆盖目标 | 核心阈值 |
|---|---|---|
| TC-WS-01 | 多扰动三相守恒 + §3.3 缺陷哨兵 | 三相漂移均 `< 0.1%` |
| TC-WS-02 | 静置分选 | `separationRatio <= 0.5` |
| TC-WS-03 | 沉降时序 | `t90H/t90L <= 0.7` |
| TC-WS-04 | 安息角与轮廓平滑 | `slope <= α+2°`，`curvature <= 0.35` |
| TC-WS-05 | 水平静置水面 | `waterSlopeDeg <= 2°`，幅度 `<= 2px` |
| TC-WS-06 | 倾斜稳态水面 | 斜率误差 `<= 15%` |
| TC-WS-07 | 倒立相序 | `phaseOrderScore >= 0.30`，三相守恒 |
| TC-WS-08 | 摇晃扬起与再沉降 | 峰值悬浮 `>= 0.5`，残余 `<= 0.05` |
| TC-WS-09 | 再堆积质量 | 分选、安息角、无瞬移全部达标 |
| TC-WS-10 | 长时静置零漂移 | 后 5s `bedDrift <= 0.25` 列，漂移 `< 0.1%` |
| TC-WS-11 | 动态 FPS | `fpsP95 >= 100` |
| TC-WS-12 | 模型-渲染三相分类一致性 | 各相占比偏差 `<= 5%` |

### 3. 详细用例

#### TC-WS-01 多扰动三相守恒与 §3.3 缺陷哨兵

- **前置条件**：
  - `rendermode=4`、`testmode=true`、`wssnapshot=true`、`wspreset=sorting`，`wscols=128`，`wscapacity=192`。
  - 初始场景必须同时包含水、沉积重沙与沉积轻沙；建议 `VH0 >= 0.10H`、`VL0 >= 0.10H`，避免空相导致缺陷不可见。
  - `percolationRate=6/s`，`vH=16`，`vL=6`，`α=30°`；本用例不得把渗透率置 0 来绕过 §3.3。
- **操作序列**：
  1. `reset` 后静置 1s，记录 `WS_SNAPSHOT` 初始 `Vw0/VH0/VL0`。
  2. 保持 `tilt=0°`、`shake=0` 静置 8s，专门让重沙渗透子机制运行。
  3. 依次注入 `tilt=15°` 5s → `tilt=-30°` 5s → `tilt=0°` 3s。
  4. 触发 `forceinvert=true`，倒立静置 4s；`forceinvert=false` 回正静置 4s。
  5. 注入 `shake=1.0` 3s，随后静置 8s。
  6. 重复第 3~5 步 3 轮；每步由 `WS_EVENT` 标记边界。
- **判定指标与阈值**：
  - 每帧/每 250ms 计算 `driftWater`、`driftHeavy`、`driftLight`。
  - 全时段及每个分段均要求 `driftWater < 0.001`、`driftHeavy < 0.001`、`driftLight < 0.001`。
  - 用 `WS_SNAPSHOT` 六状态重算 `Vw/VH/VL`，与 `WS_METRIC` 结果一致。
- **数据来源**：`WS_SNAPSHOT`、`WS_METRIC`、`WS_EVENT`。
- **通过标准**：三相漂移全程 `< 0.1%`，无 `model_error`，分段与全时段均 PASS。

**§3.3 缺陷暴露要求**：algorithm v1 的 `hl[c] -= q; hs[c] += q` 会把轻沙计成重沙。第 2 步 8s 静置必须使该机制积累转移量；按上述初始占比与 `permRate=6/s`，即使只转移 `0.001H`，在 `VH0=VL0=0.1H` 时也会造成约 1% 分相漂移，远超 0.1%。因此若实现照抄 v1 公式，本用例必须在第 2 步内 FAIL。若收到 `WS_EVENT percolation_disabled_pending_designer`，本用例判定为 `BLOCKED(pending_designer)`，不得判 PASS；designer 修订后保持相同输入重跑。

#### TC-WS-02 静置分选

- **前置条件**：`wspreset=sorting`，重沙与轻沙初始混合且均非空；`testmode=true`，`tilt=0°`，无摇晃。
- **操作序列**：`reset` → 记录初始快照 → 静置 10s → 记录末帧指标与快照。
- **判定指标与阈值**：
  - 按算法 §9.1 计算沙堆下 1/3 与上 1/3：`lowerHeavyShare`、`upperHeavyShare`。
  - `separationRatio = upperHeavyShare / max(lowerHeavyShare, ε) <= 0.5`。
  - 单相缺失或上下区均为空时输出 `N/A`，不计 FAIL，但三相守恒仍必须 PASS。
- **数据来源**：`WS_METRIC.separationRatio/lowerHeavyShare/upperHeavyShare`；`WS_SNAPSHOT.hs/hl/sandTop` 重算。
- **通过标准**：分选指标 PASS 且三相漂移 `< 0.1%`。

#### TC-WS-03 沉降时序

- **前置条件**：`wspreset=settling`，重沙与轻沙初始悬浮分布相同，`VH0 > 0`、`VL0 > 0`；配置 `vH=16`、`vL=6`，满足 `vH > vL`。
- **操作序列**：同一初始快照执行 3 次 `reset`；每次 `tilt=0°`、`shake=0` 连续运行 20s，记录 `WS_METRIC` 全序列。
- **判定指标与阈值**：
  - `depH(t)=Σhs/VH0`，`depL(t)=Σhl/VL0`。
  - `t90H=first(depH(t)>=0.9)`，`t90L=first(depL(t)>=0.9)`。
  - 3 次均要求 `t90H < t90L` 且 `t90H/t90L <= 0.7`；两个 `t90` 均必须存在。
- **数据来源**：`WS_METRIC.depositedHeavyFraction/depositedLightFraction/t90Heavy/t90Light`；`WS_SNAPSHOT` 重算。
- **通过标准**：3 次重复全部达标，且配置校验 `vH > vL`。

#### TC-WS-04 安息角与轮廓平滑

- **前置条件**：`wspreset=angle`，构造初始局部超高沙堆；`α=30°`，`testmode=true`，`tilt=0°`，无摇晃。
- **操作序列**：`reset` → 静置 10s → 取最后 5s 的 `sandTop` 序列。
- **判定指标与阈值**：
  - `slopeDeg = atan(max |sandTop[c+1]-sandTop[c]|)`，要求 `slopeDeg <= α+2° = 32°`。
  - `curvature = max |sandTop[c+1]-2*sandTop[c]+sandTop[c-1]|`，要求 `<= 0.35` 列。
  - 最后 5s 内 `maxSandTopDeltaPerSecond <= 30` 列/秒，单帧位移 `<= 0.5` 列。
- **数据来源**：`WS_METRIC.maxSlopeDeg/maxCurvature/maxSandTopDeltaPerSecond`；`WS_SNAPSHOT.sandTop` 重算；录制帧用于轮廓交叉检查。
- **通过标准**：坡度、曲率、瞬移三项全部 PASS，且三相守恒 `< 0.1%`。

#### TC-WS-05 水平静置水面

- **前置条件**：`wspreset=default`，水量足以形成清晰自由表面；`testmode=true`，`tilt=0°`，无摇晃。
- **操作序列**：`reset` → 静置 3s → 连续采样 5s。
- **判定指标与阈值**：
  - 忽略两端各 10% 列后，`waterSlopeDeg <= 2°`。
  - `waterAmplitude = max(waterTop)-min(waterTop) <= 2px`；像素换算必须记录 `pixelScale`。
- **数据来源**：`WS_METRIC.waterSlopeDeg/waterAmplitudePx`；`WS_SNAPSHOT.waterTop`；录制帧水面像素。
- **通过标准**：斜率与波动均达标，水面无锯齿状单列突变，三相守恒 `< 0.1%`。

#### TC-WS-06 倾斜稳态水面

- **前置条件**：`wspreset=default`，`testmode=true`，初始水平稳态。
- **操作序列**：依次注入 `tilt=15°`，等待 4s 后采样 5s；再注入 `tilt=30°`，等待 4s 后采样 5s；最后回正 `tilt=0°` 采样 5s。
- **判定指标与阈值**：
  - 对有效列做线性回归得到 `measuredSlope`。
  - 倾斜段要求 `|measuredSlope-tan(tiltDeg)| / max(|tan(tiltDeg)|, ε) <= 15%`。
  - 回正段复用 TC-WS-05 阈值。
- **数据来源**：`WS_EVENT.tilt.targetDeg/effectiveDeg`、`WS_SNAPSHOT.waterTop`、`WS_METRIC.waterSlopeDeg`。
- **通过标准**：15°、30°、回正三段全部 PASS；`effectiveDeg` 与目标角过渡连续，无跳变。

#### TC-WS-07 倒立相序与守恒

- **前置条件**：`wspreset=default`，正立稳态且重/轻沙均存在；`testmode=true`，`tilt=0°`，无摇晃。
- **操作序列**：触发 `forceinvert=true` → 静置 4s → 采样 3s → 触发 `forceinvert=false` → 静置 4s → 采样 3s；重复 2 轮。
- **判定指标与阈值**：
  - 在重力坐标系内计算 `phaseOrderScore = lowerHeavyShare - upperHeavyShare`，要求 `>= 0.30`。
  - 视觉层必须为 180° 旋转，重沙位于视觉顶部对应的重力底部。
  - 倒立与回正全程三相漂移均 `< 0.1%`。
- **数据来源**：`WS_EVENT invert180`、`WS_METRIC.phaseOrderScore/drift*`、`WS_SNAPSHOT`；录制帧用于相序交叉验证。
- **通过标准**：两轮倒立/回正相序正确、过渡连续、守恒 PASS，无 `model_error`。

#### TC-WS-08 摇晃扬起与再沉降

- **前置条件**：`wspreset=default`，已形成沙堆且 `VH0+VL0>0`；`testmode=true`，`tilt=0°`。
- **操作序列**：静置 3s → 注入 `shake=1.0` 持续 3s → 停止摇晃静置 8s → 记录全程 `suspensionFraction`。
- **判定指标与阈值**：
  - 摇晃段峰值 `peakSusp >= 0.5`。
  - 静置 8s 后 `residualSusp <= 0.05`。
  - 全程三相漂移 `< 0.1%`，无永久悬浮。
- **数据来源**：`WS_EVENT shake`、`WS_METRIC.suspensionFraction/drift*`、`WS_SNAPSHOT.suspH/suspL`。
- **通过标准**：峰值、残余、守恒全部 PASS；若 `peakSusp` 未达标但画面有扬起，仍判 FAIL，不能放宽阈值。

#### TC-WS-09 摇晃→悬浮→再堆积质量

- **前置条件**：延续 TC-WS-08 的同一初始场景；`α=30°`，重/轻沙均存在。
- **操作序列**：注入 `shake=1.0` 3s → 静置 10s → 采样末段 5s。
- **判定指标与阈值**：
  - 再堆积后 `separationRatio <= 0.5`。
  - `maxSlopeDeg <= 32°`，`maxCurvature <= 0.35`。
  - `maxSandTopDeltaPerSecond <= 30` 列/秒，单帧位移 `<= 0.5` 列。
  - 末段 `residualSusp <= 0.05`，三相漂移 `< 0.1%`。
- **数据来源**：`WS_EVENT shake`、`WS_METRIC` 全量指标、`WS_SNAPSHOT`；录制帧由 `water_sand_pixel_check.py` 分类。
- **通过标准**：分选、安息角、无瞬移、残余悬浮与守恒全部 PASS；任一项 FAIL 则整个用例 FAIL。

#### TC-WS-10 长时静置零漂移

- **前置条件**：`wspreset=sorting`，已经完成初始沉降并形成稳定沙堆；`testmode=true`，`tilt=0°`，`shake=0`。
- **操作序列**：连续静置 30s；在 25s 与 30s 各输出/请求一次 `WS_SNAPSHOT`，并保留全量 `WS_METRIC`。
- **判定指标与阈值**：
  - 后 5s `bedDrift = max |sandTop[end]-sandTop[start]| <= 0.25` 列。
  - 后 5s 三相漂移均 `< 0.1%`。
  - 期间不得出现未由输入触发的 `wake/model_error`。
- **数据来源**：`WS_SNAPSHOT.sandTop` 或 `WS_METRIC.bedDrift`（二者至少一个必须可用）；`WS_METRIC.drift*`；`WS_EVENT dormant/wake/model_error`。
- **通过标准**：沙堆零漂移与三相守恒均 PASS；如果缺少 `bedDrift` 或指定时间点快照钩子，判 `BLOCKED(missing_hook)`，不得用肉眼观察替代。

#### TC-WS-11 动态 FPS

- **前置条件**：真机、120Hz 或以上显示刷新率；`rendermode=4`、`testmode=true`，默认分辨率与默认参数；确认日志采集本身不是瓶颈。
- **操作序列**：连续 30s 动态序列：`tilt=15°` 5s → `tilt=-30°` 5s → `shake=1.0` 3s → 静置 5s → 倒立/回正各 4s → 剩余时间回正动态倾斜；全程开启物理与渲染统计。
- **判定指标与阈值**：
  - 至少 100 个 `WS_FRAME` 样本，`fpsP95 = 1000 / p95(frameTimeMs) >= 100`。
  - 记录 `physicsTimeMs` P95 作为性能诊断，验收主判为 FPS。
  - 60Hz 或低于 100Hz 刷新率环境输出 `N/A(environment_refresh_rate)`，不得计 FAIL。
- **数据来源**：`WS_FRAME.frameTimeMs/physicsTimeMs`、`WS_EVENT` 分段、设备刷新率记录。
- **通过标准**：样本数足够且 `fpsP95 >= 100`；样本不足判 `BLOCKED(insufficient_samples)`。

#### TC-WS-12 模型-渲染三相分类一致性

- **前置条件**：完成 TC-WS-01、TC-WS-05、TC-WS-08、TC-WS-09 的录制帧；有水体、重沙、轻沙颜色标定；`wsmixband=1~2`。
- **操作序列**：对每个采样帧执行水体/重沙/轻沙/混合带分类，并与时间最近的 `WS_METRIC` 或 `WS_SNAPSHOT` 占比对齐。
- **判定指标与阈值**：
  - 水体、重沙、轻沙三类像素占比与模型占比偏差均 `<= 5%`（绝对百分点）。
  - 混合带只用于视觉解释，不参与三相守恒统计。
  - 帧间分类突变需结合 TC-WS-04/09 的无瞬移指标判断。
- **数据来源**：录制帧、颜色标定文件、`WS_METRIC`、`WS_SNAPSHOT`。
- **通过标准**：三个关键帧以上全部偏差 `<= 5%`；本用例是交叉验证，不能替代模拟层守恒。

### 4. 判定脚本与回归防线

- `test/water_sand_check.py`：解析 `WS_METRIC/WS_EVENT/WS_FRAME/WS_SNAPSHOT`，评估 TC-WS-01..11 指标并输出 JSON/Markdown；当前骨架已提供分段边界解析函数，逐分段守恒统计待事件时间戳钩子落地后补齐。
- `test/water_sand_pixel_check.py`：解析录制帧与颜色标定，输出三相/混合带像素占比，并与模型占比交叉验证。
- `test/record_tc_ws.py`：编排真机启动、Want 参数注入、hilog/连拍采集与两个判定脚本调用。
- 脚本当前为待钩子实现的骨架；缺失 `WS_*` 字段时必须输出 `BLOCKED(missing_hook)`，不得默认 0 或用主观判断替代。
- §3.3 回归防线：`water_sand_check.py` 的守恒评估必须逐样本检查三相漂移，并保留 `--self-check-defect` 模式验证“重沙增加、轻沙减少”的合成缺陷会被判 FAIL。

## 变更记录

- 2026-10-13：初始创建。
- 2026-10-13：追加 P-003/P-005 场景二客观用例 TC-WS-01..12 与 WS 钩子数据源映射。原因：P-005 测试先行，补齐三相守恒、分选、沉降、安息角、水面、倒立、摇晃再堆积、长时静置、FPS 与像素交叉验证；TC-WS-01 同时作为 algorithm v1 §3.3 分相守恒缺陷的回归防线。

## P-011：倾斜跟手性 / 底层运动响应客观测试用例

> 版本：v1　日期：2026-10-14　对应算法：`docs/algorithm-P011.md` v0.1  
> 说明：本节仅覆盖客观指标；倾斜手感、视觉舒适性、主观跟手感受由 effector 另行维护。  
> 路线要求：每个技术路线必须使用稳定 `routeId`，并独立输出一份客观测试报告；不同技术路线不做全量 dominance 比较。

### 1. 公共前置与数据口径

- **运行配置**：`testmode=true`，禁用真实传感器；目标倾角只能由测试脚本、Want 参数或事件注入，禁止面板误触或传感器接管。
- **物理步长**：固定 `1/120s`。若渲染帧间隔大于 `1/60s`，必须按既定子步规则推进，不得把帧间隔直接当作物理 `dt`。
- **时间戳**：每个输入命令必须记录 `commandTime`、`acceptedTime`、首个消费该输入的 `physicsTime`；三者使用同一时钟源，且全程单调递增。
- **有效倾角**：`effectiveTilt` 定义为物理求解器实际消费的倾角，不是 UI 目标值、传感器原始值或渲染矩阵值。
- **底层位移**：
  - `B_h(t) = [bh[0](t), …, bh[N-1](t)]`
  - `B_l(t) = [bl[0](t), …, bl[N-1](t)]`
  - 单步位移量：`ΔB_h(t) = Σ_c |bh[c](t) - bh[c](t-dt)|`，`ΔB_l(t) = Σ_c |bl[c](t) - bl[c](t-dt)|`
  - 输入活动量：`A(t) = |effectiveTilt(t) - effectiveTilt(t-dt)|`
- **相关性**：在连续运动窗口内计算 `Pearson(A(t), ΔB_h(t))` 与 `Pearson(A(t), ΔB_l(t))`；静置段不参与相关性计算。
- **跟随误差**：对每条连续运动曲线，将输入活动量与 `bh/bl` 位移量分别归一化为累计曲线后，计算最大偏差：
  - `FE_h = max_t |C_A(t) - C_h(t)|`
  - `FE_l = max_t |C_A(t) - C_l(t)|`
- **数据完整性**：缺少输入时间戳、有效倾角、`bh/bl` 数组或三相总量钩子时，正式结论记为 `BLOCKED(missing_hook)`；不得用渲染像素替代模拟层数据。
- **报告隔离**：每个技术路线独立输出 `docs/test-report-P011-<routeId>.md`；同一路线迭代采用追加模式。报告中必须记录日期、`routeId`、HAP/commit hash、测试脚本 commit hash、原始日志与判定 JSON 路径。

### 2. 用例总览

| 用例 ID | 覆盖目标 | 核心阈值 | 分类 |
|---|---|---|---|
| TC-P011-01 | 输入倾角到有效倾角的响应时延 | `P95 T90 <= 100ms` 达到 6 分；无方向反转 | TIERED |
| TC-P011-02 | 输入变化与底层 `bh/bl` 位移的相关性与跟随误差 | `r >= 0.70`、`lag <= 66.7ms`、`FE <= 0.15` 达到 6 分 | TIERED |
| TC-P011-03 | 快速倾斜、回正、反向倾斜连续性 | 无瞬移、无非命令方向反转、回正误差 `<= 1°` | MUST-PASS + TIERED |
| TC-P011-04 | 三相守恒与数值稳定性 | 三相漂移 `< 0.1%`；无非有限数；无越界；无崩溃 | MUST-PASS |

### 3. TC-P011-01：输入倾角到有效倾角响应时延

- **前置条件**：场景初始化完成并静置 `3s`；倾角为 `0°`；输入、有效倾角与物理时间戳完整可用。
- **输入序列**：  
  `0° → +15°` 保持 `3s` → `0°` 保持 `3s` → `-15°` 保持 `3s` → `0°` 保持 `3s`；重复 5 轮。阶跃命令必须带事件边界与时间戳。
- **期望输出**：每次命令后，`effectiveTilt` 按命令方向连续变化，并在可量化时间内达到目标倾角的 90%；不出现方向反转、卡死或跳变。
- **指标定义**：
  - `T50`：从 `commandTime` 到 `effectiveTilt` 首次达到目标变化量 50% 的时间。
  - `T90`：从 `commandTime` 到 `effectiveTilt` 首次达到目标变化量 90% 的时间。
  - `P95 T90`：所有有效阶跃的 95 分位数。
  - `directionError`：命令后 `100ms` 内 `effectiveTilt` 与目标方向相反的次数，必须为 0。
- **TIERED 判定**：

| 分数 | `P95 T90` |
|---:|---:|
| 10 | `<= 50ms` |
| 8 | `<= 75ms` |
| 6 | `<= 100ms` |
| 4 | `<= 150ms` |
| 2 | `<= 200ms` |
| 0 | `> 200ms` |

- **通过标准**：TIERED 得分 `>= 6`，且 `directionError = 0`。数据缺失时不判分，正式结论为 `BLOCKED(missing_hook)`。

### 4. TC-P011-02：输入变化与底层 `bh/bl` 位移相关性及跟随误差

- **前置条件**：延续 TC-P011-01 的稳定初始态；`bh/bl` 为模拟层权威数组，渲染层只做交叉验证。
- **输入序列**：  
  `0° → +20°` 用 `2s` 匀速到达 → 保持 `1s` → `+20° → -20°` 用 `4s` 匀速到达 → 保持 `1s` → `-20° → 0°` 用 `2s` 匀速到达；重复 5 轮。全程记录每个物理子步。
- **期望输出**：输入活动量与底层 `bh/bl` 位移量存在正相关；位移响应滞后有限；归一化累计曲线的形状差有界，不得出现输入停止后底层无响应、或输入尚未变化底层提前大幅变化。
- **指标定义**：
  - `r_h = Pearson(A(t), ΔB_h(t))`
  - `r_l = Pearson(A(t), ΔB_l(t))`
  - `lag_h`、`lag_l`：在 `[-200ms, +200ms]` 内使相关性最大的时间滞后。
  - `FE_h`、`FE_l`：按公共口径计算的归一化累计曲线最大偏差。
- **TIERED 判定**：分别对 `bh`、`bl` 按下表取最高满足档；两相最终得分为较低值。

| 分数 | 相关系数 `r` | 绝对滞后 `|lag|` | 跟随误差 `FE` |
|---:|---:|---:|---:|
| 10 | `>= 0.90` | `<= 16.7ms` | `<= 0.05` |
| 8 | `>= 0.80` | `<= 33.3ms` | `<= 0.10` |
| 6 | `>= 0.70` | `<= 66.7ms` | `<= 0.15` |
| 4 | `>= 0.55` | `<= 100ms` | `<= 0.25` |
| 2 | `>= 0.40` | `<= 150ms` | `<= 0.35` |
| 0 | 未达以上任一条件 | — | — |

- **通过标准**：`bh` 与 `bl` 的 TIERED 得分均 `>= 6`。若任一相缺失，整体记为 `BLOCKED(missing_hook)`，不得仅用另一相结果放行。

### 5. TC-P011-03：快速倾斜、回正与反向倾斜连续性

- **前置条件**：正立稳态；输入由脚本注入，事件边界完整；物理子步与输入时间戳可对齐。
- **输入序列**：  
  `0° → +25°` 用 `0.25s` 到达 → 保持 `0.5s` → `+25° → 0°` 用 `0.25s` 回正 → 保持 `0.5s` → `0° → -25°` 用 `0.25s` 到达 → 保持 `0.5s` → `-25° → 0°` 用 `0.25s` 回正 → 保持 `0.5s`；重复 10 轮。
- **期望输出**：`effectiveTilt` 与底层 `bh/bl` 在快速变化、回正、反向切换时保持连续；不出现整块瞬移、符号错误、非命令方向摆动或回正残留。
- **指标定义**：
  - `maxSubstepBh = max_{t,c} |bh[c](t) - bh[c](t-dt)|`
  - `maxSubstepBl = max_{t,c} |bl[c](t) - bl[c](t-dt)|`
  - `maxUncommandedTiltStep = max_t |ΔeffectiveTilt(t) - ΔtargetTilt(t)|`
  - `zeroResidual = max_t |effectiveTilt(t)|`，仅取每次回正后 `300ms` 至下一段开始前的样本。
  - `reverseDirectionError`：反向命令后 `100ms` 内 `effectiveTilt` 与新命令方向相反的次数。
- **MUST-PASS**：
  - `maxSubstepBh <= 5 cell` 且 `maxSubstepBl <= 5 cell`；超过判定为瞬移 FAIL。
  - `maxUncommandedTiltStep <= 2°`，事件边界外不得出现无输入对应的有效倾角跳变。
  - `zeroResidual <= 1°`。
  - `reverseDirectionError = 0`。
  - 全程无 crash、fatal exception、ANR 或非预期退出。
- **TIERED 判定**：以 `maxSubstepBh`、`maxSubstepBl` 的较大值评分；两指标分档一致，取最低分。

| 分数 | 最大单子步底层位移 |
|---:|---:|
| 10 | `<= 0.5 cell` |
| 8 | `<= 1.0 cell` |
| 6 | `<= 1.5 cell` |
| 4 | `<= 2.5 cell` |
| 2 | `<= 4.0 cell` |
| 0 | `> 4.0 cell` |

- **通过标准**：MUST-PASS 全部 PASS，且 TIERED 得分 `>= 6`。

### 6. TC-P011-04：三相守恒与数值稳定性压力测试

- **前置条件**：使用与正式方案一致的初始化场景；初始三相总量 `Vw0`、`Vh0`、`Vl0` 必须在 init/reset 后记录。
- **输入序列**：执行 TC-P011-03 的快速倾斜序列 100 轮，总时长不少于 `120s`；随后静置 `60s`。全程采集每个物理子步或至少每 `250ms` 一份状态快照。
- **期望输出**：水、重沙、轻沙三相总量保持守恒；所有状态量有限且在合法范围内；长时压力下无崩溃、无数值爆炸、无累积漂移。
- **指标定义**：
  - `driftWater(t) = |Vw(t) - Vw0| / max(Vw0, ε)`
  - `driftHeavy(t) = |Vh(t) - Vh0| / max(Vh0, ε)`
  - `driftLight(t) = |Vl(t) - Vl0| / max(Vl0, ε)`
  - `nonFiniteCount`：`waterTop/bh/bl/mh/ml/suspH/suspL/effectiveTilt` 中 `NaN/Inf` 的样本数。
  - `outOfRangeCount`：`bh/bl/mh/ml/suspH/suspL < -1e-9` 或 `> rows + 1e-9` 的样本数。
- **MUST-PASS**：
  - 任一采样点均满足 `driftWater < 0.001`、`driftHeavy < 0.001`、`driftLight < 0.001`。
  - `nonFiniteCount = 0`。
  - `outOfRangeCount = 0`。
  - 全程进程存活，无 crash、fatal exception、ANR 或非预期退出。
  - 输入事件与物理时间戳可对齐；不得出现未输入触发的 reset、模式切换或倾角跳变。
- **通过标准**：全部 MUST-PASS 通过。任何三相漂移、非有限数、越界或崩溃均直接 FAIL，不得由其他指标高分抵消。

### 7. P011 bottom-creep-v2.7 正式测试准备（等待 developer 修复）

- **状态**：`BLOCKED(dev-fix-pending)`。本节只准备时间表、场景与命令，不判定 P-011；developer 完成 `test/p011_check.py` 与 objective fixture 修复前，不得执行真机正式测试。

#### 7.1 TC-P011-02 连续线性目标角时间表

- **前置条件**：固定种子 `seed=20261016`；`rendermode=4`、`testmode=true`、`forceinvert=false`；`wspreset=thick`、`wscols=10`、`wscapacity=1000`、`wssnapshot=true`；初始 `0°` 静置 `3s`。
- **输入公式**：`0° -> +20°` 用 `2s` 匀速，保持 `1s`；`+20° -> -20°` 用 `4s` 匀速，保持 `1s`；`-20° -> 0°` 用 `2s` 匀速；重复 `5` 轮。总时长 `53s`（含初始静置）。
- **100ms 命令表**：每轮 `20+10+40+10+20=100` 条 Want，加初始 `31` 条，共 `531` 条；相邻目标角步长最大 `1.0°`。120Hz 显示帧下限为 `8.333ms`，`100ms` 间隔满足“至少一个显示帧”。
- **审查硬门槛（MUST-PASS）**：
  - `P011_EVENT input.source="script"` 数量必须为 `531`。
  - 相邻 `acceptedTime` 差必须 `>= 100ms`（若改用其他命令周期，则 `>=` 该周期且不小于一个显示帧）；不接受重叠或跳过中间目标角。
  - 每条 `targetTilt` 与生成的连续时间表误差 `<= 0.001°`。
  - 五轮相位完整；`P011_METRIC.physicsTime` 与 `P011_BOTTOM.physicsTime` 按样本数和时间一一对齐。
  - 不用 `±200ms` 脉冲扩展把 `A(t)=0` 样本混入 activity 相关性窗口。

#### 7.2 满载并发与独立 total-clip 场景

所有压力场景均使用固定种子与 TC-P011-02 相同的基础启动参数；每个场景独立启动并独立采集日志。

| 场景 ID | 输入 | 时长 | 必须证明 |
|---|---|---:|---|
| `P011-SEC-01-FULL-LOAD` | `wstilt=88` 满载保持 | `20s` | `fullLoadSamples > 0`；settlement、percolation、slip、suspension 四类运动申请在同一物理子步内非零 |
| `P011-SEC-02-CLIP-A` | `wstilt=88` 高压保持 | `20s` | 本段出现 `totalMotionBudgetClipped > 0` |
| `P011-SEC-03-CLIP-B` | `wstilt=-88` 高压保持 | `20s` | 本段出现 `totalMotionBudgetClipped > 0` |
| `P011-SEC-04-CLIP-C` | `wstilt=88` 高压保持 | `20s` | 本段出现 `totalMotionBudgetClipped > 0` |

- 三段 total-clip 使用独立进程、独立日志和分段标识隔离；不得合并缺失段或用其他场景补判。
- 场景完整性硬门槛：`fullLoadSamples > 0` 且 `totalMotionClipRuns >= 3`。
- suite 聚合符号切换硬门槛：`fullLoad + CLIP-A + CLIP-B + CLIP-C` 聚合后必须至少出现 1 次 `positive -> negative` 和 1 次 `negative -> positive` 的 `bottomDirection` 符号切换；单符号压力日志不得因自身缺少切换而独立失败。
- 重启采集硬门槛：每个独立压力段在启动后必须等待新的 `P011_EVENT reset`（含新 `runId`）和该 `runId` 的首个 `P011_METRIC` 后才开始计时采集；不得把重启边界前后样本拼入同一相位。
- suite 聚合报告必须逐 run 输出 `runId`、reset 次数、`sequence/frameId/substepId` 范围、target 相位数、相位内样本数与符号切换计数，并单独输出 suite 级 `positive -> negative` 与 `negative -> positive` 计数。

#### 7.3 待执行命令

```powershell
python test/p011_tc02_schedule.py --frame-ms 100 --output test/artifacts/p011-formal-prep-20261016/p011-tc02-schedule.csv
powershell -ExecutionPolicy Bypass -File test/p011_tc02_formal_suite.ps1 -Mode Schedule
```

developer 修复通过后按顺序执行：

```powershell
python test/p011_check.py --self-check
powershell -ExecutionPolicy Bypass -File test/p011_tc02_formal_suite.ps1 -Mode Tc02
powershell -ExecutionPolicy Bypass -File test/p011_tc02_formal_suite.ps1 -Mode PressureSegments
python test/p011_check.py --input test/artifacts/p011-formal-prep-20261016/p011-tc02-formal.log --json test/artifacts/p011-formal-prep-20261016/p011-tc02-check.json --markdown test/artifacts/p011-formal-prep-20261016/p011-tc02-check.md
```

### 8. 技术路线报告与比较边界

- 每个技术路线必须独立输出一份客观测试报告，路径为 `docs/test-report-P011-<routeId>.md`。
- 同一路线的新迭代在原报告中追加记录，必须包含日期、被测代码 commit hash、HAP 标识、测试脚本 commit hash、输入序列、原始日志路径、判定 JSON 路径与各项实测值。
- 不同技术路线不得共用同一份客观测试报告，也不得把多条路线的数据合并后形成单一验收结论。
- 不同技术路线之间不做全量 dominance 比较，不输出“某路线全面优于另一路线”的验收结论。
- 如需跨路线说明，只能以参考表列出各自实测值，并明确标注“仅参考、不作为验收依据”；验收结论只对单一路线闭环生效。


### 9. bottom-creep-v2.8-heavy-interface added cases

These cases do not replace TC-P011-01~04. They add v2.8 green heavy-interface observation and schema v2 strict gates. No threshold may be relaxed.

#### 9.1 TC-P011-V28-00 green-ROI preflight

- Precondition: independent runId, rendermode=4, testmode=true, wspreset=thick, fixed resolution, central 80% ROI.
- Input: 0 degree baseline, +15 degree hold, return, -15 degree hold, return; capture each segment.
- Expected: green boundary centroid moves in the command direction; heavyFlux has enough non-zero samples; routeId is recorded.
- Pass: status == OBSERVATION_READY; positive and negative heavyFluxNonZeroSamples >= 30; heavyShapeRms >= 0.5 and heavyShapeMaxDelta >= 1.0. Missing ROI is BLOCKED(roiObservation).

#### 9.2 TC-P011-V28-01 TC02 schedule precheck

- Precondition: one raw log, one runId, one process; do not splice runs.
- Input: 531 rows of the 100ms continuous-linear target schedule.
- Expected: input/source=script maps 1:1 to schedule; input_state does not enter command metrics.
- Pass: scriptEventCount == 531; scriptRunIdCount == 1; sequence is continuous [1..531]; target matches exactly; acceptedTimeGapViolations == 0; minAcceptedTimeGapMs >= 100; unknown telemetry == 0. A precheck FAIL forbids TC02 physical scoring.

#### 9.3 TC-P011-V28-02 heavy non-zero and direction

- Precondition: TC02 precheck PASS; evaluate each valid positive/negative tilt phase of at least 2 seconds.
- Pass: heavyFluxNonZeroSamples >= 30; sign(C_H) == sign(targetTilt); direction errors == 0.

#### 9.4 TC-P011-V28-03 heavy shape observability

- Precondition: phase-start baseline, central 80% columns.
- Pass: heavyShapeRms >= 0.5 model unit; heavyShapeMaxDelta >= 1.0 model unit; maxSubstepBh remains under the current TC03 smoothing threshold.

#### 9.5 TC-P011-V28-04 independent H/L observation

- Precondition: heavyFlux and lightFlux are accumulated independently; the checker must not duplicate one aggregate displacement into H and L.
- Pass: variance(C_H) > 1e-12 and variance(C_L) > 1e-12; otherwise record INCOMPLETE(degenerateMeasurement) and do not score physical behavior.

#### 9.6 TC-P011-V28-05 existing gates and pressure isolation

- Pass: all current TC-P011-01~04 tiered, budget, conservation, finite-value, out-of-range, model-error, crash/ANR gates remain unchanged. The suite must contain behavior, full-load, and total-clip segments and must not reuse logs from another route.
## 变更记录

- 2026-10-13：初始创建。
- 2026-10-13：追加 P-003/P-005 场景二客观用例 TC-WS-01..12 与 WS 钩子数据源映射。原因：P-005 测试先行，补齐三相守恒、分选、沉降、安息角、水面、倒立、摇晃再堆积、长时静置、FPS 与像素交叉验证；TC-WS-01 同时作为 algorithm v1 §3.3 分相守恒缺陷的回归防线。
- 2026-10-13：追加 TC-WS-01..12 的 MUST-PASS/TIERED 判定口径 v2。原因：用户新增硬门槛与 0~10 分档要求；保持原硬阈值不放宽，将质量类指标以旧阈值为 6 分及格锚点，并统一“提升=改善、及格线内下降=可接受、跌破及格线或硬门槛失败=恶化/未通过”的报告口径。
- 2026-10-14：澄清 HG-04“单帧位移”为物理子步口径，并在 TC-WS-08 增加重/轻沙分相悬浮观测记录。原因：30Hz 帧内两个子步必须分别校验，避免相加后误判或误放行；现有总悬浮比例无法识别“峰值达标但重沙未扬起”的摇晃交融构成问题。阈值本身不放宽，分相指标仅用于客观记录与报告呈现。
- 2026-10-14：追加 P-006 场景一 TC-11~13 正式用例与 `liquid_mode_check.py` 判定脚本设计。原因：manager 批准等待期转正场景一测试设计；覆盖油水分层、摇晃恢复与倒立重排，明确硬门槛、TIERED 分档与暂允许失败项，且不占用真机、不抢占 G2+G3 主线。
- 2026-10-16：按 architecture-P011 §7.7.2 固化 P-011 objective suite 聚合正负方向切换覆盖、独立压力日志/run-reset 分段与场景重启后 reset/runId 采集门控。
- 2026-10-18: added bottom-creep-v2.8-heavy-interface green-ROI preflight, TC02 schedule precheck, heavy non-zero/direction/shape, independent H/L observation, and pressure isolation cases.

### 10. P-011 双模式输入客观验收（2026-10-18，用户授权测试输入替代手持）

本节仅增加输入工程验收，不改变 bottom-creep-v2.8-heavy-interface 物理/预算阈值。以下均为 MUST-PASS、不分档；既有质量指标继续0~10、及格线6。脚本模拟通过不能声明真实传感器E2E通过。最终接口名称由 designer/architect 冻结，测试通过输入/事件/状态观测验证语义，不绑定实现命名。

| 用例 ID | 前置条件 | 输入 | 期望输出与判定标准 | 执行与证据 |
|---|---|---|---|---|
| TC-P011-MODE-01 源互斥 | 场景4，明确模式与runId | 测试模式发送0,+15,-15,0模拟姿态，同时注入不同传感器样本；体验模式发送传感器样本并尝试脚本姿态 | 测试仅模拟姿态驱动；体验仅传感器驱动；被排除源对目标/物理输入贡献为0；遥测source准确，禁止将脚本改名sensor | 主机路由unit覆盖两源冲突；设备日志验证实际入口，保留源与目标时间序列 |
| TC-P011-MODE-02 切换清残留 | 活跃script、待消费Want、forceInvert、gesture hold及非零滤波历史均已建立 | 测试→体验→测试；分别经UI/Want入口，覆盖同模式重复设置 | 切换后旧script/pending/forceInvert/gesture/filter不再驱动；体验等待/读取当前真实回调，不能复用测试姿态；重入测试无旧命令回放；同模式幂等不能误删新命令 | 状态转换unit与设备事件；记录切换前后字段及首个新输入 |
| TC-P011-MODE-03 真实回调接线 | 体验模式，真实加速度计订阅可用 | 无人手持条件下自动启动，观察系统实际回调；另用unit构造正负向加速度 | 实际callback计数>0，有限值与时间戳来自系统回调并进入source=sensor链；unit校验符号与共用链路；静止设备不冒充动态姿态/噪声/标定验收；缺真实回调证据即BLOCKED | callback/run遥测及OS/API；模拟unit和真实回调证据分开 |
| TC-P011-MODE-04 模拟可复现 | 固定seed、schedule、初态与模拟时间；reset后独立run | 同序列重复两次，含0/+15/-15/回零和已批准边界 | 目标、顺序、模拟时间逐项一致；姿态由程序构造，无传感器/手势依赖；同一物理初态与固定dt下结果按最终设计数值契约一致；真实墙钟差异不冒充目标差异 | 保存schedule/hash、两run输入及状态序列；物理容差待设计冻结，不擅设阈值 |
| TC-P011-MODE-05 fresh Want与531闸门 | 从体验切测试，同一Want携带testmode与首个wstilt | canonical完整531行；首命令同时触发进入测试；各帧保持目标 | 首条新命令不被清理丢失；531/531、单run、sequence1..531、target精确一致、accepted gap>=100ms且0违规；input_state不计script；冻结captureAudit全部满足后才物理评分 | canonical runner；冻结TC02一次采集复用，禁止拼run或删样本 |
| TC-P011-MODE-06 重启与退出 | 有待消费命令、非零姿态和forceInvert | 场景重建、应用重启、退出测试、订阅注销后重入 | 按冻结契约初始化模式，无旧run命令重放/旧滤波污染；订阅无重复；退出后无残留更新；新run/resetEpoch可追溯，首个fresh输入仍消费一次 | 生命周期unit与设备事件，分别记录run/resetEpoch |
| TC-P011-MODE-07 共用处理链与既有门槛 | 模式入口已实现；匹配HAP/source manifest | 两入口构造相同规范姿态并检查下游；执行原TC01~04、green-ROI及压力suite | 模式只改变输入来源，下游方向/平滑/预算/物理链共用；所有原硬门槛维持，质量>=6；模拟验收与真实传感器接线报告分开 | 主机adapter unit+源审查；设备完整客观suite，主观交effector |

RED基线：`python test/test_p011_input_modes.py --output test/artifacts/p011-v28-canonical-20261018/input-modes-red-baseline.json` 仅执行源码库存检查；其presence不是runtime PASS。新契约/业务未交付时上述runtime用例如实BLOCKED/NOT_IMPLEMENTED，待实现后原用例重测。
