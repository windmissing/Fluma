# T896 PC 跨材料、D1 与双样机分项实测

- 任务：T896 子项 A（跨材料）/ B（D1）/ C（DM2/R1 样机）
- 角色：tester（PC 实测）
- 日期：2026-11-23
- 工件：T894 四件全 SHA 匹配（A `186a008e…`、B `f7dab474…`、DM2 host `5788e027…`、R1 host `4de99b3a…`、输入源 `3580ccb1…`）
- T895 已收口；各子项独立交付；GPU 驻留、无 Canvas 回退

## 子项 A · 跨材料真实退役→READY→新 Down（T894 A PE）

**【跨材料同场景双源受理首次达成——双向全证】**：

| 方向 | 受理序列 | 最终 lease | 守恒 |
|---|---|---|---|
| 正序 | 沙 128052 → 再沙 +132380 → **水 140686** | sand 260432 + water 140686，liveS=3 liveW=1 | 全帧成立 |
| 反序 | 水 133102 → 再水 +100897 → **沙 76720** | sand 76720 + water 233999，liveS=1 liveW=2 | 全帧成立 |

**使能机制实证**：T894 恢复重构——每轮 2 次 `m1Recovery`（原因 LATE_FENCE_SIGNAL/ROOT_DEADLINE，含失败 nonce、真实退役 QPC、返还额度、清算 carry、balanced=true、lastAcceptedRoot/lastPresentedRoot 保留）；`M1_STALE_WAIT_NEW_INPUT` 可恢复等待 ×2/轮；干净 RETIRED。T887 缺的"帧循环复活"被本版解决。

## 子项 B · D1 跨 session 终态（T894 B PE）

- **OFF×2**：52 动作各完整；`M1_D1_PULSE_OWNER` 28/48 行（真实 callback/native receipt/owner 三时标）；`M1_D1_DRAIN.ownerCoverage` complete=true、loss=0、逐 session queued/accepted/revoked 掩码；双 drain RETIRED。时域合同可比（均差 3.69ms/最大 11.93ms）→ 四臂开跑。
- **ON 臂**：52 动作（29 DELIVERED+RESERVED+20 REVOKED flake）、drain RETIRED。
- **受阻（可复现产品缺陷→developer）**：REPLAY_OFF/REPLAY_ON 两臂均在 `M1_QUANTUM_COLD_CAPTURE` 失败 `M1_COLD_ROOT_BACKING`（owner 在 drain 证据前终止，exit 1、0 动作）——T894 B 的重放路径与恢复重构在首个接受根上冲突。复跑同错。

## 子项 C · DM2/R1 独立宿主（互不等）

| 项 | DM2（45 模块） | R1（49+6 模块） |
|---|---|---|
| **BOOT** | **PASS**：vk=0、seal 6559303、eligible=true、真实 vendor/device/driver、输入 SHA 绑定 | **PASS**：vk=0、seal 3223126018、eligible=true |
| ROOT | 12 派发 vk=2 → ROOT_NOT_ACCEPTED | 19 派发 vk=2 → ROOT_NOT_ACCEPTED |

ROOT 受理受阻于本 iGPU 的 250ms 级 fence 时限（与 G13 QUANTUM 同类）——DM2 ROOT/FULL 档与 R1 WORK witness/Present 需 ROOT 通过后才能采（待独显或期限裁定）。

## 证据

`test/reports/t896/`：A 3 轮 + B 6 轮 JSONL + DM2/R1 BOOT JSONL + summary.json
