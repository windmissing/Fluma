# T82 触摸接线修复后 DEMO 复测执行案

日期：2026-10-26（Asia/Shanghai）  
责任：effector  
任务卡：6adef380e4b06ee3573cc257  
设备：VYG-AL00（hdc 62T0225B10010020）  
构建基线：worktree 隔离构建 @ `114a7ee`（T56 完整 scene + T71 输入节点 0b1f7e3 + T85/T86 前置），entry/src 树指纹 `403b08514017b326`  
结论状态：**待执行**（verdict 以回执为准）

## 0. 任务边界

1. **验证目标**：T71 无绘制输入节点修复后，设备上完成 **事件→准入→LAND→可见沙** 全链闭环；覆盖"零水冷启、真实注沙落床、松手停止边界、手持重力方向"四项（任务标题全量范围）。
2. **判据升级**：T74 的 L1–L7 事件链判据复用，但本轮是 **PASS 判定**（T74 是定位 FAIL）。L1–L5 任一 FAIL = T71 修复无效，如实 FAIL 交 T69/T71；L6/L7 FAIL = 发布/渲染线问题，分流 T56/T88/T57。
3. **呈现判定红线**（清单要求）：**采集结果不得以 swap 回执代屏幕通过**——L7 必须含截图像素证据（容器区可见沙），`present code=0/swap=true` 仅是接口回执。
4. **手持重力**：手持倾斜为本轮可做项（真机在手）；传感器 tilt 值如实记录（`tick` source/value、60 帧摘要 effectiveTiltDeg），重力方向主观观察（沙向低端流动）。强度标定 NOT_CALIBRATED 不打分，方向正确性 0/1 记录。
5. 不可改产品代码（T44/T74 边界沿用）；tools/分析器/回执不受限。
6. 同机串行：T82 → T78 → T57；本卡独占设备窗口。

## 1. 被测实现走读（commit 114a7ee）

- **输入**：`P051TouchSourceAdapter`（T71）——同框无绘制 Stack（`HitTestMode.Block` + onTouch）覆盖 XComponent（`HitTestMode.None`）；几何 `onAreaChange` 首次实测前 `ready=false` 拒输入；单主指状态机（DOWN/UP/Cancel/多指/AMBIGUOUS_RELEASE）；`model.start` 显式回执（accepted/reason/stopGeneration）；与模型内部 stop（`sync()`/MODEL_STOP）双向同步。
- **诊断**（T72 环形流，事件 kind 全集新增）：`input.RECEIVE`（MOVE 限频 1/8，ordinal 计全部）、`input.MAP`（x=128u/W, y=256(1−v/H)）、`input.START`（reason=ACCEPTED 等）、`input.STOP`（UP/CANCEL/GEOMETRY_OR_GATE/INVALID_PRIMARY_POINT/…）；其余沿 T74：`tick`/`sensor`/`dry.request`/`dry.receipt`/`wet.receipt`/`present`/60 帧摘要。
- **渲染**：`P013SceneController.drawNewPipelineFrame`（T56 scene compositor）替代旧 `submitDualFieldSurfacePx`；`present` 诊断字段变为 `code/requestedSwapSucceeded/currentFrameSucceeded/presentationState(STALE)/firstFailureStage`；状态文本三态：正常/`STALE <token>；<stage>`/`新管线故障：<failure>`。
- **模型**：`start` 域校验（x∈[0,128) y∈[0,256]）→ ACCEPTED；tick 内 `touching` 时 dry.request（8 unit/s）→ injectDryGestureSand → receipt → 非 NO_OP 拒绝即 stop。

## 2. 场景矩阵

