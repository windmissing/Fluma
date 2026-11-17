# T510 · GPU 真实控件命中源链与首帧精确补证 v2（T537 签包域）

日期：2026-11-17，tester，真机（011124264R000337），设备串行。
被测：T537-m1-final-signed.hap（签包收据 test/reports/t257/T537-m1-final.json）。
依据：T530 纠偏（544d263：实际 allowed 无 CURRENT 谓词；没有 Down 原件不能认 NOT_READY 拒；Swap 非系统像素）+ T537 §2/3。
证据：`test/reports/t491/t510v2/`。

## 本轮新证（全部 T537 增强域，不重跑已证 reset/export）

### 1. 三域同帧诊断（T537 §2 三域证据全取）

- 诊断三色图 hold 帧**出现在系统截屏**（diag1.jpeg：红/绿/蓝三竖条实拍）——Swap 真实成功、合成器/窗口层正确呈现。
- native envelope 原件（T537_PIXEL_CAPTURE_ENVELOPE_V1，`t537-pixel-1794892701614.json`）：
  - `swapSucceeded=true`、rawGl=0、rawEgl=0
  - `fboRgba8` 三 ROI = (255,0,0)/(0,255,0)/(0,0,255)，`defaultRgba8` 送显前同值——FBO 与默认目标全对
  - geometryRaw：render/touch 节点 size 1256×1406、windowOffset 同 (0,164)、layout=1 同代——两节点原框一致
  - GL→屏幕坐标转换（bottom-left→top-left，+164 窗口偏移）：系统 PNG 抽样 (209,866)/(628,866)/(1046,866) = (254,0,0)/(0,255,1)/(0,0,254) —— 与 FBO 差 ±1（JPEG 有损）
- **三域一致：像素错域缩到"无"——present/FBO/合成器链正确**（与 T530 "不再称合成器"纠偏一致并实证）。
- 无损 PNG：设备 `snapshot_display` 仅支持 jpeg 输出（.png 恒拒）→ 无损 PNG 域 NOT_MEASURED，设施边界回 architect（capture.py 接口已就绪，仅缺无损源）。

### 2. Down→Offer→SOURCE 链（T537 §3 inputStages，T530 要求的一次保链）

导出原生时间线（executionDiagnosticsV1）原件 `execution-diag.json`，`inputStages:T537_INPUT_STAGE_V1`：
- Down（code1 PAGE_HANDLER）nativeNs=87568946795002，raw 含 type/count/rawEventTimestamp/local/window/screen/node=t537-touch/layout=1——**Down 原件首次在档**（T530 指出的缺失件）
- **Down→首个 NATIVE_OFFER（code7）延迟 = 18.634 ms**（T530 10.353ms 单位结论域一致）
- **5 次 Offer reply 全部 = `ENQUEUED_NOT_ACK`**——真实 native 接受回执，无 NOT_READY 拒绝（T530 纠偏"没有 Down 原件不能认 NOT_READY 拒"实证方向正确：非 CURRENT 结构封锁不存在）
- 128B LE 命令 raw128B_LE 在档（5 包完整原件）
- 源收据#0 0.0000（UI 行）——**SOURCE 尚未消费队列**：状态行 GPU_WORK_PENDING STALE（与 T477 上屏 STALE_PAGE_SESSION 家族一致、T530 "root1 成功/后 root 失败"一致）→ 源链断点缩到 **后继 root 调度/上屏域**，非输入链
- counts=[1,0,1,0,1,23,11,0,23]、dropped=31、retiredScopes=2（Move 突发耗尽普通尾、首 Down/谓词/回执被保护——按设计工作）

### 3. 冒 PASS 边界

- 诊断三色图通过 ≠ 真实沙水呈现/SOURCE 消费/性能通过（T537 文档原句）——本轮如实分列
- SYNTHETIC/模型输入不冒真实手势 ✓
- jpeg-only 差异 ±1 不作为像素级精确匹配证据（无损域 NOT_MEASURED）

## 判定

- **输入链（Down→Offer）本轮闭合**：Down 原件、18.634ms 延迟、ENQUEUED_NOT_ACK 回执全部在档——T530 要求的"一次保 Down→Offer→SOURCE"中 Down/Offer 两段已证。
- **SOURCE 消费与后继 Q**：NOT_REACHED（GPU_WORK_PENDING STALE 域）——与 T530 root1 成功/后 root 失败归因一致，后继 root 调度域归 T536/T530 已建卡工程线。
- **无损 PNG**：NOT_MEASURED（设备仅 jpeg）→ architect。
- 本卡核心遗留（源链完整到 SOURCE receipt→Q）依赖 T536 完整 8 族 stage 特化包，按 T530 移交不重派新增日志。

## 原件

diag1.jpeg（系统层诊断帧）、capture1/2.json（T406 包裹回执）、t537-pixel-envelope.json、
execution-diag.json（inputStages 全量）、boot-diag.json、l1..lI.json（全程 layout）、
crash-hilog.txt 等设备运行原件。
