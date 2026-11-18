# T408 attempt11 · T570 §4 窄补证（路径 A：无焦点 prepare，T540-final 包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T540-final-signed.hap（T570 归因消费的同一包）。
证据：`test/reports/t491/t408vB/`。

## T570 §1 判据直接命中：STALE = 真实 run + geometry 硬换代

路径 A（T570 §4 要求的对照路径：确认固定键盘→**无输入焦点**直接 prepare）：

| 原件 | run | geometry | page | obs.serial |
|---|---|---|---|---|
| prepA-21（PREPARING） | p011-run-…-5984525-**3** | **13** | 3 | — |
| prepA-28（INVALIDATED PREPARE_STALE） | p011-run-…-6069903-**15** | **16** | 3 | 21 |

- **run 换代 + geometry 13→16**：命中 `PrepareReadiness.observe` 的 HARD invalidate
  入口（`prior.nativeReady && run 改变 → invalidate(PREPARE_STALE)`）
- 与 T570 §1 对 prep-40/41 的裁定完全一致：**真实 owner 换代**，不是 blur 推 serial
  （本路径全程无输入焦点，inputSerial=7 无 blur 事件源）
- 时间线：两次 prepare 点击之间 renderer 重建了 run（5984525→6069903，相隔约 85s）——
  **每次 prepare 失败后的窗口状态变化（键盘收起链）触发 renderer run 重建**，
  使下一次 prepare 必然撞 hard invalidate

## 对 T570 归因的补强

1. prep-40/41 的"真实换代"裁定获得**独立同构证据**（不同 session、同判据命中）
2. kbCfg=TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD（确认按钮生效后保持）——
   排除"确认丢失"变量
3. STALE 链的真正驱动 = **prepare 失败→窗口链变化→run 重建→下次 prepare 撞 hard**
   的循环，而非 prepare 窗口内回调推 serial（attempt10 报告的"blur 推 serial"假设
   在路径 A（无焦点）下不成立，予以修正）

## 剩余域

- READY→reserve→HOLD→严格 PNG→标定：仍 NOT_REACHED（需 T574 v4 见证/换代码包——
  renderer run 重建触发器须等同 attempt 原件归因后修，T570 已明确不先泛化重启）
- T574（v4 窄见证）developer 未开始——首失效 witness 不可取，本轮以完整冷原件
  （prepA-21/28 + l1–l5 布局序列）替代

## 判定

- T570 §4 路径 A 窄补证完成：STALE 循环根因从"serial 推高"修正为"**真实 run/geometry
  硬换代循环**"——attempt10 报告相应结论按 T570 口径修正（原件留存）
- 回 architect：renderer run 重建触发器（prepare 失败后的窗口链）需 T574 见证域
  同 attempt 原件后归因
