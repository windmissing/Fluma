# P-011/P-012 掉沙 V1-V4 返修复测回执 v1（effector）

- 日期：2026-10-25；执行：effector；设备 VYG-AL00（OpenHarmony 7.0/API 26），全程 testmode（传感器不接管，重力/姿态由脚本与手势构造）
- 任务：6adcd2dd【P-011/P-012·掉沙V1-V4返修复测】真机复测掉沙准入 + shake 通路 + 长时注入稳定性（真机队列第 1 位，逐笔 `WS_EVENT gesture_sand` 取证，P4）
- 返修内容（本轮受测）：`entry/src/main/ets/pages/SandCanvas.ets`（+180 行）+ `entry/src/main/ets/physics/WaterSandField.ets`（+254 行），**位于工作区未提交状态**，已含上一轮移交的四项：① 逐笔 `WS_EVENT gesture_sand`（requested/admittedH/admittedL/rejectedReason）② `WS_EVENT shake_lift`（lifted/columns）③ `WS_EVENT pan_update`/`pan_end`（coordinateSource/accepted/updates）④ shake 门控 `MODE_LIQUID`→`MODE_WATERSAND` + Want `shake` 直通 + 手势边界 `resetPanGestureBoundary()`
- 构建：`entry-default-s138-signed.hap`（3,368,591 B），来源 main HEAD `e117a60` + 工作区修复 hunk（同一工作区另含 P-013 渲染 WIP，其 overlay 默认关闭 `p013Mode=CANVAS_BASELINE`，未进入本次测量路径）
- dida365 MCP 全程 403/unauthorized，任务正文未能读取，按标题 + 预登记口径执行，本回执本地归档
- 工具：`tools/fallsand_fixretest_p1/p2/p3.ps1`（采集）、`tools/analyze_fallsand_fixretest.py`（逐笔取证解析）

## 一、总体结论

| # | 任务子项 | 结论 |
|---|---|---|
| 1 | 掉沙准入（逐笔 `WS_EVENT gesture_sand`） | **PASS（链路已通）**：V1 41 笔 / V2 42 / V4 42 / V3b 34 / 长时 1135 笔，**准入率 100%**，`rejectedReason` 全为 `None`；手势识别成立（`coordinateSource=finger_local`、`accepted=true`）。**但暴露两个 P0 缺陷：注入量记账错误（放大约 43×）、注沙画面不可见** |
| 2 | shake 通路 | **PASS**：shake=1.0 → 7128 笔 `shake_lift`（columns=128、lifted 累计 16954），末态 `suspF=0.988`（沙 98.8% 扬起悬浮）、`depH=0.020`；对照 shake=0 → `suspF=0.00098`、`phaseOrd=0.99999`、`dormant=true`。**扬起强度无上限，需 designer 定标** |
| 3 | 长时注入稳定性 | **PASS**：90s 连续注入 4823 帧，`frameTimeMs` max 238ms，**>1s 间隙 0 帧**（H01 门槛未触发），dormant 仅 0.5%（字段持续活跃），零真崩溃、零真 ANR |
| 4 | V1-V4 重录 | **部分完成**：V1/V2/V4 录像与逐笔取证齐全；V3 原定义（4px 微划）**0 笔注入**（低于 Pan 识别 slop），改 30px 连注后 34 笔全准入但**未触发 `CAPACITY_BLOCKED`**，"满容拒注"语义仍未达成 |

**综合判定：上一轮移交的四项返修全部在真机上生效——掉沙准入链路打通、逐笔取证可用、shake 通路打通、长时注入零冻结。但"掉沙"在真机画面上仍不可辨认，且注入量记账存在量纲缺陷（P0），两项修复前不宜对"手势掉沙"出主观验收结论。**

## 二、子项 1：掉沙准入逐笔取证（PASS，附两项 P0 缺陷）

### 2.1 逐笔 `WS_EVENT gesture_sand` 取证

