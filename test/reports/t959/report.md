# T959 · G15 干前沿与 G16 同 fence 晚尾集显复测（effector）

2026-11-24 · 开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。GPU 驻留、shader 渲染，无 Canvas 回退，250ms 业务门原值。工件：`.codex/local-test/t955-g15-package`（`bd66e841`，roleSet `7d92f63e`，check-package 过，ops 25/27/29 干前沿候选重制）与 `t955-g16-release`（`4e619c8e`）+ 新生成 `t955-g16-seeds`（d74e416c/eca4c474 与 T928 合同 SHA 一致）。

## ① G15 · 负臂干前沿原字 + 正反砂丘

**负臂（mirror −45°，`--quantum-timestamps --raw-envelope`，24.2s）**：

| 判据 | 旧 T947 基线 | 新 T955 实测 |
|---|---|---|
| COMPLETION word6/7 | (4, 7) 物理 reason 7 | **(1, 0)** 裁剪型、reason 0 |
| 首错（stage/micro/col/operand） | 26/0/19/40 | **全零** |
| J 档（120+6i） | 六档全 (j,4,26,0,19,40) | J1=(1,1,0,0,0,0)，其余零 |
| 列 19–21 F | (0, 0, 1395, 1395) 冻结 | (0, **805, 42473, 42473**) 演化推进 |
| 面 19 entry/trial/end | (0, −64868, −64868) | **全零**（干前沿共享零冲量 live） |
| 面 21 湿边 | 64868 承载 | impulse 1,924,219,444（双侧湿面合法承载） |
| column_stage 19 试量 | trial −64868 | 0（静止掩码试量随零有效冲量重读） |
| selectedJ | 1 | 1 |

**干前沿物理门消除实证**：T952 分期通量规则在真实 GPU 运行中生效——col 19 干前沿共享零冲量、面 21 湿边保留原压力耦合、前沿不冻结（F 从 1395 推进到 42473）。

**正反砂丘**：neg/pos/neg2/pos2 四跑全部 `vk=2 GRANT_DEADLINE`（2404 派发 250ms iGPU 超窗，T947/T954 三证同款）、completion (1,0)、首错全零——**正反对称，无物理拒绝**。业务拒绝只剩 iGPU 窗口（前置=独显或减派发变体）。

**同根画面**：不可达——typed 接受需要 2404 派发在 250ms 内完成（iGPU 不可达），`--present` 无接受根不产出；如实登记。

## ② G16 · B13 全域余时 + B52 原 fence 晚尾

`diagnostics` 入口三臂（seeds 按 T928 合同重生成）：

| 臂 | 结果 | 余时诊断 |
|---|---|---|
| B13 static 128×2 | reason 11/9、B=200 C=27 **Ic=32**、19.1ms fresh | `remaining-v1`：record0 micro15/seg0 hit=940,253,321ns→remainingOut 939,913,353；record1 journal[9..18] hit=935,867,717→remainingOut 923,228,058；**completedMicrosteps=15、rescanState=1**（自由漂移资格线索）、word63=0 |
| B52 static 256×1 | 超窗 255.9ms fresh=false | **`G16_PRIVATE_LATE_TAIL`**：identityOK=true、firstReason 11/9、segments=87、iterations=464、candidates=134；**`late-completion.u32le`** 原字（words 40-47: 2,11,9,58,1）；timing：wait 254.96ms→**late 等 114.6ms lateVkResult=0**（迟到完成到达） |
| B52 fast 256×1 | reason 11/9、B=800 C=134 **Ic=464**、239.6ms fresh | record1 hit=940,006,492→remainingOut 882,251,392；completedMicrosteps=58、**rescanState=2**（事件见证） |

**不晋升迟到根（三臂一致）**：exit 3、`acceptedRoot=null`、`G16_PRIVATE_TIME_EVIDENCE` 行 originalGeneration=1（热 fence）与 coldGeneration=2（冷诊断）**分开记录**、evidenceSHA 封存——T955 合同逐项满足。

**GPU 细段**：`G16_PRIVATE_TIMING` 行含 ROOT_BEGIN/FREEZE_END/MICRO_32_END 等完整点集 + record/submit/wait/late 四段 QPC + 36bit×83.333ns 周期。

## 结论表

| 子项 | 判定 |
|---|---|
| G15 负臂干前沿原字 | **过**：T952 规则 live 生效——4/7 物理拒绝消除（col19 零冲量+湿边承载+前沿推进），原字全档 |
| G15 正反砂丘原门 | **过（负臂语义）**：正反 4/4 对称无物理拒绝、业务门只剩 iGPU 窗 |
| G15 同根画面 | **不可达**（iGPU 250ms 窗，前置独显/减派发） |
| G16 B13 全域余时 | **过**：两事件段记录+重扫 state 1 |
| G16 B52 晚尾+GPU 细段 | **过**：LATE_TAIL+late-completion 原字+114.6ms 晚到确认+完整时标点集 |
| 不晋升迟到根 | **过**：三臂 acceptedRoot=null、热/冷 fence 代分开 |

证据根 `test/reports/t959/runs/`（g15 四臂 raw+jsonl+g15-result、g16 三臂 diagnostics 全件+jsonl+g16-result）。
