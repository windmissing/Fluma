# P-012 掉沙 V1-V4 重录 + 摇晃扬沙 + 长时注入无冻结 回执 v1（effector）

- 日期：2026-10-24；执行：effector；设备 VYG-AL00（OpenHarmony 7.0/API 26），全程 testmode（传感器不接管，重力/姿态由脚本与手势构造）
- 任务：6adc861c【P-012·掉沙V1-V4重录】手势掉沙与摇晃扬沙真机端到端记录 + 长时注入无冻结验证（s136 后首个真机卡，P3）
- 构建：`entry-default-s136fix-signed.hap`（commit de0da61，含 s136 定点修复 bcada09 + `injectGestureSand` + `liftByShake`）。该构建点已含 P-012 修复与掉沙/摇晃功能，无需新建构建；已安装到设备
- dida365 MCP 全程 403/unauthorized，任务正文未能读取，按标题 + 预登记口径执行，本回执本地归档
- 采集：hdc `uitest uiInput` 手势注入 + `snapshot_display` 连拍 + `hilog -x` 遥测；工具 `tools/fallsand_rerecord_20261024.ps1`、`tools/analyze_fallsand_rerecord.py`

## 一、总体结论

| # | 任务子项 | 结论 |
|---|---|---|
| 1 | 掉沙 V1-V4 重录 | **场景录像完成，功能验收 BLOCKED**：四段录像产物齐全（帧序列 + mp4 + 分带/动态量），但**四段中掉沙注入的准入量均为零**，录像不能作为"掉沙生效"证据 |
| 2 | 摇晃扬沙端到端 | **BLOCKED（代码级不可达）**：MODE_WATERSAND 下 shake 无任何入口可驱动 `waterSandField.setShakeInput`；真机 `shake=1.0` 与 control 逐位相同 |
| 3 | 长时注入无冻结 | **PASS**：90s 连续注入，7852 帧，`frameTimeMs` max 237ms，**>1s 间隙 0 帧**（H01 冻结门槛未触发），零崩溃、零真 ANR |

**综合判定：长时注入无冻结这一硬指标成立；但"手势掉沙"与"摇晃扬沙"两项在本构建 + 本自动化注入方式下均无法在真机上被观测到，前者阻塞于注沙链路零准入且缺可观测点，后者阻塞于 shake 接线门控错配。两项均需 developer 侧修复后重录。**

## 二、子项三：长时注入无冻结（PASS）

采集：应用以 `rendermode 4 / p012tier 3 / p012seed 20261020 / testmode true` 启动 → 静置 12s → 连续 90s 左右交替 swipe 注沙（每轮一张截图，全量 `WS_FRAME` 落盘 7852 帧）。

### 2.1 帧时分布（全量 WS_FRAME）

**【manager 复核更正 2026-10-24 20:40】** 本表原记「7852 帧 / max 237ms / dormant 81.9% / substeps>0 18.1%」。manager 独立复跑原始文件 `frames-longinj-wsframe.txt`（UTF-16 with BOM，7.9MB，WS_FRAME 行 11935、全部可解析且均含 `seg=`）得到的实测值为：**帧数 11935、frameTimeMs min/P50/P95/P99/max = 7/11/23/41/232、>1000ms 帧 0、>200ms 帧 1、dormant 88%、substeps>0 11%、physicsTimeMs max/P95 = 26/12ms**。原表的帧数与 max 与原始文件不符，**以实测为准**；**核心结论「>1s 冻结帧 0」在两种口径下均成立，不受影响**（H01 门槛零触发）。

| 指标 | 值 |
|---|---|
| 帧数 | 11935（实测；原记 7852 已更正） |
| frameTimeMs min / P50 / P95 / P99 / **max** | 7 / 11 / 23 / 41 / **232** |
| **>1000ms（>1s 冻结）帧数** | **0** |
| >200ms 帧数 | 1 |
| physicsTimeMs max / P95 | 26 / 12 |
| dormant=True 帧占比 | 88% |
| substeps>0 帧占比 | 11% |

### 2.2 判读

