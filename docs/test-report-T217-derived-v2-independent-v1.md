# T217 派生量 V2 独立验证报告（v1）

日期：2026-10-28；tester；不使用真机。被测：T216 `4d3584d`（exact70 derived V2 writer/publication/checkpoint domain）。依据 T214 §10 DD01–12 接力规格。执行器 `test/p217_derived_v2_independent.cjs`，原始 JSON `test/reports/t217/independent.json`。

## 结论

**8 PASS / 0 FAIL / 1 NOT_RUN（DD-12 设备接力）**。T216 开发自测基线复跑全绿（exact70 3208/0、tuple 7/7、owner 310 roots、publication/checkpoint PASS）。

## 分项

| ID | 内容 | 结果 |
|---|---|---|
| DD-01 | 原 T202/T208 证据保持：本卡未改任何既有断言文件（T216 dev 声明+本卡核） | PASS（保持性核） |
| DD-02 | **独立 BigInt oracle**（自实现 2^-1074 单位编码 m×2^(e-1)/subnormal m、ties-even RN64 位型构造，自写种子 0x7ee1a7/LCG 不同参数）：15 边界值（MIN_VALUE/MAX 精确/半舍两侧/1e-300/subnormal/±0/255.9375）+ 1200 组随机三累积二消去逐位对拍 | PASS |
| DD-03 | 闭合 facade 配置拒绝：non-isolated/V1 policy/未知 backend/未知 registry/nmax<2/nmax>513/sourceId=0 全部 DERIVED_CONFIG_UNSUPPORTED（7/7） | PASS |
| DD-04 | 实际 tuple writer 经 facade 全程：3 源准入 + 60 步 + counters 32 项 finite | PASS |
| DD-06 | 源成功前缀：NaN dt 安全 no-op 不破坏状态；重复 sequence 被拒 INVALID_IDENTITY 且首源保留 | PASS |
| DD-07 | strict dry domain：5 源 + 300 步，逐步 published copy OK，envelope finite ≤256、top∈[0,256] 无违反 | PASS |
| DD-08 | publication 身份：同 owner 两次 copy 内容逐字节一致；冷导出为独立复制（改导出 [0]=999 不污染后续导出） | PASS |
| DD-09 | renderer gate 离线：经真实 P013ScenePreparation 严格打包（未替换谓词），copy OK | PASS |
| DD-11 | counters 全 finite 非负 + work 表行存在 + allocation ledger 诚实标记（NOT_TOTALLED/UNBOUNDED_EXTERNAL）在档 | PASS |
| DD-12 | 设备执行规格（T218 接力） | NOT_RUN（另卡） |

## 独立性声明

- DD-02 oracle 完全自写（编码单位公式与 T215 一致、RN 位型构造独立实现），**不调用生产 Exact70 做参考**；随机流与开发测试（种子 216/LCG 1664525）不同。
- 不引用 T207/T216 设计模型作产品判定。
- DD-04/06/07/08 经公开 facade 合同路径（`inject/step/createPublishedSlotCold/copyPublishedRenderStateInto`）；仅 DD-07/08 读取 published slot 的 `['state']` 反射访问（Node 测试口径，与 T216 交付声明一致，非生产 hook）。

## 边界

- 覆盖 DD-01/02/03/04/06/07/08/09(离线部分)/11/12；DD-05（F64 图条件等价与数值 policy 差异分表）与 DD-10（生命周期/route/migration 隔离的完整矩阵）的剩余子项在 T216 自测已覆盖（v1-baseline 6 命令×3 backend、checkpoint 篡改/坏版本/重复恢复），本卡为窄回归未重跑宽矩阵，如实登记。
- 真机 DD-12（同 commit 场床/live 粒子两态可见性、N192/512 service/峰值）归 T218 设备接力。
- failure 注入矩阵（T214 §10 列出的 11 个固定注入点）经 facade 层面覆盖 DD-06（重复注入拒绝/前缀保留）；私有 faultPlan 全 11 点矩阵属 T216 dev 自测（checkpoint/publication 注入已核），本卡独立复核了 facade 可达子集。