| 场景 | gesture_sand 笔数 | 准入笔数 | 拒绝原因分布 | 单笔 requested | 单笔 admitted | 累计 admittedH/L | model_error(code5) |
|---|---|---|---|---|---|---|---|
| V1 静置掉沙 | 41 | **41（100%）** | `None`×41 | heavy 4.0 / light 4.0 | 4.0 / 4.0 | 164.0 / 164.0 | 0 |
| V2 倾斜掉沙 | 42 | **42（100%）** | `None`×42 | 4.0 / 4.0 | 4.0 / 4.0 | 168.0 / 168.0 | 0 |
| V4 雾带交界 | 42 | **42（100%）** | `None`×42 | 4.0 / 4.0 | 4.0 / 4.0 | 168.0 / 168.0 | 0 |
| V3b 同列连注 20 次 | 34 | **34（100%）** | `None`×34 | 4.0 / 4.0 | 4.0 / 4.0 | 136.0 / 136.0 | 0 |
| 长时注入 90s | 1135 | **1135（100%）** | `None`×1135 | 4.0 / 4.0 | 4.0 / 4.0 | 4540.0 / 4540.0 | 3405 |

配套手势识别证据：`WS_EVENT pan_update` 的 `coordinateSource` 全为 `finger_local`（无 `missing_finger`）、`accepted` 全为 `true`；`WS_EVENT pan_end` 的 `updates` = 42（V1）/ 43（V2）/ 43（V4）/ 54（V3b）/ 1159（长时）。**上一轮"手势未到达"的怀疑已排除。**

### 2.2 P0-1：注入量记账量纲缺陷（每笔恒为 8 单位）

所有事务的 `requested` 恒为 `heavy=4.0 / light=4.0`（即总量 8 单位 = `SAND_RATE_UNITS_PER_S(8) × dt`，dt 恒等于 1 秒上限）。取 `gesture_sand.inputTime` 序列核对：

| 相邻 inputTime 差 | 值 | 换算 |
|---|---|---|
| 最小 | 11,126,222 | = **11.1 ms**（≈ 一帧 @90Hz） |
| 中位 | 33,238,717 | = 33.2 ms（3 帧） |
| 最大 | 66,735,441 | = 66.7 ms（6 帧） |

**`ArkUI GestureEvent.timestamp` 是纳秒单调时钟**（首值 2284653591177862 ns ≈ 26.4 天），而 `routeGestureSand` 按毫秒处理：

```ts
const dtSec = Math.max(0, Math.min(1, (now - this.lastPanRouteTime) / 1000));
```

`(now - last)` 实为 ~1.1e7 ns → `/1000` 得 1.1e4 秒 → 被 `Math.min(1, ...)` 截断为 **1 秒**。后果：

- 真实速率应为 8 单位/秒，实际为 **8 单位/事件**（90Hz 下放大约 **90×**）。
- 一次 600px 划动（44 事件）准入 344 单位，设计口径仅 8 单位 → **放大约 43×**。
- 90s 长时注入累计准入 9080 单位，设计口径 720 单位 → **放大约 12.6×**。

**建议修法**：`dtSec = Math.min(1, (now - last) / 1e6)`（ns→s），并对 dt 加下限保护与首帧剔除。

### 2.3 P0-2：注沙在真机画面上不可辨认

注沙前后（`A-00-baseline` vs `A-after-swipe-v600`）逐列几何**完全不变**：

| x | 沙床顶 y（基线 → 注沙后） | 水面 y（基线 → 注沙后） |
|---|---|---|
| 200 / 300 | 1974 → 1974 | 497→497 / 487→487 |
| 450 / 640 / 800 / 950 / 1050 | 1974 → 1974 | 883 → 883 |

差分图：全场 >20 灰阶差异仅 55,329 px（1.5%），集中在水体行带 1200–2000、横向铺满容器宽度（x 178–1096），呈**散布弱纹理**而非沙体/喷流；水体区绿砂倾向 6.7 → 6.4、亮度 162.7 → 163.5（无方向性变化）。画面动态量 mean 仅 0.13（V1）/ 0.15（V2）/ 0.10（V4），对比长时注入 1.23（8×）。

同时账本显示注入沙**已沉积**（depH=1.0000、suspF=0.0009、phaseOrd=1.0000、dormant=true），即物理侧认为沙已入库，而可见沙床零增长 → **外源沙进入的是 in-water 层（mh/ml），渲染层未把该层画成可见沙体，也未画出落砂喷流**。

