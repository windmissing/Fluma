# T215 明水 B 独立验证报告

日期：2026-10-28。任务：6ae168e3e4b0d25fef5ec666（tester，离线，目标7/P3）。消费：T209 4bdf050（`developer-T209-clear-water-B-v1.md`），按 T196 §11 B01–B14 与 T203 §B。

## 执行器与独立点

`test/p215_clear_water_b_independent.cjs`（12 用例）。独立 oracle：

1. **自持 BigInt 精确质量参考**：直接从 binary64 位型推导 2^-1074 单位（normal `m×2^(e-1)`、subnormal `m`），加/比较全序独立实现——**不调用产品 ExactMass/replay 作 oracle**。
2. adversarial 累加序核：BigInt 序不变性对照 RN 浮点序敏感性（暴露 incremental P 累加风险——产品以 canonical P 为准，exact acc 不代替）。
3. B01–B14 场景独立构造（不镜像 T209 自测顺序），整 frame 计数按 `CW_B_COUNTERS` 名单独立推导。

## 结果：12/12 PASS（原始 JSON：test/reports/t215/independent.json）

| # | 场景 | 结果 | 关键证据 |
|---|---|---|---|
| B-10 | 独立 BigInt oracle | PASS | encode 往返（.125/.0625/1/1.5/2^-1074/MIN_VALUE/2^-52/2^46-2^-6）、比较全序、加法进位、域拒绝（≥2^46/负/NaN→null） |
| B-09 | adversarial 累加序 | PASS | BigInt (2^-53+1)+2^-53 ≡ 2^-53+(1+2^-53) 序不变；RN 浮点序敏感（暴露点成立） |
| B-01 | A 保留 + dt0 | PASS | dt=0 后指纹逐位不变、pending 队列保留、counters/arenaHeader 全不变；未 attach B → step UNSUPPORTED_FEATURE |
| B-02 | source 成功后 step 失败 | PASS | W 破坏 → step 拒绝但 sourceCommit 保留；第二子步失败保留第一；库存无半成功 |
| B-03 | 水五界局部准入 | PASS | water 1 → admitted=1、仅 W[64] 变化（W[63]=0） |
| B-07 | full/partial LAND | PASS | P[64]=0.25 落床、W 全局不变、nextId 不因 LAND/释放回退（birth 序号 source 时已定） |
| B-08 | 床/budget | PASS | 10 步链 budgetUsed 计数推进（refund 在步内） |
| B-11 | 源身份/旧 run | PASS | 旧 runId → STALE_RUN 拒绝，无半日志 |
| B-12 | 整 frame 乘数 | PASS | whole-audit（entry+work+replay）≤3、wholeCacheRebuilds/wholeIndexRebuilds 零（无逐 birth rebuild） |
| B-13 | B snapshot 隔离 + restore | PASS | 调用者改 events/W 副本不影响运行；无 restoreB/restoreSnapshot 入口 |
| B-14 | C/页面 NOT_IMPLEMENTED | PASS | EmptySandDemo 无 snapshotB 入口、engine 无 FrameToken |
| B01-A | A 回执在 B attach 下保留 | PASS | replay 物化回执保留 kindSuccessSeq=1/transactionId；I_sand=.125 账一致 |

## 基线

T209 开发自测 35/35 复跑 PASS。

## 语义记录

- source 双 enqueue 模式（T188/T209 submit 惯例）：第二次 enqueue 物化 replay 回执（replay=true），但 kindSuccessSeq/transactionId 保留原成功值——回执字段保留语义成立。
- nextId 语义：birth 时分配，LAND/slot 释放不回退不前进。
- ExactMass 70 limb：三个完整 U16 词 + 尾数高 5 位全精，域 <2^46。

## 边界

- **A/B 离线 PASS 不冒可视水或设备性能**（无设备、无 C 渲染）。
- **C/页面/切源/透明同帧 NOT_IMPLEMENTED**（B-14 核：无页面入口、无 FrameToken）；snapshot restore 未实现。
- 效测（入水减速/积床抬面/低头流/倾角方向/切源停止）归 effector 后续；设备归后续真机卡。
- 失败上报：无。无阻断。
