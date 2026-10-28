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
- **操作序列**：静置 3s → 注入 `shake=1.0` 持续 3s → 停止摇晃静置 8s → 记录全程 `suspensionFraction`、`suspensionFractionHeavy`、`suspensionFractionLight`。
- **判定指标与阈值**：
  - 摇晃段峰值 `peakSusp >= 0.5`。
  - 静置 8s 后 `residualSusp <= 0.05`。
  - 全程三相漂移 `< 0.1%`，无永久悬浮。
- **数据来源**：`WS_EVENT shake`、`WS_METRIC.suspensionFraction/suspensionFractionHeavy/suspensionFractionLight/drift*`、`WS_SNAPSHOT.suspH/suspL`。
- **分相悬浮记录**：
  - 数据来源钩子：`WS_METRIC` additive 追加 `suspensionFractionHeavy`、`suspensionFractionLight`，随既有 250ms 指标周期输出；计算口径为 `sum(suspH)/max(initialHeavy,eps)` 与 `sum(suspL)/max(initialLight,eps)`。二者必须能用最近 `WS_SNAPSHOT.suspH/suspL` 与 init 阶段记录的相初值重算交叉校验；缺失任一字段判 `BLOCKED(missing_hook)`，不得默认为 `0`。
  - 记录方式：按 `WS_EVENT shake` 边界分段，保留摇晃段与静置 8s 段的全量采样序列，分别计算 `peakSuspHeavy`、`peakSuspLight`、`residualSuspHeavy`、`residualSuspLight`，并保留对应的 `WS_METRIC` 原始时间戳。
  - 报告呈现：在 TC-WS-08 结果表中将总量峰值/残余与重、轻沙分相峰值/残余并列输出，并绘制三条悬浮比例时间序列（或以同一时间轴列出采样值）。分相指标当前为观测记录，不新增 PASS/FAIL 阈值、不放宽既有阈值；若总量峰值达标而重沙峰值接近 `0`，必须在失败分析/设计反馈中明确记录“摇晃交融构成未达成”。
- **通过标准**：峰值、残余、守恒全部 PASS；若 `peakSusp` 未达标但画面有扬起，仍判 FAIL，不能放宽阈值。

#### TC-WS-09 摇晃→悬浮→再堆积质量

- **前置条件**：延续 TC-WS-08 的同一初始场景；`α=30°`，重/轻沙均存在。
- **操作序列**：注入 `shake=1.0` 3s → 静置 10s → 采样末段 5s。
- **判定指标与阈值**：
  - 再堆积后 `separationRatio <= 0.5`。
  - `maxSlopeDeg <= 32°`，`maxCurvature <= 0.35`。
  - `maxSandTopDeltaPerSecond <= 30` 列/秒，任一物理子步位移 `<= 0.5` 列；30Hz 一帧包含 2 个子步时分别检查两个子步，不得相加后套用 `0.5` 列。
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

### 5. 2026-10-13 判定口径 v2（追加：MUST-PASS 与 TIERED）

> 追加原因：用户要求将不变量类指标设为不可放宽的硬门槛，将质量类指标改为 0~10 分档；同时 algorithm v2 已引入守恒渗透公式，需要统一后续报告口径。
> 生效范围：仅 TC-WS-01..TC-WS-12。本节不降低、不替换 v1 已写入的任何硬阈值；v1 的质量阈值在本节中等同于 6 分及格锚点。
> 判定优先级：MUST-PASS 失败时，即使所有 TIERED 指标为 10 分，用例与整体结论仍为 FAIL/未通过；`BLOCKED(missing_hook)` 与 `N/A(environment_refresh_rate)` 表示证据不足或环境不适用，不得折算为分数，也不得默认 PASS。

#### 5.1 全局硬门槛（MUST-PASS）

| 门槛 ID | 指标 | 判定标准 | 适用范围 |
|---|---|---|---|
| HG-01 | 无崩溃 | 被测进程持续存活，无 fatal exception、ANR、native crash 或测试编排导致的非预期退出 | TC-WS-01..12 全部执行段 |
| HG-02 | 三相守恒 | `driftWater`、`driftHeavy`、`driftLight` 在全时段与每个分段均 `< 0.001`（0.1%）；不得用平均值、末帧值或静默重置掩盖分段超限 | 全部产生物理状态的用例；TC-WS-01/07/08/09/10 为显式验收项，其余用例作为全局门禁 |
| HG-03 | 无数值爆炸 | 所有 `WS_*` 数值为有限数；六状态值不得为负，且任一单列/总量绝对值不得超过 `max(wscapacity, 2 × max(Vw0, VH0, VL0))`；该上限只可按场景配置固定，不得为通过临时调大 | TC-WS-01..12 全部执行段 |
| HG-04 | 无粒子瞬移 | `maxSandTopDeltaPerSecond <= 30` 列/秒，且任一物理子步位移 `<= 0.5` 列；30Hz 一帧包含 2 个子步时分别检查两个子步，不得相加后套用 `0.5` 列。判定口径与 `WS_FRAME.maxSandTopSubstepDeltaObserved/maxSandTopSpeedObserved/substeps/substepDt` 对齐，任一子步超限即 FAIL | TC-WS-04、TC-WS-09；TC-WS-12 的帧间分类突变也必须回溯该门槛 |
| HG-05 | 无非预期模型错误 | 不得出现未由测试输入触发的 `model_error`、非预期 `wake` 或状态静默重置 | TC-WS-01..12；TC-WS-10 的长时静置为显式验收项 |
| HG-06 | 守恒数据一致性 | `WS_SNAPSHOT` 六状态重算出的三相漂移与 `WS_METRIC.drift*` 在 0.1% 判定下结论必须一致；一个 FAIL 一个 PASS 时判 FAIL | TC-WS-01 及其他提供快照与指标交叉数据的用例 |
| HG-07 | §3.3 缺陷哨兵 | 合成“重沙增加、轻沙减少”缺陷必须被判 FAIL；若实现等效于 algorithm v1 的 `hl[c] -= q; hs[c] += q`，TC-WS-01 必须在第 2 步静置段 FAIL | TC-WS-01 及 `--self-check-defect` 回归 |

#### 5.2 用例分类矩阵

| 用例 | 硬门槛（MUST-PASS，不可分档/不可放宽） | 分档指标（TIERED，0~10 分，`>=6` 及格） |
|---|---|---|
| TC-WS-01 | HG-01~HG-07；三相漂移每相每分段 `<0.1%`；无 `model_error`；v1 缺陷必须暴露 | 无。该用例只判 PASS/FAIL，质量分记 `N/A(hard_gate_only)` |
| TC-WS-02 | HG-01~HG-06 | `separationRatio` |
| TC-WS-03 | HG-01~HG-06；`vH > vL` 配置校验；3 次 `t90` 均必须存在 | `t90Ratio = t90H/t90L`（3 次重复取最低分） |
| TC-WS-04 | HG-01~HG-06；`maxSandTopDeltaPerSecond <=30`、单帧位移 `<=0.5` 列 | `maxSlopeDeg`、`maxCurvature` |
| TC-WS-05 | HG-01~HG-06 | `waterSlopeDeg`、`waterAmplitudePx` |
| TC-WS-06 | HG-01~HG-06；`effectiveDeg` 必须有限且无非输入跳变 | `tiltSlopeErrorPct`（15° 与 30° 两段分别评分取最低分）；回正段复用 TC-WS-05 两项评分 |
| TC-WS-07 | HG-01~HG-06；视觉层必须为 180° 旋转且相序方向正确；倒立/回正过渡无非输入跳变 | `phaseOrderScore` |
| TC-WS-08 | HG-01~HG-06；无永久悬浮由残余项参与及格判定，若数值发散或状态爆炸仍按 HG-03 硬失败 | `peakSusp`、`residualSusp` |
| TC-WS-09 | HG-01~HG-06；无粒子瞬移（HG-04） | `separationRatio`、`maxSlopeDeg`、`maxCurvature`、`residualSusp`，取最低分 |
| TC-WS-10 | HG-01~HG-06；后 5s `bedDrift <= 0.25` 列；后 5s 三相漂移 `<0.1%`；不得出现未输入触发的 `wake/model_error` | 无。该用例是稳定性不变量用例，只判 PASS/FAIL，质量分记 `N/A(hard_gate_only)` |
| TC-WS-11 | HG-01、HG-03、HG-05；至少 100 个有效 `WS_FRAME` 样本为证据完整性门槛，不足判 `BLOCKED(insufficient_samples)` | `fpsP95`；`physicsTimeMs` P95 仅记录诊断，不评分、不参与及格判定 |
| TC-WS-12 | HG-01~HG-06；帧间突变不得违反 HG-04；至少 3 个关键帧为证据完整性门槛 | `maxPhaseDeviationPct`（水体、重沙、轻沙三类偏差的最大值；混合带不评分） |

#### 5.3 分档标准（0~10 分，`>=6` 及格）

分档原则：旧阈值一律落在 6 分，表示“达到原验收及格线”；10 分为显著优于原阈值；4 分及以下表示低于原验收线。除 `N/A` 与 `BLOCKED` 外，分数必须由脚本按实测值计算，不得人工给分。

| 指标 | 方向 | 10 分 | 8 分 | 6 分（及格锚点） | 4 分 | 2 分 | 0 分 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `separationRatio` | 越低越好 | `<=0.20` | `<=0.35` | `<=0.50` | `<=0.65` | `<=0.80` | `>0.80` 或上下区数据无效 |
| `t90Ratio = t90H/t90L` | 越低越好 | `<=0.40` | `<=0.50` | `<=0.70` 且 `t90H<t90L` | `<=0.85` 且 `t90H<t90L` | `<=1.00` 且 `t90H<t90L` | `>1.00`、顺序错误、任一 `t90` 不存在或重复数据无效 |
| `maxSlopeDeg` | 越低越好 | `<=28°` | `<=30°` | `<=32°` | `<=35°` | `<=38°` | `>38°` |
| `maxCurvature` | 越低越好 | `<=0.20` | `<=0.27` | `<=0.35` | `<=0.45` | `<=0.55` | `>0.55` |
| `waterSlopeDeg` | 越低越好 | `<=0.5°` | `<=1.0°` | `<=2.0°` | `<=3.0°` | `<=4.0°` | `>4.0°` |
| `waterAmplitudePx` | 越低越好 | `<=0.5px` | `<=1.0px` | `<=2.0px` | `<=3.0px` | `<=4.0px` | `>4.0px` 或缺少 `pixelScale` |
| `tiltSlopeErrorPct` | 越低越好 | `<=5%` | `<=10%` | `<=15%` | `<=20%` | `<=30%` | `>30%`、斜率符号错误或数据无效 |
| `phaseOrderScore` | 越高越好 | `>=0.60` | `>=0.45` | `>=0.30` | `>=0.20` | `>=0.10` | `<0.10` 或相序方向错误 |
| `peakSusp` | 越高越好 | `>=0.90` | `>=0.75` | `>=0.50` | `>=0.35` | `>=0.20` | `<0.20` 或摇晃段数据无效 |
| `residualSusp` | 越低越好 | `<=0.010` | `<=0.025` | `<=0.050` | `<=0.075` | `<=0.100` | `>0.100` 或末段数据无效 |
| `fpsP95` | 越高越好 | `>=120` | `>=110` | `>=100` | `>=80` | `>=60` | `<60`；低刷新率环境按原规则 `N/A(environment_refresh_rate)` |
| `maxPhaseDeviationPct` | 越低越好 | `<=2.0` | `<=3.5` | `<=5.0` | `<=6.5` | `<=8.0` | `>8.0`、关键帧不足或颜色标定无效 |

评分细则：

- 多次重复、多个分段或多个同类指标的 TIERED 得分取最低分；不得用高分区抵消低分区。
- 混合用例的 TIERED 总分为其分档指标最低分；任一硬门槛失败时总分不参与“可接受”判定，结论为 FAIL/未通过。
- 硬门槛阈值不得降低。若需修改硬门槛，只能先反馈 manager 决策并记录原因；不得为了通过测试临时放宽。
- `N/A` 仅允许按原用例规则出现，必须记录原因与证据；不得把缺失钩子、样本不足或环境不满足写成 `N/A` 来规避 FAIL/BLOCKED。

#### 5.4 后续报告口径

后续 `docs/test-report-P003-P005.md` 追加执行记录时，必须为每个 TIERED 指标输出：实测值、0~10 分、及格状态、与上一版的分数变化；为每个 MUST-PASS 指标输出：实测值、阈值、PASS/FAIL。趋势判定如下：

| 情况 | 趋势/结论 |
|---|---|
| 任一 MUST-PASS 失败 | 恶化/未通过，整体不得判 PASS；不得被 TIERED 高分掩盖 |
| TIERED 分数上升 | 改善 |
| TIERED 分数下降但仍 `>=6` | 可接受，报告中必须标注下降幅度与具体指标 |
| TIERED 分数跌破 `<6` | 恶化/未通过 |
| 上一版 `N/A/BLOCKED`，本轮有效得分 `>=6` | 改善（解除阻塞并达到及格） |
| 本轮 `N/A/BLOCKED` | 不可比较，必须保留阻塞原因；不得推定为改善或恶化 |

---

## P-006：场景一“两种不互溶液体（油水分层）”客观测试用例（等待期设计版）

> 版本：v1　日期：2026-10-14　对应实现方向：`SandGrid` M1' 液体模式（`rendermode=3`）
> 说明：本节将 `test/TC_TEST_CASES.md` 中 TC-11~13 草案转正，只覆盖场景一与共享容器动态目标；主观观感仍由 effector 维护。TC-11~13 编号仅在本 P-006 语境内使用，不与场景二 `TC-WS-*` 混用。
> 执行边界：本轮只做用例与判定脚本设计，不占用真机、不修改业务代码、不抢占 G2+G3 主线；正式执行等待 manager 单独排期。

### 1. 公共前置与数据口径

- **启动配置**：`rendermode=3`（液体模式）、`testmode=true`、`forceinvert=false`；测试期间禁用传感器，倾角、摇晃与倒立只能由测试脚本或 Want 参数注入。
- **初始场景**：沿用当前液体模式固定初始化：轻相（蓝/油）在上、重相（绿/水）在下；init 钩子必须记录 `cols/rows/cellPx/initialHeavy/initialLight`。
- **分段边界**：摇晃、倒立开关、模式切换与 reset 必须输出带 `boundary:true` 的事件；事件边界不得计入相邻帧连续性或瞬移判定。
- **权威口径**：分相守恒以模拟层粒子计数为准；渲染层像素面积只做交叉验证，不得替代粒子计数。缺失模拟层钩子时正式结论为 `BLOCKED(missing_hook)`。
- **像素标定**：每轮先用竖直 BGR 切片标定背景、纯轻相、纯重相与混合色带阈值；忽略容器边框与左右两端各 10% 列。像素口径必须记录标定参数与每帧分类计数。

### 2. 测试钩子与判定脚本设计

后续实现独立脚本 `test/liquid_mode_check.py`，不复用或修改 `water_sand_check.py` 的场景二判定逻辑：

1. **输入**：hilog 文件、连拍帧目录、帧时间戳 manifest、颜色标定 JSON、用例 ID；输出 JSON 与 Markdown 报告，报告路径按问题独立为 `docs/test-report-P006.md`。
2. **产品钩子**：建议新增 `LIQ_EVENT` 与 `LIQ_METRIC`，仅在 `testmode=true && rendermode=3` 时输出，避免与 `WS_*` 混淆：
   - `LIQ_EVENT init/reset/shake/invert/mode/model_error`：含 `boundary`、时间戳、`forceInvert`、目标倾角与摇晃强度。
   - `LIQ_METRIC`：每 250ms 输出 `heavyCount/lightCount/emptyCount`、两相质心、界面每列行号、界面波动格数、有限数标记与仿真时间。
   - `LIQ_FRAME`：每帧输出 `frameTimeMs/physicsTimeMs`，用于 FPS 与逐帧连续性诊断。
