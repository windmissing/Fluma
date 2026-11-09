# T408 · 单沙独立标定与冻结映射真机配对（attempt1 阻断报告）

2026-11-09，tester。正式解锁按 test/t413/README.md 新 attempt（T414 最终签包 SHA256 7850376c9ceec002e72008b79b77ddc785bf0ab1d4ccafc71a7e9b2f2e55f61f 已核并安装成功）。本 attempt 未完成采集，阻断于页面"收键盘并准备"动作，交 architect 裁决。离线前置 T419（commit 4db40a2）与 T418（commit 5113266）已交付且无语义分歧。

## 1. 阻断现象

- 页面"收键盘并准备"（`t413-prepare`）点击后 `probeStatus` 恒定 `KEYBOARD_HIDE_FAILED`，`configuring` 不切换，动作区控件（`t413-reserve/status/export/clock/seal/cancel/release/configure`）从未出现——l0–l10 十次 `dumpLayout` 一致。
- 预约无法建立，本 attempt 无法进入 T413 §4.2 流程。四次尝试（直接连点×3、Back 键、点画布失焦、主动唤起键盘后收起）全部确定性复现。

## 2. 根因定位（hilog 逐帧核对，pid 6345）

`inputMethod.getController().hideTextInput()`（page.inc prepare 第 38 行，await）在本设备 API 26 上返回的 Promise 以 `ImsaKit: HideTextInput,not bound!` + `OnComplete,failed [runStatus:9, errorCode:55]` 终止。时序：

1. `clearFocus()` 成功（AceFocus: user lost focus → Column）；
2. 失焦已触发 `PageChange CloseKeyboard, sessionId=1`，键盘实际收起（AceKeyboard `IME shown, hide kb`、WMSKeyboard `safeHeight: 0`、输入法侧 `onKeyboardHide` 三方确认；s4→s5 截图对照：键盘从弹出遮挡到完全收起）；
3. `hideTextInput()` 因输入会话已解绑（not bound）回传错误码 55，Promise reject → catch → `KEYBOARD_HIDE_FAILED`。

即：键盘收起由 clearFocus 路径真实完成，hideTextInput 是其后冗余调用，其 rejection 是"无会话可隐藏"被 API 当错误回传，并非键盘未收起。无 JS 崩溃（faultlog 空），页面其余功能正常。

## 3. 为何不能自行继续

- page.inc `reserve()` 前置要求"独立 configure/close-keyboard action 已完成"（`if(this.configuring||...) CONFIGURATION_OR_ARCHIVE_BUSY`），`configuring` 只在 prepare 的 try 成功分支置 false。
- tester 红线不改业务代码；uitest 无法直接执行 ArkTS `clearFocus`/状态切换。
- 几何未漂移佐证（供裁决参考）：键盘收起后 XComponent bounds `[51,990][1229,2706]` 与冷启 l0 完全一致，重启后亦一致。

## 4. 证据

`test/reports/t408/attempt1/`：l0/l1/l5/l7/l8 布局 JSON + s4（键盘弹出遮挡）与 s5（键盘收起后仍 KEYBOARD_HIDE_FAILED）截图。hilog 关键序列 13:31:16.742–751（clearFocus→CloseKeyboard→hide→not bound→errorCode:55）。

## 5. 所需前置 / 接手角色（交 architect 裁定）

- (a) developer 修订 page.inc prepare：hideTextInput 容错（catch errorCode:55，或调整 hide/clearFocus 次序），重签包后按 README 新 attempt。
- (b) 或确认"点击画布失焦+键盘实际收起"为 prepare 等效完成（几何身份已核对一致），放宽 configuring 切换条件——属页面合同变更，需 architect 决定并同步 T413 文档。

T408 原卡保持"进行中/看评论"，不冒通过、不登记未执行项为完成。
