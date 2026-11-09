# T408 · 单沙标定与冻结映射真机配对 · attempt2 报告（准备阶段新缺口，交 architect）

2026-11-09，tester，真机 attempt2（T428 签包解锁续办）。本 attempt 完成 T426 §6.1 preflight 与 §6.2 准备负控的真机执行，发现新的真机级缺口：**FrameCallback onIdle 从未到达 prepareIdle，两帧稳定观察无法达成，prepare 全部 3000ms 超时失败**。标定与配对未开始（预约被 READY 门正确拦住）。

## 已完成部分（全部真实采证）

| 步骤 | 结果 |
|---|---|
| 签包核验 | `T428-finalized-signed.hap` SHA256 `eb6ae00b...ad26a35` 本地逐位一致，安装成功 |
| preflight（T426 §6.1） | 主窗口、全屏手机确认；点击「确认全屏手机固定键盘」（具名 preflight 配置来源），状态行此后显示 `CONFIGURING`；截图 `preflight-confirmed.jpeg` |
| 真实编辑观察 | 点击 ROI 输入框触发焦点/编辑，Back 收键盘；冷档 `inputs[0].everEditing=true` 且 `keyboardAreaSeen=true`——真实编辑历史接线工作正常 |
| prepare 执行（第 1 次） | `PREPARING` 正确切换动作区（t413-reserve/export/clock/status/seal/cancel/release/configure 控件出现），3000ms 后 `FAILED PREPARE_TIMEOUT:WAIT_OBSERVATIONS` |
| prepare 执行（第 2 次，渲染诱导后） | swipe 制造渲染需求后重新 prepare，仍 `FAILED PREPARE_TIMEOUT:WAIT_OBSERVATIONS`，复现确定 |
| 冷档与 SHA | 每状态独立落盘 `t426-prepare-<session>-<gen>-<id>-<state>-<serial>.json`，WS_EVENT 带 SHA 收据（hilog 18:09:13.162/16.203 两条原件在档） |

## 根因定位（冷档逐谓词核对）

FAILED 冷档快照显示 **T426 §4 READY 谓词的全部其它条件均已满足**：

```
windowActive=True windowRegistered=True windowId=680
keyboardConfiguration=TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD deviceType=phone windowMode=1
avoidUnit=WINDOW_PX avoid=四边全零 keyboardHeight=0 keyboardAnimation=False
fullyVisible=True componentPresent=True transformIdentity=True
component=[51.2,990,1177.6,1716] 窗口内完整、viewport 1178×1716 与浮点 box 一致
nativeReady=True nativeGeometry=3==geometry=3 vp2px=3.5 session/controller/surface/context/run 全有效
inputs 三项 created=true editing=false focused=false（everEditing=true 且 keyboardAreaSeen=true）
```

唯一未满足：**两帧稳定观察**。冷档 events 仅含 `BEGIN` + `FAILED`，**FRAME trace 0 条**（两次 attempt 的 PREPARING/FAILED 冷档一致）；`observationSerial` 22→26 的增量全部来自窗口监听（avoidAreaChange 等），`prepareIdle` 在真机上从未执行。

## 根因分析

`T426PrepareFrame extends FrameCallback`（SDK `@ohos.arkui.UIContext`：`onFrame` 在下一帧渲染时调用；`onIdle` 在渲染完成后距下一 VSync **>1ms** 才调用，否则**延迟执行**）。真机上页面静止时段 VSync 稀疏（hilog Vsync W 级日志约每 2 分钟一条），且即使 swipe 诱导渲染，`onIdle` 仍一次未达——`prepareIdle` 永不执行 → 两帧稳定计数永不建立 → `WAIT_OBSERVATIONS` 直到 3000ms 看门狗超时。

对照：同页 `DerivedFrameV2`（主渲染循环）只用 `onFrame`，在真机长期工作。差异就在 `onIdle` 的延迟语义——合成 harness（ArkUI/窗口桩）无法暴露此差异，T428 开发 16/0 的帧桩按“注册即回调”语义执行。

## 未完成项

1. T426 §6.2 负控（PREPARING hide/show、READY 后窗口/焦点变化、OPEN 中遮挡）——被 READY 不可达阻断，仅语义由 T432 合成差分覆盖
2. 标定（中性厚床/非零 gains/CONTROL A/B/LOCK A/B）
3. V2 single/pair 配对判读
4. attempt1 历史 hilog 补证（原件已丢失，如实保留缺口标记）

## 证据

- `test/reports/t408/attempt2/`：4 份 prepare 冷档（2×PREPARING + 2×FAILED）、5 份布局、3 张截图、SHA 收据 hilog 原件行
- 首错/复现：点击 t413-prepare → 状态行 `FAILED PREPARE_TIMEOUT:WAIT_OBSERVATIONS`，4 次确定性（含渲染诱导）

## 已尝试

① 直接 prepare ② 点击画布/输入框触发焦点与渲染后 prepare ③ swipe 大幅诱导渲染后立即 prepare ④ 回配置区重新 prepare——全部相同结果。

## 所需前置（交 architect 裁决，接 developer）

1. **(a)** developer 修订 `T426PrepareFrame`：观察点改为 `onFrame`（同 `DerivedFrameV2` 先例，真机已验证可达）或 `postFrameCallback`+短延时 `postDelayedFrameCallback` 兜底，重签包后新 attempt；
2. **(b)** 或裁定 `onIdle` 在目标设备上的可达性前提（如强制页面持续渲染需求），并入 T426 合同修订。
3. attempt1 历史 hilog 缺口保持原样（已丢失不补造）。

本卡保持“进行中→看评论”，不冒通过；预约/hold/时钟/颜色工具链已全部就绪（T413/T414/T415/T418/T427/T432），通道解锁即可续标定。