3. **指标定义**：
   - `phaseDrift = max(|count(t)-count(0)|/count(0))`，重、轻相分别计算。
   - `interfaceRangeCells`：忽略两端 10% 列后，每列重相上边界行号的 `max-min`，除以 `cellPx` 换算为格。
   - `mixedBandShare`：像素分类中混合色带面积 / 液体总面积；仅表示视觉碎块/暂混程度，不代表质量交换。
   - `phaseOrderCorrect`：按重力坐标系判断重相质心位于轻相下方；倒立渲染方向只作交叉验证。
   - `recoveryTime`：从停止摇晃或倒立开关边界起，到相序正确且 `interfaceRangeCells<=3` 并持续 1s 的首时刻。
   - `rearrangeTime`：TC-13 从倒立开关边界到重排稳定（同上判定）的时长。
4. **判定顺序**：先解析事件与钩子完整性，再判 MUST-PASS，最后计算 TIERED 分；任一硬门槛失败不得被高分掩盖。像素与模拟层同指标结论冲突时按数据一致性失败处理。
5. **当前实现缺口**：上述 `LIQ_*` 钩子尚未实现；在补齐前，脚本可先做像素层诊断，但正式结论只能为 `BLOCKED(missing_hook)`。

### 3. 用例总览

| 用例 ID | 覆盖目标 | 核心阈值 | 分类 |
|---|---|---|---|
| TC-11 | 静置油水分层与水平界面 | 两相漂移 `<0.1%`；界面波动 `<=2` 格；稳态零抖动 | MUST-PASS + TIERED |
| TC-12 | 摇晃暂时混合后恢复分层 | 两相漂移 `<0.1%`；混色曲线先升后降；恢复 `<=4s`；终态相序正确 | MUST-PASS + TIERED |
| TC-13 | 倒立/回正动态重排 | 两相漂移 `<0.1%`；重排 `2~4s`；相序随重力翻转；无阶跃 | MUST-PASS + TIERED |

### 4. 详细用例

#### TC-11 液体模式静置分层

- **前置条件**：液体模式初始化完成，正立、无摇晃、无倒立；至少采集 30s 与 100 帧。
- **操作序列**：静置 30s，全程采集 `LIQ_METRIC`、连拍帧与 `LIQ_FRAME`；末 5s 定义为稳态段。
- **MUST-PASS**：
  - 进程持续存活，无 fatal exception、ANR、native crash 或非预期退出。
  - 重、轻相 `phaseDrift < 0.001`，全时段与每个分段逐样本判定。
  - 稳态段相序正确：重相质心位于轻相下方。
  - 稳态段相邻帧像素分类变化 `<=0.1%`，不得出现自发流动或零抖动伪影。
  - 所有 `LIQ_*` 数值为有限数，事件序列无未输入触发的 `invert/shake/reset/model_error`。
- **TIERED**：界面波动按 `<=0.5/1/2/3/4/>4` 格映射 `10/8/6/4/2/0` 分，`>=6` 及格；稳态像素变化按 `0%/0.05%/0.10%/0.20%/0.40%/>0.40%` 映射同档。
- **通过标准**：硬门槛全部 PASS 且两项 TIERED 均 `>=6`。

#### TC-12 液体模式摇晃暂时混合与恢复

- **前置条件**：延续 TC-11 稳态；摇晃输入由脚本注入，禁止面板误触或传感器接管。
- **操作序列**：注入 `shake=1.0` 左右往复 6s → 停止静置 30s；全程按事件边界分成混合段与恢复段。
- **MUST-PASS**：
  - 全程两相 `phaseDrift < 0.001`，激烈摇晃段逐样本判定。
  - 混合段 `mixedBandShare` 存在可测上升，恢复段存在单调或带小噪声的收敛下降；不得只看终态。
  - 恢复终态相序正确，界面波动 `<=3` 格，且不存在永久乳浊。
  - 恢复 `recoveryTime <= 4s`；超过 30s 未恢复判永久混合 FAIL。
  - 事件边界与帧时间戳可对齐；摇晃期间不得出现未输入触发的倒立或 reset。
- **TIERED**：峰值混色占比按 `>=30%/20%/10%/5%/2%/<2%` 映射 `10/8/6/4/2/0` 分；恢复时间按 `<=2/3/4/6/8/>8s` 映射同档；终态混色占比按 `<=1%/2%/5%/8%/12%/>12%` 映射同档。三项取最低分，`>=6` 及格。
- **通过标准**：硬门槛全部 PASS 且 TIERED 最低分 `>=6`。

#### TC-13 液体模式倒立与回正重排

- **前置条件**：正立稳态；倒立只通过 `forceinvert` 或脚本注入的 180° 手势触发，事件必须带边界。
- **操作序列**：开启倒立 → 静置至稳定 → 关闭倒立 → 静置至稳定；两段均至少覆盖 8s。
- **MUST-PASS**：
  - 全程两相 `phaseDrift < 0.001`。
  - 每次事件后的相序在重力坐标系中正确翻转；倒立关闭后必须回到正立相序。
  - 每次 `rearrangeTime` 均为 `2~4s`：短于 2s 判跳变，长于 4s 判拖尾。
  - 重排过程相邻帧质心位移与像素分类变化连续，不得出现整块瞬移或颜色整体跳变。
  - 事件边界处不参与跨边界连续性判定，但必须通过事件日志与渲染方向交叉验证。
- **TIERED**：重排时长按落在 `2.5~3.5s`、`2.2~3.8s`、`2.0~4.0s`、`1.5~4.5s`、`1.0~5.5s`、其他映射 `10/8/6/4/2/0` 分；过程平滑度按最大相邻帧质心位移 `<=0.5/1/2/3/4/>4` 格映射同档。两项取最低分，`>=6` 及格。
- **通过标准**：硬门槛全部 PASS 且 TIERED 最低分 `>=6`。

### 5. 暂允许失败项（只限等待期基线，不构成放行）

| 临时项 | 影响用例 | 允许状态 | 解除条件 |
|---|---|---|---|
| `LIQ_METRIC` 粒子计数与质心钩子缺失 | TC-11~13 | `BLOCKED(missing_hook)`；像素结果只能作诊断 | developer 补齐钩子并由 tester 校验解析 |
| 已知 bug A：倒立 OFF 后物理不回正 | TC-12、TC-13 | `FAIL(known_defect_A)` 作为 RED 基线记录 | developer 修复并复测通过 |
| 面板/BindSheet 自动化误触风险 | TC-12、TC-13 | 执行编排缺陷，不归因物理实现；必须修正脚本或使用 Want/事件注入 | 脚本不再依赖不可靠面板坐标 |

上述状态不得改写为 PASS，也不得用 TC-11 的静态 PASS 抵消 TC-12/13 的动态 FAIL。硬门槛阈值不因临时项放宽。

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

### 10.1 2026-10-18 green预检工具硬闸门澄清

TC-P011-V28-00的ROI就绪与heavy物理门槛必须分别输出：roiReady只表示三张图均可识别绿色；physicalGatePassed必须要求正负两个phase各自heavyFluxNonZeroSamples>=30、heavyShapeRms>=0.5、heavyShapeMaxDelta>=1.0且值有限。两者均满足才能OBSERVATION_READY/exit0；ROI缺失保持BLOCKED(roiObservation)/非零，ROI存在但物理未过为FAIL(heavyObservation)/非零。原诊断字段保留，不增加或放宽物理阈值。回归命令：`python -m unittest discover -s test -p test_p011_preflight_gates.py`；真实新包FAIL与旧真实PASS均只读输入，输出至临时文件，禁止改冻结证据。

## 11. P011 SLIP 七相观测正式用例（2026-10-19，target-based 转正）

> 本节只固化客观验收；不替代 TC-P011-01~04。正式校验器为 `test/p011_slip_formal_check.py`，报告为 `docs/test-report-P011-slip-v1.md`。

### 11.0 阶段一验收清单（2026-10-20 阶段边界裁决改版）

> 依据 `docs/priority.md` 2026-10-20「P-011 阶段边界复议裁决」第 4/6/7 条制定。SLIP-02 整包移出阶段一 MUST-PASS；本清单取代此前「SLIP 全 PASS 才算阶段一完成」的旧口径。阶段一不得标记 P-011 CLOSED，也不得写成 SLIP 全 PASS。

**阶段一 MUST-PASS（缺一不可）**：

1. **SLIP-01（豁免口径）**：逐边 tan30 硬约束，按 §11.1 的 ±45° 四相位参考线形式豁免执行；0°/±15° 及其它相位照旧，豁免子步安全闸门完整执行。
2. **SLIP-03 安全部分**：守恒、库存、容量、有限值、终点悬沙 `<=5%`、每相位最后 1s 坡度稳定（`<=1°`）——不延期，阶段一继续生效。
3. **SLIP-04 全部**：local/bottom/total 预算上限原值、clipping 对账 `<=1e-9`、三类预算超额为 0、账本对齐零缺失零错配——不延期。
4. **新阶段一 A/B/C 标准**（来自裁决第 1~3 条）：
   - **A（双角滞回/状态机）**：逐边 start/hold/stop 语义、真实 active 转换、限流不误报 stop、物理边身份与遥测对齐全部通过；按既定 fit/holdout、CI、样本数门槛完成 `thetaStart/thetaStop` 估计，不填默认阈值；`thetaStop` 映射 ±45° 全局窗口的适配检查延期阶段二；标定③继续 `BLOCKED`。
   - **B（安全限幅与提交 guard）**：每提交子步按实际接受参考角检查全边 `|ΔS−tan(effectiveTiltAccepted)|<=tan30°` 零违约；只允许完整事务可行时提交；可行域为空必须不提交并记录 `hold/BLOCKED` 原因；预算、守恒、库存、容量、账本对齐保持现行 MUST-PASS；无限 hold 不算收敛或体验通过。
   - **C（现有机制效果）**：分别验证 ±15° 静止回归无新增运动/越界（稳定保持段按 §11.1.1 的 `start=0 + 表面变化有界` 判定，不要求既有 tan4 relax 停工）、滑移方向与 active/stop 及预算裁剪语义正确、有可复核用户可见的局部渐进斜坡响应（raster/录像可见证据支持，非零 grant 不算通过）；报告全宽 OLS 坡度、R²、通量与 run 时间作诊断/能力记录；守恒、库存、容量、预算、±15° 安全检查不得退化。

**移出阶段一 MUST-PASS（DEFERRED / NOT VALIDATED，阶段二）**：

- **SLIP-02 整包**：`30°±2°` 收敛、`R²>=0.90`、target-based `t90<=5100ms` 及适用的 ±45° 终点稳定/悬沙收敛约束——不以 FAIL 阻断阶段一完成，原始测量继续记录（诊断/能力数据），供阶段二可达性审计使用；`DEFERRED` 不是 PASS。

### 11.1 TC-P011-SLIP-01 逐边 tan30 硬约束

- **前置条件**：完整七相 ledger；每个子步有同 `sequence/frameId/substepId/physicsTime` 的 `P011_METRIC.effectiveTilt` post-update 快照。
- **输入**：每个提交物理子步的 `stateBefore.S` 与 `stateAfter.S`。
- **判定标准**：对每子步全部 127 边，`|S[c+1]-S[c]-tan(effectiveTilt_committed)|<=tan30°`；总违约数必须为 0。禁止使用下一子步或上一相位 `context.effectiveTilt` 作为提交倾角。
- **豁免范围（2026-10-20 manager 裁决）**：仅 `positiveOvershoot`、`positiveOvershootReturn`、`negativeOvershoot`、`negativeOvershootReturn` 四个相位（±45° 稳定平台及其 overshoot/return 子步）豁免上述 tan30 参考线形式判定；依据见 `docs/priority.md`「±45° 补采 gate 几何冲突裁决」。`0°`（baseline/return）、`±15°`（positive）及所有其它相位照旧执行 tan30 判定，不得放宽。豁免 ≠ 坡度达标、≠ 无物理越界、≠ run 自动合格；有限值、守恒、库存、容量、预算、账本对齐、边身份与 stop 语义闸门在豁免子步仍完整执行。
- **分类**：MUST-PASS。

#### 11.1.1 ±15° 稳定保持段静止回归口径（2026-10-20 designer 裁决，DEVIATION-1）

依据本次用户授权裁决及 [预验证报告 §2](preverify-P011-c-plus-slip-v1.md)，澄清 §11.0 C 的稳定保持段判定。**当前 thick、`angleDeg=4°` 机制中，±15°平台上持续的 relax 请求及非零实际 grant 是预期的残差修正行为，不单独判 DEVIATION/FAIL；不要求 `relaxTriggered=false`、`relaxRequestedTotal=0` 或 `relaxAppliedTotal=0`。** 输入角保持不变不等于材料已无残差；已有滑移持续工作也不等于新增 start。本裁决不改机制、不降低 start 死区门槛，不证明材料完全静止或双角标定完成。

- **取段与身份**：分别检查+15°和−15°，使用已冻结的实际接受平台识别口径：目标与accepted角相差≤0.20°、速率数值静止（≤1e-9°/s）、连续≥3个提交子步且跨度≥200ms、窗口角极差≤1e-6°；不得用target、planner冻结或无提交墙钟时间充当平台。到达/ramp、平台进入、保持段分列，不能按是否发生start事后选段。缺少任一符号、真实状态链或合法稳定窗口，记 `INCOMPLETE/BLOCKED`，不得以空样本给PASS。
- **start死区，MUST-PASS**：稳定保持段全部127边真实 `inactive→active` 的转换数必须严格为 **0**，逐子步核验activeBefore/After及前后连续性；任何1次新增start即FAIL，不以比例、平均、持续时间或已有grant抵消。平台开始时已active的边可以继续修正，但须保留入段状态及此前转换记录，不把它伪装成平台新start或删去其历史。
- **表面有界，MUST-PASS**：为避免 `max|ΔS|` 歧义，同时列明空间坡差与时间位移，全部采用原始128列S和同子步actual accepted角，不用平滑raster替代：
  1. 空间：`max_c |(S_after[c+1]−S_after[c])/dx − tan(thetaAccepted)| ≤ tan30°`，按SLIP-01零违约；dx=1时可推得 `max_c |S_after[c+1]−S_after[c]| ≤ tan30°+|tan(thetaAccepted)|`。恰15°时右端约0.845299，不把0.173等本次观测值临时设成新阈值，也不误用 `|ΔS|≤tan30°` 代替倾角参考约束。
  2. 时间：`max_c |S_after[c]−S_before[c]|` 逐提交子步核验现有 **HG-04** 的≤0.5列且变化速度≤30列/秒（即≤`min(0.5,30*physicsDt)`）；帧内多子步分别检查，不把相邻列高差当成时间位移。local/bottom/total额度与现行数值契约原值保持，不新增epsilon或追加额度。
- **既有安全与终点检查仍必过**：SLIP-03的守恒/库存/容量/有限值、每相位末1s坡度变化≤1°及终点悬沙≤5%，SLIP-04全部预算与对账、逐writer guard均不豁免。持续relax若引发任一超限，照常FAIL；`start=0`不能掩盖表面跳变、持续失稳或越界。
- **报告字段与适用边界**：每符号列稳定记录数/时长、入段active数、真实start/stop数、relax请求/实际量、空间最大坡差、最大倾角残差、时间最大单步位移/速度及末1s稳定指标；相对平台首帧的累计形变另作诊断，不称“零运动”。预验证135/135持续请求、均值request≈12.09/grant≈2.88只按本条重新归类为机制预期，**不追认正式PASS**。其旧tan4请求状态不能冒充v3已执行的双角状态；未来真双角路线若出现与自身start/stop/请求契约不一致的非零grant，仍判机制/身份缺陷，不能套用本条豁免。

