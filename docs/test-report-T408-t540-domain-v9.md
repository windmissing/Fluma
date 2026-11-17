# T408 · 单沙独立标定与冻结映射真机配对 attempt9 v1（T540-final 包域）

日期：2026-11-17，tester，真机（011124264R000337）。
被测：T540-final-signed.hap SHA256 `550e537a…`（T532_WINDOW_HOST_RASTER_V3）。
依据：T532 裁定（0.2px 不归 checker/唯一根因；待新包严格位置→READY/HOLD→标定，原阈值/ROI 保持）。
证据：`test/reports/t491/t408v9/`。

## 核心达成：T532 §3 全 raw + 整数核（0.2px 域消解）

T540 整数 window-host 结构真机在位：
- `t532-window-root` (0,137,1256,1661)、`t532-window-host` 同框（脱离浮点 allocation 父框）
- `t463-raster` uitest 实测 bounds **(51,990)-(1205,2634)** 与 desired (51,990,1154,1644)
  **精确整数一致**
- raster JSON（T532_WINDOW_HOST_RASTER_V3 generation=9 geometry=11）：
  - hostOriginWindowPx=(0,136,1256,2526)、requestedLocalOnHost=(51,854,1154,1644)
    → screen y = 136+854 = 990 **整数**
  - nodeAncestry 5 项原件（root→host→raster→xcomponent/touch，parent 链、local/window/screen、
    4×4 恒等矩阵）全部在档，无 51.799987… 小数
- **attempt8 的 PLACEMENT_MISMATCH 0.2px（actual.x=51.8）在本轮 actual 读回中消失**
  ——T532 归因"实际定位差异来自浮点 allocation 父框"被 T540 修复路径真机验证

## FAILED PREPARE_TIMEOUT:KEYBOARD_VISIBILITY_UNKNOWN（预约/HOLD/标定域阻塞）

- `PrepareReadiness.ets:121`：`sessionEverEdited && !keyboardAreaSeen → UNKNOWN`
- keyboardAreaSeen=true 需要 PrepareSnapshot.avoid 含非零宽高矩形（键盘弹出时窗口 avoid 回调）
- 试过：预约→点输入框→uitest inputText→键盘开/收→prepare ×4 次组合——均 UNKNOWN
- 根因：**uitest inputText 注入不触发真实 IME**，avoid 回调从未到达（设施边界，非产品缺陷）
- attempt8 时该流程成功过（旧包无此新校验分支或当时 IME 真弹）——T540 新加严校验在
  uitest 注入环境下不可满足

## 判定

- **位置严格域：PASS**（三框全 raw 在档、整数精确、desired==actual 无 0.2px）——T532 工程合同验证完成
- **预约→READY/HOLD→PNG→独立新标定：NOT_REACHED**（prepare 键盘域阻塞，设施边界）
  → 按 T540 交接"tester 接新包先收 T532 §3 全 raw，严格三框/native 一致后再做 READY→预约→HOLD/PNG→独立新标定"
    ——三框域完成，后段因 IME 设施边界阻塞，回 architect 裁决取证路径（真人工触摸弹 IME，或 avoid 域独立验证方案）
- 旧 a8 不改记 PASS ✓（保持原状态）

## 原件

l1..lA.json（全程 layout，含 raster V3 JSON 原文）、raster JSON 解析（本轮记录）。
