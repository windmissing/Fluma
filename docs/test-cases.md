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