结论：本静止回归子项采用 **`两个符号各自 start=0 + 表面空间/时间变化有界 + 全部既有安全/终点闸门通过`**；relax非零本身不构成违约。此前“稳定保持段relax grant必须为0”的解释在当前tan4路线不适用。tester据此核对正式报告/检查器口径；本次仅更新用例文档，未修改checker或采集证据。

### 11.2 TC-P011-SLIP-02 ±45° raster 收敛与 target-based t90

- **适用阶段**：**阶段二（DEFERRED / NOT VALIDATED，2026-10-20 裁决）**。依据 `docs/priority.md` 2026-10-20「P-011 阶段边界复议裁决」第 4/6 条：整个 SLIP-02 目标包（含 `30°±2°` 收敛、`R²>=0.90`、target-based `t90<=5100ms` 及适用的 ±45° 终点稳定/悬沙约束）移至阶段二可达性审计之后，不以 FAIL 阻断阶段一完成，移出阶段一 MUST-PASS；但仍须持续列为 `DEFERRED / NOT VALIDATED`，**不是 PASS**。
- **测量要求保留**：本用例不删除、测量要求不删除——raster 全宽轮廓解析、OLS 坡度/R²/RMSE、target-based t90 等原始测量在阶段一继续记录，作为诊断/能力数据供阶段二可达性审计使用。
- **前置条件**：`positiveOvershoot` 与 `negativeOvershoot` 的 raster 全宽轮廓均可解析。
- **输入**：相位内所有 raster 样本。
- **判定标准**：相位最后一次 raster 全宽 OLS 线性坡度 ∈ `30°±2°`，`R²>=0.90`，并报告 RMSE（rad/column）；`t90` 为 target-based 首次线性坡度达到 `27°`（30° 目标的 90%）的相对时间，且 `<=5100ms`。禁止以本相位实测终值的 90% 计算 t90。
- **分类**：MUST-PASS。

### 11.3 TC-P011-SLIP-03 相位终点稳定与悬沙预算

- **前置条件**：七相 ledger 完整，初始沉积沙总量取首子步 `stateBefore` 的 `bh+bl` 总和。
- **输入**：每相位最后 1s ledger 状态与相位终点 `suspH+suspL`。
- **判定标准**：每相位最后 1s OLS 坡度变化 `<=1°`；相位终点 `suspH+suspL<=0.05×初始沉积沙总量`。
- **适用阶段**：**不延期，阶段一继续生效**。守恒、库存、容量、预算、有限值等安全不变量按 2026-10-20 阶段边界裁决不随 SLIP-02 目标包延期；其中「每相位最后 1s 坡度稳定」作为安全/稳定不变量在阶段一继续执行，±45° 相位的收敛达标（30°±2°）归属 SLIP-02 阶段二口径。
- **分类**：MUST-PASS（安全不变量部分）。

### 11.4 TC-P011-SLIP-04 预算与账本对齐

- **前置条件**：完整 ledger 与同子步 `P011_METRIC`。
- **输入**：逐子步 `budgets`、audit 预算超额字段、`context.effectiveTilt`、同子步 metric post-update `effectiveTilt`。
- **判定标准**：local/bottom/total 预算上限全程原值，不放宽、不追加：`localUsed<=localLimit`；`bottomUsed<=min(bottomRequested,bottomLimit)`；`totalUsed<=min(totalRequested,totalLimit)`。每子步还须满足 `abs(bottomClipped-(bottomRequested-bottomUsed))<=1e-9` 与 `abs(totalClipped-(totalRequested-totalUsed))<=1e-9`；clipping 是共享预算竞争下的设计内行为。relax 可正常工作，不要求 `relaxAppliedTotal==0`；其合法性由预算上限、SLIP-01 tan30 约束与 §19.5 writer guard 分别检查。三类预算超额仍须为 0。ledger `context.effectiveTilt` 与同子步 `P011_METRIC.effectiveTilt` 均为 `updateTilt()` 后参考，并绑定同一子步身份字段；缺失或身份错配数为 0。
- **适用阶段**：**不延期，阶段一继续生效**。预算上限、clipping 对账、三类预算超额、账本对齐等全部安全不变量按 2026-10-20 阶段边界裁决不随 SLIP-02 目标包延期，阶段一 MUST-PASS 保持原值。
- **分类**：MUST-PASS。

### 11.5 v1 历史证据 RED 基线

- **输入**：`test/artifacts/p011-slip-overshoot-v1/`（只读）。
- **历史旧口径结果**：旧 checker 把 `relaxAppliedTotal==0` 和 `clipped==0` 当作 MUST-PASS，导致 1,738 个子步被记为预算违约；其中 relax 在 ±45° 崩塌段正常工作，clipping 也发生于倾斜阶段的共享预算竞争。该历史计数仅说明旧判定口径，不代表真实预算越界。
- **修订口径重算**：预算 MUST-PASS 改为原预算上限 + clipping 对账（误差 `<=1e-9`）+ 三类预算超额为 0；不因 relax 工作或发生裁剪而失败。刷新后的实际 checker 结果见 `docs/test-report-P011-slip-v1.md`，硬坡度、raster 收敛、target-based t90 等其它 RED 项保持独立判定。
- **豁免不溯及既往（2026-10-20）**：上述 ±45° 参考线形式判定豁免不追认 run-001/002 的旧判定，也不重写历史结论；历史报告中的 FAIL 记录原样保留，新旧判定结果在 `docs/test-report-P011-slip-v1.md` 中并列可查。

### 11.6 KNOWN_FAIL / DEFERRED_TO_PHASE_2 legacy FAIL 台账（2026-10-20，C-2 路由）

> 依据 `docs/priority.md` 2026-10-20「P-011 阶段一·B CONDITIONAL_PASS 裁决」第 3 条（C-2）。完整台账（定性、证据、纪律、阶段二重测计划）见独立清单 `docs/known-fails-P011-v1.md`。要点：2 个 legacy FAIL（`bottomCreep_tiltMovesLowerLayers`、`p011_commandSignFlipResetsPhaseAndReversesDisplacement`）为新闸门如实暴露的旧引擎真实 tan30 违约，不归因于 4b1f635 新回归；阶段一维持文档化 FAIL 并在总验收报告中明示。不放宽 tan30 判据、不改场景参数/测试期望、不删除失败证据、不删除 checker 白名单标记；阶段二由 tester/designer 结合 B·续 guard 与候选机制复核重测。它们不阻断【阶段一·B】实现完成，但未裁定前不得声称已解决。

### 11.7 TC-P011-OBS3-CALIBRATION 新 protocol clean pilot 可观测性

- **适用 run**：观测③ calibration run；新 protocol 必须预注册 schedule `0/+15/0/+30/0/-15/0/-30/0`、schedule SHA256、各平台 hold 时长、平台角容差、候选阈值身份、排除规则与随机化方案。旧 ±45° schedule/run 仅作历史证据，不计入新 protocol 的 fit/holdout 或 clean pilot bracket。
- **平台定义**：仅以逐子步实际 `effectiveTilt` 及其 rate 判定平台；目标角或命令相位不能代替 accepted plateau。平台容差必须从冻结 manifest 读取；缺失时报告 `INCOMPLETE / PROTOCOL_METADATA_MISSING`，不得由 checker 自选容差。ramp、非零 rate、planner hold、请求拒绝、平台容差外样本均不属于平台。
- **真实 bracket**：start 只接受真实同边 `activeBefore=false → activeAfter=true`；stop 只接受真实同边 `activeBefore=true → activeAfter=false`。需为同 run、同候选、同物理 edge 的紧邻连续有效子步，且前后实测 `alpha=atan(abs(d)/dx)` 跨过候选阈值。必须记录 request/grant/budgetScale、effective/accepted angle 及拒绝/限流状态。`holdActive`/`holdInactive`、平台到达或结束、reset、数据断档均不是转换。预算限流（含 grant=0）期间不得构造 stop/bracket；planner hold 或拒绝请求期间出现的 transition 只能列原始遥测，不能列合格 bracket。
- **删失与 run 级状态**：若 start 搜索上端无真实越阈 start，且安全可提交样本最大实际 `alpha` 约为 29.9°，该 start cell 标 `RIGHT_CENSORED_AT_TAN30`，整体标 `INCOMPLETE`；这不是阈值 FAIL、PASS、点估计或“阈值高于上限”的证据。无真实 stop 时标 `STOP_TRANSITION_UNOBSERVED`，不得以 hold/限流补 stop。cell 样本不足标 `INCOMPLETE / DESIGN_PENDING_OBSERVATION`；四 cell（`start/+`、`start/-`、`stop/+`、`stop/-`）分开统计。
- **验收边界**：clean pilot 仅审查 schedule/平台可达性、真实 bracket yield、右删失及拒绝/限流归因；`thresholdVerdict=NOT_EVALUATED`，不得判阈值 PASS/FAIL。hard tan30、逐边安全、守恒、库存、容量、预算、ledger/telemetry 对账仍由既有 safety checker 原样 MUST-PASS；本用例不豁免、不改变任何安全门槛。既有独立 run 数、每方向/每候选转换数、分层 cell 样本下限、run-cluster CI、fit/holdout 隔离与误判率标准全部不变。clean pilot 不计入 fit/holdout，也不授权扩样或改阈值。
- **checker/证据**：`test/p011_calibration_protocol_check.py` 输出 calibration 可观测性与 `INCOMPLETE`/删失分类；协议 metadata 缺失、planner telemetry 缺失必须显式列出，不得猜值。已有 `p011_slip_formal_check.py` 及安全 checker 继续独立执行；checker 适配不得降低 safety MUST-PASS、样本门槛、CI 或 holdout 标准。

### 11.8 TC-P011-OBS3-V2 观测③ v2 协议闸门（平台识别+四 cell+删失码，2026-10-20 预注册）

> 依据已验收的 `docs/observation-protocol-P011-v2-preregistration-v1.md` §4-§7。校验器：`test/p011_obs3_v2_protocol_check.py`（独立 v2 通道，不修改 v1 `p011_calibration_protocol_check.py` 与 `p011_slip_formal_check.py` 的既有输出）。协议身份固定：`protocolId=p011-observation3-v2`、`protocolRevision=preregistration-v1`、`analysisRole=pilot`。本用例只改闸门不运行采集；旧七相 run、随机化 manifest、平台资格不迁入。

- **协议元数据闸门**：manifest 必须携带 `calibrationProtocol.protocolId/protocolRevision/scheduleTargets=[0,15,0,30,0,-15,0,-30,0]/scheduleSha256`（64 位 hex）；缺失或不匹配逐项记入 `protocolMetadataErrors`，不自动补默认值。run 须 `runRole=calibration` 且 `observationRouteId=natural-relax-hysteresis-v1`。
- **平台识别（§4）**：仅以实际 `effectiveTilt` 判定平台，禁止用 targetTilt 代替——到达候选平台 = `abs(effectiveTilt−T)≤0.20°`（T∈{0,±15,±30}）且 `effectiveTilt` 与 `thetaAccepted` 数值一致（容差 `1e-9°`）。0.20° 仅区分平台身份，不是 tan30 安全余量，残差硬闸门照常逐边核验。稳定接受平台另需：`abs(effectiveTiltRate)≤1e-9°/s`、窗口内 effectiveTilt 极差 `≤1e-6°`、连续 ≥3 提交子步、首尾跨度 `≥200ms`、无缺步/重排/重置/身份改变。资格自完成窗口的当前记录起算，不回填窗口前段；离开即重置，重新进入须重新累计 200ms。`thetaAccepted` 缺失时记 `TELEMETRY_INCOMPLETE`，不视为一致。
- **planner 资格分离（§4.2）**：稳定在 ≈29.9° 可记为到达诊断，但本子步或前一 bracket 端点存在 `thetaRequested≠thetaAccepted`（1e-9° 容差）即失去标定资格；checker 按 `plannerLimitedStableRecords` 计数并另列，不把 hold 伪装成 stop 或 bracket。
- **四 cell 统计（§5）**：`start/+、start/-、stop/+、stop/-` 按**稳定平台 `effectiveTilt` 符号**归属（禁止 donor/残差/命令符号代替）；0° 相为 neutral 诊断，零相 stop 不移入正负 cell。物理边身份 `c=interfaceIndex=min(donor,receiver)`，搬运记录须 `|donor−receiver|=1`，否则丢弃。状态样本（`activeBefore` 归类）与真实转换（false→true=start、true→false=stop）分母分列；`grant=0`、phase/run 结束、reset、planner hold、限流均不算转换。合格 bracket 另需同 run/候选/几何/相位/稳定段、`substepId` 连续、前一 `activeAfter`=本步 `activeBefore`、两端平台合格且非预算裁剪。每 `(runId, phaseIndex, c)` 只取首个合格转折；**每 run 每主 cell 最多 3 个**，按时间/substepId/c 取前 3，不按残差选样。
- **七类删失/未观测码（§6，全部不产点估计）**：`RIGHT_CENSORED_AT_TAN30`（start 高端无双侧 bracket 覆盖，报告实际残差端点与约束原因，不补事件、不拟合、不记 PASS）；`STOP_TRANSITION_UNOBSERVED`（细分 neverActive/alreadyStoppedBeforePlateau/stillActiveAtEnd/onlyPlannerHold/onlyBudgetLimited/noEligibleBracket，不把结束/hold/grant=0/inactive 到达当 stop）；`START_OUTSIDE_ELIGIBLE_WINDOW`（start 只在 ramp 或进平台已 active，不推高端删失也不入拟合）；`PLATFORM_UNREACHED` / `PLATFORM_UNSTABLE`（不凭 target=30 推 alpha=30）；`TELEMETRY_INCOMPLETE`（未知不当作无转换/无 hold/已删失）；`RUN_INVALID_HARD_GATE`（隔离整个 run，不挑好子步）。
- **pilot 三维判定（§8，独立输出，禁止改写为标定 PASS）**：`clean`（元数据零错+硬闸门零失败+遥测零缺失）、`platformReachability`（四个非零相均有 ≥200ms 稳定窗口且无 planner 限制记录）、`pilotCoverage`（四 cell 均 ≥1 首转折 bracket → `FOUR_CELL_OBSERVED`，任一为 0 → `INCOMPLETE` 附删失原因）。允许 `clean=PASS, platformReachability=PASS, pilotCoverage=INCOMPLETE` 组合并如实报告。`thresholdVerdict=NOT_EVALUATED` 恒定，`thetaStartEstimate/thetaStopEstimate/CI` 恒 null；pilot 不计 34-run 配额、不触发 bootstrap、不读 holdout。
- **安全闸门**：tan30、预算、守恒、库存、容量、有限值、遥测对齐、边身份、stop 语义全部由既有 safety checker 原样 MUST-PASS，本通道不豁免、不复制、不弱化。
- **checker/证据**：`python test/p011_obs3_v2_protocol_check.py --ledger <ledger.ndjson> [--manifest <manifest.json>] --output <out.json>`。空 manifest/缺字段输出见 `test/artifacts/p011-slip-formal-v1/obs3-v2-empty-baseline.json`；最小合成验证（平台窗口+四 cell 归属+stop bracket 计数+每 run 每主 cell ≤3 上限）见同目录 `obs3-v2-minimal-verify.json`。正式采集 run 的判定由运行任务执行，本用例不跑。

### 11.9 TC-P011-OBS3-V3 观测③ v3 协议闸门（八 leg+资格公式+覆盖矩阵，2026-10-20 方法冻结）

> 依据已冻结的 `docs/observation-protocol-P011-v3-preregistration-v1.md`（`protocolId=p011-observation3-v3` / `protocolRevision=preregistration-v1`，METHOD_FROZEN / NO_CAPTURE_AUTHORIZATION）。校验器：`test/p011_obs3_v3_protocol_check.py`（独立 v3 通道，不修改 v1/v2 checker）。合成单测：`test/test_p011_obs3_v3_protocol_check.py`（38 断言全绿）。核验跑真数据由【观测③·v3核验】任务串行执行，本用例只改闸门。

