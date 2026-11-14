# T410 · GPU V2 同窗双源与负控真机补证（v1：正确来源包 T467-t410-final 真机执行；首 root GPU 超期域先于全部六负控 cut，双源同窗/负控 NOT_REACHED 如实记录）

2026-11-14，tester，真机 62T0225B10010020（本时段独占）。**来源按 2026-11-13 更正**：`test/reports/t483/install-t410.json` 实际包 `T467-t410-final-signed.hap`，SHA256 `8f1c45f711f1aaf9e07e6b68d65a8692ed5eaaecebffc54c5f7e60266bda39f6`（本地 sha256sum 逐位一致），Want `--pb grsm1 true --ps gpusha <完整包SHA>`；旧 T450 `63b0fd25…` 未使用。证据：`test/reports/t491/t410/`（5 截图 + 5 份 boot 诊断 JSON + WS_EVENT 转录）。

## 一、原子身份与空场 QUANTUM（T407 §5 前置）

- 冷启 PID 29845；页面：`沙水 DEMO V4 · GPU M1`、`GRS_M1_PROGRESS_CLOCK_V2 / run 0:0 / epoch 3 / session 0 / CREATING_NEW`——**actual revision V2 原子身份在位**。
- 新空场后 run 0:1 / session 1；空场 QUANTUM 未接受（见三）。

## 二、真实沙水 offer（显式源链）

- 两次新空场后各执行两次 (147,263) pointerDown 投沙：页面**最近源收据恒 `#0.0000`、物理恒 `0:0`**——SOURCE 未被消费（与 T495 同域：首 root 超期撤权后 mailbox 冻结）。
- 点"开始2秒同窗采集"：采集态保持"未采集"（同窗采集通道在 run 撤权后不启动）。沙→水切换点击已执行（当前源文字区路由正常）。

## 三、首错与六负控执行度

- boot-final.json：attempt3 `firstFailure = V2_GPU_PROGRESS_TIMEOUT root 1`，**b−a = 250.0ms 整、detected−deadline = 0.149ms**——与 T495 量化一致，真机首 root GPU 队列耗时结构性超期。
- **六负控（GAP/GPU 超期/量子 pending4/SOURCE FIFO16/旧代/lateSwap）全部 NOT_REACHED**：首错发生在 root 1（首个 QUANTUM），负控 cut 所需的多 root/SOURCE 积累从未发生。未到 cut 明记，不以 STALE_PAGE_SESSION 页面态冒任一负控 PASS。
- 30fps 有效新内容：**NOT_MEASURED**（无有效帧，画布黑屏）。

## 四、结论

正确来源包身份链完整；V2 原子身份/页面/采集入口在位；**同窗双源与六负控被 T448/T490 首 root GPU 超期域阻断**（同根于 T495 的 250.37ms 独立量化）。本卡不重跑 T495 已证时域，等待呈现域修复后接续。
