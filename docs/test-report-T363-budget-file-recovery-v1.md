# T363 预算原文件定点补取（T352 私有文件已追回核验，无重跑）（真机只读，完成）

2026-11-06，tester。按 T361 归因需求**只读补取** T352 会话既有私有预算文件——**该文件已在 T360 会话（今日，commit c996388）从设备沙箱顺带拉回归档，本卡完成核验与 SHA 定档，未重跑任何场景**。

## 1. 文件身份与追回路径

- **文件**：`v2-budget-1793882460817.json`（设备侧生成于 2026-11-05 20:41，T352 会话"导出帧预算"按钮点击产物）。
- **追回**：T360 会话经 `hdc file recv` 从应用沙箱 `/data/app/el2/100/base/com.example.helloworld/haps/entry/files/` 拉回，归档为 `test/reports/t360/v2-budget-t352-era.json`。
- **SHA256 `f320e6a6bfd4f7f22d14a3ab572075cdc39b00f830f4ee0d7eae569c537f0ee3`**（58488 bytes）。

## 2. T352 权限原错勘误（如实）

- T352 会话判"诊断开销 NOT_MEASURABLE（预算导出无 hilog 可读输出）"——**判读修正**：导出设计即写私有沙箱文件非 hilog（T361 已指出），当时 hilog 无输出≠出口失效。
- 操作原错：T352 时未尝试沙箱拉取（`hdc file recv` 当时的 `/d/...` 路径前缀 PortableGit 污染问题遮蔽了其它通道）；T360 已确立 PowerShell/沙箱绝对路径拉取手法。
- **T352 报告 P5 判读由 NOT_MEASURABLE 修正为 MEASURABLE（文件在档）**——本报告即勘误件，T352 原报告版本保持不动。

## 3. 文件内容核验（128 帧覆盖）

| 项 | 值 |
|---|---|
| revision | `T351_EXCLUSIVE_FRAME_BUDGET_V1` |
| 覆盖 | **RECENT_128_FRAMES**（capacity 128，rows 128 全满）|
| 窗口 | frame 1369→1496，commit 2747→3001（T352 触摸注入+源停后段）|
| live | **101 恒定**（UP 后，与 T352 帧率坍塌段吻合）|
| wallMs | **p50=759ms / max=767ms** |
| **rawCallbackIntervalNs** | **p50=767.7ms / p95=769.0ms** |
| incomplete | true（overwritten 1368——长 run 环形覆盖，如实）|

## 4. 供 T361 归因消费的关键数据

1. **分母核验**：rawCallbackInterval p50=767.7ms 与 T352 hilog 差分（触摸后 p50 660.5ms/p95 768.5ms）同量级同窗——T352 的 fps 分母（墙钟差分）与设备真实 callback 间隔一致，**分母无伪**。
2. **主导阶段指向**：wallMs(759ms) ≈ callbackInterval(768ms)——**每帧 wall 几乎占满整个 callback 间隔（预算占用率 ~99%）**，即帧预算耗尽是帧率坍塌的直接原因；单帧内 exclusive 阶段分解在 rows 逐帧字段中（fillCopyBytes/consumerCopyBytes/derivedWriteBytes/authorityComparedBytes 等）可供 T361 逐帧分解。
3. **无需冷 manifest 工程、无需重跑**：文件完整可读（incomplete=true 仅因环形覆盖，128 帧窗内数据完整）。

## 5. 边界

只读补取（无重跑、无场景执行）；不改业务/阈值；T352 原报告版本保持；本文件与 T360 报告互引。

证据：`test/reports/t360/v2-budget-t352-era.json`（SHA256 上文）；T360 报告 §1 追回记录。
