# T346 · V2 粒子不沉降离线 1:1 复现诊断报告（决定性判别实验）

日期：2026-10-31。责任：developer（鸿蒙开发工程师）。任务卡：滴答 `6ae5af24e4b0e9cb68f4bd3a`。
合同依据：docs/architecture-T345-v2-no-settle-attribution-v1.md（§3 Harness 规格、§5 修复路由）。
状态：**OFFLINE_NOT_REPRODUCED / DEVICE_LAYER_EXCLUDED_CODE_SIDE / ROUTE_TO_TESTER_DEVICE_EVIDENCE**。

## 0. 结论（二选一，无含糊）

**离线不复现**。V2 全链在 Node 1:1 执行下物理完全正常：粒子从 y=220 单调下降、着陆入床（mh 增长）、幸存粒子静息在床面。断点不在代码层，按 T345 §3 判据归入**设备层**。但设备层根因已有**强证据定向**（见 §3）：不是传感器/生命周期旁路，而是**帧率坍塌导致模拟时间不足**——T343 观测窗口内设备累计模拟时间 ≤3.3s，小于 220 单位落差所需 3.44s，粒子物理上不可能完成沉降。

## 1. Harness（1:1，零移植偏差）

- 链：`P051DerivedFacadeV2`（V2 facade）→ `WaterSandField` + `P011ParticleHot/M2Field/M2Sink` + derived/ 全部文件——**直接 require 真实 ArkTS 源码**（TypeScript transpile，与 T216 七套件同机制），不是重写。
- 序列（T345 §3 逐字）：`new P051DerivedFacadeV2({nmax:512,...})` → `inject(×3, requestedH=8×sec, y=220, x=64, seed=51)`（sec 取 T343 实录 0.0777/0.1329/0.1440）→ `step(1/60)×600` → 每 60 步转储五项。
- 产出：test/t346_v2_no_settle_repro.cjs + test/reports/t346/harness-run.json。

## 2. 五项转储结果（10 窗）

| 窗 | 步 | live | pYmin/mean/max | motion | req | mediumMode | dStatus | mh |
|---|---|---|---|---|---|---|---|---|
| 1 | 60 | 30 | 215.02/215.02/215.02 | 30 | 0 | DRY_AIR_V1 | 0 | 0 |
| 2 | 120 | 30 | 200.24（同） | 30 | 0 | DRY_AIR_V1 | 0 | 0 |
| 3 | 180 | 30 | 175.66 | 30 | 0 | DRY_AIR_V1 | 0 | 0 |
| 4 | 240 | 30 | 141.27 | 30 | 0 | DRY_AIR_V1 | 0 | 0 |
| 5 | 300 | 30 | 97.09 | 30 | 0 | DRY_AIR_V1 | 0 | 0 |
| 6 | 360 | 30 | 43.11 | 30 | 0 | DRY_AIR_V1 | 0 | 0 |
| 7 | 420 | 25 | 0.577（着陆） | 0 | 25 | DRY_AIR_V1 | 0 | 0.577 |
| 8-10 | 480-600 | 25 | 0.577（静息） | 0 | 25 | DRY_AIR_V1 | 0 | 0.577 |

- (a) liveCount 30→25、pY 单调下降至床面 ✅；(b) motionCount=30 活跃收集 ✅；(c) mediumMode 恒 `DRY_AIR_V1`（§2 c' 检查点排除）✅；(d) derivedV2.status 恒 0、barrier 无 fault ✅；(e) mh 0→0.577 床增长（LAND 落地）✅。
- 注入三笔全部 `ok=true adm>0 rej=none`（0.6216/1.0632/1.152，与 T343 实录逐笔同量级）。stepFailures=0。
- 物理时间线：pY=215@1s → 43@6s → 着陆@~6.5s——**220 落差需 ~6.7s 模拟时间（重力加速段 + 终速段），600 步(10s) 足够**。

## 3. 设备层根因定向（T343 hilog 重析，非新采）

对 T343 双进程 hilog（hilog_full.txt=进程48139、hilog_v2.txt=进程43905 前代）按 1s 节流行重建真实 tick 率与模拟时间（test/t346_device_time_reconstruction.cjs、test/t346_v2log_recon.cjs，证据 JSON：test/reports/t346/device-log1-full.json、device-log2-v2.json）：