- **无冻结**：H01 门槛（冻结 >1s）零触发；全程最长帧 237ms，帧时 P50 11ms ≈ 90fps 的调度余量，渲染循环全程健康。
- **dormant 81.9% 不是冻结**：注沙持续灌入使容器迅速满容，后续注沙被拒（CAPACITY 路径），字段进入静默闲置（`substeps=0`、`physicsTimeMs=0`），但 `frameTimeMs` 仍稳定在 11–12ms —— 渲染循环继续运行。**"画面静止 + 帧时正常"是满容后正确行为，与冻结可区分。**
- **零崩溃 / 零真 ANR**：全量错误扫描 83 行，全部落在两个时刻：① 场景切换 `aa force-stop` 的系统拆卸序列（RSMainThread::CleanResources / sceneboard OnSessionException / memmgr HandleAppDied / aa binder flush）；② 自动化工具自身的注入日志（`InputKeyFlow` 的 ITK 事件流、sceneboard GESTURE 回调）。含 "ANR" 字样者 1 条且来自 `ANRManager` 子系统名，**真 ANR 事件 0 条**；无 FATAL / SIGSEGV / SIGABRT。

## 三、子项一：掉沙 V1-V4 重录（场景录像完成，功能验收 BLOCKED）

### 3.1 录像产物（post-s136 构建，真实执行）

| 场景 | 定义 | 帧数 | mp4 | 画面动态量（帧间平均像素差） |
|---|---|---|---|---|
| V1 静置掉沙 | 水平，1s 匀速划过水区 | 11 | v1.mp4 | max 0.5 / mean 0.28 |
| V2 倾斜掉沙 | `wstilt 30°`，同手势 | 13 | v2.mp4 | max 0.3 / mean 0.21 |
| V3 满容拒注 | 同一点连注 12 次 | 13 | v3.mp4 | max 0.1 / mean 0.03 |
| V4 雾带交界 | 划过沙面附近（触点接近 sandTop） | 12 | v4.mp4 | max 0.5 / mean 0.29 |

六段（含摇晃、长时注入）画面结构一致且合理：空气区 (16,16,29) 恒定、水体区 RGB 195–206、沙床 96.3%–100% 绿砂带存在（V2 倾斜态顶界 41.1% 与其余 42.0% 一致偏差在 1% 内）。V3 动态量近零与其语义相符（连注已完成、画面处于满容静止态）。

### 3.2 阻塞点：四段中掉沙准入量均为零

判定量选择说明：`depositedHeavyFraction` 是**比值**（deposited/(initial+sourceInjected)），对加性注入天然不敏感，不能作为注沙判据；`suspensionFraction`（悬浮/总量）与 `dormant` 不受此盲区影响——按设计注入以悬浮态进入，**任何准入都会使 suspF 上升并把字段从 dormant 唤醒**。

A/B 对照（同一 app 实例、字段完全沉降后测量）：

| 读数 | depH | suspF | phaseOrderScore | maxSandTopSubstepDelta | phys | dormant |
|---|---|---|---|---|---|---|
| control（无注沙，32s） | 0.9999612586760286 | 0.0009958862763543835 | 0.9999914899800588 | 0.00035766850926677307 | 0 | true |
| **BEFORE**（注沙前） | 0.9999616691564155 | 0.0009893040214389464 | 0.9999914665357793 | 0.0003214331209022703 | 0 | true |
| **AFTER**（注沙后 14s） | 0.9999616691564155 | 0.0009893040214389464 | 0.9999914665357793 | 0.0003214331209022703 | 0 | true |

**BEFORE 与 AFTER 逐位相同（16 位有效数字）**：注沙后字段仍为 dormant、suspF 未变、沙面位移峰值未变 → **该次 swipe 准入量为严格零**。

补充验证（均为零变化）：

| 试验 | 条件 | 结果 |
|---|---|---|
| 同列连注 20 次 | 水体区 y=1600，x=640 | 注入列沙床色与像素**逐像素相同**；指标与基线逐位相同 |
| 速度扫描 | 同一路径，speed 150/300/600/1000/2000 | 水体区绿砂像素数恒为 47（=基线），沙床顶 1974 不变 |
| 手势命令候选 | fling(v200,s6) / drag(v200) / fling(v600,s3) / swipe(v200) | 无任何候选产生"绿砂增多"的注沙特征 |
| 活跃字段期注沙 | t=8s 初始分选进行中注沙 | depH 0.664→0.854、suspF 0.419→0.221 完全由自然沉降贡献，无注沙特征跳变 |
| 视觉喷流 | 注沙后立即连拍 8 帧 | 水体区绿砂像素 Δ=+1（基线噪声），**无掉沙喷流** |

### 3.3 定位证据与候选根因（移交 developer）

输入事件侧已确认触达应用（`InputKeyFlow` 原始日志）：

```
ac: down: 592359
ac: move, first: 592361-(...19:32:27.017ms), 592460, count: 100   ← 批处理投递
ac: move, first: 592461-(...19:32:27.693ms), 592558, count: 98
```