**建议**：renderer 侧确认外源沙的可见表现路径（沙 tracer 喷流 / 混合带 / 沙床高度），并在 `WS_EVENT gesture_sand` 之外补一条"注入沙落点"的渲染侧可观测点。

### 2.4 P1：超量注入触发 §19.5 守卫（不冻结，但打断分相）

超量注入把沙面推过 tan30 守卫 → `WS_EVENT model_error {"code":5,"column":-1}`（`TILT_PLANNING_BLOCKED`）高频触发：长时 3405 次；V1/V2/V4/V3b 各 0 次（注入量较小）。

**该错误按设计不冻结模型**（`WaterSandField.ets:2015-2022` 对 `TILT_PLANNING_BLOCKED`/`PREVIEW_COMMIT_MISMATCH` 豁免 `frozen=true`），实测字段持续步进（phys 8–18ms、dormant=false），与上一轮 D-C1 的"永久冻结"缺陷不同。但沉积被回滚导致沙滞留悬浮（suspF 峰值 0.768），分相过程被打断（phaseOrd 0.5675 → 需等沉降恢复至 0.9999）。**修复 2.2 的量纲缺陷后该现象应自然消失。**

### 2.5 P1：手势竞争（drag 与长按互斥）

`uitest uiInput drag`（慢速长拖）被 400ms 长按识别器抢走：实测 `drag-v200` 产生 `shake_lift`×247、`tilt`×60、`invert180`×1、`model_error`×66，而 `pan_update`=**0**、`gesture_sand`=**0**，画面进入倒立态（绿砂像素 32877→29377，Δ−3500；depH 0.157、suspF 0.869）。**自动化脚本必须用 `swipe`（速度 ≥200）而非 `drag`；否则触发 testMode 切换与强制倒立。** 另 4px 位移的微划（V3 原定义）低于 Pan 识别 slop，0 笔注入。

## 三、子项 2：shake 通路（PASS，强度待定标）

| 读数 | shake=1.0 | shake=0.0 对照 |
|---|---|---|
| `WS_EVENT shake_lift` | 7128 笔（每笔 columns=128，lifted≈2.5e-5） | — |
| 累计 lifted | 16954.18 | — |
| depH | 0.0201 | 0.99996 |
| depL | — | 0.99756 |
| suspF | **0.98794** | 0.00098 |
| phaseOrderScore | 0.0000 | 0.99999 |
| physicsTimeMs / dormant | 39 / false | 0 / true |

**判定：shake 通路已真正打通**——上一轮"MODE_WATERSAND 下两条入口都不可达"（`SandCanvas.ets:908` 门控条件与操作对象错配 + Want `shake` 只喂 `liquidController`）已被修正，本轮 shake=1.0 使沙 98.8% 扬起悬浮、与对照组形成完全相反的场态（对照完全沉降、dormant）。

**遗留（需 designer 定标）**：shake=1.0 把 98.8% 的沙长期扬起且 `phaseOrderScore` 归 0（分相完全打散），该强度远超"摇晃"的真实语义；需给出 shake 强度上限与"扬起后自动回落"的目标曲线。**另 `shake_lift` 逐 substep 一条，24s 产生 7128 行，长时开启有日志膨胀风险，建议节流或聚合。**

## 四、子项 3：长时注入稳定性（PASS）

采集：`rendermode 4 / p012tier 3 / p012seed 20261020 / testmode true` → 静置 12s → 90s 左右交替 swipe 连续注沙（每轮截图 + 全量 `WS_FRAME` 落盘）。

| 指标 | 本轮（返修后） | 上一轮基线（返修前） |
|---|---|---|
| 帧数 | 4823 | 7852 |
| frameTimeMs min/P50/P95/P99/max | 11 / 21 / 42 / 48 / **238** | 9 / 11 / 24 / 42 / 237 |
| **>1000ms（>1s 冻结）帧数** | **0** | 0 |
| >200ms 帧数 | 1 | 1 |
| dormant=True 占比 | **0.5%** | 81.9% |
| substeps>0 占比 | 30.8% | 18.1% |
| 注入事务 / 准入 | 1135 / 1135（100%） | 0（注入链未通） |
| 累计准入 | 9080 单位 | 0 |
| 真崩溃 / 真 ANR | **0 / 0** | 0 / 0 |

