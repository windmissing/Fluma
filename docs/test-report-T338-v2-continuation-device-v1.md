# T338 V2 续证工程三件设备复核（M1-M4）（真机，完成——M3 NOT_RUN 如实）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T331 交付内容（混合落盘于 commit 3435c28，归属注记 26184ab）。

## 0. 包身份与取证工装变更（T151 三件套，两处如实登记）

- **T338b-signed.hap SHA256 `7aafce3f628c3bd7a9d147b4b2f2a9c06da1bf24f5f8ba091792ca2756b439e1`**（HEAD `e8d70d1` + 工装 wiring，localSign + verify-app PASS）。
- **工装变更 1（T311_STAGE 观测通道恢复）**：`EmptySandDemo.ets`（P051 粒子路线页）补 `configureParticleComplexityObservation` + 1Hz `pumpParticleComplexityCold`——**该通道在所有入库树中均未接线**（git log -S 全历史仅 f2f177a 的 physics 侧定义，pages 零调用；T312 的 T311_STAGE 行来自其私有 wiring 构建）。观测语义，不改物理/审计。
- **工装变更 2（T337b 包，同日）**：`SandCanvas.ets` 池诊断按钮 onClick 补同一通道 + setInterval pump（主工程粒子路径页）；本轮 SandCanvas 路径实测 R=0（主工程注入走 wet 路径、粒子未 born——live=0），遂改走 P051 粒子路线页完成采集。

## 1. M1 watchdog 死亡族设备复核：**PASS**

- **T311_STAGE 真机行**：`R=148, attemptCumulativeMs=150, B=399, N=512, live=250, retry=0, bytes=18.4MB`（counts/totalMs/maxMs 全 20 stage 在）。
- **持续注入 ~4 分钟**（pan swipe 批次×多轮），**跨 3 个 run（runId -1/-2/-3），pid 42629 全程不变**；`THREAD_BLOCK_6S=0`、jscrash=0。
- 对照 T312 死亡基线（R=261 时 audit 1165ms + sort 777ms 累计 → THREAD_BLOCK_6S 杀）：本包 R=148 时 attempt 总耗时 **150ms**（13× 余量）；T331 非机 R320=68ms 互证。
- 附注：T311_STAGE 有界 emit 封顶 32 行（`emitted===32` 预算），封顶后 R 读数不可得；R>300 的 attempt 累计由线性 ~1ms/R 外推 + T331 非机 R320=68ms 支撑（如实披露，不冒充实测 R300+）。

## 2. M2 单事件 ≤600ms：**PASS**

T311_STAGE 行 maxMs 全 20 stage **≤5ms**（对照 T312 基线单事件 1449ms@R261）——增量证书+sort 消除后单事件 3 个数量级改善。

## 3. M3 冷采 captureMs ≤4ms：**NOT_RUN（如实）**

`captureMs` 属 **T284Cold（P051 生成树 `test/t284/apply.cjs` 应用产物）**，不在主工程包——本包无法测。需 t284 生成树重建+重签单独复核（独立 harness），不混入本卡。

## 4. M4 增量证书回退路径观察：**PASS（0 次，如实记录窗口）**

观测窗口（R 0→148）内 `auditFallbacks=0`——增量证书谓词全过、journal 未溢出；T331 非机 D1-D4 同为 0。回退代码路径在位（`auditParticleHot.ets:1284 inc===2 → auditFallbacks++`）但未触发，符合预期（增量路径设计目标即零回退）。

## 5. 结论

- **M1/M2/M4 PASS；M3 NOT_RUN（属另一生成树，如实登记）**。T331 三件（增量审计证书/sort 消除/attemptCumulativeMs）真机行为与非机回归一致，死亡族场景存活确认。
- 两处取证工装变更如实登记（T311_STAGE 通道恢复×2）；SignHap 00303116 已知独立登记。

证据：`test/reports/t338/`（t338_device_M_verdicts.json + hilog_full.txt + layout0-2.json + demo_diag.txt）；签包 `.codex/local-test/t318-sign/T338b-signed.hap`。
