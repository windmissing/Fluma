# T510 · GPU 真实控件命中源链与首帧精确补证 tester 报告 v1

- 卡：`6af8605ce4b010f489353c04`（tester，P5，真机，设备串行 62T0225B10010020）
- 日期：2026-11-16
- 被测包：**T492-m1-delivery**（T505 §5 指定增强 delivery，非旧 handoff）
  - HAP `801c2d63ecd7a82d9e48bf92f0cbb6312a0c64074b477e527c0622150a9aa1b0`（本地逐位核验 MATCH，verifyApp=PASS）
  - 包内 libgrsm1.so `32947a028c8b88c2959c15c2ada1387337656200a820171b46c77f9c66973d2a`
  - execution_diagnostics.h = `4493f69416e8…`（含 firstQuantum/faultRoot/tail 增强，T505 §1 确认）
- 进程：run1 PID 54243（11:52 冷启）→ run2 PID 31918（12:19 冷启）
- 证据：`test/reports/t491/t510/`（9 截图 + 3 boot 档 + 2 execution 档 + timeline 分片 + 6 UI dump）

## 1. 每动作 UI 树实时 bounds 确认（T505 §5 要求）

每动作前 dumpLayout 取实时 bounds（禁止沿用 T495 猜测坐标）：

| 控件 | run1 bounds（1280×2832 px 域） |
|---|---|
| 导出启动诊断 | [416,164][864,304] → 中心 (640,234) |
| 沙/水/停止/新空场 | [178,651][346,791] / [374,651][542,791] / [570,651][794,791] / [822,651][1102,791] |
| XComponent | [0,1081][1280,2552] |
| 光学/诊断 toggle | [667,2566][1026,2706] |
| 导出原生时间线（面板展开） | [388,1865][892,2005] |
| SHA TextInput（面板展开） | [0,1249][1280,1389] |

**关键工程发现**：光学/诊断面板展开时 XComponent 从 UI 树消失（layoutWeight(1) 被面板挤压出可视区）——
T495 报告的"投沙(147,263)"类坐标在该域打在面板/标题上，与 T505 §3 判断一致。

## 2. 两次独立显式新空场（T505 §5.3）→ PASS

run1：4 次独立 click（两次双击间隔>1s，每 click 独立）→ 4 次独立 UI_RESET（boot-t510-b.json transitions）：

| # | epoch | native ns | 间隔 |
|---|---|---|---|
| 1 | 3→4 | 260764943760312 | （解锁恢复） |
| 2 | 4→5 | 260855143114687 | +90199.4ms |
| 3 | 5→6 | 260916684540156 | +61541.4ms |
| 4 | 6→7 | 260984193742968 | +67509.2ms |
| 5 | 7→8 | 260985717694062 | +1524.0ms |

- native RESET_NATIVE_REQUEST count=7（execution 档 stages），RESET_HANDLER_ENTER/RESTART_ENTER/
  OLD_FRAME_CANCEL/NEW_RUN_ENTER/NEW_RUN_RETURN/NEW_FRAME_BIND 每组 4 条（actions 档 hex 原文解码）
- epoch 单调 2→8、native ns 严格递增、无同 hash 重复档（3 份 boot 档 SHA 各异 55135/324882/B）
- **重复同 hash 不能算两次的 T505 判据在此满足**：每 RESET 独立 ns+epoch 增量

run2 复核：UI_RESET 2→3、3→4（attempt3 session1 ff:UI_RESET → attempt4 session2 READY）。

## 3. 导出链确认命中（T505 §5"export 按钮与 boot 导出不同"）→ PASS

- **导出启动诊断**（bootDiagnostics）：3 份 boot 档，t456-boot-*.json，WS_EVENT 带 bytes/sha256
- **导出原生时间线**（GpuTimelineSnapshotCold.collectSnapshot）：首次 PACKAGE_SHA_REQUIRED 拒绝
  （manifest state=INCOMPLETE reason=Error: PACKAGE_SHA_REQUIRED）——按页面设计先填 SHA TextInput
  （uitest inputText 填 `801c2d63…`，dumpLayout 确认填入）→ 二次导出成功：
  **4096 行 / missing=0 / duplicates=0 / firstSerial=1 lastSerial=4096 / 32 分片 / 1569040B**
  （identity: module=libgrsm1.so, route=GpuResidentM1, packageSha256=801c2d63…, pid=54243）
- **execution 快照**（diagnostic492 Capture）：4 份，含 stages(83)/groups(4)/actions(32)/packets 域

## 4. 源链（T505 §5.2）→ NOT_MEASURED（GPU 状态域结构性封锁，非控件问题）

