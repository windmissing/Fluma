# T487 · GPU M2 受控 BASE 数学与聚合补证（v1 阶段：112 件既存原件勘误 + per-origin 聚合补证全过；受控 BASE GPU patch 场景待续）

2026-11-13，tester，离线不使用真机，commit `1de3340`（工作区）。按 T484 工程方案（`docs/architecture-T484-m2-controlled-base-fixtures-v1.md`，355d632）执行。本报告覆盖任务书 §5 的前半：既存 112 件原件 manifest/SoA 解码、N1c/N3/N4 与报告解释勘误、聚合补证；**§3 受控 BASE GPU patch 数值场景（tie/nearest/radius/RNE 等 8 场景）未在本轮执行**——T479 host（`.codex/local-test/t479-host/`）的 shader 为旧版 eb479a7b，按 T484 §5 要求新 GLSL 必须用 T482 coherent 修复冻结版（b7d0a027，已核产品 `assemble_kernels.py` 组装输出 = b7d0a027 一致），host 编译域扩展与 GPU 执行留下一轮，不冒交。

## 一、既存原件勘误（T479 报告解释纠偏，全部 PASS）

| 判据 | 结果 | 说明 |
|---|---|---|
| A1 原件清点 | PASS | `.codex/local-test/t479-c` 实存 **112** 个 bin（T479 报告称 105 件）：24 组 snapshot（bank/pub/work-control/work-origins）96 件 + command/completion 16 件；逐件 bytes/SHA256 已入 `test/reports/t487/t487-correct-aggregate.json` manifest |
| B1 SoA 解码 | PASS | 地址公式 `field*K+slot`（K=192）经 n1-mid/n1-after 原件逐字段核（mass/x/y/vx/vy/id/tag/sourceId/origin/generation/wait/medium）；T479 报告「同列两次 SOURCE 无 merge 说明超出 radius」**勘误**为「decide 先找最低空槽，满池才 STEP_PAIR；两次接受 SOURCE 的 origin 分别 1/2，未运行 pair 选择，不能证明 radius/tie/RNE」；「输入 y 不是出生 y」**再勘误**为「command binary64 经原 Q20 floor 后直接赋出生 y（SoA y 区=2*K 全 0 与 shader outRow[2]=y 一致），T479 把 SoA 当 AoS 扫描误读」 |
| C1 N1c checked | PASS | 原 `numeric_audit.py:84` `or True` 无条件通过已废弃；checked 判据 = `mutationSerial U64 差值 == 本 root 的 MERGE+BIRTH`（DIRECT/END 不计）：n1 delta=2==0+2、n4 delta=2==0+2、n6 delta=4==2+2（n6 基线取 n2 满池 mut=192） |
| D1 N3 勘误 | PASS | x=288.5 超出 x 合法域 [0,128)Q（134217728）→ 输入 reject 65538，是**输入域负控**而非 pair 半径负控；半径作用于 y 差，真正半径场景归 GPU patch 阶段 |

## 二、聚合补证（无需等待注入，T484 §4，全部 PASS）

判据 = 逐槽（live 槽按 originLo）重建 Σmass（**先算，独立期望**）与 WORK sourceOrigins 区精确偏移 `2*(origin-1)` 的 U64 对原件读回（**后对**）逐位对照；不扫全数组。

| 原件 | 逐槽重建（期望） | WORK 区实际 | 槽数 | 判定 |
|---|---|---|---|---|
| 17-n1-after | origin1=131072, origin2=131072 | 一致 | 2+2 | PASS |
| 23-n4-after | origin1=131072, origin2=131072 | 一致 | 2+2 | PASS |
| 19-n2-after | origin1=23487936（192×122333） | 一致 | 192 | PASS |
| 24-n6-after | origin1=23487936（**merge 不减来源质量**），origin2=131072 | 一致 | 190+2（两个 244666 merge 粒） | PASS |

F1 高字零判据 PASS（全部 U64 对高字=0，符合本版本 audit 要求 source 序号高字 0/每源准入≤CQ；不构造不合法高字正例）。G1 期望冻结顺序 PASS（期望来自 bank 侧逐槽求和，实际来自 WORK 区读回，独立来源，无 completion 反推）。

## 三、结果与边界

- 总判定 **10/10 PASS**（A1/B1/C1/D1/E×4/F1/G1）；报告 JSON：`test/reports/t487/t487-correct-aggregate.json`（含 112 件 SHA manifest 与聚合明细）。
- 脚本：`test/t487/correct_and_aggregate.py`（独立判读，未改 T479 `numeric_audit.py` 原件——按 T484「不重开已交 T479」边界，勘误以本报告与本脚本为准）。
- **未执行（下一轮，同卡续做）**：T484 §3 受控 BASE 8 场景（同距离 tie / nearest 非最小 b / radius −1/0/+1 / 非 eligible 夹中 / RNE 正负 tie / 抵消与大乘积 / x-y 非整数中点 / merge 后重选）需在测试域扩展 T479 host 的 `SourceFixture`（`edit`/`read` 已具备 `glBufferSubData`+barrier 能力），patch 已接受 bank 的 live 槽 x/y/vx/vy（SoA `field*K+slot`），用 T482 修复冻结版 shader（b7d0a027）重编译 host 后 GPU 执行；独立期望由 `test/t446/m2_oracle.py` 的 brute_pair/merge_fields/RNE 先冻结。
- **未冒交**：fixtureWitnesses（T484 证据索引）6 pair+6 RNE 均为 SPEC_WITNESS_ONLY，本阶段未 GPU 执行，不计产品 PASS。
