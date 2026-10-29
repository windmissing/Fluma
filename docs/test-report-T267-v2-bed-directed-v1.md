# T267 V2 落床定向续证 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-29 · 状态：**BLOCKED（定向锚点未达成，新缺陷证据回 architect 首次归因）**
- 被测包：**T266-v9-signed.hap**（SHA `9e0bd6b2d28f56b450cd2bb1cd0a9bee7a9b4f9b9e002e3c3eecb6482493895d`，源 `5ffadf8`）
- 页面：`pages/P051DerivedV2Demo`（`--ps p051v2 1`；重日志臂加 `--ps t266heavy true`）
- 归档：`test/reports/t267/`（t267a/b/c.jpeg、t267_tail.txt 300 行 T266_/T248_ 原始日志）

## 矩阵锚点执行状态（沿 T243 v2 定向续证，未重跑 T218 全矩阵）

| 锚点 | 状态 | 证据（多 run 汇总） |
|---|---|---|
| 出生 | ✅ | 各 run firstBirthCommit=387/1669/929/129，`T266_SOURCE` ok=true 序列完整 |
| 墙钟 20s | ✅ | 会话连续运行 >1 分钟（pid 35894 frame 1→840 连续） |
| 累计成功子步物理秒 ≥8 | ✅ | run1（pid 35894）acceptedPhysicsSeconds=**28.57**；run5（59535）=**15.7**；run6（42185）=**9.93**；run7（51385）=7.0、run8（62459）注入中——T266Clock 字段本身正确累计 |
| 源注入与拒绝（勘误口径） | ✅ | `sourceAdmittedMass`=38.2–50.36（非 50 整数）、`sourceRejectedCount`=1（CAPACITY_BLOCKED，池 512 满）与 T249 报告"23 成功 1 拒绝"类勘误同型；`admittedEquivalentSeconds`=6.26–6.28（mass/8 显式） |
| 首 LAND / 床量变化 | ❌ **BLOCKED** | 各 run publication `bh/bl/mh/ml` 全 0、bedColumns 全 0、states 全空/零；注入沙（admitted 38–50 单位）下落未触底，与 T249/T218 现象一致 |
| 首生后接受模拟 ≥8s（`acceptedPhysicsSecondsSinceFirstBirth`≥8 锚点） | ❌ **BLOCKED（新缺陷）** | 见下 |
| 随后 3 个完整 CURRENT 同 token 帧 | ⚠️ 部分 | CURRENT 回执连续（receipt.state=READY/CURRENT、requestedFrameToken 完整长 token）；锚点帧（带 boundedTrace/nativeStages）仅 1 次在 frame1 触发且内容为空态，3 锚点帧未采到 |

## 新缺陷证据（回 architect 首次归因；tester 不修）

**缺陷 A（T266Clock `acceptedPhysicsSecondsSinceFirstBirth` 不随 step 累计）**：四个独立 run 中该值分别在首生后固定为 0.20/0.27/0.57/0.80 不再增长（`firstBirthCommit` 之后的每一帧 step 均成功，`acceptedPhysicsSeconds` 同期持续增长）。当前实现只在 birth 同帧读时刻值、无跨帧累加——**首生后 ≥8s 锚点在当前实现下不可达**，需 architect 裁决修法（clock 累加 vs 锚点改用 `acceptedPhysicsSeconds - acceptedAtFirstBirth`）。

**环境约束（非缺陷，如实）**：设备在失去前台交互/注入后数秒内被 Doze 冻结（SUSPEND_MANAGER `DOZE_BY_STOP_LIFECYCLE`），帧停、无自恢复；`power-shell wakeup` 单独保活无效，需 `aa start`（会复用进程不新建 run）+ 屏幕亮。本轮通过注入期 keep + 会话内连续运行已保证物理秒 ≥8 达成；床可见性等待时间受此约束，未能在单 run 内无限延长。

## 与 T249/T218 一致的下落观察

注入 50 单位沙（池 512 满、CAPACITY_BLOCKED 后池内 512 粒），连续观察 1–6 分钟：publication `minY/maxY` 未见触底、床全 0——与 T249（y∈[124,127] 悬浮）和 T218 现象一致，属 V2 下落速度/触底问题线，继续归 architect 分流。

## NOT_RUN / 边界（如实）

- 环满/截断：本轮 boundedTrace/nativeStages 未见 INCOMPLETE/NOT_CAPTURED 以外的截断（`landEvents:"NOT_CAPTURED_USE_WRITER_HOOK"`、`pixel:"NOT_CAPTURED_USE_NATIVE_SAME_TOKEN_HOOK"` 为设计内非采样标注，非截断）。
- 像素级床可见性：设计要求 tester 显式 capture；本轮床量恒 0 无像素可验，NOT_RUN。
- 分段 p50/p95：正常臂仅 60 帧摘要（renderInclusive≈5.1ms 稳定），heavy 臂 boundedTrace 样本不足 3 帧，不编造统计。
- 收益对照：无整步稳定运行基线，NOT_MEASURED。

## 证据

- `t267_tail.txt`：pid 62459 会话 T266_FRAME/SOURCE 与 T248_STEP/LAND 原始 300 行。
- `t267a.jpeg`（正常臂运行中）、`t267b/c.jpeg`（注入后状态）。
- 报告内 clock/receipt 数值全部来自 hilog 原始行摘录（可复查）。

## 失败上报

**BLOCKED → architect**：缺陷 A（sinceFirstBirth 不累计，锚点不可达）+ 既有下落不触底线（T249/T218 同现象）。复现：装 T266-v9 → `aa start --ps p051v2 1 --ps t266heavy true` → uinput keep 注入至池满 → 观察 T266_SOURCE clock `acceptedPhysicsSecondsSinceFirstBirth` 冻结于首生帧值。