两 run 多轮画布内触摸（uitest swipe 同点/长按/drag、面板收起后（s7 确认画布可见）再 3 次 pointerDown）：

- actions 档（96 槽环形）仅有 4×SOURCE_SELECT + 4 组 RESET 链；**POINTER_DOWN_READY_COORDINATES 零条、
  UPLOAD_32U32 零条、packets 空且 packetDropped=0**（DiagnosticOfferLocked 从未被调）
- 画布触摸期 GPU 状态：run1 s8 `V2_PULSE_CLOCK_GAP`（物理 0:4564 冻结）；run2 r2s2
  `GPU_WORK_PENDING`（物理 0:81→0:559 推进）——两态均为 **STALE 前缀（state≠CURRENT）**
- `GpuM1DemoController.down()` 判据：`s===null||!this.allowed(s) → reason='NOT_READY'` 拒绝；
  `tickSnapshot` 仅在 `this.allowed(s)&&s.sourceOpen&&s.sourceKind===kind` 才构造 32U32 offer
- **结论：GPU 超期域（T448/T490/T495/T505 链）不进入 CURRENT → 源链被状态域结构性封锁**，
  与 T410/T437/T461 同根。非控件命中问题（画布触摸可达——r2 物理 0:81→0:559 期间触摸响应）。
- 按卡面"实际控制失败按最小切点回 architect，不重派新增日志"：切点=首 root 250ms 超期域
  （T505 §2 检测界），等待呈现域修复后复测。

## 5. 首帧/呈现精确证据（T505 §5.1/5.4，execution 档 stages）

| stage | count | first ns |
|---|---|---|
| ROOT_INTENT | 9218 | 257397081427812 |
| PROGRESS_TIMEOUT | **2** | 257397332286562（首root 界，与 T495 250.37ms 同域） |
| FIRST_REVOKE | 2 | 257397332300312 |
| ROOT_ACCEPT | 4603 | 260765482189062 |
| ACCEPTED | 4603 | 260765482217187 |
| DRAW_ENTER | 4603 | 260765482274218 |
| SCENE_FBO_ENTER/EXIT | 4603/4603 | — |
| SCENE_LAYER | 36824 | — |
| **EGL_SWAP_ENTER** | **4603** | 260765510836093 |
| **EGL_SWAP_RETURN** | **4603** | 260765521189375（Swap 返回 103.5ms 后） |
| POST_DRAW_PERMIT_EXIT | 4603 | 260765521421562 |
| DRAW_EXIT | 4603 | — |

**关键结论（推翻 T505 §4 的"present 独因"保留假设）**：`EGL_SWAP_RETURN` 4603 次与 ENTER 相等——
**eglSwapBuffers 真实调用并返回**（首次 rawEgl 域待逐条核，但返回路径计数完整）。ACCEPT→DRAW→FBO→
Layer→Swap→PostPermit 全链 4603 次闭环。黑屏不在 GL present 调用层，在显示管线更下游
（合成器/layer 送显/屏幕域）。root1 INTENT.kind=2 与 T505 §2 一致。

## 6. NOT_MEASURED / 未完项

- 32U32 packet→Offer→SOURCE receipt→后继 Q 闭环：**NOT_MEASURED**（CURRENT 域未达成，
  down() 结构性拒绝；等待呈现域修复，非本卡可自行继续——前置=architect 裁定 GPU 超期域修复）
- 系统 PNG 分项、newPhysicalFrames 逐帧、native presented token 账：本次导出探针在位
  （evidenceCapture 域），CURRENT 域达成后同窗采集
- 短视频/系统动作 trace：本轮以 UI dump+截图+诊断档替代（录屏工具链 GAP 窗口冲突，
  T495 已记录）；不影响本卡判定域
- faultRoot/firstQuantum 尾部：execution 档 groups 含 firstQuantum/faultRoot 字段（helper 增强
  已核），首错 PROGRESS_TIMEOUT 2 次的 tail 域在 execution-t510.json groups[0].tail

## 7. 判定

- RESET/导出链：**PASS**（T505 §5.3/导出按钮区分全证）
- 呈现链：**PASS 到 Swap 层**（EGL_SWAP_RETURN 闭环计数——新事实，修正 T505 §4 保留假设）
- 源链：**NOT_MEASURED**（GPU 状态域结构性封锁，最小切点=首 root 250ms 超期域，回 architect）
- 无产品 FAIL 判定：全部阻断在已归因的 GPU 超期域（T448/T490 链），控件/注入/导出链自身健康
