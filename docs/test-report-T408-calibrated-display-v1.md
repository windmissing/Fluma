# T408 · 单沙独立标定与冻结映射真机配对 — 标定 native 读回数据完整；截图 hold 覆盖 4 轮未达成，按 README 规则拒绝进入 calibrate（不容差凑 PASS），移交 architect

- 卡面：T406 签包，先独立诊断 shader 标定（capture 全程位于 hold），冻结唯一 calibrationId → plan → 静态/动态配对判读。
- 包：T406-complete-signed.hap SHA256 `bb7b0558f4118c57896ca58024829034cf981b196e89dfeba47a0397d1f57cc9`（本地核验一致）。
- 路由：`p051v2=1 --ps singleSand 1`。证据：`test/reports/t408/`（51 文件，SHA 见 t408_verdicts.json）。

## 已完成（标定前置证据完整）

1. **独立几何采集**：页面 `T406_GEOMETRY_V1` 显式输出浮点全局 box 与 vp2px：globalX=14.628574916294642、globalY=627.7142857142857、widthVp=336.45713588169644、heightVp=129.42857142857142、vp2px=3.5 → native 1178×453 与浮点几何换算一致（336.457×3.5=1177.6、129.429×3.5=453.0）。
2. **独立标定图 native 读回完整**（manifest `t394-2-1-30778.json`）：phase.name=CALIBRATED_ROI、captureStatus=CAPTURED、presentationState=CURRENT、三 ROI `(8,8,32,32)/(565,210,48,48)/(1100,400,48,48)` 分布左下/中间/右上、双目标 RGBA 各 22528B=预期、`calibrationShaderSha256=a851733d…f021454` 显式输出、manifest sample token == hold token 逐位一致。
3. **capture.py 原始调用记录**：`T406_CAPTURE_CALL_V1` 收据（startMonoNs/endMonoNs/stdout/exitCode）齐备，未手填任何时刻。
4. **独立时钟同步**：/proc/uptime ↔ Python monotonic 三组同步对，观测 rate≈0.9943（作为 uncertainty 记录）；两种映射法（单点 offset / 线性 rate）结论一致。

## 未达成：截图 hold 覆盖（4 轮如实）

| 轮 | 结果 |
|---|---|
| 1 | 封存失败（无 hold 文件）；截图为裸 uitest（无 capture.py） |
| 2 | hold-2 生成，但 capture.py 映射后在 hold 封存端外 ~206s（截图执行于封存点击之后） |
| 3 | probe 未 CAPTURED（第二 evidence 未 ack 占用 probeBusy）；hold-3 当时不存在 |
| 4 | hold-3 生成但属第三轮 probe 生命周期；第四轮截图映射在 hold-3 窗外（封存后） |

每轮的 capture 收据（calib2/3/4-capture.json）与 hold 文件均原样保留；映射计算过程可复算。**按 T406 README 规则 3（整个截图调用必须位于 hold 中）与规则 6（calibrate 必须拒绝），未运行 calibrate**——不做容差拟合、不用手填时刻替代、不冒 PASS。

## 未行使（被上游阻断）

calibrationId 冻结、planId 冻结、静态 single、动态 pair V2 判读、正反控（1px/1码/缺列/过期身份/越 hold）。T396 历史 NOT_JUDGABLE 未改写。

## 移交 architect（需开发输入）

截图 hold 覆盖是设备端 CAPTURED→封存点击之间的时序约束：手动 uitest 节奏下 capture.py（PowerShell 启动约 2-5s）无法稳定落在 hold 窗内（第二轮 hold span 87s 仍因操作顺序错位失败）。需要 developer 明确以下之一后 tester 可继续：
1. 页面侧 hold 延长/冻结期扩展（如封存前保持 hold 开放固定时长并在 UI 显示剩余窗口）；
2. 或自动封存时序（CAPTURED 后页面自动以固定延迟封存，tester 只需在该延迟内截图）；
3. 或确认正确的手动操作节拍（当前"预约→等 CAPTURED→截图→封存"顺序在第四轮仍错位）。

## 边界

未改业务；未改半径/shader/阈值；不逐 ROI/臂找平移；capture 全部走 capture.py（后三轮）且收据齐备；时钟映射未从被测图拟合。

---

## attempt7（2026-11-14，T469-release cab50816… 新整数内框版）

来源：唯一签包 `T469-release-signed.hap` SHA256 `cab5081663aab4626b8b3dad9cbd203e724f1d2224c05db1ed744ac9180eacdf`（本地逐位一致）；旧 T455 `7b13fb3e…` 未使用。按 `test/t469/README.md` 执行。证据：`test/reports/t491/t408/`（11 截图 + prepare 档 + t413-status + 布局 dump）。

### 执行链

