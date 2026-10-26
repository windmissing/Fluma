# T74 触摸零源设备补采执行案

日期：2026-10-26（Asia/Shanghai）  
责任：effector  
任务卡：6adeb1e9e4b017ab33f703e0  
设备：VYG-AL00（hdc 62T0225B10010020）  
构建基线：worktree 隔离构建 @ `545f45e`（HEAD：T72 被动诊断 0c86c47 + T66 收尾 545f45e + T55/T75 材质），树指纹 `5b34bc45ce9cc5b7`  
结论状态：**待执行**（verdict 以回执为准）

## 0. 任务边界（与 T69 §7.2 对齐）

1. **目标**：对 T59 干场候选（`DRY_AIR_V1`，T73 离线 17/17 PASS）在真机上做**事件链逐级取证**：`RECEIVE → MAP/START → CLOCK → REQUEST → RECEIPT → PUBLICATION → 新管线呈现`，找出**首个可证失败点**，交 T69 按 §7.2 表格选 T71 实施分支。
2. **不等 T71 修复**；不把 T74 的 run 与 T63 旧湿场混同（介质=干场 DRY_AIR_V1，页面=EmptySandDemo）。
3. **渲染合同**：仅 XComponent+shader 新管线；页面当前呈现若为 FAULT/stale 帧如实记录（T68 合同：**失败 FAULT 或标明 stale 的上一有效帧**），**不因缺完整画面阻断根因取证**——present code/swap 仅是接口回执，事件链证据优先。
4. **不可改产品代码**（T44 边界沿用；T74 同为 effector 采集卡）：只采集、记录、出报告；发现缺陷记证据交归因，不自改 `entry/src`。tools/test 脚本与回执文档不受限。
5. **诊断流为被动有界环形缓冲**（T72 交付）：256 条 ring、单行最多 2 事件、每次 flush ≤8 行、`dropped` 计数明示缺口；`sequence` 连续性可检测丢失区间。**不得因 dropped 将证据判全无效**，但须在报告中披露 dropped 与覆盖缺口；Down/Up/Cancel/首次拒绝/状态转换类事件丢失则对应阶段记 MISS。
6. 真机队列：T74 当前就绪独占设备；串行，不并发。

## 1. 页面与入口（EmptySandDemo 现状走读，commit 545f45e）

- 入口：`EmptySandDemo` 页（EntryAbility：`--ps emptysanddemo 1` → `pages/EmptySandDemo`；main_pages 已注册）。
- 触摸：`.onTouch` 挂在 **XComponent 本体**（libraryname:'p013renderer'，native DispatchTouchEvent=nullptr，T69 判定契约缺口——**T74 正是要测这条链是否真零事件**）。raw 事件 → `touch()` → `model.start/move/stop`。
- 干场注入：`model.tick` 内 `touching` 时用 `P011GestureRateClock` 采帧间秒数，正秒数发 `P051DryAdmissionRequest`（`requestedH=8*seconds, L=0, sourceId=51`）→ `field.injectDryGestureSand` → receipt（`dry.receipt` 诊断：admitted/rejectedReason）→ 非 NO_OP 拒绝即 `stop('admission:...')`。
- 重力：加速度计 → `P051GravityInputAdapter`（`sensor` 诊断 + `tick` 的 source/staleCount）；设备平放 USB 连接时张力可能产生非零 tilt/shake——**如实记录，不标定**。
- 渲染：`submitDualFieldSurfacePx` → `present` 诊断（code/swap/dualFailure）；状态文本"新管线等待/拒绝"含 dualFailure。
- 诊断事件 kind 全集（T72）：`touch`（原 TouchType/x/y）、`model.start/move/stop`（reason）、`tick`（source/staleCount/value/active）、`sensor`（投递计数）、`dry.request`（sequence/requested/x/y）、`dry.receipt`（admitted/reason）、`wet.receipt`（仅湿路径，干场不应出现）、`present`（code/swap/reason）、`demo_gravity_config`、`empty_sand_demo`（60 帧摘要：input/config/tilt/dry metrics）。
- 坐标映射（页面现状）：`x = touches[0].x / surfaceWidth * 128`、`y = (1 - touches[0].y / surfaceHeight) * 256`——surfaceWidth/Height 来自 `onAreaChange`（vp）。T69 §5.2 要求实测内容区；本卡记录 `onAreaChange` 实际值（经 present 的 boxW/boxH 或 SurfaceReady 日志核对）。

## 2. 场景矩阵（固定输入、固定步骤）