- **协议元数据与执行身份（§1/§2）**：manifest 须携带 protocolId/protocolRevision/scheduleSha256/configuredThresholds；configured 阈值须在搜索窗内（start `[25°,30°)`、stop `[15°,25]`、stop<start）。`executedThresholds` 必须含 thetaStart/StopDeg、slopeStart/Stop 及 `stateMachineVersion/sourceCommit/sourceFileSha256/buildSha256`——仅复制 manifest 到日志不满足；不一致或缺失整 run 记 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH + UNOBSERVABLE`，估计恒 null。事件字段命名为 `observedStartTransition/observedStopTransition`，与 configured/executed 分列。
- **decision 时点契约（§2.1）**：`decisionG=(S[c+1]−S[c])/dx`、`decisionR=decisionG−tan(effectiveTiltAccepted)`、`decisionAlpha=atan(abs(decisionR))`，取值点为状态机读残差选分支之前；`thresholdUsed` 记录 `{branch, angleDeg, slopeLimit, comparator}`；`transitionReason` 区分 START/STOP_THRESHOLD_CROSSED 与 HOLD_INACTIVE/HOLD_ACTIVE。状态翻转必须来自运行状态机，不由 probe 按候选重构。
- **八 leg 与边界归属（§3）**：平台序列 `0→+15→+A→+15→0→−15→−A→−15→0`（H0-H8，每平台 hold 5100ms）；L1-L8 leg 身份采前 schedule 赋值，不按观测重分段。半开区间：终点命令归下一平台。命令步距 100ms、角步距 0.05°（主档 0.5°/s）/0.10°（敏感性 1.0°/s）；`n=|T1−T0|/(0.1·v)` 与峰值网格（`A=15.1..29.9` 共 149 点）必须整除可精确到达。
- **bracket 资格公式（§5.2）**：same run/executed candidate/geometry/fixed edge/rate tier/same preregistered leg（或合格稳定平台段）；相邻已提交子步且无非提交 attempt 隔断；前一 `activeAfter`=本步 `activeBefore`；两端无 planner clamp/hold/reject/rollback（`thetaRequested≠thetaAccepted` 1e-9° 即不清白）；两端无预算/guard/库存/容量裁剪歧义（requested=0 且 granted=0 的真实 stop 不算限流）；start false→true 且 `alphaPrev<thetaStartExecuted≤alphaNow`；stop true→false 且 `alphaPrev>thetaStopExecuted≥alphaNow`；**`bracketWidth=|alphaNow−alphaPrev|≤0.5°`**——宽事件记 `BRACKET_TOO_WIDE` 不切分，bootstrap 也不能补救。
- **3 点单调窗口（§5.2）**：两端+紧前同 leg decision 按事件方向单调（start 非降、stop 非升，允许相等）；窗口仅由相邻记录决定，不向前搜索有利端点；不满足记 `NONMONOTONE_LOCAL_WINDOW`，不平滑 alpha、不删中间记录造 bracket。
- **首转折规则（§5.2）**：对每 `(runId, segmentId, fixedEdge)` 按时间序保留首个满足全部规则的真实转折（start/stop 先到先得，非各留一个）；后续转折留原始审计不入估计；采前选段固定，不按峰值/宽度裁剪。
- **每 run 每主 cell ≤3**：按时间/substepId/c 取前 3，第 4 个起保留审计不入估计（`cappedTransitions` 单列）。
- **七类+新增输出码（§6/§8，全部不产点估计）**：`RIGHT_CENSORED_AT_TAN30`（v3 新定义：**不再要求平台到达**，四条件全满足才可赋——执行身份真实+inactive 风险态完整合法已提交记录且 alpha 上界可审计低于候选 start+planner/guard 约束证据明确是 tan30 边界+leg 身份/符号完整；被拒 attempt 不作观测端点）；`STOP_TRANSITION_UNOBSERVED` **九细分**（NEVER_ACTIVE/STOP_BEFORE_ELIGIBLE_SEGMENT/STILL_ACTIVE_AT_END/ONLY_NEUTRAL_ZERO_ENDPOINT/ONLY_PLANNER_HOLD_OR_CLAMP/ONLY_BUDGET_GUARD_OR_STOCK_LIMITED/BRACKET_TOO_WIDE/NONMONOTONE_LOCAL_WINDOW/CONTINUITY_MISSING）；`START_OUTSIDE_ELIGIBLE_WINDOW`；`PLATFORM_UNREACHED/UNSTABLE`（v3 不要求平台产生 start，仅诊断）；`TELEMETRY_INCOMPLETE`；`RUN_INVALID_HARD_GATE`；`EXECUTION_THRESHOLD_IDENTITY_MISMATCH+UNOBSERVABLE`；`EXECUTION_ENVELOPE_UNRESOLVED`（冻结网格内无 A 满足 SHADOW_SAFE && SHADOW_COVERED，不填 29.9° 默认）；`SHADOW_PREDICTED_CENSOR`（影子可见删失，不替代真实 run 证据）。
- **pilot 三维判定（§8）**：`clean`/`platformReachability`/`pilotCoverage` 独立输出，允许 clean=PASS+reachability=PASS+coverage=INCOMPLETE 组合如实报告；`thresholdVerdict=NOT_EVALUATED` 恒定，`observedStartTransition/observedStopTransition/thetaStart/StopEstimate/CI` 恒 null；不拟合 wNoise、不解封 holdout。
- **可达覆盖矩阵（§7）**：四主 cell+24 子 cell 行零记录也保留；列含 cellId/parentCell/residualSign/noiseBand/atRisk 系列/alpha 系列/trueEventCount/eligibleBracketCount/stateSampleCount/cappedCount/runCount/required/deficit/noiseDefinable（pilot 恒 `UNDEFINED_NO_FIT_NOISE`）/exclusionCounts/coverageStatus 五值（OBSERVED/NOT_OBSERVED/UNDEFINED/PROVEN_UNREACHABLE_IN_REGISTERED_CLASS/UNKNOWN）/structuralAssessment；影子与试采同构分表 `source=shadow|pilot`。
- **统计门槛常量（§6 原值继承）**：34run/pair（24fit+10holdout，每组每方向≥5run）、四主 cell ≥30 状态/≥10run、每 run ≤3、每方向每 type ≥20 事件/≥5run、24 子 cell 同门槛、bootstrap 10000 次 seed=20261019、CI 宽≤2°、fit/holdout 差≤1°、holdout 出 fit CI 外≤5%、子 cell 误判率≤5%、lower95(start)>upper95(stop)、点间隔≥2wNoise、start 上界<30°、敏感性档独立 68run——全部写进 checker `statsGates` 常量。
- **安全闸门**：现行硬闸门（tan30/planning、逐 writer guard、预算、守恒、库存、容量、有限值、账本对齐、边身份、stop 语义）零 capture 豁免，由既有 safety checker 原样执行；本通道不豁免、不复制、不弱化。
- **单测证据**：`python test/test_p011_obs3_v3_protocol_check.py` 输出 `PASS=38 FAIL=0`——覆盖平台窗口正反例（非零 rate 重置/慢 ramp 不冒充/不回填）、八 leg 边界归属、bracket 宽度 0.5° 边界值（0.5 合格/0.6 TOO_WIDE）、3 点窗口单调/非单调、首转折取舍、每类输出码至少 1 例（含 RIGHT_CENSORED 四条件正反例）、pilot 三维独立组合、每 run ≤3 cap、覆盖矩阵结构。

## 12. 观测①b：激活逐边转折遥测交付验收（2026-10-19，架构冻结）

### 11.10 TC-P011-OBS3-D1 观测③ D-1 整改核验用例（LEGACY 轨迹对比+双条件资格门分流，2026-10-21 RED 基线先行）

> 依据 manager D-1 裁决（priority.md 2026-10-21「D-1 correction 越权激活裁决」：Option A + 显式路线隔离）与 architecture-P011 §26.2 跨版本对比方法。checker：`test/p011_obs3_d1_verification_check.py`；证据 runner：`test/run_p011_obs3_d1_verification.cjs`（Node 1:1 通道，纯 checker 层，真机不涉及）。本用例为【观测③·v3核验】6ad709d6 第 6 项核验点的前置就位：RED 基线如实记录，整改交付后直接跑绿。

- **D1-C1 LEGACY 轨迹对比（§26.2 方法复用）**：LEGACY run（无候选阈值）240 步同场景（8 列 thick、±20° 每 24 步交替、totalBudget=1000）必须 240/240 全 ok、零 BLOCKED，且 14 个定点锚点（每 20 步采样 + step25 首个历史分歧点 + step214 历史 BLOCKED 点）与 5e60f2d 冻结基线逐值一致（effectiveTilt/sandTop[0]/bh[0] 容差 1e-9，sandTop 列和容差 1e-6）。锚点数值冻结于 checker 与 runner 双侧；任一锚点偏差即 RED。**当前构建实测：240/240 ok、14 锚点全过（D1-C1 PASS）**——developer 的 Option A 整改（correctionRouteActive 资格门）已落工作区，轨迹等价已恢复。
- **D1-C2 双条件资格门分流（裁决第 2 条）**：四场景断言 apply 路线归属——(a) LEGACY 默认（状态机 off+无 route）→ legacy relaxAngle apply；(b) 候选 run（`natural-relax-hysteresis-v1` 身份、状态机 armed 但**未绑定** correction route）→ 同样 legacy apply（**v3 身份不是 correction 的隐式开关**）；(c) 显式 correction route（`observationRouteId=relax-correction-weighted-v1` **且**候选阈值 armed）→ 才走 `applyCorrectionSlot`（必要+充分双条件）；(d) route 绑定但状态机 off → legacy apply（必要条件缺失）。legacy 路线附加语义断言：requested>0 必须伴随 applied 流（D-1 缺陷签名 requested>0/allowed=0 在 legacy 路线出现即 FAIL）；correction 路线断言 slot 实际运行。**当前构建实测：四场景全 PASS**。
- **D1-C3 correction 遥测进 audit（D-1b，RED 基线）**：correction route run 的每个已提交子步 audit 必须携带 `correctionRequested/Granted/Rejected/StalledCount` 四字段且满足闭合恒等式 `requested==granted+rejected`（1e-9）。**当前构建实测：`P011NaturalObservationAudit` 无这四个字段 → RED_BASELINE（by design）**；developer 补齐 D-1b 后本用例转绿，correction 效果报告方具备放行资格（裁决第 3 条）。
- **RED 基线证据**：`test/artifacts/p011-obs3-d1/d1-verification-evidence.json`（runner 产出：240 步轨迹锚点实测值+四场景 gate/路由观测）与 `d1-verification-result.json`（checker 判定：overall=RED_BASELINE，redBaselineCases=[D1-C3]）。运行命令：`node test/run_p011_obs3_d1_verification.cjs <evidence.json>` + `python test/p011_obs3_d1_verification_check.py --evidence <evidence.json> [--ledger <corr-ledger.ndjson>] --output <result.json>`。
- **边界**：只写用例与 checker 断言，不改产品代码、不降门槛、不授权采集；LEGACY 基线锚点为冻结值，禁止随实现漂移改锚点（漂移=行为变更，须走 D-1 复审而非改测试）。整改交付后本用例直接跑绿即 = v3核验第 6 项核验点提前就位；architect 复核与 tester 修复构建重跑仍按裁决第 4 条串行执行。

### 11.11 TC-P011-OBS3-V3-VERIFY 观测③ v3 核验（五项清单 + D-1 整改核验，2026-10-21 执行完毕）

> 核验对象 commit `742efa6`（D-1 整改构建）；**原核验结论已废弃并在 742efa6 上全部重跑**（被测构建行为漂移）。报告：`docs/verification-report-P011-obs3-v3-d1-v1.md`。六项结论：全 PASS（D-1b 遥测按裁决定位如实记录 RED，不阻断）。

- **核验 1 执行身份 PASS**：checker 区分 configured/executed 且要求实现身份四字段；候选 run 实测 `executedThresholds{26,20}` 与配置一致、实现身份齐备——v2 根因（阈值未进状态机）确认已修；decision 字段（decisionG/R/Alpha/Ordinal/thresholdUsed/transitionReason）实测 70/70 边完整。
- **核验 2 相名/完整性 PASS**：`P011ScheduleV3.ets` 九平台+八 leg+半开区间+CSV 契约落地（runner 内单测绿）；九相执行证据按 ±15/±30 相分列，无记录相位如实报 UNREACHED/UNSTABLE，第 9 回零段由 baseline/*Return 相位名承载。
- **核验 3 硬安全 PASS**：全部 MUST-PASS 原样生效（safety checker schemaVersion 2 判定不变、runner 76/76）；BLOCKED attempt 可审计实测（attemptId/commitStatus='blocked'/rollbackReason='PREEXISTING_INFEASIBLE_STATE'/planner telemetry 在账）。
- **核验 4 新 ramp 资格过滤 PASS**：预注册 leg、bracket ≤0.5°（TOO_WIDE 不切分）、首转折、3 点单调窗口、决策残差非空全部在 checker 实现；合成单测 38/38（742efa6 重跑）。
- **核验 5 输出规则 PASS**：codeCatalog 12 码分支齐全（含 v3 新码 EXECUTION_IDENTITY_MISMATCH/ENVELOPE_UNRESOLVED/SHADOW_PREDICTED_CENSOR）；身份失败传播实测（四 cell 携带 mismatch 码、overall=INVALID）；全部不产点估计。
- **核验 6 D-1 整改 PASS（D-1b 如实记录）**：D1-C1 轨迹对比（240/240+14 锚点含 step25/step214）PASS；D1-C2 四场景分流 PASS（缺陷签名零命中）；D1-C3（D-1b 四遥测字段）如实记录 RED——按裁决第 3 条不阻断本核验，转绿前 correction 效果报告不放行。
- **遗留**：D-1b 遥测四字段落 audit（developer，correction 效果报告放行前置）。本核验不追认历史 run、不发布阈值估计；pilot 待 manager 批准。

> 本节为 manager 观测①b 任务的验收 checklist 备注与交叉引用；完整判定口径、代码锚点、工程边界与交付要求见 `architecture-P011.md` §19.13。四项 MUST-PASS，全部勾选后才可提交正式七相 run 冻结证据。

- [ ] **□1 转折记录字段完整**：正式七相 run ledger `slip[]` 每条边记录 activeBefore/activeAfter/transitionType(start|stop)/effectiveTilt/残差 r 全部非空有效，transitionType 与状态组合一致。
- [ ] **□2 无 unavailable 残留**：正式七相 run ledger `slip[]` 中 transitionType=unavailable 计数=0；计数范围仅 relax 滑移通道，bottomCreep/suspensionDiffusion 的设计内 unavailable 不计入且不得伪造；合成自测 ledger 允许。
- [ ] **□3 transitionTelemetryAvailable=true**：每个 ledger record `context.transitionTelemetryAvailable===true` 且 schema v3；合成 ledger 单测（`test/test_p011_transition_telemetry.py`）全部 PASS。
- [ ] **□4 物理逻辑与参数零改动**：relaxAngle 判据、local/bottom/total 预算、全部物理常量逐字不变；probe=null 同输入状态序列等价。

验证资产：`test/p011_v210_natural_observation_check.py`（`validate()` 承载 □1/□3 合成校验）、`test/test_p011_transition_telemetry.py`（合成 ledger 转折契约用例）。正式 run 的 □2 专项计数在 ledger 冻结后执行，计数口径详见 architecture §19.13.1。

## 13. P-011 路线 C 双场粒子 DUALFIELD-* 客观用例（2026-10-24 预注册，RED 基线）

> 依据 `docs/preregistration-P011-dual-field-particle-v1.md`（`protocolId=p011-dual-field-particle-v1` / `revision=preregistration-v1`，METHOD_FROZEN / NOT_IMPLEMENTED / NO_CAPTURE_AUTHORIZATION）与 `design-P011-falling-sand-vertical-routes-v1.md` §4/§6；路线 C 为用户 2026-10-24 选型（真实活动粒子 + 128 列高度场，手势掉沙与摇晃扬沙共同架构）。可执行闸门：`test/p011_dual_field_gate_check.py`；合成自测：`test/test_p011_dual_field_gate_check.py`（36 断言全绿）。真实归档账本 RED 基线证据：`test/reports/dual_field_gate_red_baseline_v1.json`。

- **守恒组（DUALFIELD-CONS-01..04）**：三相+粒子守恒恒等式 `VH_actual = Σ(bh+mh+suspH) + Σ_particle(H) mass`、`VH_expected = VH0 + sourceInjectedH`（外源只增一次），逐子步 drift≤1e-9；susp↔粒子交换原子性（互斥双计检测）；落床部分授予余量滞留不丢不双计（`granted+remainder==requested`）；depositedFraction 分母=累计期望、t90 跨 epoch 无效。
- **预算组（DUALFIELD-BUD-01..03）**：LAND/CROSS_COLUMN/LIFT 四层计费逐层可审计（local `28·dt` → bottom `10·dt` → total `300·dt` → §19.5 tan30 guard 联合预演）；跨列 300/s 上限且无免费传输通道；满额链（确定性合并保总量+一阶矩 → susp 列均值降级 + `PARTICLE_DEGRADED_TO_COLUMN` 旗标）质量闭合。`Nmax=512` 为候选非冻结值。
- **生命周期组（DUALFIELD-LIFE-01..04）**：reset/invert 粒子重映射与清理；模式切换（TEST↔EXPERIENCE）/手势清理后已提交账本保留；snapshot/checker 扩展且旧数据不假迁移；旧无源 run 等价回归（04a 关闭守卫今日可执行期望 PASS；04b 配对锚点待 C 实现交付时由 tester+developer 共同冻结，漂移=行为变更走复审）。
- **来源分账组（DUALFIELD-SRC-01..03）**：`sourceKind=GESTURE_EXTERNAL|SHAKE_LIFT` 两入口不混账（`bySourceKind` 闭合、LIFT 禁触 IH/IL）；admitted=0 零粒子生成；被拒注入零粒子。
- **场景组（DUALFIELD-GES-01..02）**：手势掉沙累积全链五观测点（首粒延迟/漏段率/交点误差/消失与 settle grant 时差/床面增长=准入沉积）；TEST 与 EXPERIENCE 同一物理角与触点路径。TIERED 阈值待效果试采后另稿冻结，本组不冻结任何观感数值。

| 用例 ID | 组 | 覆盖 | 门类 | 今日 RED 状态 |
|---|---|---|---|---|
| DUALFIELD-CONS-01 | 守恒 | §3 三相+粒子守恒恒等式（per-substep drift≤1e-9） | MUST-PASS | NOT_IMPLEMENTED（缺 `particles` 块） |
| DUALFIELD-CONS-02 | 守恒 | susp↔粒子互斥原子性（事件账本双计检测） | MUST-PASS | NOT_IMPLEMENTED（缺 `particleEvents`） |
| DUALFIELD-CONS-03 | 守恒 | 落床部分授予余量滞留不丢不双计 | MUST-PASS | NOT_IMPLEMENTED（缺 `particleEvents`） |
| DUALFIELD-CONS-04 | 守恒 | depositedFraction 分母=累计期望+t90 epoch 无效 | MUST-PASS | NOT_IMPLEMENTED（缺 `particles`/epoch 联动字段） |
| DUALFIELD-BUD-01 | 预算 | LAND/CROSS/LIFT 四层计费逐层可审计 | MUST-PASS | NOT_IMPLEMENTED（缺事件预算字段） |
| DUALFIELD-BUD-02 | 预算 | 跨列 300/s 上限+无免费传输通道 | MUST-PASS | NOT_IMPLEMENTED（缺 `CROSS_COLUMN` 事件） |
| DUALFIELD-BUD-03 | 预算 | 满额合并→降级链质量闭合（PARTICLE_DEGRADED_TO_COLUMN） | MUST-PASS | NOT_IMPLEMENTED（缺 `MERGE/DEGRADE` 事件） |
| DUALFIELD-LIFE-01 | 生命周期 | reset/invert 粒子重映射与清理 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-LIFE-02 | 生命周期 | 模式切换/手势清理/已提交账本保留 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-LIFE-03 | 生命周期 | snapshot/checker 扩展+旧数据不假迁移 | MUST-PASS | NOT_IMPLEMENTED（缺 schema v7） |
| DUALFIELD-LIFE-04 | 生命周期 | 旧无源 run 等价回归（04a 关闭守卫可执行；04b 配对等价） | MUST-PASS | **04a 今日可执行（期望 PASS）**；04b NOT_IMPLEMENTED |
| DUALFIELD-SRC-01 | 来源分账 | sourceKind 两入口不混账 | MUST-PASS | NOT_IMPLEMENTED（缺 `bySourceKind`） |
| DUALFIELD-SRC-02 | 来源分账 | admitted=0 零粒子生成 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-SRC-03 | 来源分账 | 被拒注入零粒子 | MUST-PASS | NOT_IMPLEMENTED（缺粒子层） |
| DUALFIELD-GES-01 | 场景 | 全链五观测点（TEST 构造触摸） | TIERED（阈值待试采冻结） | BLOCKED（缺粒子层+真机/seg3 计时，等 s134 修复构建 6adb3508/6adb7038 或附数据污染声明） |
| DUALFIELD-GES-02 | 场景 | TEST/EXPERIENCE 同一物理角与触点路径 | MUST-PASS（路径同一性） | BLOCKED（缺粒子层+双模式真机对照） |

- **RED 基线规则**：NOT_IMPLEMENTED 须列确切缺失字段路径（不用渲染像素/tracer 点数/视觉估计冒充）；BLOCKED 依赖真机采集或 seg3 计时修复（等修复构建或附数据污染声明）；路线关闭守卫（`particleRouteActive` 缺失或 false 时任何粒子字段出现即 LEAKAGE FAIL）今日可执行；任何用例转绿必须逐粒子/逐笔可溯源（事件 `particleId` 全链可追），不接受只有聚合量的 PASS。
- **安全闸门继承（零豁免）**：tan30（≤tan30+1e-9 引擎容差）、逐 writer guard、local/bottom/total 预算、守恒 1e-9、库存、容量、有限值、账本对齐、边身份、stop 语义由既有 safety checker 原样执行；不改冻结阈值（26/20 双角为 factory-recommended 用户可配值；`gestureSandRate`/`Nmax`/速度律/观测点阈值一律非冻结，试采后另稿）。
- **红线**：不用 B tracer 视觉点冒充 C 的守恒/落床/扬沙证据；不改冻结阈值、不追认旧 route 结果、不用新 schema 重解释 108-run 旧批；不自行放宽门槛、不删不利规则；资料缺失不补造；本组不授权采集，C 的 route/build/pilot 须 architect 验收 + manager 批准后单开。
- **证据通道**：账本级闸门 `python test/p011_dual_field_gate_check.py --ledger <ledger.ndjson> --output <out.json>`（路由身份/泄漏守卫/守恒恒等式/原子性/预算闭合/来源分账断言，字段缺失输出 NOT_IMPLEMENTED 及缺失清单）；合成自测 `python test/test_p011_dual_field_gate_check.py`；字段级单测（C 实现后建 Node 1:1 通道，参照 `test/run_p011_gesture_inject_units.cjs`）；设备级场景（真机 ledger+可选录像交叉）。

## 14. P-011 终点未沉积沙比例 endpoint protocol revision 用例条目（2026-10-24 发布）

> 依据 `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（`protocolId=p011-endpoint-terminal-nondeposited` / `protocolRevision=endpoint-terminal-nondeposited-v1` / `endpointMetricRevision=terminal-nondeposited-v1.1` / `endpointAuthority=APPROVED`，METHOD_FROZEN / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_KALIBER_MANDATORY / NO_CAPTURE_AUTHORIZATION），上游为用户 2026-10-24 裁决 `docs/decision-P011-terminal-nondeposited-fraction-v1.md` 与 designer 定稿 `docs/design-P011-terminal-nondeposited-fraction-v1.md` §7。字段名权威 `docs/architecture-P011-dual-field-particle-v1.md` §5.6/§1.2。**本节为新增并行条目**：现行 `TC-P011-SLIP-03` / `TC-WS-08` / `TC-WS-09` 条目原样有效，不改写；协议落地前双口径并报，任一单口径不单独放行。

