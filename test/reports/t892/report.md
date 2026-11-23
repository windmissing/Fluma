# T892 PC 停源落床与长窗原件补采

- 任务：T892 PC 停源落床与长窗原件补采
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：`.codex/local-test/t882-a-final.exe`（`86e8ad81…`）；三 SPIR-V 沿原 SHA
- GPU 驻留、shader 渲染、无 Canvas 回退；双源/D1 复测等 T891 工程结论（未入本卡）

## run1 · 活粒非零时真实 UP（停源落床+持续存在）

- **UP 触发条件实测**：lease 检测 activeSand=1（活粒非零，t+0.3s）即发真实 UP。
- **落床过程**：UP 后 **83 帧（约 3.6s）** activeSand 1→0——粒子沉降完毕。
- **床留存**：UP 后 119 帧库存**恒 121460**，零损耗（守恒 stock==admittedTotal 全帧成立）。
- 120 量子全 QUEUED + 干净 RETIRED；单 run 身份贯穿全帧；~24fps。

## run2 · 忙点预约冷档（事件与前后库存原件）

- **预约**：`--m1-cold-on-busy` 首量子忙段预约成功，`M1_COLD_PAGE` 双页读出（65536+28800=94336 B），busyRoot 绑定 run nonce。
- **冷档**：M1PCOLD1 94408 B，完整 32 字节摘要验证通过，**T848 join exit 0**（lineage=NEXT_FRAME）。
- **事件区**：q_events 2 条 + s_events 2 条（各 256 B 根级事件）。
- **前后库存**（档内 Completion 原字）：q sandBefore/sandAfter=136607；s sandBefore 0→sandAfter 136607；sandLive=2（采集时刻活粒）。
- RETIRED 干净。

## 明确缺证（按卡面要求登记）

1. **逐笔 LAND 事件缺证**：Completion 的 `bedEventCount`（word 95）在 q 与 s 两份完成字中**均为 0**——bed 事件账本字段在布局中存在但交互路径未记录任何逐笔落地事件（事件区仅根级事件）。归产品接口缺口（bed 事件发射未接线或需其他触发）。
2. **ROI 逐像素配对缺证**：PC 接口仅单采样点像素（0xff525355），无 ROI/像素级床读出。

## 交付

- 证据：`test/reports/t892/`（bed-dual.jsonl + cold-busy.jsonl + m1pcold1.bin + summary.json）
