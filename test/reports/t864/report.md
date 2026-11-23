# T864 · 目标7 PC 双源首阶段实际验证

2026-11-23 · effector · 真实 Windows PC（Intel UHD Graphics 630，Vulkan 1.2 core）。全程 GPU 驻留+shader 管线，无 Canvas 回退。

## 工件

- PE：`.codex/local-test/t860-paged-release.exe`，SHA256 `b02294dc…c7e4`（含 `--m1-dual-source`+`--m1-source-trace` 的最新构建；t862-release.exe 无 trace 选项，实测确认）。
- 三 compute SPIR-V 沿 T856 原件；双源图形 SPIR-V m1_full_scene.frag.spv 9004 B（SHA `d7d73a46…8151`）经 PE 内嵌使用，干沙对比沿 T860 SHA。
- 驱动：PostMessageW 注入 WM_LBUTTONDOWN/UP（按键）与 WM_KEYDOWN/CHAR/UP（'1'/'2' 切源）。

## 实测结果（5 轮，run1–run5）

| 轮 | 场景 | 受理 SOURCE | 呈现帧 | 备注 |
|---|---|---|---|---|
| run1 | 沙→切水→按中切回→重按 | 沙 1 次（124585 Q） | 25 | 4 阶段完整时序 |
| run2-water | 空场景先按 '2' 再按 | **水 1 次（129635 Q）** | 11 | 水源自空场景准入 |
| run3-sand | 纯沙 | 沙 1 次（134668 Q） | 7 | activeSand=2 |
| run4-dual | 沙→切水→切回→重按 | 沙 1 次（135076 Q） | 0 | 首个续帧 quantum 失败 |
| run5-dual | 沙→切水 | 沙 1 次（126391 Q） | 1 | 同上 |

### 已验证（卡面范围）

1. **两源准入**：沙（material=0）与水（material=1）均经同一 Vulkan owner、原许可与 SOURCE V2 命令受理（`m1InputProfile=FULL_M1_V2`、`emitQ=16777216=16×2^20`）。受理后 `m1SceneLease` 逐帧报告：
   - 沙：sandStockQ=124585/126391/134668、activeSand=1–2
   - 水：waterStockQ=129635、activeWater=1（自空场景）
2. **键 1/2 切换与撤权**：`M1_SOURCE_MODE material 0↔1` 实测；切换即时撤销待消费项（ingress result=4 Revoked，每次 4–16 条）并清空 held；按中切换（run1 相 C）同样触发 MODE+撤销。
3. **重按**：切源后重新按下产生新 hold（run1 hold 4/7/10），受理路径可达。
4. **同代 Present**：M1_QUANTUM_PRESENT_QUEUED 帧均带 `m1SceneLease`（同 ABI `GRS_GPU_SCENE_Q20_M1_V2`、同 run 已接受库存、逐帧 acceptedRoot 递增），窗口像素采样一致（0xff525355，信息值）。
5. **库存一致性**：Isand/Iwater 累计 == 对应 stockQ，帧间守恒。

### 未达成项（如实登记）

- **跨源同场景累积**（同一 scene run 内先沙后水都被受理）：5 轮尝试均未达成。阻塞点：双源模式首帧 Present 后的续帧 QUANTUM 以 `FENCE_OR_PERMIT_FAILED` 失败（`m1QuantumFenceVkResult=2` VK_TIMEOUT、fenceNs<deadline，即许可/时钟被撤而非真超时）。run1 存活 25 帧后失败；run4/run5 在首个 quantum 即失败；run2/run3 在失败前正常收尾。单源模式（T856/T863）同 GPU 续帧 quantum 反复成功——双源负载下的时钟/许可脆弱性归 developer/architect 分析，原始数据全部保留。
- **空箱独立呈现、完整水面光学**：未单独取样（沿后续目标），像素采样点为固定信息值。

## 交付

- 原始 JSONL×5（run1/run2-water/run3-sand-then-water/run4-dual/run5-dual）含全量 M1_OFFER_RETURN/TERMINAL 追踪（serial/hold/material/result/root），供 T852 复核。
- `summary.json` 汇总各轮受理、切换、撤权、呈现帧与发现。
- 发现一：双源续帧 QUANTUM FENCE_OR_PERMIT_FAILED（见上，跨源累积阻塞）。
