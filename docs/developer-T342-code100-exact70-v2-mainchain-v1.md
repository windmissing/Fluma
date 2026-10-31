# T342 · code100 材质故障修复（exact70 V2 主链接入核验）交付 v1

日期：2026-10-31。责任：developer。任务卡：滴答 `6ae5877fe4b0f69bc58437e8`。
依据链：T202 `b9f15aa`（跨模块合同不一致归因）→ T207 `f2b3e83`（exact70 数值 V2 设计）→ T214 `c280602`（工程化）→ T216 `4d3584d`（实施）。manager 2026-10-31 方向变更裁定：CPU 仿真线最高优先。

## 1. 主链接入核验（三件之一）

**核验结论：V2 主链（P051DerivedFacadeV2 隔离 owner → WaterSandField.enableDerivedV2Cold → DerivedCoordinatorV2 全 writer 屏障 → updateSandTop 走 derivedV2.flush()）在 exact70 V2 下不再产生跨域微负残差。**

逐条对位（T207 §2 严格域）：

| T207 设计要求 | 实测（N192/512 各 300 步=600 步逐帧） |
|---|---|
| 床四库存有限且各 ≥0 | 全程成立（每列每步断言） |
| suspH/L=0（dry 合同） | 全程成立 |
| cache finite/nonnegative、空列严格 +0 | exact70 整数累加器非负保证 + RN64 空集 +0（DerivedViewV2 roundAt + count==0 ⟺ value==0 不变式断言） |
| 包络严格 top≤envelope≤capacity | DryDomainV2 flushBeforeRead 逐列：`mix=bh+bl, top=(mix+mh)+ml, P=cacheH+cacheL, envelope=((top+suspH)+suspL)+P`，与 T207 §2 完全同序 |
| 严格拒绝语义不放宽 | code100 归档反例经同一 renderer 仍被拒（见 §2）——修复在物理链，renderer 零改动 |

**消费域核验**：`copyPublishedSceneStateInto` → DerivedCommitV2.copyPublishedInto（seal 匹配）→ P013FrameAdapter 严格谓词（`bottom<bh≤mix≤top≤water≤capacity`，mask 16/32/64/128/256 逐位）——publication 消费域与 T207 设计一致。六份 strict pack（N192/512 × step 0/123/299）全 OK。

## 2. code100 复现反例（三件之二）

从 T180 `0a6fe16` 同 frameToken 失败材料快照（`docs/analysis-T202-reassembled-material-v1.json` 20 数组 rawHex）解出完整输入：col 57 `waterTop=colParticleMassH=−3×2^-54`（F64 bits `0xbca8000000000000`）、床全零、128 列中仅此列负——导出为 `test/t342/code100_input.json`。

**反例验证（mainchain_verify.cjs Part 2）**：
- 归档输入重建 V1 形态 state（numericPolicy 未定义=V1、无 seal）经同一 `P013ScenePreparation.pack`（带 diagnosticFrame）→ **仍被拒**：`SCENE_MATERIAL_DATA`、column=57、predicateMask=128（top>water）、stage=SOURCE_VALIDATION——与 T202 归因逐字段一致，**renderer 严格语义零放宽的反例对照成立**；
- V2 主链 600 步从未产生该形态（每帧严格域断言通过）——**code100 输入条件在 exact70 V2 下不再可触发**。

## 3. 非机回归（三件之三）

| 套件 | 结果 |
|---|---|
| T216 全套（run_t216_all：exact70/tuple/owner/checkpoint/publication/regression/allocation） | 7/7 套件 exit 0 |
| T216 六份 strict pack 复跑 | 6/6 OK（N192/512 × step 0/123/299，`strict-pack-rerun.json`） |
| T342 主链核验（600 步逐帧严格域+6 pack） | 全绿（`mainchain-verification.json`） |
| code100 反例对照 | REJECTED_AS_ARCHIVED（column 57/mask 128） |
| T342 切换就绪 S1-S4 | 4/4（见 §4） |
| 生产 hvigor（强制重编） | CompileArkTS 8.7s+PackageHap 绿（SignHap 00303116 已知如实保留） |

## 4. V1→V2 生产切换准备（三件之四）

**S1** V1 生产默认不变：EmptySandDemo 模型仍是普通 WaterSandField（无 V2 facade），V1 默认 state 过 renderer gate ✓
**S2** V2 显式入口可用：P051DerivedV2Demo 页注册在册；facade 仅接受 V2 policy 四元组（isolated/EXACT_V2/CPU_ORIGINAL_F64/T216_DRY_WRITERS_V2），错配显式抛 DERIVED_CONFIG_UNSUPPORTED，无静默回退 ✓
**S3** 旧 GPU 路径拒 V2：TxPairBackends 在 native configure **前**抛 BEFORE_CONFIGURE（configured=0 实证），如实报错不降级 ✓
**S4** renderer gate：V2 声明无 seal 的伪造 state 被拒 ✓

**切换决策：DEFERRED_TO_T343_DEVICE_CONCLUSION**——V1 生产默认不撤销；T343 真机 D1-D6 结论为门。

## 5. 红线遵守

- renderer 严格拒绝语义零放宽（无 EPS/max/钳零/H 放宽）——反例对照实证；
- 失败 FAULT 或真实 STALE 完整旧帧（本次未触碰）；
- 不改物理 dt/阈值/守恒（T331 增量证书等的冻结约束原样）；
- 无 GPU 方案新增投入（T279/T280/T281 保留现状）。

## 6. 包签名状态

本卡纯非机，**无手签包**（SignHap 00303116 已知失败如实保留，未手签）。T343 真机复测的包由执行 tester 按 T151 流程补签，三件套=手签包+sign/verify 原始输出+SHA256 归档。

## 7. 证据清单（test/reports/t342/）

`mainchain-verification.{txt,json}`（600 步主链+反例）、`switch-readiness.json`（S1-S4）、`t216-all-suites.txt`（7 套件）、`strict-pack-rerun.json`（6 pack）、`production-build.txt`；`test/t342/{code100_input.json,mainchain_verify.cjs,switch_readiness.cjs}`。

## 8. 变更记录

- 2026-10-31 v1：初版。主链接入核验（600 步逐帧严格域+六 pack）、code100 反例（归档输入仍被拒=renderer 零放宽+V2 链不再可触发）、T216 全套 7/7+六 pack 复跑、切换就绪 4/4（决策延至 T343）、生产构建绿。
