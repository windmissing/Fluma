# T408 · attempt5：T455 修订真机首验（观察泵全绿；新发现 0.4px viewport 严格不等谓词）

2026-11-10，tester。设备 62T0225B10010020，T455-release 签包 SHA256 7b13fb3e4fecca3867e16d484539f7ea6f659e02549d5c42c5755c3ca2725d33（与卡面一致）。

## 已证（attempt2 FRAME trace=0 阻断已解除）

1. **T449_FRAME_OBSERVATION_V2 真机冷档已取**（两份）：
   - `t455-prepare-s…-1-1-1-FAILED-9.json`（未编辑路径）：POST 31 / FRAME 30 / LAYOUT 30 / DRAW 30 / SAMPLE 30 / STALE_DROP 0，FAILED `PREPARE_TIMEOUT:KEYBOARD_VISIBILITY_UNKNOWN`（未编辑时正确拒）。
   - `t455-prepare-s…-1-1-2-FAILED-29.json`（完整编辑路径）：键盘出现→"已确认固定键盘"→收键盘并准备；POST 31 / FRAME 30 / LAYOUT 30 / DRAW 30 / SAMPLE 30 / STALE_DROP 0；`keyboardAreaSeen=true`、`sessionEverEditing=true`、`layoutGeneration=2`（键盘避让布局代变化被正确追踪）、`windowId=685`、avoid 全零、`keyboardConfiguration=TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD`。
   - **两帧稳定观察在真机按合同工作**：onFrame 严格递增、didLayout/willDraw 逐帧、零丢弃。
2. 操作路径真机可行：输入框点击→键盘弹出→确认固定键盘→收键盘并准备，全程 uitest 可驱动（T408 attempt1 的 KEYBOARD_HIDE_FAILED 域未复现）。

## 新发现（交 architect 裁决，非冒 PASS）

第二次准备 FAILED `PREPARE_TIMEOUT:PREPARE_NATIVE_NOT_READY`，根因定位到谓词最后一项严格比较：

- `PrepareReadiness.ets:130`：`s.viewportWidth!==r.width`（严格 `!==`）；
- 真机 vp2px=3.5 时 `component.width=1177.5999755859375`（XComponent getRectangleById 浮点）而 `viewportWidth=1178`（native viewport 取整），差 0.4000244140625px；
- 其余子条件全绿：`nativeReady=true`、run/surface/context/controller/session 全在、`nativeGeometry===geometry=3`。

这是设备 vp 缩放取整不对称导致的结构性不可满足（与 P-011 hard-gate ±45° 类似的谓词域问题），属生产谓词设计问题：浮点几何 vs 整数 viewport 的等值判定需容差或同源化。

## 未完成（如实）

- hold/独立时钟/PNG/颜色配对未到：被上述谓词挡在 READY 之前。
- native GPU M1 host 独立链（T410/T437 同域）本页不依赖，但 CLOCK_GAP 设备域仍待 T451 分流。

## 移交

architect 裁决 `viewportWidth!==component.width` 严格谓词（容差/同源/取整对齐三选一），修复或裁定后 tester 重跑 attempt6 续标定。
证据：`test/reports/t408/attempt5/`（2 冷档 JSON + 6 截图 + layout）。