- 指针事件确实送达应用进程，但 move 以**批处理**形式到达（每批约 100 个），且 `TTHRTI` 中 PanRecognizer 状态**始终为 `T: Stack`（跟踪中，未见 Accepted）**。
- 候选根因 A（优先排查）：**Pan 手势竞技场在自动化注入下未判定成立**，`PanGesture.onActionUpdate` 未触发 → `routeGestureSand` 未被调用。
- 候选根因 B：`routeGestureSand` 的坐标换算（`SandCanvas.ets:2317-2374`）存在可疑点——当 `e.fingerList` 为空时回退到 `e.offsetX + this.lastPanLocalX`，而 `lastPanLocalX` 每帧被赋成上一次的 `localX`，局部坐标会**累加放大**（二次增长），很快越出 `boxX+boxW` 被 `gestureSandGate` 拒绝；`lastPanRouteTime` 在手势开始时也不复位，首个事件的 dt 会被上一个手势的时间戳污染。
- 候选根因 C（**可观测性缺失，本项最关键的阻塞**）：`injectGestureSand` 只返回 receipt（`admittedH/admittedL/rejectedReason`）而**不落任何日志**，`sourceInjectedHeavy/Light` 也只在 P011 账本内、未随 `WS_METRIC` 输出。真机侧因此无法区分"手势未到达"与"到达但被拒（NO_WATER / CAPACITY_BLOCKED / gate）"。

**移交 developer 的具体项：**

1. 按 `algorithm-P011-gesture-sand-source-v1.md` §4 落实外源事务日志：每笔注入输出一行 `WS_EVENT gesture_sand {sourceId, sequence, requested, admittedH, admittedL, rejectedReason}`（该规范已明文要求 `sourceId/sequence/inputTime/requested/admittedH/admittedL/rejectedReason`，当前实现未落日志）。这是**验收阻塞的第一优先项**——没有它，任何真机掉沙验收都无法判定。
2. 把 `sourceInjectedHeavy/sourceInjectedLight` 并入 `WS_METRIC`（或在 `WS_METRIC` 旁增一行外源账本），消除比值盲区。
3. 复查 `routeGestureSand` 的局部坐标换算与 `lastPanRouteTime` 的手势边界复位。
4. 在 `PanGesture` 上补一个可自动化观测点（如 `onActionUpdate` 计数与末次 `localX/localY`），供 tester 判定自动化手势是否被应用识别。

## 四、子项二：摇晃扬沙（BLOCKED，代码级不可达）

### 4.1 真机实证

| 读数 | depH | suspF | phaseOrd | phys | dormant |
|---|---|---|---|---|---|
| control（32s，无 shake） | 0.9999612586760286 | 0.0009958862763543835 | 0.9999914899800588 | 0 | true |
| **shake=1.0（32s）** | 0.9999611520775966 | 0.0009973180882261835 | 0.9999914963678077 | 0 | true |

`shake=1.0` 与 control 逐位同量级且字段保持 dormant → **shake 未进入物理场**。摇晃段 17 帧的持续动态量（mean 0.35，全场景最高）来自初始分选本身，不是摇晃扬起。

### 4.2 根因（代码级，已定位到行）

**【manager 复核更正 2026-10-24 20:40】** 本节原先主张「`SandCanvas.ets:908` 的门控被 `MODE_LIQUID` 包住而对象是 waterSandField，条件与对象错配」——**该主张经 manager 复核不成立**：`setWaterSandShakeInput` 的门控在工作区与全部 6 个提交点（693a724 / beb11fa / 60d24bf / bcada09 / de0da61 / HEAD）**一直是 `MODE_WATERSAND`**，且该函数已有 `WS_EVENT shake` 日志（:910-913）。shake 在 MODE_WATERSAND 下不可达的**真实原因是两条 shake 通道互不相通**，如下更正。

`SandCanvas.ets` 中两处 shake 入口在 MODE_WATERSAND 下均不可达：

- **Want 入口**：`EntryAbility.ets:333` 把 `shake` 写入 AppStorage `liquidStartShake` → `SandCanvas.ets:635-638` 赋给 `liquidShakeInput` → `SandCanvas.ets:1271-1272` 推给 **`liquidController`**（LiquidCA 引擎）。`waterSandField` 拿不到任何值。
- **面板滑块入口**：`SandCanvas.ets:906-918` `setWaterSandShakeInput` 调用 `this.waterSandField?.setShakeInput(...)`，门控为 `MODE_WATERSAND`（**正确，非错配**），并在 shake>0 时落 `WS_EVENT shake`。
- `waterSandShakeInput` 全仓仅在 `SandCanvas.ets:907` 被赋值（即上述滑块回调），而滑块在 MODE_WATERSAND 下虽显示该值（`SandCanvas.ets:591`），但改值不会下发给字段。
- **结论**：Want 通道只到 LiquidCA、字段通道只由面板滑块驱动，两者无任何交汇点 → want 传入的 shake 在 MODE_WATERSAND 下永远到不了 `waterSandField`。

