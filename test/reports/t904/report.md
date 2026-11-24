# T904 PC 真实根冷档重放与 DM2/R1 截止退役复测

- 任务：T904 子项 A（REPLAY 真实根冷档复采）/ B（DM2/R1 两臂复测）
- 角色：tester（PC 实测）
- 日期：2026-11-24
- 工件：T903 三件全 SHA 匹配（M1 `e72b53c6…`、DM2 `3ef1e999…`、R1 `694ad0c2…`）；输入 `3580ccb1…`
- 依据：docs/developer-T903-v1.md + docs/architecture-T902-v1.md；GPU 驻留、无 Canvas 回退

## 子项 A · REPLAY 真实根冷档复采——T903 修复实证

三个 REPLAY 臂全部修复通过：

| 臂 | 52 动作 | COLD_COPY | 重放档 | drain |
|---|---|---|---|---|
| REPLAY_OFF | 28 DELIVERED+21 REVOKED+2 AWAIT+1 OFF | **3 份**（首笔真实 SOURCE + 2 QUANTUM） | 3 件 M1PROOT1 | RETIRED（coverage complete/loss=0） |
| REPLAY_ON | 29+20+2+RESERVED | 3 份 | 3 件 | RETIRED（coverage complete） |
| cold-reanchor | 48 DELIVERED+2+2 | 4 份 | 4 件 | RETIRED（coverage complete） |

- **真实接受根触发**：`COLD_COPY_SUBMITTED/WAIT/READBACK` 完整生命周期，带 baseBank/finalBank/publicationSlot/eventStart-End 背板字段；首笔为**真实 SOURCE 根**（nonce 与 run 绑定）、次笔起为真实 QUANTUM 根
- **10 件 M1PROOT1 档全部 32 字节摘要验证通过**
- **T896 的 `M1_COLD_ROOT_BACKING` 空箱根缺陷不再复现**（T902 归因的预览顺序缺陷已由 T903 修复）

## 子项 B · DM2/R1 截止与退役复测（各两臂）

四臂一致结论：

| 项 | DM2（12 派发） | R1（19 派发） |
|---|---|---|
| BOOT | vk=0、seal 6559303、eligible、真实 8086:3e98/1656924 | vk=0、seal 3223126018、eligible |
| ROOT | 250ms 原边界下 vk=2、**GRANT_DEADLINE**、gate TAIL_NOT_READY | 同 |
| **RETIRE** | 同 fence 追加等待 **vk=0** | 同 fence 追加等待 **vk=0**（T902 时 R1=2 的缺口已修） |
| 时间戳臂 | LATE_RETIRED_DIAGNOSTIC **6/6 点位**可用（queryCreateVk=0、36bits/83.333ns） | **7/7 点位**可用 |
| GPU 分段 | PREPARE 39.2ms / SOURCE_WRITER 71.0ms / AUDIT_COMPARE_PACK_SEAL 130.7ms / TAIL_COPY 0.007ms | PREPARE 32.6ms / ACTUAL_WRITER 182.6ms / REPLAY_AUDIT_APPLY 166.6ms / COMPARE_PACK_AUDIT_SEAL 31.6ms |

`ROOT_TIMELINE` 保全链路：inputFrozen/expectedReady/recordEnter-Exit/submitEnter-Exit（vk=0）/14-15 次 2ms 轮询/lastPoll/stop/gate/readback。实际接受未达成（iGPU 250ms 界）→ 按卡面"实际接受才接 ROOT/FULL 或 WORK/Present，晚完成不得计入接受"，ROOT/FULL 档与 R1 WORK/Present **不计入接受**。

## 证据

`test/reports/t904/`：A 3 臂 JSONL + 10 重放根 + B 4 臂 JSONL + summary.json