### 14.0 冻结口径速查（条目判据均引用此处，不重复定义）

| 项 | 冻结值 |
|---|---|
| 分子 | `M = U + P`；`U = Σ(stateAfter.suspH)+Σ(stateAfter.suspL)`；`P = particles.massH + particles.massL`（= `pState∈{1,2}` 求和；`state=0` 不计） |
| 分子细则 | `DEGRADE`/`TO_SUSP` 累计量永不入分子；`MERGE` 只计现存包一次；`LAND` 只扣 `granted`（余量留 `state=2` 仍计 `P`）；摘要与逐粒不同加；B tracer 不计 |
| 分母 | SLIP：`R_SLIP` = run 首子步 `stateBefore` 的 `Σ(bh+bl)`，整 run 冻结；WS：`E = VH0+VL0+IH+IL`（`VH0/VL0` 取首条 `audit.heavyTotalBefore`/`lightTotalBefore`） |
| 阈值 | 5%（不变），`epsMass = 1e-9` material-volume unit（绝对容差，复用现 SLIP checker `EPS`） |
| 判定式 | `M(k) ≤ 0.05×R + epsMass`，末 1s 窗内**每个已提交子步**均须达标 |
| 固定终点 t_e | SLIP：每相位 occurrence 的最后一个已提交子步；WS-08：shake-off 生效后满 8.0s 的最后一个已提交子步 |
| 末 1s 窗 | `[t_e−1000ms, t_e]` 闭区间，按单调实际 elapsed（`context.endpointSampleElapsedMs`）选取；覆盖不足或 `sourceEpoch` 变化 → `INCOMPLETE` |
| 采样身份 | post-commit 同子步读 `stateAfter` + `particles`；连接键 `(runId, sequence, frameId, substepId)`；相位 occurrence 身份 `context.endpointPhaseOccurrenceId` |
| 状态码 | `NOT_IMPLEMENTED(missing_field)` / `INVALID_LEDGER` / `INVALID_BASELINE` / `INVALID_DENOMINATOR` / `INCOMPLETE`；诊断码 `PERSISTENT_PARTICLE_RESIDUE` / `NOT_OBSERVABLE`；均不得记 PASS |
| 双算 | 同 run 同时算 legacy（`U`，原分母，原 PASS/FAIL，标 `legacy-susp-only`）与新口径（`M/R` + 末窗）；正式判定用 `M/R`，**不得**用显示分项之和 `M/E`；报告并列 `R`/`E`/`U`/`P`/`M` 及各自比例 |
| 报告字段 | `endpointMetricRevision` / `endpointAuthority` / `endpointReferenceKind`（`SLIP_INITIAL_BH_BL`/`EXPECTED_TOTAL`）/ `endpointReferenceMass` / `endpointFraction` / `windowPeakFraction` / `massWindowPass` / `endpointStatus` / `endpointWindowStatus` / `endpointStatusDetail` / `legacyEndpointFraction` / `legacyMetricSuspensionFraction` / `endpointDiagnosis` |

### 14.1 用例条目

| 用例 ID | 覆盖 | 门类 | 判定要点 | 今日状态（缺哪个遥测字段） |
|---|---|---|---|---|
| TC-P011-ENDPOINT-01 | SLIP-03 相位终点未沉积比例（新口径） | MUST-PASS | 每相位 occurrence 的 t_e 处 `M(t_e)/R_SLIP ≤ 0.05 + epsMass/R_SLIP`；`R_SLIP` 取首子步 `stateBefore` 的 `Σ(bh+bl)` 且整 run 不变；正式判定用 `M/R_SLIP` 不用 `M/E` | NOT_IMPLEMENTED（缺 `context.particles.massH/massL`） |
| TC-P011-ENDPOINT-02 | WS-08 静置 8s 终点未沉积比例（新口径） | MUST-PASS | t_e = shake-off 生效后满 8.0s 的最后一个已提交子步；`M(t_e)/E ≤ 0.05 + epsMass/E`；`E = VH0+VL0+IH+IL`（准入只增一次） | NOT_IMPLEMENTED（缺 `context.particles.massH/massL`） |
| TC-P011-ENDPOINT-03 | 末 1s 质量窗（每子步达标） | MUST-PASS | 窗内每子步 `M(k) ≤ 0.05×R+1e-9`；输出 `windowPeakFraction` 与 `endpointFraction` 且二者不得互替；单步低谷不得蒙混（反例 §10 第 3 行）；`sourceEpoch` 恒定 | NOT_IMPLEMENTED（缺 `context.endpointSampleElapsedMs` + `context.particles.*`） |
| TC-P011-ENDPOINT-04 | 双算与报告身份 | MUST-PASS | 同 run 同时输出 legacy 与新口径；legacy 标 `legacy-susp-only` 并保留原 PASS/FAIL 与原分母；`legacyEndpointFraction`（历史分母语义 `U/R`，R 为该 case 历史分母：SLIP `R_SLIP`、WS `A_hist=D+U`）与 `legacyMetricSuspensionFraction`（现行代码路径 `U/max(A,EPS)`，`A=D+U+P`）分列，二者在 C run 中必然不同，禁止互为别名、禁止任一单独放行 | NOT_IMPLEMENTED（checker 未实现双算；判据归 tester，实现归 developer） |
| TC-P011-ENDPOINT-05 | 采样身份与 elapsed 覆盖 | MUST-PASS | post-commit 同子步读取；连接键 `(runId, sequence, frameId, substepId)` 可核；相位 occurrence 分组不拼接同名重复相位；窗按单调实际 elapsed 选取，不用 `sum(physicsDt)`、不用 250ms `WS_METRIC` 抽样替代；覆盖不足记 `INCOMPLETE` | NOT_IMPLEMENTED（缺 `context.endpointSampleElapsedMs` + `context.endpointPhaseOccurrenceId`） |
| TC-P011-ENDPOINT-06 | 分子语义不变量 | MUST-PASS | `state=2` 计入 `P`、`state=0` 不计；`DEGRADE`/`TO_SUSP` 累计量不入分子；`MERGE` 不重复计；`LAND` 只扣 `granted`（余量留 `state=2`）；摘要与逐粒和一致（不一致 → `INVALID_LEDGER`） | NOT_IMPLEMENTED（缺 `context.particles.*` + `context.particleEvents[]`） |
| TC-P011-ENDPOINT-07 | 状态分类与判定顺序 | MUST-PASS | 先过守恒/库存/容量/guard/预算/有限值/账本对齐/边身份硬闸门，再判效果终点；五码分流各自可触发；`R≤0` → `INVALID_DENOMINATOR`；失败闸门与不可观测项分开记录；硬闸门失败不得用 5% PASS 掩盖 | NOT_IMPLEMENTED（checker 未实现状态码分流） |
| TC-P011-ENDPOINT-08 | 滞留诊断与不落结论 | TIERED（诊断，不产放行结论） | 终点仍有 `state=2` → 报 `PERSISTENT_PARTICLE_RESIDUE` 交 manager 审查；逐粒滞留时长因架构稿不存 age → 记 `NOT_OBSERVABLE`，**不得**宣称长期不落已排除；5% 数值 PASS 不替代其余出口条件 | NOT_IMPLEMENTED（缺 `context.particles.byState.CONTACT_WAIT`；逐粒年龄字段 rev1 不存在→该子项长期 `NOT_OBSERVABLE`） |

