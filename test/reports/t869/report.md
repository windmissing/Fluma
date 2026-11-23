# T869 PC 异步热页与首根忙点预约独立验证

- 任务：T869 目标14 PC 异步热页与首根忙点预约
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：t860-paged-release.exe（`b02294dc…`，cold-on-busy/capture-hot）+ t860-d1-release.exe（`f41be707…`，root-on-busy）
- 全程 GPU 驻留，无 Canvas 回退

## 结论

| 验证项 | 结果 | 证据 |
|---|---|---|
| 首根忙点预约（--m1-cold-on-busy） | PASS | runA：预约 request=run nonce，UI 线程在首 QUANTUM fence 忙段预约；64 KiB 顺序页 ×2（65536+28800=94336 B）读出，页 SHA 与整载一致 |
| 分页冷档完整性 | PASS | runA `m1pcold1.bin` 94408 B=72 封套+94336 载荷，完整 32 字节摘要与 sha256(raw) 一致；`test/t848/check.py` join **exit 0**（root/scope/payloadSHA/lineage=NEXT_FRAME 全过） |
| 首根分页与呈现 | PASS | runA Present QUEUED→RETIRED；指针生命周期 DOWN/UP/CAPTURE_LOST 全记录 |
| 后继根忙点预约（--m1-root-on-busy） | PASS | runB：`M1_HOT_CAPTURE_RESERVED`→单页读出；**busyRoot 15823393860401 与首量子 m1QuantumNonce 精确相等**（实际认领根与忙点 nonce 匹配）；m1ColdBacking 记录 bankSlot/publicationSlot/eventArena/eventEnd 与三 backing 指针、fence=0、invalidate=0 |
| M1PROOT1 封套 | PASS | 62792 B：magic+8×LE U32 长度+32 B 摘要+4 段（Completion256=1024、accepted bank=29696、arena[0,eventEnd)=256、publication=31744），后 4 长度为 0；摘要与 sha256(raw) 逐字节一致 |
| FROZEN 热页（--m1-capture-hot）+ T865 判定 | PASS | runC：`M1_HOT_CAPTURE` 带完整 256 字 Completion；blocker=1（SOURCE fence 忙段）、忙段 65.1 ms、safePoint 递增；**T865 `hot()` 判定 PASS（FROZEN_HOT_PAGE）**——预约绑定、时序、SEAL/nonce/run/completionSHA 全匹配、无悬挂预约 |

## 交互注入

真实 Down→持按→Up（PostMessageW 直发窗口消息，同 T856 方法）。

## 负证据（如实保留）

1. `--m1-d1` 与 `--m1-root-on-busy` 互斥（d1 校验要求 m1ColdFile 为空而 root-on-busy 会设置它）→ usage exit 2；四臂 D1 与根采集须分进程跑。已实测确认。
2. runB 采集完成后的续帧 QUANTUM 失败 FENCE_OR_PERMIT_FAILED（VK_TIMEOUT 但 fenceNs<deadline，即许可撤销）——与 T864 单源续帧同一 Intel UHD 630 时限；被采集根本身完整（fence=0、owner=0）。

## 交付

- 证据：`test/reports/t869/`（runA/runB/runC JSONL + m1pcold1.bin + m1proot1.bin + summary.json）
- 完整 D1 四臂与任意根 D2 沿 T860 交接继续等相应接口原件（本卡按卡面仅测已交入口）
