# T868 拆分候选实际 GPU 资格与场证据（PC）

- 任务：T868 目标13拆分候选实际GPU资格验证
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：`.codex/local-test/t833-candidate-release.exe`（SHA256 `78ee42e9` 系卡面指定件，与 T833 交付一致）
- 路线：全部 GPU 驻留（Vulkan compute + 装配管线），无 Canvas 回退

## 结论

| 验收项 | 结果 | 证据 |
|---|---|---|
| 角色创建（候选模块槽） | PASS | 每轮 17 个候选模块槽全部编译并带各自 SHA256（OP_37/OP_48/OP_61/OP_00_9_11/OP_00_0_5/OP12_14/OP15/OP16/OP17_PRIVATE_MOTION/MMS/LEDGER/OP_18/OP6/OP7 三态/OP8） |
| OP66 资格 | PASS（8/8 轮） | 所有 run op66=QUALIFIED |
| SOURCE 受理 + 原档 | PASS | REST/F05/F05-isozero/revoke-after 四轮 source fence=0，`PC_G13_T833_RAW_V1` 原档 1530368 B 落盘（六段 BASE/CANDIDATE/REPLAY/WORK/CTRL/CTRL2，SHA 见 summary.json） |
| 撤权 before | PASS | `REVOKED_BEFORE_SUBMIT`：提交前撤权，无 vkQueueSubmit、无归档 |
| 撤权 after | PASS | source 完整提交并归档（f793b39c…），随后撤权、QUANTUM 跳过，终态 `T833_CANDIDATE_REVOKED` |
| 两 scope | PASS | F02 走 `PRIVATE_PROJECT` 路线（私有 OP17 三角色）；REST/F03/F05 走 `PC_SPLIT_CANDIDATE` |
| F02 场件 | PASS（准入层） | config128+state 原件 op65 QUALIFIED（c[7]=3 私有投影） |
| F03 场件 | PASS（准入层） | 周期边界=2、nu=1/16 剪切本征模（exactNuBits 兼容 c[12..17]）op65 QUALIFIED |
| F05 场件 | PASS（准入层） | 闭盒 MMS：墙面速度显式置 0、pi 为 Q32.32 定点宽对（gauge 和=0）op65 QUALIFIED |
| QUANTUM 数值级 | 受阻（非缺陷） | 所有路线 fence=2（VK_TIMEOUT）：850 次派发超 250ms 许可在 Intel UHD 630 上结构性不可达——T861 交接已预期需目标独显环境 |
| F03 首步 30 步振幅/收敛 | 未采（同上） | 生产不放行；被 250ms 量子时限阻塞，待独显 |

## 场件生成（tester 侧取证工具）

`test/t868/make_fixtures.py` 逐字对齐 C++ 校验（boot.cpp 1022-1040、wire.h ValidateConfig、units.glsl coldAudit）：

- Config128 基于 kRestConfig（64×64、sw=49536）
- `config[60..68]`=SHA256(state)（LE 字序）、`config[76..84]`=SelfHash
- F03：c[6]=2、c[7]=1、c[18]=1/16 且 c[17]=0x404f4000（exactNuBits: m=62.5/(1²×1000)=1/16）
- F05：pi 单元为 Q32.32 定点（sfloat(pi,-32) 读取）；墙面 u/v 显式置 0（fieldAudit 期望 floatBitsToUint(0) 精确相等，浮点 sin(π)≈1.2e-16 会失配）
- 三轮失败迭代（fail 26→20→35→0）证明校验链逐层打通

## 负证据（如实保留）

1. `revoke-before` 第一次失败 `T833_REAL_PULSE_ABSENT`（250ms 脉冲窗被饿死）——重试成功；记录为调度脆弱性。
2. QUANTUM 全路线 850 派发 250ms 超时（fence=2）——归目标独显环境，非产品缺陷。
3. F03/F02 档无量子归档（超时路径不导出）。

## 交付

- 证据：`test/reports/t868/`（8 轮 JSONL + 4 个 source 原档 + fixtures + summary.json）
- 原始场件：`test/reports/t868/fixtures/{F02,F03,F05}-config128.bin / -state.bin`（SHA 见 fixtures.json）
- commit：见 git log（T868 evidence only）