### 14.2 不涉及项（本节条目一律不触碰）

- `TC-WS-08` 的 `peakSusp ≥ 0.5` —— 另案裁决与另起 revision（卡 `6adc64ffe4b017ab33dccb43`），不以 `peak(U+P)/E` 替代；
- `TC-WS-09` 的采样段（静置 10s、末段 5s）不变，新末 1s 窗嵌在既定末段内；
- `SLIP-02/03/04` 的 `30°±2°` / target-based `t90≤5100ms` / `R²≥0.90` 与全部安全闸门不动；新残余 PASS 不得追认 SLIP-02 或 P-011 CLOSED；
- `DUALFIELD-*` 16 例 estimand 与 GES 五观测点零改动，不另造通用 5% PASS；
- `DUALFIELD-CONS-04` 沉降 t90 口径不动（实现缺口由 developer 按 CONS-04 修）；
- P-012 `E2`/`E4` 不从本指标推出 PASS，`E4` 沙丘 mask 只取真实沉积床层；
- 历史报告结论不改、不追认、不推翻；本节不含任何 estimand 变更（变更须另起 revision）。

### 14.3 现行 checker 适配摘要（不改代码，实现归 developer）

SLIP 侧 `test/p011_slip_formal_check.py`：**保留不动**——`initial_deposit_total`（= `R_SLIP`，语义已正确）、tan30 判据与 `SKIP_TAN30_PHASES` 豁免、四层预算、守恒/库存/容量/有限值、边身份、收敛诊断块、SLIP-03 legacy 原公式与原 PASS/FAIL。

**需新增**（判据与落点详见 revision §7.2）：每子步 `M=U+P` 聚合与分子细则校验（M1–M8）、固定终点按 occurrence 身份选取、末 1s 窗按 elapsed 另取、窗内逐子步达标判定、`endpointFraction`/`windowPeakFraction` 输出、窗口资格（覆盖 + `sourceEpoch` 恒定）、五码状态分流、`R≤0` 判定、`legacyEndpointFraction` + `legacyMetricSuspensionFraction` 两字段（旧判据 + 原分母 + 原 PASS/FAIL + 身份标记）、回算标 `RETROSPECTIVE_DIAGNOSTIC`。

**易错点**：末 1s 有两个窗——legacy 窗（`physicsTime` 选取，服务冻结的坡度稳定与 legacy 悬沙判定，样本选择必须保持现行实现不变）与新质量窗（`endpointSampleElapsedMs` 选取），两者样本集可能不同，须分别披露，禁止合并成一个「最后 1s」。

**迁移期**：legacy 照常输出；新口径缺 `particles` 或 `endpointSampleElapsedMs` 时输出 `NOT_IMPLEMENTED(missing_field)` 并列缺失字段路径；任一单口径都不单独放行；不得向旧账本补造 v7 字段，不得用结构 `P=0` 冒充实测粒子质量。

### 14.4 证据通道与交接

