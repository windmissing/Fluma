# T410 · GPU V2 同窗双源与负控真机补证（attempt2 阶段报告）

2026-11-09，tester。真机 62T0225B10010020（防熄屏 600s 已设）。T421 签包 SHA256 5450ad3052654de11994e180870e0a6b9e0db7c80d2a6e95aa572003367ae751 逐位核验并安装成功。**主目标（同窗双源/六负控/30fps）未行使**——阻断于预约 SHA 输入通道，详见 §3。已完成的真机验证为 T421 预约实现提供了真实设备闭环证据。

## 1. 已完成：实际 V1/V2 单次预约完整闭环（真实设备）

| 档 | scope | 生命周期 | 原件 |
|---|---|---|---|
| V1 | epoch2 / run1 / session1 / **V1** | RESERVED→ARMED（safePointSerial=1，lastBlocker 空=真实安全点）→FROZEN DURATION_REACHED | `test/reports/t410/v1-attempt2/` |
| V2 | epoch3 / run2 / session1 / **V2** | 同上（safePointSerial=2） | `test/reports/t410/v2-attempt2/` |
| V2 补充 | epoch4 / run3 / session1 | 同上（safePointSerial=3） | `test/reports/t410/v2-run3/` |

三档 manifest `state=COMPLETE`、包 SHA 逐位匹配、BEGIN/END 事件在场。**T421 预约实现在真机 V1/V2 双 revision 下均正常工作**——与 T410 attempt1 的 CAPTURE_BUSY 持续拒绝（六次跨包复现的结构性停顿家族）形成直接对照，T421 的预约制修复真实有效。

## 2. 发现（交 T411/architect）

- **T411 checker schema 版本差**：T421 后产品 requiredEvents 输出 `string[]`（新增 FIRST_OFFER 等），checker 仍期待 T409 旧对象格式（readySeen/present 等）；archive_check 的 firstSerial 字段亦不匹配。事件实际在场（人工核 EVENTS 页）。属 checker 版本化跟进，非档缺陷。
- **新空场按钮坐标**：`[822,497][1102,637]` 中心 (962,567)——早前误点 (460,567)（水源按钮）导致误诊"换代失效"。

## 3. 阻断：同窗双源窗口被预约 SHA 输入通道阻断

**现象**：预约恒 `REJECTED INVALID_CAPTURE_CONFIG`。

**根因定位**：`armCapture` 的 `packageSha256` 取自 `this.packageSha`（@State），仅由诊断面板 SHA TextInput 的 `onChange` 写入。本设备 IME 对 uitest 注入文本的处理**不触发 ArkTS onChange**——框内 UI 文本与 @State 脱钩。四条注入通道全部穷尽：

1. 单次整串 inputText（64 位，框内 match=True）→ REJECTED
2. 分段 inputText×16（0.15s/0.5s/0.6s 节奏）→ IME 竞态乱序+自动插句号（`。`），@State 更不可用
3. inputText+回车（keyEvent 66）→ REJECTED
4. **逐字符 keyEvent×64**（KEYCODE_0-9/A-F，物理按键路径，框内 match=True）→ 仍 REJECTED（证明 onChange 只由 IME 编辑通道触发，按键事件不经该路径）

V1 成功会话（本日早间）的 @State 写入是 IME 竞态偶发成功——非确定性，不可复现依赖。

**为何不能继续**：双源窗口预约需要合法 config，而 SHA @State 无法通过任何可用自动化通道写入。tester 红线不改产品代码。

**已试**：以上四通道 + 三次冷启重试循环（r1/r2 均拒）+ V2 切换后新 run 重预约 + 面板展开态直接预约。

**所需前置（交 architect 裁决）**：
- (a) developer 给 GpuResidentM1 增加 want 参数通道（如 `--ps gpusha <sha>`）或诊断面板"一键填本包 SHA"按钮（产品变更，最小面）；
- (b) 或提供等价无 UI 输入的预约入口（NAPI 已有 beginEvidenceCapture——若可从 tester 侧 hdc 直调则给调用规范）。

**接手角色**：architect 裁决 → developer 实施 → 本卡续办（V2 双源窗口、六负控、30fps 全部未行使，预约门已证可开）。

## 4. 边界

- 本报告仅真机 attempt2 阶段档；卡保持"进行中"，未行使项不冒通过。
- T411 checker 版本差与本卡阻断均已分别登记对应卡。
