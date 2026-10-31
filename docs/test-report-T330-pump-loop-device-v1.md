# T330 Pump 多批循环真机验证（J1-J5 + 双源链复测）（真机，BLOCKED→新发现移交）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T329 实施 commit `e66f1c2`。

## 0. 实际包身份（T328 移交裁定 1 口径）

- tester 按 T151 流程补签：HEAD `e66f1c2`（工作区 cpp/ets 零未提交改动）hvigor 重建（BuildNativeWithNinja 3.7s + PackageHap 刷新，SignHap 00303116 已知独立登记）→ Python 重打包（绕 sign-tool 11017001）→ hap-sign-tool localSign + verify-app **PASS**。
- **T330-signed.hap SHA256 `8f03cbae437e320644dbeb9e62b9fa5fa765c1fda33384ef4222883cb61bbdf2`**，页面 TextInput 外部输入核对一致，hilog GRS_TIMELINE_COLD 回读同值。三件套：手签包 + sign/verify 输出 + SHA 归档（`.codex/local-test/t318-sign/T330-*`）。

## 1. J1-J5 逐条判定

| 判据 | 结果 | 证据（原 ns，`t330_device_J_accounting.json`） |
|---|---|---|
| **J1 单 Pump 封尾 & submitPumpWallNs ≤8ms** | **FAIL（新发现根因）** | 单 Pump 封尾本身达成：run1/run2 各 13 批在**单轮 Pump** 内全部发出（Pump 段 103→364 = 39.04ms / 440→701 = 42.54ms），13 批 dispatch 本体合计仅 2.25/1.48ms——但**末批 `glFenceSync` 单调用停顿 37.65ms（run1）/41.28ms（run2）**，把单 Pump 拖到 39-42.5ms，超 8ms 预算约 5 倍 |
| **J2 FenceEnter ≥1 首达** | **PASS** | FenceEnter/Exit 各 2 次（run1/run2 各一），T318 时代 0 次 → 封尾首次真机到达 |
| **J3 accepted ≥1** | **FAIL** | 封尾转 FENCE_PENDING 后，下一轮 bridge GAP 判定（anchor→pause 46.12/49.15ms > 33ms 窗，原值）→ Run::Pause → submitting 仍 true → **CancelSubmit(code=2=FENCE_PENDING)** 删 fence，root 永不 accepted（accepted=0，两 run 一致） |
| **J4 首批 SubmitExit code=16** | **PASS** | 全部 39 对 SubmitExit code 均为 16，UINT32_MAX 修复真机生效 |
| **J5 GAP 注入反证（窗语义未破坏）** | **PASS** | anchor→pause 46.12/49.15ms 原值触发 CLOCK_GAP（PauseEnd code=6 flags=3），窗判定不豁免不放宽——GAP 语义完好 |

**核心新发现：真机 `glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE)` 在 12 批未 flush 的命令缓冲后创建 fence 时停顿 37-41ms（两 run 可复现），而非 PC SwiftShader 仅首批慢的特性**——13 批循环本体（~2.2ms）远在预算内，唯一阻塞源是 fence 调用；它同时击穿 8ms Pump 预算与 33ms GAP 窗，使 J1/J3 失败。这与 T318 时代"批间隙 8-9.9ms×13 轮"的配比问题不同——**循环已生效，瓶颈转移到 fence 创建**。

## 2. 附带采集

- **双源同 token 上屏链**：NOT_RUN（accepted=0 前提不满足，不冒进）。
- **逐批遥测连续性（INV-5）**：PASS——run3 carry 0..121 单调无跳，dispatch 122 个连续。
- **run3**：12 批后导出窗口截断（NATIVE_DROPPED），未及 fence，行为与 run1/2 前段一致。
- **NATIVE_DROPPED 登记**：环容量 1024，多批循环产生更密集事件（run1 376 dispatch vs T318 时代 50），三次导出均 INCOMPLETE（GREW_DURING_EXPORT ×1 + NATIVE_DROPPED ×2，fail-closed 语义正确）——本卡判据数据在溢出前已完整落盘；环扩容属工具链事项，交 manager/developer 另行评估，不阻塞判据。

## 3. 结论与移交

- **J2/J4/J5 PASS；J1/J3 FAIL（同一根因：末批 glFenceSync 真机停顿 37-41ms）**。
- 移交 architect/designer：**glFenceSync 停顿归因与修复方向**（候选：末批前分批 glFlush / fence 创建独立 Pump / 驱动行为确认）——Amendment 1 的循环结构本身工作正常，不需要回退。
- T328 裁定"6 项已验证条款不回退"维持：nonce 一次消耗（SubmitRootBegin 每 run 恰 1）、carry 单调、取消无崩溃（新 run 正常 epoch 2→3→4）、submitting≠inflight、41 字典、逐批 ≤8ms（12/13 批仍 ≤0.36ms）全部无回归。
- 签包口径执行：tester 补签三件套归档，SignHap 00303116 独立登记。

证据：`test/reports/t330/`（manifest×3 + part 全套 + t330_device_J_accounting.json + layout0-6.json）；签包 `.codex/local-test/t318-sign/T330-signed.hap`。
