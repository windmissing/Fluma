# T474 · 单沙整数光栅计划独立差分（v1）

- 日期：2026-11-11
- 角色：tester（离线卡，不使用真机）
- 结论：**PASS 14/14**。typescript 转译 T469 实际 `RasterGeometry.ets` 驱动（`test/t469/RasterGeometry.ets`，与生成页同源），判据从 T463 floor 分配合同独立推导，不复用开发 31 项断言。
- 代码：`test/t474/raster_diff.cjs`
- 证据：`test/reports/t474/t474-raster-diff.json`
- 结果供 T408（不卡设备）。

## 覆盖范围（卡面要求 → 14 项）

| 项 | 覆盖 |
| --- | --- |
| G1 n±ε / 半整数 | floor 分配：1178−1ulp→1177、1178→1178、1178+ε→1178、1177.5→1177（n−ε 在 double 下落到下界，floor 语义=floor(实际 double 值)） |
| G2 小于 1 / NaN | width 0.5→RASTER_ALLOCATION_EMPTY；NaN→RASTER_ALLOCATION_INVALID |
| G3 非整数 scale | vp2px=3.5 父框 1177.6×1609 → 计划 1177×1609（floor 截断） |
| G4 外位置动 floor 不变 | parent.x 0→2：floor 尺寸不变但 generation 推进（旧 permit 失效）；同输入不重建 |
| G5 旧 viewport±1 / 迟 layout 旧锚 | geometry 失配→STALE_PLAN；旧 layout→WAIT_NEW_LAYOUT；native viewport 1177≠计划 1178→NATIVE_MISMATCH（acknowledge 后 native 门可达） |
| G6 READY 间变化 | 1178→1178.9 同 floor 但 generation 推进（open hold 失效、首因保持） |
| G7 OPEN 首因 | 未分配计划 predicate = RASTER_NOT_ALLOCATED |
| G8 两帧观察 / 位置域 | 首帧 changed=true、同值二帧 false；screen=actual+window 原点一致全门通过；screen 错位→RASTER_POSITION_DOMAIN |

## 语义要点（独立推导确认）

1. **谓词门顺序**：revision/valid → layoutAcknowledged → geometry → 组件尺寸 → 越界 → 位置域 → 层一致 → native 一致。observe 变更后需 acknowledge 新 layout，深层谓词（native/位置域门）才可达——G5C/G8C 初版误判为差分器未 acknowledge 所致，非产品缺陷。
2. **snapshot(geometry) 是生产消费路径**：controller.plan.geometry 字段本身恒 0，由 snapshot(geometry) 覆写副本后供 predicate 消费——native.geometry 需与 snapshot 覆写值一致。
3. **screen 域恒等式**：screen=(actual 坐标+window 原点)；位置域破坏（错位 99,99）被 RASTER_POSITION_DOMAIN 正确拒绝。

## 缺口如实

- 真机三域（连续外框/实际 XComponent 框/EGL 整数）与 packet 一致性归 T408（T469 README 指定路径），本卡离线夹具不覆盖 ArkUI 回调与原生状态。
- 复用 T458/T432 已证范围（帧观察/两帧稳定），未重跑无关矩阵。
