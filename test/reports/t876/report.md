# T876 PC 双源续帧与同场景沙水独立复测

- 任务：T876 PC 双源续帧与同场景沙水独立复测
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：`.codex/local-test/t872-release.exe`（SHA256 `6ec0ec5c9e401ff4f7000e305f4acc989c2e5f9db35b8e6bfc97474ec8669871`，卡面指定 T872 件）
- 三 compute SPIR-V 沿原 SHA；全程 GPU 驻留，无 Canvas 回退
- 5 轮实测：sand→water×4（含 10s/8s 长按+前台守卫）+ water→sand×1

## 核心结果

### T872 续帧修复实证（本卡最大正向发现）

T864 时期的双源续帧 QUANTUM `FENCE_OR_PERMIT_FAILED` 不再复现：

| 轮次 | 首源受理 | 续帧量子 | 终态 |
|---|---|---|---|
| run1 sand→water | 沙 97584 Q | **213 连续全 QUEUED** | PRESENT_RETIRED |
| run3 sand→water | 沙 100011 Q | 198 QUEUED + 1 STALE_WAIT | PRESENT_RETIRED |
| run5 sand→water | 沙 116887 Q | **279 连续全 QUEUED** | PRESENT_RETIRED |
| run2 water→sand | 水 124733 Q | 4 QUEUED + 2 ROOT_DEADLINE(fence=2) | 等待（真实短等待结果） |

逐帧 generation/physicsCommit 递增、sourceCommit 恒 1——有序脉冲队列+许可退役重构在此 iGPU 上功能性生效。

### 守恒与同根库存

- 全部帧 `sandStockQ == sandAdmittedTotalQ`（97584/100011/116887/129279；水轮 124733）——受理总量=存量，帧间零损耗。
- 单一 run 身份贯穿全帧；像素 0xff525355 常值（信息值）。
- STALE_WAIT_NEW_INPUT 可恢复等待路径实测触发（run3），随后 POINTER_READY→WAIT→RETIRED。

### 真实输入

真实 Win32 消息全程记录：DOWN/UP/CAPTURE_LOST/CANCEL/FOCUS_LOST（run3 水按被环境抢焦点 CANCEL——真实事件如实入档）；键 1/2 模式切换全记录。

### 首拒

run2/run4 续帧 ROOT_DEADLINE（fence=2 VK_TIMEOUT）原始收据保真。

## 未达成（真缺拍单列）

**同场景第二源受理**（5 轮均未达成）——可复现的结构性结果，非测试伪影：

`nextPublication` 的 `takeFresh()` 要求 `!clock.activeRemaining`（帧边界）；T872 有序脉冲每 10ms 补充量子，使 activeRemaining 在整个持按期间恒 >0（run5 中 279 个量子耗尽了全部 6s 水按时长，帧边界从未出现）。**T872 修好续帧的同时使帧边界变稀，模式切换后的第二源结构性无法进入**。归 architect/developer 裁定（如切换沿强制帧边界或量子耗尽暂停）。

旁证：两材料各自从真实场景独立受理全证（沙×4、水×1）；切换撤权/清 held/重按路径全真实。

## 交付

- 证据：`test/reports/t876/`（5 轮 JSONL + summary.json）