- **无冻结**：H01 门槛（>1s）零触发，最长帧 238ms。
- **字段持续活跃**：dormant 占比由上一轮 81.9%（注沙全被拒→闲置）降至 0.5%，substeps>0 占 30.8%，说明注入真正进入物理并持续驱动沉降。
- **注入-沉降循环正常**：指标序列 depH 0.272→1.000、suspF 0.768→0.001、phaseOrd 0.568→0.9999（分相恢复）、t90H 达 29.4/35.2s，随后随继续注入再回落（depH 0.586、suspF 0.454）——属"注入-再悬浮"的正常往复。
- **错误分类**：78 行、39 个时间戳簇（09:36:04–09:37:57，覆盖整个运行窗口）。含 "ANR" 字样 1 条且来自 `ANRManager` 子系统名 → **真 ANR 0 条**；12 条 "Exception" 全为 AMS/sceneboard 能力终止生命周期序列（onAbilityDied → NotifySessionExceptionInner → RegisterSessionExceptionFunc）；子系统分布 InputKeyFlow 25（自动化注入事件流）、WMSLife 6、RSMainThread 4、MM 4、BinderInvoker 4、AMS 3、SCBMain 3。无 FATAL / SIGSEGV / SIGABRT。

## 五、子项 4：V1-V4 重录

| 场景 | 帧数 | mp4 | 注入取证 | 画面动态量 max/mean | 备注 |
|---|---|---|---|---|---|
| V1 静置掉沙 | 11 | v1.mp4 | 41 笔全准入 | 0.5 / 0.13 | 画面不可见（见 2.3） |
| V2 倾斜掉沙（wstilt 30） | 12 | v2.mp4 | 42 笔全准入 | 0.5 / 0.15 | 同上 |
| V3 满容拒注（4px 原定义） | 13 | v3.mp4 | **0 笔** | 0.1 / 0.01 | 微位移低于 Pan slop，未触发手势 |
| V3b 满容拒注（30px 连注 20 次） | 13 | v3b.mp4 | 34 笔全准入 | — | **未触发 CAPACITY_BLOCKED**，累计 272 单位仍全准入 |
| V4 雾带交界 | 12 | v4.mp4 | 42 笔全准入 | 0.4 / 0.10 | 画面不可见 |
| 长时注入 | 24 | longinj.mp4 | 1135 笔全准入 | 2.1 / 1.23 | 唯一有可见动态的段 |

**V3 "满容拒注"语义未达成**：原定义用 4px 微划（低于 Pan 识别 slop → 0 笔注入）；改 30px 同列连注 20 次后 34 笔全部准入、累计 272 单位，**仍无 `CAPACITY_BLOCKED`**。说明当前容量守卫未在注沙路径上被触达（可能因注入落在悬浮态、容量判据作用于沉积步）。需 developer 明确"满容拒注"的触发条件与可达性，tester 再据此冻结用例。

## 六、红线与移交

- 本轮未改任何主干代码：对工作区修复只做只读走查 + 真机只读采集；新增脚本在 `tools/`，未提交。
- 构建来源含未提交的 P-013 渲染 WIP（overlay 默认关闭，未进入测量路径），已在 §抬头标注，供溯源。
- 采集密度约束：hdc `snapshot_display` + `file recv` 单帧约 1–2.7s，设备无系统级录屏命令，帧序列为 4–7fps 采样，非连续视频；**注沙是否发生以逐笔 `WS_EVENT gesture_sand` + 字段状态量（suspF/dormant/sandTop 位移）判定，不以画面判读**。
- `uitest uiInput` 第 5 参为速度（有效 200–40000）；4px 位移不触发 Pan；`drag` 被长按抢走（触发 testMode 切换 + invert180）。
- 滴答清单 403，任务 6adcd2dde4b017ab33e2cb99 无法回写，本地归档。

### 移交 developer（按优先级）

