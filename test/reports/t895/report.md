# T895 PC 空箱重力像素与流场退役诊断实测

- 任务：T895 子项 A（空箱/重力/像素）+ 子项 B（G13 拒绝根退役诊断）
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：`.codex/local-test/t890-observation-timing-release.exe`（`1e49f2a6…`，T890 A/C 共用件）；T893 判定器
- GPU 驻留、shader 渲染、无 Canvas 回退；两子项独立留原件

## 子项 A · 空箱、重力、同根观测

### 空箱（T893 empty 判定 PASS）

`EMPTY_READY` 帧（真实窗口 784×461）：
- **acceptedRoot=null、sourceCommit=0、physicsCommit=0、sandStockQ=0、waterStockQ=0、particles=0**——空箱零库存/null root 实证
- 真实初始化 fence（vkResult=0）+ 进出 QPC；首空帧进 WSI 队列后才开 pointer READY 与 SOURCE 接受链
- **T893 `empty()` 判定：PASS（EMPTY_PREVIEW_JOINED）**

### ±30° 重力边界（同 grant）

WM_APP+3 真实消息注入（绝对角度，×256 编码）：

| angle256 | 结果 |
|---|---|
| +3840（+15°） | ACCEPTED_INPUT |
| +7680（+30°） | ACCEPTED_INPUT |
| +9999（>30°） | **ANGLE_OUT_OF_RANGE** |
| −7680（−30°） | ACCEPTED_INPUT |
| −10023（<−30°） | **ANGLE_OUT_OF_RANGE** |

±30° 约束双边界实测执行；每次输入记真实 QPC/序号。

### 同根 ROI 观测与冷原字

`m1Observation`（FENCED_READBACK）：
- 同 root（15944411374190）+ slot/generation/imageIndex/graphics fence/serial 全绑定
- format=44（SHADER_SRGB_UNORM）、TOP_DOWN、768 B 真实像素（3 个 8×8 rect：床/粒/背景）+ SHA256
- 同 nonce 冷原字 `a-obs-*.m1root`（63048 B，M1PROOT1，摘要验证通过）+ `M1_OBSERVATION_COMPLETE`

**逐笔 LAND（单列）**：仍缺证（同 T892：bedEventCount=0，交互路径无逐笔落地事件）。

## 子项 B · G13 拒绝根同 fence 退役诊断（full/hot 两臂）

两臂（F02 fixture，iGPU 250ms 界使 QUANTUM 拒绝）一致：

| 项 | b-full | b-hot |
|---|---|---|
| 首退出 | GRANT_DEADLINE | GRANT_DEADLINE |
| 退役等待 | vk=2 | vk=2 |
| 设备排空 | 1191.6ms | 1023.3ms |
| **LATE_RETIRED_DIAGNOSTIC** | businessRejected=**true**、acceptedRoot=**null** | 同 |
| 同 fence 退役确认 | fenceVkResult=**0**（retireConfirm QPC 实测） | 同 |
| **非阻塞时间戳读** | queryState=**AVAILABLE_POINTS**、**18/18 点位可用** | 同 |
| writtenMask | 393215（0x60000）保留 | 同 |
| 有效位/周期 | 36 bits / 83.333ns | 同 |
| GPU 分段 | root 1699ms、actual 730ms、replay 509ms、comparePack 2.7ms | root 1523ms、actual 542ms、replay 516ms、comparePack 2.7ms |

**结论**：拒绝根在同 fence 真正退役（vk=0）后**非阻塞读时间戳成功**——queryState=AVAILABLE_POINTS、全 18 点位、written mask 保留、无错误接受（businessRejected=true/acceptedRoot=null）；full/hot 两臂行为一致。GPU 分段数据（root≈1.5-1.7s vs 250ms 预算）与 T879 结论互证。

## 交接（DM2/R1）

T890 交付的 DM2/R1 独立 native owner（`owner.h` 接口）需角色宿主程序（真实 device/queue/buffer+typed gate）——本卡 PC 侧无宿主工程，**留交接**：接手角色按 owner.h 搭建样机宿主后采 DM2 ROOT/FULL 原件与 R1 同根通量 WORK。

## 证据

`test/reports/t895/`：a-empty-gravity.jsonl、a-gravity-limit.jsonl、a-obs-*.m1root、b-full/b-hot.jsonl、summary.json