| 臂 | 目的 | 时序 |
|---|---|---|
| R0 | 冷启冒烟：页面可达、诊断流活着、present 状态 | `emptysanddemo 1` 冷启 → 15s 静置（无触摸）→ dump |
| R1 | 单点触摸全链：Down→hold→Up | 冷启 → 5s → `uiInput swipe 700 1200 700 1200 3000`（同点 3s"按住"，swipe 同点=无 Move 的持握）→ 5s → dump+截图 |
| R2 | 拖动全链：Down→Move…→Up | 冷启 → 5s → `uiInput swipe 400 1200 850 1200 2000`（T63 同款中心滑动 200px/s 慢速）→ 5s → dump+截图 |
| R3 | 右壁与边界 | 冷启 → 5s → `uiInput swipe 1160 1200 1220 1700 2000`（T63 右壁动作）→ 5s → dump |
| R4 | reset 后再触摸（非空 reset 取证） | 冷启 → 3s → swipe 中心 → 5s（粒子落床）→ click 重置按钮坐标 → 3s → 再 swipe → 5s → dump |

时序纪律：每臂 `hilog -r` 清缓冲；动作与 snapshot 交错；冷启后关 USB 弹窗（240,620）；`power-shell timeout -o 600000` 防熄屏，收尾 `-r` 恢复；uitest swipe 第 5 参为速度非时长（T63 记录为 200px/s 慢速语义，实际注入按 velocity 参数；两历史值 200/600 不混用，本卡统一 600 默认并在报告注明）。

## 3. 逐级判读表（首个可证失败点 → T71 分支，按 T69 §7.2）

| 级 | 证据（demo_diag kind 序列） | 通过条件 | 首失败 → T71 分支 |
|---|---|---|---|
| L1 RECEIVE | `touch` 事件（type/x/y） | Down/Move/Up 按动作出现；x/y 为 local vp | 无任何 `touch` 行且动作坐标在内容区内 → **§5 独立无绘制输入节点**（XComponent 吞事件实锤） |
| L2 MAP/START | `model.start`（x/y 物理域）+ `model.move` | start 仅一次/gesture；坐标 (u=W/2,v=H/2)≈(64,128) | RECEIVE 有而 start 无/move 未更新 → 查 geometry/门控 hunk |
| L3 CLOCK | `tick`（source/staleCount）+ 持续正样本 | 按住期间 tick 连续；stale 不早发 | start 后 stale 立即 stop → **查 sensor 投递/250ms 门**（T50 策略归 designer，不取消门） |
| L4 REQUEST | `dry.request`（requested>0） | 按住跨 2 帧后出现正请求 | 正 tick 无 request → touching/时钟漏接（**禁止补固定质量**） |
| L5 RECEIPT | `dry.receipt`（admitted>0 或明确 reason） | ACCEPTED 或具体拒因；`wet.receipt` 不应出现 | request 有而 receipt 无 → sink 缺失；拒因 CAPACITY/OCCLUSION 等按原样记录 |
| L6 PUBLICATION | 60 帧摘要 `metrics`/`dry` 非零、sourceEpoch/源累计增长 | 准入后 publication 跟进 | 正准入但 metrics 全零 → 发布链问题 |
| L7 RENDER | `present` code=0/swap=true + 截图可见沙 | 接口回执成功 | 回执成功但画面无沙/FAULT → **转新管线 packet/token 路径（T68 合同），不再称触摸零源** |

每臂产出：首个可证失败点级别 + 原始证据行；多臂冲突时以最完整臂为准、冲突披露。

## 4. 采集与分析

- `tools/t74_touch_source_capture.ps1`：R0–R4 五臂（前台执行）；输出 `test/artifacts/t74-touch-source-capture-20261026/`。
- `tools/analyze_t74.py`：解析 `WS_EVENT demo_diag` chunk（`revision=P051_PASSIVE_DIAGNOSTICS_V1`、dropped/first/lastSequence、rows[]），重建事件链 → 逐级 L1–L7 verdict；parse `empty_sand_demo` 60 帧摘要与 `present` 统计；输出 `t74-analysis.json`。hilog 截断容忍（BOM 检测/utf-8 优先/括号补全）；sequence 断档标 GAP+dropped。
- 截图人工审看记录可见沙有无（主观锚点，不替代 L7 接口回执）。
- 环境/身份披露：commit `545f45e`、树指纹 `5b34bc45ce9cc5b7`、HAP SHA256（构建后填）、设备 ID、每臂起止时间。

## 5. 交付与移交

- 回执 `docs/effect-report-P051-T74-device-touch-capture-v1.md`（追加模式，带 commit hash）：逐臂 L1–L7 表、首个可证失败点、dropped/GAP 披露、present FAULT 现状、与 T73 离线结论一致性。
- 结果交 T69（designer）按 §7.2 选 T71 分支；manager 登记。若 L7 前（事件链）全通而画面 FAULT，明确"触摸零源不成立，转 T56/T78 渲染线"。
- 卡完成评论"已完成"+产物入库；不改清单他行、不动滴答（无授权时留 manager）。

## 6. 失败与重试

- 臂内页面崩溃/黑屏：`hilog -x` 全量落盘 + `hidumper` 进程状态，记 FAULT 重跑一次；再现则停止并归档。
- uitest 注入不触发（连 `touch` 都无）：先验证 R0 诊断流活着（sensor/tick 行），排除"页面完全没跑"再下 L1 结论。
- 连续两臂同因失败：暂停报告。