1. **P0 — 注沙速率量纲**：`ArkUI GestureEvent.timestamp` 为纳秒，`routeGestureSand` 按毫秒折算导致 dt 恒为 1 秒上限 → 单笔恒 8 单位、放大约 43×。改用 `/1e6` 并加 dt 下限保护与首帧剔除。
2. **P0 — 注沙可见性**：账本已沉积（depH≈1.0）而可见沙床逐列零增长，画面动态量 0.13/0.15。需 renderer 给出外源沙的可见表现（喷流 / 混合带 / 沙床高）并补渲染侧可观测点。
3. **P1 — 容量守卫可达性**：V3b 累计 272 单位仍 100% 准入、无 `CAPACITY_BLOCKED`；需明确满容判据与触发路径。
4. **P1 — 手势竞争**：`drag` 被 400ms 长按抢走（连带 testMode 切换 + invert180）；建议长按与 Pan 的竞争规则显式化，或在 testMode 下禁用长按。
5. **P2 — shake 强度定标 + `shake_lift` 节流**：1.0 强度扬起 98.8% 沙且分相归零；`shake_lift` 24s 产 7128 行，需节流。
6. **P2 — `model_error` 量级**：长时 3405 条 `code=5` 建议聚合上报，避免与真正的异常混淆。

## 七、产物索引与入库状态

**入库 37 个 artifact**（34 个文本证据 `.txt` 全部 + 3 张决策关键截图 `A-after-swipe-v600.jpeg` / `B-shake-1.0.jpeg` / `B-control-0.0.jpeg`），另随附本卡回执与 4 个采集/分析工具。

**未入库但保留在工作目录**（`test/artifacts/p012-fallsand-fixretest-20261025/`，原因逐项标注）：

| 未入库项 | 数量 | 原因 |
|---|---|---|
| 签名 HAP `entry-default-s138-signed.hap` | 1 | 仓库惯例不跟踪 HAP 二进制（`git ls-files test/artifacts` 仅 20 个文本/图） |
| 录像 mp4（v1/v2/v3/v4/longinj） | 5 | 派生视频，可由帧序列复算；体积 183–508KB |
| 补充截图（A-after-{drag-v200,fling-v200-s6,swipe-v200} / C-longinj-final / D-v3b-final / A-00-baseline / B-control-0.0） | 6 | 次级视觉证据，日志侧已可复算 |
| 原始帧序列 `frames-v1|v2|v3|v3b|v4|longinj/` | 85 帧 | 采样帧（hdc 单帧 1–2.7s，4–7fps），体积约 20MB |

目录 `test/artifacts/p012-fallsand-fixretest-20261025/` 全量内容：

- 构建产物：`entry-default-s138-signed.hap`（3,368,591 B）
- 掉沙逐笔取证：`A-events-{swipe-v600,fling-v200-s6,swipe-v200,drag-v200}.txt`、`A-metric-series-*.txt`、`A-metric-last-*.txt`、`A-metric-baseline.txt`、`A-00-baseline.jpeg`、`A-after-*.jpeg`
- shake 通路：`B-events-shake.txt`（7128 笔 shake_lift）、`B-metric-series-{shake,control}.txt`、`B-metric-last-{shake,control}.txt`、`B-shake-1.0.jpeg`、`B-control-0.0.jpeg`
- 长时注入：`C-longinj-wsframe.txt`（4823 帧）、`C-longinj-gesture_sand.txt`、`C-longinj-metric-series.txt`、`C-longinj-metric-tick.txt`、`C-longinj-metric-last.txt`、`C-longinj-errors.txt`、`C-longinj-final.jpeg`、`frames-longinj/`（24 帧）
- V1-V4 重录：`frames-v1|v2|v3|v3b|v4/`（61 帧）、`v1.mp4/v2.mp4/v3.mp4/v4.mp4/longinj.mp4`、`D-events-v{1,2,3,3b,4}.txt`、`D-metric-v{1,2,3,3b,4}.txt`、`D-v3b-final.jpeg`
- 工具：`tools/fallsand_fixretest_p1.ps1`（逐笔取证 + shake）、`tools/fallsand_fixretest_p2.ps1`（长时 + V1-V4）、`tools/fallsand_fixretest_p3.ps1`（V3b 满容拒注）、`tools/analyze_fallsand_fixretest.py`（取证解析）