- 判定协议权威：`docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（本节字段名与版本号与之逐字一致）。
- checker 适配对象：`test/p011_slip_formal_check.py`（SLIP 侧）、WS 侧残余项采集（`pages/SandCanvas.ets` `WS_METRIC`）；实现后由 tester 按本节条目出双报告（legacy + 新口径并报），designer 验证，manager 确认适用范围。
- 交接链：architect 字段 revision 对齐（`6adc64ffe4b017ab33dccb44`）→ developer 实现（`6adc6513e4b017ab33dccdb0`）→ tester 双报告 → designer 验证 → manager 确认适用范围。

## 15. P-011 WS-08 摇晃未沉积沙峰值 peak protocol revision 用例条目（2026-10-25 预注册，RED 基线）

> 依据 `docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md`（`protocolId=p011-ws08-peak-nondeposited` / `protocolRevision=ws08-peak-nondeposited-v1` / `peakMethodId=peakNonDepositedFraction` / `peakMetricRevision=shake-peak-nondeposited-v1` / `peakAuthority=APPROVED` / `peakProtocolEffective=false`，METHOD_FROZEN / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_CALIBER_MANDATORY / NO_CAPTURE_AUTHORIZATION）。上游为用户五项裁决（manager 2026-10-24 23:55 下达：采纳候选 B、分子含全部活粒子含 state=2、分母用 E、固定 shake-on 后 3s 窗每提交采样、原 0.5 及格线与分档锚点迁移）与 designer 迁移设计稿 `docs/design-P011-ws08-peak-nondeposited-migration-v1.md`。执行证据：`test/reports/peak_protocol_v1_execution.json`（8/8 反例 MATCH）；RED 基线：`docs/peak-red-baseline-P011-ws08-peak-nondeposited-v1.md`。**本节为独立并行条目**：不改 §14 endpoint 条目、不改现行 `TC-WS-08`/`TC-WS-09`/`TC-P011-SLIP-03` 条目判据；协议落地前摇晃峰值子项双口径并报，任一单口径不单独放行。

### 15.0 冻结口径速查（条目判据均引用此处，不重复定义）

| 项 | 冻结值 |
|---|---|
| 主量 | `peakNonDepositedFraction = max_{k∈W} (U(k)+P(k))/E(k)`；同提交先求和再取 max；并列留最早提交键 |
| 分子 | `M=U+P`；`U=Σ(stateAfter.suspH)+Σ(stateAfter.suspL)`；`P=particles.massH+particles.massL`（=`pState∈{1,2}` 求和；含 state=2 接触待准入；`state=0`、B tracer 不计；`DEGRADE`/`TO_SUSP` 累计量不入分子；`MERGE` 只计一次；`LAND` 只扣 granted） |
| 分母 | `E=VH0+VL0+IH+IL`（`VH0/VL0` 取首条 `audit.heavyTotalBefore`/`lightTotalBefore`；WS-08 无外源时恒定、`sourceEpoch` 不变） |
| 摇晃窗 | `W=[t_on, t_on+3000ms)` 左闭右开；成员=窗内实际消费该 shake-on 输入的全部成功提交子步；提前 off 或窗内输入非原 1.0 → 资格不符 |
| 采样 | 每成功提交子步（post-commit 同子步读 `stateAfter`+`particles`）；连接键 `(runId, sequence, frameId, substepId)` + `context.endpointPhaseOccurrenceId`；时钟复用 `context.endpointSampleElapsedMs`；记录 `integratedPhysicsSeconds`；250ms 样本仅作 `OBSERVED_SAMPLED_MAX` 下界 |
| 分档 | ≥0.90→10；≥0.75→8；≥0.50→6（及格）；≥0.35→4；≥0.20→2；<0.20→0；用未四舍五入值评分 |
| 及格比较 | `M(peakCommitKey) ≥ 0.5×E`，不新增向下放宽容差；库存/分相守恒仍用各自 1e-9 容差 |
| 分相 | `peakNonDepositedHeavyFraction`/`peakNonDepositedLightFraction`；某相 `E_phase=0` 记 `N/A`（不 EPS 造比例）；总量峰值提交同时记录 H/L；独立分相峰不相加；不新增分相通过阈值 |
| 状态码 | `NOT_IMPLEMENTED(missing_field)` / `INVALID_LEDGER` / `INVALID_BASELINE` / `INVALID_DENOMINATOR` / `INCOMPLETE`；诊断码 `NOT_OBSERVABLE`；均不得记 PASS；硬闸门失败只留候选峰值诊断 |
| 双算 | `legacyPeakSuspReported`（旧原字段/原样本/原窗口，标 `legacy-susp-only`）、`legacyPeakHistoricalReference`（同 W 逐子步 `h=U/(D+U)`）、`legacyPeakLiveFormula`（同 W 逐子步 `l=U/max(A,EPS)`）、`sameReferenceSuspOnlyPeak`（`max_W U/E`）；窗口/密度/分子分母效应分开归因 |
| 可达性 | `M(k)=M0+cumAcceptedLift−cumSettledOrLanded`；必要条件 `M0+300·tau_W ≥ 0.5E`（`tau_W`=W 内成功子步 `ΣphysicsDt` 秒）；不暗增 300/s、不延长摇晃、不调低 0.5 |
| 报告字段 | `peakProtocolId` / `peakProtocolRevision` / `peakMetricRevision` / `peakAuthority` / `peakProtocolEffective=false` / `peakWindowStartElapsedMs` / `peakWindowEndElapsedMs` / `shakeOnEffectiveElapsedMs` / `shakeOffEffectiveElapsedMs` / `peakWindowSampleCount` / `peakWindowCoverageStatus` / `missingFields` / `peakNonDepositedFraction` / `peakStockMass` / `peakReferenceMass` / `peakCommitKey` / `peakSampleElapsedMs` / `peakScore` / `peakStatus` / 分相两项 / 双算五项 / 初态响应三项 |

### 15.1 用例条目

| 用例 ID | 覆盖 | 门类 | 判定要点 | 今日 RED 状态（缺哪个遥测字段） |
|---|---|---|---|---|
| TC-P011-PEAK-01 | 摇晃窗固定 3s 与输入身份 | MUST-PASS | `t_on` 取输入链实际接受 shake-on 的单调 elapsed；`W=[t_on, t_on+3000ms)` 左闭右开；窗右边界 shake-off 后提交不纳入；提前 off 或窗内输入非原 1.0 → `INCOMPLETE`；预更新 shakeEnergy 不替代输入接受证据 | NOT_IMPLEMENTED（缺 `shakeOnEffectiveElapsedMs`/`shakeOffEffectiveElapsedMs` + 每子步输入消费身份） |
| TC-P011-PEAK-02 | 每提交采样与连接键 | MUST-PASS | 窗内每个成功提交子步 post-commit 同子步采样；连接键 `(runId,sequence,frameId,substepId)` 可核 + `context.endpointPhaseOccurrenceId`；时钟用 `context.endpointSampleElapsedMs`；记录 `integratedPhysicsSeconds`（不代替 3s 实际时间）；断档/提前退出 → `INCOMPLETE`；冻结/回滚/漏记不当休眠、不伪造提交 | NOT_IMPLEMENTED（缺 `context.endpointSampleElapsedMs` + `context.endpointPhaseOccurrenceId` + `context.integratedPhysicsSeconds` + 峰值窗 occurrence manifest） |
| TC-P011-PEAK-03 | 主量与分档判定 | MUST-PASS | `f(k)=(U+P)/E` 同提交求和后取 max；并列留最早提交键；分档 10/8/6/4/2/0；及格比较 `M≥0.5E` 无新容差；四舍五入不参与判定；单提交可为峰，异常尖峰先查守恒/身份不删除 | NOT_IMPLEMENTED（缺 `context.particles.massH/massL`） |
| TC-P011-PEAK-04 | 分相诊断 | TIERED（诊断，不产放行结论） | `peakNonDepositedHeavyFraction`/`LightFraction`；某相 `E_phase=0` 记 `N/A`；总量峰值提交同时记录 H/L；独立分相峰不相加（反例：heavy 66.67%@t1 + light 49%@t2 ≠ 总量 52%@t3）；不新增分相阈值；heavy 几乎未扬起时登记"摇晃交融构成未达成" | NOT_IMPLEMENTED（缺 `context.particles.*` 分相质量） |
| TC-P011-PEAK-05 | 双算与归因分离 | MUST-PASS | 同 run 同账本并报 `legacyPeakSuspReported`（全序列原身份，保留原 PASS/FAIL）/`legacyPeakHistoricalReference`（`D+U=0` 记 `INVALID_DENOMINATOR` 不阻断新量）/`legacyPeakLiveFormula`（EPS 钳位如实报告）/`sameReferenceSuspOnlyPeak`；`OBSERVED_SAMPLED_MAX` 仅作下界；窗口/密度/分子分母效应分开列，不得把旧峰与新峰之差全归因于物理改善；恒等式 `f−l=P/E`、`f−h=P·D/[E·(D+U)]` 仅守恒且 `D+U>0` 时成立 | NOT_IMPLEMENTED（checker 未实现双算；判据归 tester，实现归 developer） |
| TC-P011-PEAK-06 | 初态/响应/滞留披露 | TIERED（诊断，不产放行结论） | `baselineNonDepositedFraction=M0/E`（shake-on 前最近可证 post-commit）；`M0≥50%` 时数值达标也不证明新增扬起，必须报净增量/LIFT；`peakNetIncreaseFraction=max_W((M−M0)/E)`；`cumulativeLiftGrantedMass` 与 guard/budget 拒绝；峰值 FLYING/CONTACT_WAIT 分项（`byState` 缺失记 `NOT_OBSERVABLE`，不宣称长期不落已排除）；不新增基线≤5%/最小 LIFT/寿命通过线 | NOT_IMPLEMENTED（缺 `context.particles.byState` + LIFT 事件账本；逐粒年龄字段不存在→长期不落子项长期 `NOT_OBSERVABLE`） |
| TC-P011-PEAK-07 | 状态分类与判定顺序 | MUST-PASS | 顺序：路由身份（无外源、基线 E>0）→ 硬闸门 → 窗绑定与资格 → 字段完整性 → 账本/基线/分母 → 效果峰；五码分流各自可触发；`E≤0`→`INVALID_DENOMINATOR`；硬闸门失败不得用峰值 PASS 掩盖（只留候选诊断）；字段/资格/覆盖无效不打 0 分凑平均 | NOT_IMPLEMENTED（checker 未实现五码分流；安全闸门由既有 checker 原样执行） |
| TC-P011-PEAK-08 | 与 endpoint 协议的边界 | MUST-PASS | 峰值不借用 endpoint 末 1s 最大值；endpoint 残余（shake-off 后满 8.0s 固定终点 + 末 1s 每提交达标）按已批准协议独立算；峰值 PASS 不签发整个 WS-08（生命周期/可见扬起/自然/无永久悬浮各按既有证据链）；停止后 8s 终点判定完全委托 endpoint 协议 | 可执行（边界核对，不依赖新字段）；数值结论仍 NOT_IMPLEMENTED |

### 15.2 不涉及项（本节条目一律不触碰）

- `TC-WS-08` 停止后 8s 终点残余与末 1s 质量窗——endpoint 协议（已批准），本峰值只委托、不重算、不借用其数值；
- 旧 `peakSusp` 字段本身——保留原名、原公式、原样本与原窗口，标 `legacy-susp-only`；另案获批并完成独立协议/实现/验证后才替代 C 的峰值子项；
- `SLIP-02/03/04` 的 `30°±2°` / target-based `t90≤5100ms` / `R²≥0.90` 与全部安全闸门不动；峰值 PASS 不得追认 SLIP-02 或 P-011 CLOSED；
- `DUALFIELD-*` 16 例 estimand 与 GES 五观测点零改动；300 unit/s 预算值非冻结，本案只用作可达性上界输入；
- P-012 `E2`/`E4` 不从本指标推出 PASS；
- 历史报告结论（含 2026-10-13 RED 基线 `shake=BLOCKED`）不改、不追认、不推翻；本节不含任何 estimand 变更（变更须另起 revision）。

### 15.3 现行 checker 适配摘要（不改代码，实现归 developer）

适配对象 `test/water_sand_check.py::evaluate_shake()`（第 476–492 行）：现行 `peakSusp` 取**全序列** `max(WS_METRIC.suspensionFraction)`、`residual=values[-1]`、二元 `peak>=0.5 and residual<=0.05`、无 TIERED。**保留不动**：legacy 输出身份（`legacy-susp-only`）、原公式、原样本/原窗口、原 PASS/FAIL/BLOCKED、其余 evaluator。

**需新增**（判据与落点详见 `docs/peak-adaptation-checklist-P011-ws08-peak-nondeposited-v1.md` 与 peak 协议 §2/§3）：固定 3s 窗与输入身份绑定、每提交采样、`M=U+P` 同提交求和、最早并列键、分档与 `M≥0.5E` 及格比较、分相诊断、五码分流、双算四 legacy 量 + `sameReferenceSuspOnlyPeak` + `OBSERVED_SAMPLED_MAX`、归因分离、可达性必要条件复算。

**易错点**：①全序列 max 与固定窗峰是不同估计对象，不得互相追认（反例 P5）；②250ms 样本峰与全子步峰之差不得归因于物理改善（反例 P7 + 归因链）；③`M0≥50%` 时数值达标不证明新增扬起；④硬闸门失败时峰值只留候选诊断；⑤残余列不再由 shake 评估器产出（归 endpoint 协议）。

**迁移期**：legacy 照常输出；新口径缺字段输出 `NOT_IMPLEMENTED(missing_field)` 并列确切路径；任一单口径不单独放行；不得向旧账本补造 v7 字段，不得用 250ms 旧数据冒充全子步峰值。

### 15.4 证据通道与交接

- 判定协议权威：`docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md`（本节字段名与版本号与之逐字一致）。
- 执行证据：`test/reports/peak_protocol_v1_execution.json`（执行器 `test/p011_peak_protocol_v1_execution.py`；分档边界、8/8 反例、可达性、归档勘查、legacy 行为、双算归因链；合成算例非测量）。
- RED 基线：`docs/peak-red-baseline-P011-ws08-peak-nondeposited-v1.md`（新量 `NOT_IMPLEMENTED`，缺字段清单 7 项；`OBSERVED_SAMPLED_MAX` 不可用——现存归档 0 个 shake 事件）。
- 交接链：manager 验收 → architect 登记独立 peak 字段 revision → developer 实现采样/流式峰值/checker → tester 同账本双报告（legacy + 新量，归因分离）→ designer 效果验证 → manager 宣布 `peakProtocolEffective`。真机需求由 manager 交 effector 串行排队（本卡不占设备）。

## 16. P-013 P1 材质耦合 CPU fixture 与五量用例条目（2026-10-25 登记，RED 基线）

> 依据 `docs/architecture-P013-p1-material-composite-v1.md` rev2（manager 22:30 验收）§4.2/§5.2/§5.4/§9/§10、`docs/design-P013-p1-material-recipe-slope-confirmation-v1.md`（designer 签收，manager 21:55 验收）§3.1/§7、`docs/reference-P013-p1-material-recipe-v1.json`（`P013_P1_RECIPE_ANALYTIC_V1`，SHA-256 `d214aa447f9f186eba159fd6953f2b42874420926a1b01b7c0977b940f1d63a6`）。用例登记：`docs/p013-p1-cpu-fixture-and-coupling-test-registration-v1.md`；RED 基线：`docs/p013-p1-material-coupling-red-baseline-v1.md`；执行证据：`test/reports/p013_cpu_reference_execution.json`（58/58 MATCH，合成复算非测量；RED 取证双树内容级扫描：正式归档 919 文本 + 并行工作区 854 文本，P1 身份 token 0 命中）。**本节为独立条目**：CPU fixture 属合成层（可执行），五量属真机/GL 层（NOT_IMPLEMENTED），两层不互相追认；`[0.8,1.2]`/`8–20/255` 仍只是旧扫描域；未冻结数值一律 `MEASURED_NOT_CALIBRATED`，不记 VALIDATED；本卡不宣布 G1 解冻。

### 16.0 冻结口径速查（条目判据均引用此处，不重复定义）

| 项 | 冻结值 |
|---|---|
| 运行时版本 | `materialRecipeVersion=P1_ALBEDO_DELTA_V1` / `slopeKernelVersion=E2_GAUSS_COL6_R18_P1_V2` / `zOriginVersion=1` / `colorPipelineVersion=LIN_SRGB_REINHARD_V1` / `textureSamplingVersion=P1_TEXEL_CANVAS_PX_V1` / `strataSamplingVersion=CONTINUOUS_COLUMN_WARP_V1` / `T_dataLayoutVersion=2` |
| 坡度管线 | 逐字沿 `buildDisplayProfiles` 四遍三点平均（[1,4,10,16,19,16,10,4,1]/81）→ 只读 S_D 做 37 项 Gaussian（σx=6 列、Rx=18、一次归一化）→ 列点差分 pb64=(S_G[c+1]−S_G[c−1])·sy/(2dx)，端点仍除 2dx → 上传前 Float32 量化 → shader 只对上传 pb 连续列插值；深度方向 σz=0 恒等 |
| 几何 | N=128；boxW=128 Canvas px、boxH=capacity=256 → dx=sy=1；S=bh+bl+mh+ml；waterTop 只进深度（`zMaterial=interpolate(S_D,x)−h(y)`）不进坡度 |
| 取色/基线 | 低端 palette H=(145,168,111)/L=(216,196,150)；M grainBase 零噪声字节 (181,182,131)（raw 混合 (180.5,182,130.5)）；`round_nonnegative=floor(v+0.5)`；先乘 gain 再 clamp248 再 round；C1 中性 128/255；C3 块 shadeIdx [48,16]→字节 [206,159] |
| 五量方法 | `methodId=P013_P1_MATERIAL_COUPLING_YCODE_V1`；`Ycode=0.2126·R8+0.7152·G8+0.0722·B8`（8bit sRGB 显示码）；CPU 沿唯一色彩链出 sRGB 再用同一 Ycode；`Ylinear` 只作诊断 |
| 状态分流 | `EMPTY_ROI` / `INSUFFICIENT_SUPPORT` / `UNDEFINED_RATIO` + 原始量；不加 EPS、不默认比值为 1 |
| 精度边界 | CPU 参考消费实际上传 Float32 包、同坐标/同 mask/同 fH/warp；Float32 量化（L1）/shader 算术（L2）/读回截图（L3）三类误差分层测量；容差由 manager 冻结，本节不新冻 |

### 16.1 CPU fixture 用例条目（合成层，可执行）

| 用例 ID | 覆盖 | 门类 | 判定要点 | 期望值来源 | 今日状态 |
|---|---|---|---|---|---|
| TC-P013-FIX-01 | 常数 fixture | MUST-PASS | bh=20/bl=20/mh=ml=12→S=64；所有列 pb=0、n=(0,0,1)；pb32 亦精确 0（不接受浮点尾差） | 参考 JSON `fixtures.flat` | 合成复算 MATCH（58/58 中本项全列零断言） |
| TC-P013-FIX-02 | 线性坡面内域 | MUST-PASS | bh[c]=20+0.2c→S=64+0.2c；c=23…104 pb64=0.2（复算最大偏离 2.56e-14）、pb32=0.20000000298023224；n(pb32)≈(−0.19611613794814378,0,0.9805806751289282) | `fixtures.ramp.selectedExpected`（c=23/64/104） | 合成复算 MATCH |
| TC-P013-FIX-03 | 同坡面边缘 | MUST-PASS | c0/127 pb64≈0.047385794656442215、c1/126≈0.10134662147899576、c22/105≈0.19999908624895824；**端点不得硬改成 0.2**（端点仍除 2dx、复制端列不重归一化，衰减即冻结规则） | `fixtures.ramp.selectedExpected`（c=0/1/22/126/127） | 合成复算 MATCH |
| TC-P013-FIX-04 | raw 脉冲 | MUST-PASS | S=100+10δ64；pb64[63]≈+0.01618533041693837、[64] 数值零、[65]≈−0.01618533041693837；肩部 c62=0.031151152748513766 等逐值一致；normal 左右相反、中心竖直 | `fixtures.rawImpulse.selectedExpected`（11 点） | 合成复算 MATCH |
| TC-P013-FIX-05 | Gaussian 单元隔离 | MUST-PASS | **已显示** S_D=100+10δ64 只做 Gaussian；pb64[63]=5(w0−w2)≈0.01800228755409705、[64] 精确 0、[65] 取负；与 FIX-04 必须不同（实测差 11.2%，用于区分漏做/重复做 DISPLAY） | `fixtures.postDisplayImpulse` + `gaussian.summary` | 合成复算 MATCH |
| TC-P013-FIX-06 | 内部层重排 | MUST-PASS | bh=40+2δ64、bl=20−2δ64→S 恒 100；pb 全 0、n=(0,0,1)；**不得由 bh 峰产生宽面法线**（"只给 bh 不够确定法线"反例） | designer 稿 §3.1 表第 6 行（输入由登记稿固定） | 合成复算 MATCH |
| TC-P013-FIX-07 | 脉冲独立解析对拍 | MUST-PASS | 内域组合核 v=w⊗displayKernel，`pb[64+d]=5·(v[d+1]−v[d−1])`；解析值/管线复算值/JSON 发布值三方一致（实测最大相对差 2.3e-13）；不复用管线代码，防两份共用实现同一错误 | 参考 JSON + designer 稿 §3.1 末段 | 合成复算 MATCH |
| TC-P013-FIX-08 | 镜像坡面（附加反例） | TIERED（反例） | FIX-02 输入取负；pb 逐列精确取负（a+b==0.0）、nz 不变（差 <1e-15） | designer 稿 §3.1（正/负坡镜像 pb 反号、nz 不变） | 合成复算 MATCH |
| TC-P013-FIX-09 | 常数层（Q 表/C3/palette/M 基线） | MUST-PASS | Q 按 `L=min(EOTF(c),1−1/4096)`、`Q=L/(1−L)`：byte255→L=0.999755859375、Q=4095，byte254→Q=111.38603216880189，byte248→Q=15.309416630892823；C1 中性 Q(128/255)=0.2752832884266815；C3 shadeIdx[48,16]→字节[206,159]→δC3=(0.13370916945955877, 0.02554165570685231)；palette 低端 H=(145,168,111)/L=(216,196,150)；M 零噪声字节 (181,182,131)（raw 混合 (180.5,182,130.5)，先 round raw 会改变旧资产字节） | 参考 JSON `colors` 节 | 合成复算 MATCH |

### 16.2 五量用例条目（真机/GL 层，NOT_IMPLEMENTED）

| 用例 ID | 覆盖 | 门类 | 判定要点 | 今日 RED 状态（缺什么） |
|---|---|---|---|---|
| TC-P013-MCP-01 | `R_B(Ω)` 亮/暗分开 | TIERED | `A_B^t=sqrt(mean_Ω(H_B(I_B^t)^2))`、`b^t=median_Ω Ycode(F,t,B关,C固定)`、`N_B=A/b`、`R_B=N_ON/N_OFF`；H_B=I−G_B*I，σB=active grainFeaturePx×Canvas→native 等比尺度、R=ceil(3σB)，ROI 内缩支持半径；亮/暗分开报 | NOT_IMPLEMENTED（无 P1 OFF/ON 渲染产物、无诊断格；`[0.8,1.2]` 仅旧扫描域不作 PASS 线） |
| TC-P013-MCP-02 | dark 绝对幅度族 | TIERED | 固定 Ωdark 报 `darkBOn/Off=A_B^ON/OFF` 及 CPU 值；C 去趋势层纹 RMS `darkCOn/Off`；保留 C1 prominence/周期/row-mean；不设最小幅度 PASS 线 | NOT_IMPLEMENTED（同 MCP-01） |
| TC-P013-MCP-03 | `Q_couple,B`/`Q_couple,C` | TIERED | `Q_X^t=A_X^t(Ωlit)/A_X^t(Ωdark)`、`D_X=Q_ON/Q_OFF`、各 A 绝对值及 CPU 预测；B/C 频带不混用；不规定 Q>1、不凭单比值推结构 | NOT_IMPLEMENTED（CPU 预测需先有 P1 shader 配方实现） |
| TC-P013-MCP-04 | `C_band` 连续性向量 | TIERED | B/C OFF 隔离臂：固定 x 同材质内扫 z，报 Y_ON−Y_OFF、CPU 曲线、相邻 native px 梯度/残差；x 横扫列边界 ±1px 与列中央匹配支持 g_edge=P95(abs(ΔxY))、g_mid 及比值；真 H/L 边界单列排除；C3 样式边界仅全材质臂标记另报、不得删去 | NOT_IMPLEMENTED（隔离臂与完整 C 臂均无产物） |
| TC-P013-MCP-05 | `ΔY_face`/`ΔΔY` | TIERED | 匹配材质/z/遮挡迎背光 ROI 中位 Y 差；报 ΔΔY=ON−OFF、CPU 预期、参考目标距离、ROI 材质构成与像素数；光向镜像对拍；平沙不新增伪左右差；8–20/255 不转 PASS 线 | NOT_IMPLEMENTED（同 MCP-01） |

### 16.3 不涉及项（本节条目一律不触碰）

- CPU fixture 的合成复算结果**不**作为 P1 实现正确性、GPU 一致性或效果证据（两层不互相追认）；
- `E3` J95/J90/std 锚点与 E/A/B CLOSED 状态不动；GLES 身份下重采分子分母（阈值不改）属 G5；
- `[0.8,1.2]`/`8–20/255`/C3 两档块值等旧扫描域不升级为冻结 PASS 线；
- designer 签收稿的 fixture 期望值、公式、版本号不改（只登记用例）；
- 参考 JSON 只读引用；更正须由 designer 另版并重发；
- P-011 全部协议/用例（endpoint、peak、SLIP、DUALFIELD）不在本节范围；
- 不使用真机（本卡纯 Node/Python 离线通道；真机归 effector 串行队列）。

### 16.4 G1 解冻前置核对结论（引用登记稿 §4）

rev2 §9 切片 2 的 G1 前置 = rev2 manager 验收（**已满足**，22:30）+ tester 登记 CPU fixture/材质耦合用例及 RED 基线（**已满足**：本节 16.1/16.2 + 两份 docs + 执行证据 58/58）；**仍缺**：G0 真机 probe（sRGB attachment、三层 z序/透明/输入透传；当前真机队列首位为掉沙 V1-V4 返修复测）与 manager 的 G1 实施授权。本节不自行宣布 G1 解冻。

### 16.5 证据通道与交接

- 判定口径权威：`docs/p013-p1-cpu-fixture-and-coupling-test-registration-v1.md`（本节字段名与版本号与之逐字一致）。
- 执行证据：`test/reports/p013_cpu_reference_execution.json`（执行器 `test/p013_cpu_reference_execution.py`；fixture 58/58、解析对拍三方一致、Q/C3 常数层、归档 RED 取证 919 文本/0 身份 token；合成复算非测量）。
- RED 基线：`docs/p013-p1-material-coupling-red-baseline-v1.md`（五量 NOT_IMPLEMENTED + 缺失项清单 + 测量前置六条件；双树取证：正式归档与并行工作区文本各 919/854 个，P1 身份 token 0 命中；并行工作区 p013 文件经核对为 G2 桥接/生命周期本地实验，非五量估计对象）。
- 交接链：manager 建 G0 probe 卡（effector 队列）→ G0 过后 manager 放行 G1 实施 → developer 交付六图导出/shader CPU 参考/七版本身份 → tester/effector 按前置六条件采五量与重复性分布 → designer 审效果 → manager 冻结数值线（此前一律 `MEASURED_NOT_CALIBRATED`）。

## 17. P-013 G2 上传器客观用例条目（2026-10-25 登记与冻结，RED 基线）

> 依据 `docs/tester-T53-g2-uploader-cases-registration-v1.md`（登记稿，字段名与期望码与之逐字一致）。上游：T45 裁决 `docs/decision-P013-T45-material-rollout-v1.md` §2「T：G2 测试先行」/§3/§4 与工程契约 `docs/architecture-P013-p1-material-composite-v1.md` rev2 §4.3/§4.4/§6/§7/§8/§9。执行证据：`test/reports/p053_g2_uploader_case_check.json`（执行器 `test/p053_g2_uploader_case_check.py`；23/23 NOT_IMPLEMENTED；T09 保全 0 违例；条款追溯 0 缺口）。**本节为独立追加条目**：不改 §16 T09 CPU fixture/五量条目，不改任何冻结判据；G2 未实现能力如实记 NOT_IMPLEMENTED / RED，不记 VALIDATED，不宣布 G1/G2 通过。

### 17.0 冻结口径速查（条目判据均引用此处，不重复定义）

| 项 | 冻结值 |
|---|---|
| 状态 | TEST_REGISTRATION_ONLY / NOT_IMPLEMENTED / NOT_CALIBRATED / NOT_VALIDATED / G2_NOT_UNFROZEN |
| 期望码指派 | 包级校验失败→400 `MATERIAL_PACKET_INVALID`；配置修订不符→402 `ASSET_REVISION_MISMATCH`；资产未就绪→401 `ASSET_NOT_READY`；不新建错误码；实现用其它既有码→FAIL 并附实际码交裁定 |
| 接口（T33/rev2 §6） | `stageMaterialBundle(desc,grainRgba,strataRgba)` / `pumpMaterialUploads(byteBudget)` / `canActivate(generation,contextEpoch)` / `activateMaterial(generation,frameToken)` / `discardMaterialStage(assetGeneration)` |
| stage 状态枚举 | `CPU_STAGED｜UPLOADING｜READY｜ACTIVE｜FAILED｜DISCARDED` |
| 字节契约 | grain=786432、strata=196608、有效总计=983040；pump 每 tick ≤65536 且只提交完整行（grain 行 1024B / strata 行 256B）；byteBudget=正整数且 ≤65536 |
| 双代规则 | 最多一代 active + 一代 pending；覆盖 pending 先显式 DISCARDED；stage/上传/校验失败不动 active；context 重建后从 CPU 缓存重传、READY 前退 Canvas |
| 原子性 | 一帧边界同 FrameToken 切换纹理/palette/uniform/fog/配置 revision；旧 active 保留至在途旧帧结束；部分代就绪激活被拒绝（401） |
| 回退 | draw/profile/swap 失败同 tick 完整 Canvas 旧路径 + 关闭整块 GLES；禁止只隐藏一层 |
| T09 保全 | TC-P013-FIX-01..09 与 TC-P013-MCP-01..05、参考 JSON（SHA-256 `d214aa44…f63a6`）、T09 执行器一条不改；执行器每次运行复核 |

### 17.1 Stage 校验类条目

| TC-P013-G2-A01 | 缺字段 | MUST-PASS | desc 缺任一 metadata 必填字段 → receipt.state=FAILED、errorCode=400、errorStage=metadata、active 不动 | T45 §2/§3.1；rev2 §6 | NOT_IMPLEMENTED（缺 bridge stageMaterialBundle/receipt 类型/400） |
| TC-P013-G2-A02 | 未知版本 | MUST-PASS | 七版本任一取值不在登记表 → FAILED/400/version；不推断缺省值 | rev2 §4.4/§6 | NOT_IMPLEMENTED（同 A01 面） |
| TC-P013-G2-A03 | 七版本不齐 | MUST-PASS | desc 未携带全部七项版本身份 → FAILED/400/version | rev2 §4.4 | NOT_IMPLEMENTED（同 A01 面） |
| TC-P013-G2-A04 | 配置修订不符 | MUST-PASS | appearanceConfigRevision 不符 → FAILED/402/appearanceConfigRevision；码语义分歧交裁定 | T45 §2；rev2 §6/§7 | NOT_IMPLEMENTED（缺 402 枚举） |
| TC-P013-G2-A05 | 身份不符 | MUST-PASS | materialBundleId/assetGeneration 身份冲突 → FAILED/400/identity；hash 基于实际字节与生成参数 | T45 §2；rev2 §4.3 | NOT_IMPLEMENTED（同 A01 面） |
| TC-P013-G2-A06 | 精确字节数不符 | MUST-PASS | grain≠786432 / strata≠196608 / 总≠983040 → FAILED/400/byteLength | T45 §2；rev2 §4.3/§6 | NOT_IMPLEMENTED（同 A01 面） |
| TC-P013-G2-A07 | byteOffset/byteLength 越界 | MUST-PASS | TypedArray 视图越界或元素类型不匹配 → FAILED/400/byteOffset；只接受匹配元素类型/长度/有效 buffer | T45 §2；rev2 §6 | NOT_IMPLEMENTED（同 A01 面） |
| TC-P013-G2-A08 | SHA256 不符 | MUST-PASS | 整包字节 SHA256 与 desc 登记值不符 → FAILED/400/hash | T45 §2/§3.1 | NOT_IMPLEMENTED（同 A01 面） |
| TC-P013-G2-A09 | alpha 非法 | MUST-PASS | 任一像素 alpha≠255 → FAILED/400/alpha；alpha 固定 255 无预乘 | T45 §2；rev2 §4.3 | NOT_IMPLEMENTED（同 A01 面） |
| TC-P013-G2-A10 | context epoch 不匹配 | MUST-PASS | stage 时 contextEpoch 与当前 surface/context 不符 → FAILED/400/contextEpoch | T45 §2；rev2 §6 | NOT_IMPLEMENTED（同 A01 面） |

### 17.2 双代与生命周期类条目

| TC-P013-G2-B01 | pending 被取代报告 DISCARDED | MUST-PASS | 新 stage 覆盖 pending：被取代代 state=DISCARDED、activeGeneration 不变、资源释放、active 仍可画 | T45 §2/§3.1；rev2 §6 | NOT_IMPLEMENTED（缺 stage/discard 接口与 receipt） |
| TC-P013-G2-B02 | stage 失败不动 active | MUST-PASS | 失败 stage 后 activeGeneration 不变且 active 代仍可绘制；pending 槽为空或保留失败条目 | T45 §2/§3.4 | NOT_IMPLEMENTED（缺 draw 入口与 receipt） |
| TC-P013-G2-B03 | 双代上限不变量 | MUST-PASS | ≤1 active + ≤1 pending；materialTextureLiveCount ≤4；无每帧 glGen/compile | T45 §3.1；rev2 §6 | NOT_IMPLEMENTED（缺遥测字段） |
| TC-P013-G2-B04 | 上下文重建后 CPU 缓存恢复 | MUST-PASS | CPU 缓存不归零；重传确认 READY 才切回；旧 epoch 帧拒绝；READY 前退 Canvas | T45 §2/§3.4；rev2 §6/§7 | NOT_IMPLEMENTED（缺 controller/回退路径） |

### 17.3 Pump 预算类条目

| TC-P013-G2-C01 | ≤65536B/tick 且只完整行 | MUST-PASS | 单次 uploadedBytes ≤65536；chunk 为完整行整数倍（1024/256）；无半行跨 tick；上传期间 active 继续绘制 | T45 §2/§3.2；rev2 §6 | NOT_IMPLEMENTED（缺 pumpMaterialUploads） |
| TC-P013-G2-C02 | byteBudget 域校验 | MUST-PASS | 正整数且 ≤65536；0/负/小数/65537 → 拒绝、uploadedBytes=0、pending 不损 | rev2 §6 | NOT_IMPLEMENTED（缺 pump 入口） |
| TC-P013-G2-C03 | 全部成功才 READY | MUST-PASS | 未传完 state=UPLOADING；全部资源与 metadata 通过才 READY | T45 §2/§3.2 | NOT_IMPLEMENTED（缺 receipt 状态机） |
| TC-P013-G2-C04 | epoch 失配废弃 pending | MUST-PASS | pump 时 contextEpoch 已变 → pending DISCARDED、等待重新 stage、不对失配上下文上传 | T45 §2/§3.2 | NOT_IMPLEMENTED（缺 epoch 校验） |

### 17.4 激活原子性反例条目

| TC-P013-G2-D01 | 部分代就绪即激活被拒绝 | MUST-PASS（反例） | grain 就绪/strata、palette 未就绪 → canActivate=false、activate 拒绝（401）、active 不变、无混代帧 | T45 §2/§4；rev2 §6 | NOT_IMPLEMENTED（缺 canActivate/activateMaterial/401） |
| TC-P013-G2-D02 | 旧 active 保留至旧帧结束 | MUST-PASS | 一帧边界同 FrameToken 原子切换整代；旧代在途旧帧结束后回收；帧只引用单一 generation | T45 §2/§3.3；rev2 §6/§7 | NOT_IMPLEMENTED（缺 activate/draw 与代遥测） |
| TC-P013-G2-D03 | 上传失败保留旧 active | MUST-PASS | GL 失败（300 系列附 stage）→ 旧 active 与旧配置保持、失败代不被任何帧引用、pending FAILED/DISCARDED | T45 §2/§3.4；rev2 §7 | NOT_IMPLEMENTED（缺失败回滚路径） |

### 17.5 回退反例条目

| TC-P013-G2-E01 | draw 失败整块回退 | MUST-PASS（反例） | 同 tick drawLegacyFull 全量重画 + 关闭整块 GLES；只隐藏一层判 FAIL；fallbackReason 遥测记录 | T45 §2/§4；rev2 §7 | NOT_IMPLEMENTED（缺 drawLegacyFull/controller/遥测） |
| TC-P013-G2-E02 | profile/swap 失败整块回退 | MUST-PASS（反例） | 同 tick 整块回退；不假设透明 clear 成功；不留空帧/双层残影；fallbackReason 遥测记录 | T45 §2/§4；rev2 §7 | NOT_IMPLEMENTED（同 E01 面） |

### 17.6 不涉及项（本节条目一律不触碰）

- 不改算法、纹理尺寸、物理路线或判线；不改冻结阈值与七版本身份；不新建数值容差线；
- 不重定 T09 既有 fixture（§16 条目只保全不改写）；五量测量仍归 TC-P013-MCP-*；
- 不改实现代码；不伪造运行结果；不宣称 VALIDATED；不宣布 G1/G2 通过；
- 不使用真机（真机取证归 G5/effector 串行队列）；
- P-011 全部协议/用例不在本节范围。

### 17.7 证据通道与交接

- 判定口径权威：`docs/tester-T53-g2-uploader-cases-registration-v1.md`（本节字段名/期望码与之逐字一致）。
- 执行证据：`test/reports/p053_g2_uploader_case_check.json`（23 例期望契约 + 状态 + 逐例缺失符号清单；T09 保全哈希登记；条款 token 追溯 53 项全过；25 个实现面符号全缺）。
- 交接链：manager 派 T54（G2 上传与接口）→ 实现面到位后 G2 运行器消费本 JSON `cases[].expectation` 逐例执行 → tester 出 G2 客观用例通过记录 → G3 材质 pass → G4 页面共存 → G5 真机双测。
- 未决项交裁定：A04/A05 期望码语义边界（400/402）、byteBudget 拒绝时的 receipt.code 精确值、E 类回退遥测字段在 G4 落地后复核——均以修订版登记处理，实现侧不得单方面改判。


## 18. 目标7 单沙明水 CW-01..18 用例条目（2026-10-27 登记，RED 基线）

- 来源：`docs/algorithm-T122-single-sand-clear-water-handoff-v1.md` §4 → `algorithm-single-sand-clear-water-v1.md` §8 CW-01..18（18 组合同逐字）。
- 状态：**全部 NOT_IMPLEMENTED（预测试 RED）**——实现面符号全缺（SAND_WATER_SOURCE_V1 / CLEAR_WATER_SCENE_V1 / CLEAR_WATER_OPTICS_V1 / 显式 W 库存 / SINGLE_SAND_CLEAR_WATER mode / SourceRequest·Receipt / injectWater / FrameToken 同帧字段，8/8 缺）。
- 用例格式（每组）：前置=目标7 mode/API 已实现；输入/期望输出/判定标准按 CW 表逐组；实现到位后由执行器 `test/p108_clear_water_red.cjs` 扩展逐组断言（当前仅符号核查+旧链对照基线）。
- 参数：κ / rateWater / rateSand / 一次上限 / dragWet / settlingSpeed / sigmaWater / tint / 高光 / 分段误差 全部 PENDING_CALIBRATION——测试仅输出测量值与候选 configRev，不贴既有门槛标签。
- 边界：dry 注入成功与旧 WET_COLUMN_V1 加水成功均不作为明水通过（CW-02 明示）；旧全局 levelWater 不是新邻接核 oracle；算法 §9 十二反例清单随实现保留核验。
- 执行证据：`test/reports/t108/red_baseline.json`（2 PASS 符号外基线 / 19 NOT_IMPLEMENTED / 0 FAIL；缺符号清单 8 项）。


## 19. T206 整体复杂度全链独立规格 TX-01..12 用例条目（2026-10-28 登记，RED 基线）

- 来源：`algorithm-T197-whole-transaction-complexity-v1.md` §12（P011_TX_WORK_TRACE_V1 计量、具名 scope、恒等式、独立规格表）。
- 状态：**全部 NOT_IMPLEMENTED（预测试 RED）**——实现面符号 7/7 缺（TX trace revision、scope ENTER/LEAVE/FAIL/CANCEL、具名 scope 常量、AUDIT 三分解、COPY/UNDO 计量、publication bytes、GPU 分项）。
- 独立恒等式检验器（执行器内建、自测通过，trace 到件后逐 scope 使用）：
  - `entered = completed + failed + cancelled + stillActive`（闭合 scope；FIRST_LONG 运行中快照另加 findStillActive/findCancelled，不硬塞 noPair/error）
  - `M_find = found + noPair + error`；`B_ent ≤ B_req 且 B_commit ≤ B_ent`（discard 不算 commit）；`AUDIT_LOGICAL = AUDIT_EXECUTE + AUDIT_CERT_HIT`（无证 skip 不存在）
  - `copyBytes(root) = Σ exclusive child copyBytes`；`ΣframeWork = direct + queued + step stages + observation + publication + IO`
- TX-01..12 逐条规格（oracle/对抗反例/上界）按 §12 表登记为 caseRows；trace 到件后由执行器扩展逐条断言。
- 边界：T197 designer 4822 例算术模型不是产品 oracle（本执行器 MODEL 核）；partial trace 缺尾标 INCOMPLETE 不称完整恒等式已验；耗时 inclusive/exclusive 并列不异时钟相减；诊断 OFF 也需低扰动计数，不能无日志填 0。
- 执行证据：`test/reports/t206/independent.json`（2 PASS 恒等器自检 / 13 NOT_IMPLEMENTED / 0 FAIL；缺符号清单 7 项）。


## 20. T223 沙水DEMO SW-01..14 用例条目（2026-10-28 登记，RED 基线）

- 来源：`algorithm-T221-sand-water-demo-v1.md` §10 SW01–14（逐条独立客观构造）+ T219 既定合同；与 T222 对齐不互设循环。
- 状态：**10 NOT_IMPLEMENTED / 4 PASS / 0 FAIL**（预测试 RED）——T221 专用实现面符号 17/18 缺（FLIGHT/deliveryMode/suffix16/event48/waterId/ARRIVE/arrivalSerial/emissionPolicy/flightPolicy/waterProof/optics16/profileRev/FlightVisualConfig/GravityAcceptanceToken/SAND_WATER_SOURCE_V2/vEmit/dragFlight/gFall）。
- 可先行 PASS（现 A/B 段对照基线，非明水 DEMO 通过）：SW-04 水沙独立（空水/仅沙不增 W/混合 eta=S+W）；SW-05 平床中央水双侧外流均>0；SW-08 生命周期（reset 入口+无 restore 假成功）；SW-09 原子性（拒绝 step 保留 sourceCommit）。
- 逐条规格（oracle/判定）按 §10 表登记；T221 实现到件后由执行器 `test/p223_sand_water_sw_independent.cjs` 扩展断言。
- 边界：T221 designer 4822 例设计模型不是产品 oracle（自持 Float64/精确算术）；全部新参数（vEmit/dragFlight/gFall/mRef/widthPx/gain/sigma/tint）PENDING_CALIBRATION；不自造 fps 阈值；E-SW1..4 效果需求交接 effector，设备另卡。
- 执行证据：`test/reports/t223/independent.json`（基线复跑 T188 26/26、T209 35/35、T215 12/12、T189 15/15 全绿）。