1. 冷启 PID 46765 → 单沙页 `单沙显式冻结截图 V2 · CONFIGURING · IDLE` 在位。
2. 确认固定键盘 → 收键盘并准备（session `s1794657849033-1`）→ 3 秒超时 **FAILED**。
3. 多轮重试（含释放→预约→归档→时钟→状态完整链）均复现；无 PNG/calibrationId 产物（find 为空）。

### 阻断根因（独立复算，交 architect）

prepare 档 `readinessReason = PREPARE_TIMEOUT:RASTER_OUTSIDE_ALLOCATION`（prepareId 1，observationSerial 11）。用 FAILED 档原始数据独立复算 `rasterNodeReason`（`test/t469/RasterGeometry.ets:59`）：

| 域 | x | y | width | height |
|---|---|---|---|---|
| actual/inner/touch（整数内框） | **51** | 990 | 1177 | 1716 |
| allocation.parent（浮点） | **51.20001220703125** | 990 | 1177.5999755859375 | 1716 |
| window | 0 | 0 | 1280 | 2832 |

`r.x(51) < parent.x(51.2)` → **RASTER_OUTSIDE_ALLOCATION**。设备 vp2px=3.5，父组件浮点 x=51.2；T469 整数内框计划 floor 了宽高但组件实际渲染 x=51 未与浮点父框对齐 → `rasterPredicate` 恒非空 → prepare 永不 READY。

下游连锁：`Button('预约').enabled(prepareGate.state==='READY')`（P051DerivedV2Demo.ets:517）→ 预约 disabled（t413-status.json 权威核：readSerial 3、state=IDLE、holdId=''）→ **HOLD_OPEN/PNG/capture.py 时钟链/配对/plan 冻结全部未达**。

### 修复方向建议（交 architect 裁定，本卡不改）

`rasterNodeReason` 对浮点父框允许 ≤1px 取整容差（`r.x < parent.x - epsilon`），或整数内框计划同步取整组件 x/y。按卡面"不逐 ROI 找平、不改半径/shader/阈值"约束不在 tester 侧改。

### NOT_MEASURED（attempt7）

PNG/ROI、跨域时钟映射与 hold 误差区间、中性厚床/非零 gains/CONTROL A-B/LOCK A-B、V2 single/pair 判读、1px/1码/缺列/过期身份/越 hold 正反控——全部因 prepare 不 READY 未达（非跳过）。attempt1-4 的 hold 时序约束移交项保持有效。

---

## attempt8（2026-11-16，T516-release 6f49d06d… 四边向内修复版）

来源：`T516-release-signed.hap` SHA256 `6f49d06da5cf4a019d96c3f55a694d6a606fc0730e538406c58e142e68ec1c22`（本地逐位一致）。证据：`test/reports/t491/t408a8/`。

### 执行链

1. `bm clean` 后冷启 PID 4695 → 单沙页在位；确认固定键盘 → 收键盘并准备（session `s1794806214122-1`）→ FAILED。
2. prepare 档（T509_PREPARE_V2）原始域：

| 域 | 值 |
|---|---|
| desiredWindowRect / desiredScreenRect | **(52, 990, 1176, 1716)** —— 与 T525 oracle 独立复算逐字一致 |
| actual / inner / touch | x=51.79998779296875, y=990, **w=1176**, h=1716（三域一致，旧 OUTSIDE 问题已修） |
| localPx | (0.79998779296875, 0, 1176, 1716) |
| screen | x=51.79998779296875（≠desired 52） |

### 新拒因（T516 严格门生效）

`readinessReason = PREPARE_TIMEOUT:RASTER_PLACEMENT_MISMATCH`——**不再是 OUTSIDE_ALLOCATION**：
attempt7 的外框外问题（51<51.2）已由 T516 四边向内分配修复，actual/inner/touch 三域宽度 1176
一致且在 allocation 内。新门按 T509 v2"实际三框逐值匹配 desired"判定：**actual.x=51.79998779
≠ desired.x=52，差 0.2px**——组件实际渲染位置与 ceil 计划位不符（51.2+localPx0.8=52.0 的
期望被平台渲染定位偏移 0.2px 打破）。**修复后的严格门正确拒绝**（不静默放行）。

### 下游连锁与移交

prepare 不 READY → 预约 disabled → HOLD/PNG/时钟链/配对全部未达（同 attempt7 结构）。
按 T509 v2 合同"若平台位置不匹配，保原值回 architect，不增加容差"：**0.2px 平台渲染位差
交 architect 裁定**（候选：窗口原点非零偏移、rtl/平移域、或 desired 基准应取 screen 而非 window）。
原始三域数据完整在 FAILED 档可复算。

### NOT_MEASURED（attempt8）

PNG/ROI、跨域时钟映射、中性厚床/非零 gains/CONTROL A-B/LOCK A-B、V2 判读、正反控——
全部因 prepare 不 READY 未达（非跳过）。
