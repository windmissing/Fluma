# T652 v1 · DM 与组合接受合同独立反例基准（T647 冻结合同消费）

日期：2026-11-19，tester，不使用真机，无 CPU 沙/流体 stepper，无 architect/
designer 检查器 import。
依据：`architecture-T647-v1.md`（ENGINEERING_DEFECT_CONFIRMED /
CONTRACT_READY_FOR_DEVELOPER）+ 冻结机器表 `architecture-T647-acceptance-v1.json`
（10 rows，原字节 SHA 绑定于基准 JSON）。
证据：`test/t652/counterexample-base.json`（family 索引 + 10 反例 + 3 正控 +
字节前像 SHA）。

## ① family×root 完整身份编目（10 行，kind 显式从不由 count 推断）

| family | SOURCE(rk1) | QUANTUM(rk2) | audits | magic | rev |
|---|---|---|---|---|---|
| R1_DEFAULT | 12 | 2024 | 0xffff | 826489159 | 1 |
| DM_DEFAULT | 12 | 2404 | 0x3ffff | 843269191 | 2 |
| G11_R1 | 19 | 2543 | 0xcffff | 1127297351 | 2 |
| G11_DM | 19 | 2923 | 0xfffff | 1127297351 | 2 |
| G9_RX | 12 | 2404 | 0x3ffff | 843269191(GNR1 profile) | 2 |

R1 默认不放宽（0xffff 保留）断言在独立编目中固化。

## ② 字节前像计算（1024B tail 独立 LE 模板）

独立实现 T647 §5 结构：w[0]=magic、[1]=rev、[2]=words、[3]=rootKind、[4]=profile、
[39]=[40]=真实 dispatch、[41]=barrier、[42]=精确 requiredMask、156..239 仅
G11C/rev2 扩展（contractRevision/parentKind/双 profile/base+proposed 视觉代/tick）、
240..255 保留零、尾 seal 槽。正控最小完整前像三个（P01 R1 SOURCE / P02 DM
QUANTUM / P03 G11_DM+扩展）SHA 已记录。

## ③ 反例目录（10 例，逐例绑定合同条款）

| id | 反例 | 条款 |
|---|---|---|
| N01 | G11_R1 尾喂 R1 reader（magic 错配） | §2 旧 reader 明确拒收 |
| N02 | SOURCE count 12 冒 QUANTUM（12≠2404） | §2 kind 显式/count 校实际 dispatch |
| N03 | R1 尾带 DM 扩展 mask 0x3ffff | §2 R1 不放宽/未知 bits 拒 |
| N04 | DM 旧 rev1/0xffff 喂 rev2 reader | §2 旧产物不能热续接 |
| N05 | 保留字 248..251 非零 | §5 240..255 零 |
| N06 | DM 尾带 G11 156..239 扩展 | §5 默认 DM2/RX 全零 |
| N07 | 中点错标 BLOCKED(4)（应 28=PATH_BOUND，细分 7/6/5） | §3 enum 不加宽 |
| N08 | 视觉 tick 用 quanta×32（rest 单位混用） | §5 ×256 checked |
| N09 | SOURCE duplicate2 带新 commit/vGen | §5 duplicate 不 mutation |
| N10 | frozenExpectedSHA 含自身（selfhash 环） | §5 不含自身/尾字节 |

## ④ enum 独立编目

canonicalReasons 31 项完整编目；blockedReasons 1..18 保留不放宽；28=PATH_BOUND
（中点谓词细分为 7=NEGATIVE_OR_EMPTY_WITH_MOMENTUM / 6=VELOCITY_DOMAIN /
5=CFL）；29=OVERFLOW 硬错不转 BLOCKED4；30=撤权按 owner 取消处理。

## 边界如实

T648/T639/T644 产品未交——不调用虚构接口、不把预测试当产品 PASS；后继接
力=真实 DM 验证 T569、组合 T600、新 RX T645（T642 基准已就绪）。本卡交付=
独立反例目录/解析 + 字节前像计算 + 测量格式 + 有限自检 + 后继 product seam
需求（machineTableSHA 绑定冻结表原字节，后续真实验证逐 word 对照）。