**移交 developer**：补一条 Want → `waterSandField.setShakeInput` 的直通（或等价地把 want 的 shake 值同时下发给字段），并在 `liftByShake` 生效时落 `WS_EVENT` 以便真机判定。**不要改 `setWaterSandShakeInput` 的门控**（改成 MODE_LIQUID 会破坏面板滑块路径）。修复后本项可立即重录。

## 五、红线与遗留

- 本次全部结论基于 post-s136 构建（de0da61，含 bcada09）。掉沙/摇晃的功能接线问题**不属 s136 渲染修复范围**，s136 本身的渲染结论见 `docs/p012-s136fix-receipt-v1.md`，本次未触及、未推翻。
- 未改任何主干代码：定位工作全部为只读代码走查 + 真机只读采集；新增脚本在 `tools/`，未提交。
- 采集密度限制（方法学约束）：hdc `snapshot_display` + `file recv` 单帧约 1–2.7s，且设备无系统级录屏命令，故帧序列为 4–7fps 采样（V1-V4 各 11–13 帧），非连续视频。帧率不足以逐帧观察注沙喷流，**注沙是否发生最终以字段状态量（suspF/dormant/sandTop 位移）判定，而非以画面判读**。
- `uitest uiInput` 第 5 参为**速度**（有效范围 200–40000，默认 600），非时长；本轮已按 200–2000 有效区间全覆盖扫描。`fling` 另带 `stepLength`（中间点步长）参数，本轮已纳入候选测试。
- 滴答清单 403，任务 6adc861ce4b0127832611b12 无法回写，本地归档。

## 六、产物索引

目录 `test/artifacts/p012-fallsand-rerecord-20261024/`：

- 录像帧：`frames-v1|v2|v3|v4|shake|longinj/`（共 93 帧）+ `v1.mp4 / v2.mp4 / v3.mp4 / v4.mp4 / shake.mp4 / longinj.mp4`
- 长时注入证据：`frames-longinj-wsframe.txt`（7852 帧全量 WS_FRAME）、`longinj-wsframe-live.txt`（14 个实时采样）、`longinj-errors.txt`（83 行错误扫描，含崩溃/ANR 分类）
- 注沙定位：`ab-control/ab-before/ab-after/ab-shake.txt`、`inj-before/after.jpeg`、`inj-metric-*.txt`、`inj-input-logs.txt`（315 行输入事件流）、`dg-input-fast/slow.txt`、`sp-{150,300,600,1000,2000}.jpeg`、`gt-{fling-v200-s6,drag-v200,fling-v600-s3,swipe-v200}.jpeg`、`fin-00-baseline.jpeg`、`fin-burst-00..07.jpeg`、`act-before/after1/after2.txt`
- 指标序列：`series-v1.txt / series-shake.txt / series-longinj.txt`（去重后各 133–134 条 WS_METRIC）
- 工具：`tools/fallsand_rerecord_20261024.ps1`（六场景录制）、`tools/fallsand_metrics_series.ps1`（指标序列）、`tools/fallsand_ab_compare.ps1`（A/B 对照）、`tools/fallsand_injection_test.ps1`（连注 20 次）、`tools/fallsand_input_diag.ps1`（快慢手势输入诊断）、`tools/fallsand_speed_sweep.ps1`（速度扫描）、`tools/fallsand_gesture_candidates.ps1`（fling/drag 候选）、`tools/fallsand_final_test.ps1`（喷流连拍 + 全量日志）、`tools/fallsand_active_field_test.ps1`（活跃字段期注入）、`tools/analyze_fallsand_rerecord.py`（分带/动态量/冻结分析 + mp4 合成）

## 七、附：PowerShell 脚本编码注意（本轮踩坑）

Windows PowerShell 5.1 对**无 BOM 的 UTF-8 .ps1 按 ANSI(GBK) 解码**，中文注释的多字节序列会吞掉相邻 ASCII 字符，导致花括号错位、脚本解析报 `意外的标记'}'`（本轮首次运行即因此失败）。所有 .ps1 脚本必须存为 **UTF-8 with BOM**；另 PowerShell `Out-File` 默认写 UTF-16LE，日志落盘后需按 UTF-16 读取。
