# T901 PC 持续落沙、沙水与流场对照实测

- 任务：T901 子项 A（持续落沙）/ B（G13 槽证据）/ C（水侧扩散/像素）
- 角色：tester（PC 实测）
- 日期：2026-11-24
- PE：A=`t899-a-release.exe`（`c09e87fa…`）、B=`t899-b-release.exe`（`d5f0b8bf…`）；T900 判定器在仓
- GPU 驻留、无 Canvas 回退；各子项独立记录

## 子项 A · 持续落沙（T899 A PE）

### 长按多 SOURCE + UP 后无新 Down

- **单次 8s 长按 → 2 笔 SOURCE 受理**（134156+134915 = 269071 Q）
- **UP 后无新 Down 持续观察**（18s）：liveSand 4→2→**0**（可观察沉降归零），库存恒 269071——落床完成且留存
- 26s 窗口 584 量子（582 QUEUED + 1 `M1_STALE_WAIT_REAL_PULSE`[T899 新态] + 1 RETIRED）；守恒全帧；~22.5fps
- **自主恢复实证**：1 次 `m1Recovery`（ROOT_DEADLINE，真实退役 QPC、额度返还 5、balanced=true、lastRoot 保留）——超期后无需新 Down 恢复

### 一 grant 一量子对照（`--m1-one-quantum-grant`）

| 模式 | grantCap | 5-8s 持按受理 | 库存增长 | liveSand |
|---|---|---|---|---|
| 默认 | 4 | 2 笔/8s | 269071 | ≤4 |
| 一 grant 一量子 | **1** | **37 笔/5s** | 132706→4,837,378 | 2→61 |

77 份量子授权全部 grantQuanta=1/grantCap=1、firstWaitBudgetNs 201–218ms——T899 A"物理重臂独立于源许可"使帧边界受理机会大增（对照区分共用 grant 余额不足与独立根耗时）。

## 子项 B · G13 14 槽证据（T899 B PE，hot+slot-snapshot）

- **14 槽 × 42 字 = 588 字原字**：每槽 32 latch + 10 ctrl，真实逐槽锁存态（slot0-6 与 slot7-13 控制字各异）；`T899_SLOT_EVIDENCE`（snapshot=true、fenceVk=0、readVk=0、acceptedRoot=null）
- **复制计数**：hot 路径业务复制 span 13.08ms（2 次复制：Completion+2352B 槽证据，对照 full 的 29 次）
- **时间戳**：LATE_RETIRED_DIAGNOSTIC queryState=AVAILABLE_POINTS、18/18 点位、writtenMask 393215、36 bits/83.333ns；GPU 分段 root 1770ms / actual 712ms / replay 552ms / comparePack 3.4ms；排空 1273ms
- **拒绝证据**：businessRejected=true、acceptedRoot=null、同 fence 退役确认 vk=0、首拒 GRANT_DEADLINE

## 子项 C · 水侧扩散/像素——缺接口明报

T852 合同需要逐列 S/P/W/J/η、边事件流（LAND/ARRIVE）与 packetProbe 像素取证。现有 PC 工件仅提供聚合 lease（库存/活跃数）、单采样点像素与（T890）ROI rect 读回——**无逐列水账、无边事件流（bedEventCount=0）、无 packet 探针接口**。T852 交接自身注明"Windows Vulkan FullM1 两相、真实业务窗口和实际像素仍待产品接入"。归 developer/architect 接口设计。

## 证据

`test/reports/t901/`：a-longhold.jsonl、a-one-grant.jsonl、b-hot-slots.jsonl + summary.json