| 臂 | 目的 | 时序 |
|---|---|---|
| R0 | 冷启零水基线：干场 W/H/L=0、诊断活、输入门 ready | 冷启 `emptysanddemo 1` → 12s 无触摸 → dump+截图 |
| R1 | 单点按住发沙→落床→松手停止 | 冷启 → 5s → 同点按住 3s（uiInput swipe 同点 3000）→ 截图 → 松手 8s（落床+停止）→ 截图+dump |
| R2 | 拖动注沙（多点落床分布） | 冷启 → 5s → swipe 中心慢拖（400,1200→850,1200, 2000ms）→ 截图 → 8s → 截图+dump |
| R3 | 停止边界专项：松手后源归零 | R2 完成后不重启：再按住 2s → 松手 → 连拍 3 张（0s/1s/3s）→ dump；判据：松手后 `dry.request` 不再新增、metrics 的 particleMass 增量停止（只减不增） |
| R4 | 手持重力方向 | **拔 USB 手持**：冷启 → 手持倾斜 ~30°（左低右高）按住发沙 5s → 观察/拍摄沙流向 → 回正 8s → 拍照；方向正确=1/错误=0（主观锚点，不做强度评分） |
| R5 | 多指与 Cancel 边界 | 冷启 → 双指同触（uitest 不支持双指则记录 SKIP 并披露）→ 拖动中 Home 键（后台不续发）→ 返回 → 新 Down 才续沙 → dump |

R4 证据链：手动拍照（相机）+ 拔 USB 前后 hilog 补采；若无法拍照则以 R1–R3 客观证据为主，R4 记 PARTIAL（不阻断主判）。

## 3. 判据（L1–L7 升级为 PASS 判定 + 主观锚点）

| 级 | PASS 条件 | FAIL 分流 |
|---|---|---|
| L1 RECEIVE | `input.RECEIVE` 出现且 DOWN/UP 与动作对应、geometryReady=true、widthVp/heightVp 为实测值 | T71 输入节点无效 → 交 T69/T71 |
| L2 MAP/START | `input.MAP` 坐标 (W/2,H/2)→(64,128)（±1）；`input.START` reason=ACCEPTED；一次 gesture 一次 start | 坐标/门控错 → T71 |
| L3 CLOCK | 按住期间 tick 连续；stale 仅在真实后台/生命周期出现（R5 专项） | — |
| L4 REQUEST | `dry.request` requested>0、速率 ≈8 unit/s（±20% 抽样核算） | 时钟/漏接 → T71 |
| L5 RECEIPT | `dry.receipt` ACCEPTED 且 admitted>0；rejectedReason 如实分类 | 拒因原样记录 → 按拒因分流（CAPACITY→物理域；DRY 门→T59/T71） |
| L6 PUBLICATION | 60 帧摘要 `metrics.particleMass>0` 或 `dry` 池计数>0；sourceEpoch 增长=准入笔数 | 发布链 → T56 |
| L7 RENDER | **截图像素**：容器区可见沙粒/沙堆（非背景像素显著>0 且分布与注入点相关）+ present code=0 无 FAULT/STALE 常态 | 画面无沙/STALE/FAULT → T56/T88（不得以 swap 代通过） |
| 主观 | 硬门槛：闪退/黑屏/持续闪烁/粒子瞬移=0；分档：发沙响应性、落床连续性、松手停干脆度 0–10（及格 6） | 硬门槛触发 → FAIL 报告 |

停止边界（R3）：松手后 `input.STOP reason=UP` → `model.stop` → dry.request 停止新增；已有粒子继续落床（合法）。后台（R5）：PAGE_INACTIVE → invalidate，返回后不追赶、须新 Down。

## 4. 工具

- `tools/t82_touch_fix_retest.ps1`（六臂，R4 手动环节内嵌提示）+ `tools/analyze_t82.py`（解析 T71 新 kind + T56 present 三态 + 截图像素统计自动判 L7 + L1–L6 verdict）。
- 产物目录 `test/artifacts/t82-touch-fix-retest-20261026/`。
- 身份披露：commit `114a7ee`、树指纹 `403b08514017b326`、HAP SHA256（构建后填）、设备 ID。

## 5. 交付

- `docs/effect-report-P051-T82-touch-fix-retest-v1.md`（追加模式）：L1–L7 表、停止边界/后台/多指专项结果、手持方向 0/1、主观分档、与 T74（L1 FAIL）对比。
- 硬门槛触发或 L1–L5 FAIL → 如实 FAIL + 分流；全 PASS → 交 manager 登记（含"不以 swap 代通过"的像素证据）。
- 完成评论"已完成"；清单行状态交 manager 登记。