| 进程 | 阶段 | 实测 tick 率 | 现象 |
|---|---|---|---|
| 48139 | 15:23:05-24（注入前 45s） | 12→10 fps | 空场即只有 ~12fps（非 60） |
| 48139 | 15:23:50 起注入 15 笔 | 7.4→2.1 fps | 注入开始即崩 |
| 48139 | 15:24:05 后（89s 墙钟） | **0.5 fps** | 每帧 2 substep、dt 钳 1/30 |
| 43905 前代 | 15:14:10 注入 13 笔 | 12→0.7 fps | 同样模式 |

**模拟时间重建**：进程 48139 自首注入至末帧（108s 墙钟）仅 98 ticks → substeps=211−15 注入=196 → 98×2 substep × (≤1/60) = **≤3.27s 模拟时间**；前代进程 50s 墙钟仅 **≤2.53s**。**两代均 < 3.44s（220/vTerm=220/64 落差时延）**——粒子在观测窗内物理上尚未走完下落行程，截图 s1≈s4「位置不变」= 慢速下落被 1s 级采样冻结的视觉假象 + fps 坍塌后近乎停止。床恒零同理（未触底则无 LAND）。commit 1157+ 连续推进与「粒子不动」不矛盾：每 tick 都在执行，只是每 1.4-2s 才有一个 tick。

## 4. 帧率坍塌本身是第二问题（性能）

12fps 空场起步（V2 barrier 每步 verifyPublicBoundary 的 256×70 精确累加器逐位对拍 + matchesAuthority 13×2 buffer 全量逐字节比较，均为 O(大常数)/步）→ 注入后每 tick 又叠 30 粒子 collect/apply ×2 substep → 0.5fps。此为 V2 包装层的**性能缺陷**（正确性无损——本 harness 证明数值全对），需后续性能卡处理，不在本卡范围。

## 5. 修复路由（按 T345 §5）

- 本卡产出二选一结论 = **离线不复现 → 设备层**。不采 tester 修法红线未动。
- **真机补证接点（T345 §3 指定）**：T218DBG 增 step 行（每 60 帧一条 `pYmin/pYmean/live/motionCount`），同批真机卡采集，验证设备模拟时间重建与粒子慢速下落。
- **新增证据建议**（超出 T345 预期、供 architect 裁决）：帧率坍塌观测（12→0.5fps）+ 模拟时间重建（≤3.3s < 3.44s）已构成「设备层=时间预算不足」的完整证据链；性能归因（barrier 校验开销）供 architect 派性能卡。

## 6. 非机回归（全绿）

- 本 harness 固化为回归用例：test/t346_harness_regress.cjs（T346-R1 沉降断言 + T346-R2 mediumMode/barrier 健康断言，2/2 PASS）。
- T216 七套件 7/7（selftest.json passed=7 failed=0）。
- T216 regression 十单元 10/10（regression.json 全 exit=0）。
- T342 strict pack：mainchain_verify.cjs 600 步严格域 + 反例对照 REJECTED_AS_ARCHIVED，switch_readiness 4/4。
- 红线核对：未改任何物理 dt/阈值/守恒；未改 renderer 严格语义；未动 T342 exact70；未改任何源码（纯诊断+测试资产）。

## 7. 交付物清单

| 文件 | 内容 |
|---|---|
| test/t346_v2_no_settle_repro.cjs | 决定性 harness（T345 §3 规格） |
| test/t346_harness_regress.cjs | harness 固化回归（2 断言组） |
| test/t346_device_time_reconstruction.cjs | 进程 48139 模拟时间重建 |
| test/t346_v2log_recon.cjs | 前代进程模拟时间重建 |
| test/t346_v2log_fps.cjs | 前代进程 fps 时间线 |
| test/t346_pack_device_evidence.cjs | 设备证据打包器 |
| test/reports/t346/harness-run.json | harness 五项转储（10 窗全量） |
| test/reports/t346/device-log1-full.json | 进程 48139 fps 行 + 重建 |
| test/reports/t346/device-log2-v2.json | 前代进程 fps 行 + 重建 |

## 8. 变更记录

- 2026-10-31 v1：初版。harness 建成并执行（离线不复现，全链数值正常）；双进程设备日志重析（帧率坍塌 12→0.5fps + 模拟时间 ≤3.3s < 3.44s 落差时延）；结论=设备层时间预算不足，代码链无断点；真机补证接点移交 tester，性能归因供 architect。
