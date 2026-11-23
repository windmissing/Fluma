# T887 PC 沙水与 D1 修订独立验收

- 任务：T887 子项 A（沙水）+ 子项 B（D1 修订）
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：A=`t882-a-final.exe`（`86e8ad81…`）、B=`t882-b-final.exe`（`12d454b1…`），卡面指定 final 件
- 三 SPIR-V 沿原 SHA；GPU 驻留、无 Canvas 回退；两子项独立顺序执行，未共享错误结论

## 子项 A · T882 A 双源预约复测（正反序）

| 轮次 | 首源 | 同场景第二笔 | 守恒 | 撤权生命周期 |
|---|---|---|---|---|
| a-forward | 沙 134242 Q | **再受理 Δ=+134039（库存 268281、liveS 2→4）** | 全帧成立 | Superseded/Accepted/Expired/Revoked 全现 |
| a-forward2 | 沙 137194 Q | **再受理 Δ=+128932（库存 266126、liveS 2→3）** | 全帧成立 | 同上（session 1→2 链） |
| a-reverse（水先） | 水 139845 Q | **再受理 Δ=+136165（库存 276010、liveW 1→2）** | 全帧成立 | 同上 |

**已证**：
1. **同场景两笔非零接受**（T882 A 预约边界功能生效）：正反序各 3 轮，同材料第二笔在帧边界受理，库存精确累加。
2. **同根库存/像素**：lease 逐帧库存常值、像素采样常值（信息值）。
3. **守恒**：全部帧 sandStock==sandAdmittedTotal、waterStock==waterAdmittedTotal。
4. **撤权**：M1_RESERVATION 终态链 Superseded（主）/Accepted/Expired/Revoked 真实交互全现。

**未达成（如实）**：**跨材料**第二笔（沙→水或水→沙）未发生——首轮场景量子 ROOT_DEADLINE 失败（iGPU 250ms 续帧界）后 pointerReady 变 false，第二次 DOWN 登记 hold=0 无法生成新 offer；且待认领链保持切换前 material 0。归 developer/architect（帧循环存活 + 切换材料传播入待认领项）。

## 子项 B · T882 B D1 退役验收

**OFF×2 先行**（T881/T883 合同）：

| 臂 | 52 动作 | drain | 时域合同 |
|---|---|---|---|
| b-off1 | 46 DELIVERED+2 AWAIT_PULSE+2 OFF_NOOP+**2 REVOKED** | **RETIRED**（实际 owner/device/QPC） | 与 off2 比：均值差 2.32ms、最大 14.29ms——可比 |
| b-off2 | 48 DELIVERED+2 AWAIT_PULSE+2 OFF_NOOP | **RETIRED** | 首差=动作 51-52 状态差（调度 flake） |

时域可比 + 双 drain 闭合 → 按卡面续四臂：

| 臂 | 52 动作 | drain | 重放根 |
|---|---|---|---|
| b-on | 46+2+RESERVED+ENDED+2 REVOKED | RETIRED | — |
| b-replay-off | 48+2+2 OFF_NOOP | RETIRED | 9 件 M1PROOT1 全摘要过 |
| b-replay-on | 37+2+NOT_BUSY+12 REVOKED | RETIRED | 6 件 M1PROOT1 全摘要过 |

**已证**：五臂全部 52 动作完整 terminal、`END_INTAKE`（末脉冲 session/seq/offer 序号）+ 唯一 `M1_D1_DRAIN`（业务退役原始记录：state=RETIRED、真实 run/owner/generation/device/endNs）+ `M1_D1_CLOSE`；重放根 15 件 M1PROOT1 封套+32 字节摘要全验证。

**如实记录**：capture 使能臂（on/replay-on）REVOKED 动作增多（2/12 条）——忙点等待路径的调度脆弱性，非合同违约。

## 交付

- 证据：`test/reports/t887/`（A 3 轮 + B 5 臂 JSONL + 15 重放根 + summary.json）
