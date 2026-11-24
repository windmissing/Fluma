# T941 · 目标 4 双态同帧与目标 6 GPU 长程 PC 实测（effector）

2026-11-24。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924），T933 同版集成档（PE 4a40a141，manifest 核过），GPU 驻留、shader 渲染，无 Canvas 回退。证据根 `test/reports/t941/runs/`。T937/T939 已完结待验收无新评论，本卡直接执行（卡面"已有进行中 GPU 运行收尾后优先本卡"）。

## ① 目标 4：双态同帧配对（goal4-dual-state3）——可关闭

方法：6s 持按建床 + 8s 沉降 → 3s 补沙（制造在途沙）→ observe-series 30 帧沿下落列（x=312）三个高度带（y220/260/300）。

**frame 40 同帧配对（一帧四证）：**

| 证据 | 值 |
|---|---|
| 配对身份 | `m1Observation.root == windowFrame.acceptedRoot == 归档 root`（同 root/lease/geometry/slot/publication，imageIndex=0、physicsCommit=38、format 44） |
| **非零床**（GPU 原 S） | S col63=74,402 / col64=616,049 / col65=58,512（共 748,963 Q） |
| **在途沙**（GPU 原 P + lease） | P col64=3,474,908 Q、`activeSand=32` |
| 账目恒等 | S(748,963)+P(3,474,908)=**4,223,871 == sandStockQ** |
| 像素（真实呈现帧） | 在途列 y260 **11 沙色 px**、近床 y300 **5 沙色 px**（(89,145,184)） |

- 帧 20–30 演化：床 S=0、P 列 2.13M→3.67M 增长（纯补沙期），帧 40 出现**床+在途同帧**——双态同帧判据成立。
- 38 个 m1root 归档；三带像素逐帧 30 帧在 `demo.jsonl`。
- **结论：目标 4 可关闭**。

## ② 目标 6：GPU 长程与阶段耗时（goal6-longrun + goal6-stage-timing）

**长窗（默认 argv，117.2s，10×6s 持按+4s 空闲+停 8s+探针）：**

| 量 | 值 |
|---|---|
| 提交/接受 | 6,574 / **6,571（接受率 99.95%）** |
| 拒绝 | CLOCK_GAP 2 + WINDOW_REVOKED 1（**无接受坍缩**——T920 长程签名在集成档已消失） |
| 退役/恢复 | 完整链 ×1（SUSPEND→RETIRED→NEW_BASELINE→HOLD_REVALIDATED） |
| 仿真推进 vs 墙钟 | 54.8s 仿真 / 117.2s 墙钟 = **0.47**（每接受 Q 记账 1/120s） |
| 重显 | presents 2,821 == distinct physicsCommits 2,821（无重复物理提交重显）；2.33 Q/呈现帧 |

**阶段耗时（同版 35 派发 + `--m1-stage-timing`，按 T933 文档的同版诊断开关，18s 短窗）：**

| 量 | 值 |
|---|---|
| GPU 量子计算 span | **中位 15.08ms**（n=1101，36-bit tick×83.333ns；max 91.1ms） |
| host record→stop | 中位 15.6ms |
| Q 完成间隔 | 中位 16.04ms → **62.3 Q/s（集显实测）** |
| 呈现间隔（fenced） | 中位 39.5ms → 25.3 fps 呈现节奏——**按 fenced present 间隔计，不把 WSI 排队当可见 FPS** |

- **归因**：GPU 量子计算中位 15.1ms > 记账节奏 8.33ms/Q——集显吞吐由 GPU 计算与呈现节拍共同界定。
- **口径区分**：以上为开发集显正确性与诊断口径；目标独显实际持续性能归用户目标环境实测（卡面合同）；`--m1-stage-timing` 轮为同版诊断开关（非用户体验变体，用户体验版仍为默认 argv）。

## 结论

| 子项 | 结果 |
|---|---|
| 目标 4 双态同帧 | **可关闭**：一帧配对非零床 S（3 列）+在途 P 列+activeSand+两区像素，账目恒等闭合 |
| 目标 6 GPU 长程 | **完成**：117.2s 长窗接受率 99.95% 无坍缩、仿真/墙钟 0.47、无重显重复；阶段耗时 GPU 中位 15.1ms/Q、62.3 Q/s、25.3 fps fenced 呈现节奏；集显计算界已定位，独显口径归用户环境 |