## 八、manager 核实项与两项说明

### 8.1 manager 已核实的修复生效项（三项，与本卡实测互证）

| 核实项 | 本卡对应实测 | 互证结论 |
|---|---|---|
| `gesture_sand` 逐笔落出 | V1 41 / V2 42 / V4 42 / V3b 34 / 长时 1135 笔逐条落出，字段含 `sourceId/sequence/inputTime/requested{heavy,light}/admittedH/admittedL/rejectedReason` | **一致**：准入率 100%、`rejectedReason` 全 `None` |
| `shake_lift` 落出 | shake=1.0 下 7128 笔，每笔含 `lifted/columns`（columns 恒 128） | **一致**：累计 lifted 16954.18 |
| 坐标不累加 | `pan_update` 的 `coordinateSource` 恒为 `finger_local`（无 `missing_finger`）、`pan_end` 的 `localX` 收敛于 256.29（= 划动终点，未随事件累加放大） | **一致**：上一轮怀疑的 `e.offsetX + lastPanLocalX` 累加放大未复现 |

### 8.2 说明一：`gesture_sand.sequence` 的"重置"是设计语义，不是计数缺陷

`resetPanGestureBoundary()`（`SandCanvas.ets:2467`，由 Pan 的 `onActionStart` / `onActionEnd` / `onActionCancel` 触发）做两件事：`gestureSourceId++` 与 `gestureSequence = 0`。

- **`sequence` 是手势内序号**：每次手势从 1 重新计数，故跨手势看会出现"回退到 1"的表象。
- **`sourceId` 是跨手势单调源标识**：随每次手势边界递增。
- **全局唯一键是 `(sourceId, sequence)`**，单看 `sequence` 不具全局意义；`seed` 亦由 `(sourceId × 100003 + sequence)` 派生，与该语义一致。
- 判读建议：跨手势对账时用 `sourceId` 分组、`sequence` 组内排序；不要用 `sequence` 的连续性判断事务是否丢失。本卡 V1 一次划动的 41 笔事务 `sequence` 为 1…41、`sourceId` 恒定（同一手势内），长时 90s 的 1135 笔分属多个 `sourceId`（每手势一个）。

### 8.3 说明二：A 段 `drag` 候选与其他三个手势的差异来源

A 段四个候选手势中，`swipe-v600` / `fling-v200-s6` / `swipe-v200` 三个均产生 `pan_update`（44/108/89 笔，`coordinateSource=finger_local`、`accepted=true`）并 100% 准入；**唯独 `drag-v200` 的 `pan_update` = 0、`gesture_sand` = 0**。

差异来源与坐标累加修复**无关**——`drag` 根本没有进入 Pan 通道：

| 证据 | 内容 |
|---|---|
| 事件构成 | `drag-v200` 产生 `shake_lift`×247、`tilt`×60、`invert180`×1、`model_error`×66，而 `pan_update`=0 |
| 机制 | `uitest uiInput drag` 是慢速长拖（200px/s 走 600px ≈ 3s），**超过画布 400ms 长按识别器的时长阈值**（`SandCanvas.ets:3134`，duration=400）→ 长按胜出，触发 `applyInputMode(!testMode)` 切换 testMode 并连带 `invert180`；Pan 被竞技场判负，`onActionUpdate` 从未进入 |
| 画面佐证 | 画面进入倒立态：绿砂像素 32877→29377（Δ−3500）、depH 0.157、suspF 0.869、phaseOrd 0.468 |
| 结论 | 这是**手势竞技场竞争**问题（长按 vs Pan），不是坐标换算问题；坐标累加修复对 `swipe`/`fling` 生效并已由 8.1 佐证 |

**给自动化脚本的约束**：注入掉沙手势一律用 `uitest uiInput swipe`（速度 ≥200），**不得用 `drag`**；也不得用低于 Pan 识别 slop 的微位移（实测 4px 微划 → 0 笔注入，V3 原定义即因此失效）。若产品侧希望长按与 Pan 并存互不干扰，需 developer 显式化二者竞争规则（或在 TEST 模式禁用长按）。
